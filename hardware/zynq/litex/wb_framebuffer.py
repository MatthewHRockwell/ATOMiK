"""
ScanlineVideoFrameBuffer — Scanline-based DDR framebuffer.

v19: 1080p via 64-bit AXI HP0 with pipelined burst reads.
Each AXI beat returns 8 bytes = 2 pixels. 64-bit wide BRAM stores
pixel pairs; read port selects high or low 32 bits based on hcount[0].
"""

from migen import *
from migen.genlib.cdc import MultiReg

from litex.gen import LiteXModule
from litex.soc.interconnect import stream
from litex.soc.interconnect.csr import CSRStorage, CSRStatus
from litex.soc.cores.video import video_timing_layout, video_data_layout


class WishboneVideoFrameBuffer(LiteXModule):
    """Scanline framebuffer with pipelined AXI burst reads (64-bit)."""
    def __init__(self, axi, hres=1920, vres=1080, base=0x48000000,
                 clock_domain="hdmi", default_enable=0):

        self.vtg_sink = vtg_sink = stream.Endpoint(video_timing_layout)
        self.source   = source   = stream.Endpoint(video_data_layout)

        stride = hres * 4  # bytes per line in DDR (32bpp)
        fetch_lines = vres
        # 64-bit AXI: 2 pixels per beat
        words_per_line = hres // 2  # 64-bit words (960 for 1920 pixels)
        burst_len = 16
        num_bursts = words_per_line // burst_len

        # ---- CSRs -------------------------------------------------------
        self.dma = dma_csrs = LiteXModule()
        dma_csrs._base   = CSRStorage(32, reset=base, description="FB base")
        dma_csrs._enable = CSRStorage(reset=default_enable, description="Enable")

        # ---- Line buffer: 64-bit wide BRAM, 2 pages ---------------------
        # Each word stores 2 adjacent pixels (8 bytes).
        # Page 0: addresses 0 to words_per_line-1
        # Page 1: addresses words_per_line to 2*words_per_line-1
        line_buf = Memory(64, 2 * words_per_line)
        wr_port = line_buf.get_port(write_capable=True, clock_domain="sys")
        rd_port = line_buf.get_port(clock_domain=clock_domain)
        self.specials += line_buf, wr_port, rd_port

        # ---- HDMI domain ------------------------------------------------
        hdmi_sync = getattr(self.sync, clock_domain)

        self.comb += vtg_sink.connect(source,
            keep={"valid", "ready", "last", "de", "hsync", "vsync"})

        rd_page = Signal()
        hcount = Signal(max=hres)
        self.comb += hcount.eq(vtg_sink.hcount[:len(hcount)])

        # BRAM address: hcount/2 (each 64-bit word holds 2 pixels)
        hcount_half = Signal(max=words_per_line)
        self.comb += hcount_half.eq(hcount >> 1)
        self.comb += rd_port.adr.eq(
            Mux(rd_page, hcount_half + words_per_line, hcount_half))

        # 1-cycle BRAM latency compensation
        de_d1 = Signal()
        hsync_d1 = Signal()
        vsync_d1 = Signal()
        valid_d1 = Signal()
        hcount_lsb_d1 = Signal()  # delayed hcount[0] for pixel select
        hdmi_sync += [
            de_d1.eq(vtg_sink.de),
            hsync_d1.eq(vtg_sink.hsync),
            vsync_d1.eq(vtg_sink.vsync),
            valid_d1.eq(vtg_sink.valid),
            hcount_lsb_d1.eq(hcount[0]),
        ]
        self.comb += [
            source.de.eq(de_d1),
            source.hsync.eq(hsync_d1),
            source.vsync.eq(vsync_d1),
            source.valid.eq(valid_d1),
        ]

        # Pixel select: hcount[0]=0 → low 32 bits, hcount[0]=1 → high 32 bits
        pixel_word = Signal(32)
        self.comb += pixel_word.eq(
            Mux(hcount_lsb_d1, rd_port.dat_r[32:64], rd_port.dat_r[0:32]))

        # XRGB8888 unpack
        self.comb += [
            If(de_d1,
                source.r.eq(pixel_word[16:24]),
                source.g.eq(pixel_word[ 8:16]),
                source.b.eq(pixel_word[ 0: 8]),
            ),
        ]

        # ---- Page swap: unconditional at hsync --------------------------
        hsync_prev_hdmi = Signal()
        hdmi_sync += hsync_prev_hdmi.eq(vtg_sink.hsync)
        hsync_rising_hdmi = Signal()
        self.comb += hsync_rising_hdmi.eq(vtg_sink.hsync & ~hsync_prev_hdmi)
        hdmi_sync += If(hsync_rising_hdmi, rd_page.eq(~rd_page))

        # ---- Cross signals to sys domain --------------------------------
        hsync_r_hdmi = Signal()
        hsync_toggle = Signal()
        hdmi_sync += hsync_r_hdmi.eq(vtg_sink.hsync)
        hdmi_sync += If(vtg_sink.hsync & ~hsync_r_hdmi, hsync_toggle.eq(~hsync_toggle))

        hsync_toggle_sys = Signal()
        hsync_toggle_sys_r = Signal()
        self.specials += MultiReg(hsync_toggle, hsync_toggle_sys, "sys")
        self.sync += hsync_toggle_sys_r.eq(hsync_toggle_sys)
        new_line = Signal()
        self.comb += new_line.eq(hsync_toggle_sys ^ hsync_toggle_sys_r)

        rd_page_sys = Signal()
        wr_page = Signal()
        self.specials += MultiReg(rd_page, rd_page_sys, "sys")
        self.comb += wr_page.eq(~rd_page_sys)

        vsync_hdmi_level = Signal()
        self.comb += vsync_hdmi_level.eq(vtg_sink.vsync)
        vsync_sys = Signal()
        self.specials += MultiReg(vsync_hdmi_level, vsync_sys, "sys")
        vsync_sys_prev = Signal()
        self.sync += vsync_sys_prev.eq(vsync_sys)
        frame_start = Signal()
        self.comb += frame_start.eq(vsync_sys & ~vsync_sys_prev)

        # ---- Sys domain: pipelined AXI burst reader (64-bit) ------------
        self.comb += [
            axi.aw.valid.eq(0),
            axi.w.valid.eq(0),
            axi.b.ready.eq(1),
        ]

        enable_sys = Signal()
        self.specials += MultiReg(self.dma._enable.storage, enable_sys, "sys")

        base_sys = Signal(32)
        self.comb += base_sys.eq(self.dma._base.storage)

        fetch_line = Signal(max=vres + 50)
        fetch_line_latched = Signal(max=vres + 50)
        line_addr = Signal(32)
        self.comb += line_addr.eq(base_sys + fetch_line_latched * stride)

        ar_idx = Signal(max=num_bursts + 1)
        r_idx = Signal(max=words_per_line + 1)

        new_line_pending = Signal()
        self.sync += If(new_line, new_line_pending.eq(1))

        guard_ctr = Signal(max=16)

        self.fetch_fsm = fsm = FSM(reset_state="FRAME_WAIT")
        fsm.act("FRAME_WAIT",
            If(frame_start,
                NextValue(fetch_line, 0),
                NextValue(new_line_pending, 0),
                NextState("WAIT"),
            ),
        )
        fsm.act("WAIT",
            If(frame_start,
                NextValue(fetch_line, 0),
                NextValue(new_line_pending, 0),
            ),
            If(new_line_pending | new_line,
                NextValue(new_line_pending, 0),
                NextValue(guard_ctr, 0),
                NextState("GUARD"),
            ),
        )
        fsm.act("GUARD",
            NextValue(guard_ctr, guard_ctr + 1),
            If(guard_ctr == 8,
                NextValue(ar_idx, 0),
                NextValue(r_idx, 0),
                NextValue(fetch_line_latched, fetch_line),
                NextState("PIPE"),
            ),
        )
        fsm.act("PIPE",
            If(ar_idx < num_bursts,
                axi.ar.valid.eq(1),
                # Each burst: 16 beats × 8 bytes = 128 bytes
                axi.ar.addr.eq(line_addr + (ar_idx << 7)),  # ar_idx * 128
                axi.ar.burst.eq(1),              # INCR
                axi.ar.len.eq(burst_len - 1),    # 15 = 16 beats
                axi.ar.size.eq(3),               # 8 bytes per beat (64-bit)
                axi.ar.id.eq(0),
                axi.ar.lock.eq(0),
                axi.ar.prot.eq(0),
                axi.ar.cache.eq(0b0011),
                axi.ar.qos.eq(0),
                If(axi.ar.ready,
                    NextValue(ar_idx, ar_idx + 1),
                ),
            ),
            axi.r.ready.eq(1),
            If(axi.r.valid,
                wr_port.we.eq(1),
                wr_port.adr.eq(
                    Mux(wr_page, r_idx + words_per_line, r_idx)),
                wr_port.dat_w.eq(axi.r.data),
                NextValue(r_idx, r_idx + 1),
            ),
            If(r_idx == words_per_line,
                If(fetch_line >= (fetch_lines - 1),
                    NextState("FRAME_WAIT"),
                ).Else(
                    NextValue(fetch_line, fetch_line + 1),
                    NextState("DONE"),
                ),
            ),
        )
        fsm.act("DONE",
            NextState("WAIT"),
        )
