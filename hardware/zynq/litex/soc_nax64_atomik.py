#!/usr/bin/env python3
"""
ATOMiK NaxRiscv RV64 SoC on HamGeek RK-ZYNQ7020-F

64-bit RISC-V soft-core with ATOMiK delta-state adapter.
Uses NaxRiscv (SpinalHDL) with Sv39 MMU for full Linux support.

Usage:
    python3 soc_nax64_atomik.py --build --xlen=64
"""

import os
import sys

from migen import *
from litex.gen import *
from litex_boards.platforms import hamgeek_rk7020f
from litex.soc.cores.clock import *
from litex.soc.integration.soc_core import *
from litex.soc.integration.builder import *
from litex.soc.integration.soc import SoCRegion
from litex.soc.cores.cpu import zynq7000
from litex.soc.interconnect import wishbone
from litex.soc.cores.cpu.naxriscv.core import NaxRiscv
from litex.soc.cores.video import VideoS7HDMIPHY, VideoTimingGenerator
from litex.soc.cores.clock import S7MMCM

# Phase 9.2 framebuffer (see wb_framebuffer.py) — Wishbone-master DMA
# equivalent of LiteX's VideoFrameBuffer, for SoCs without LiteDRAM.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wb_framebuffer import WishboneVideoFrameBuffer
from ps_uart0_bridge import PSUart0Bridge

# Phase 9.2: framebuffer lives 128 MiB into main_ram, well clear of kernel
# (0x40000000-0x408xxxxx), trampoline (0x40A00000), DTB (0x40EF0000),
# OpenSBI (0x40F00000), and initramfs (0x42000000-0x43E00000).
FRAMEBUFFER_BASE = 0x48000000
FRAMEBUFFER_HRES = 1920
FRAMEBUFFER_VRES = 1080

# Configure NaxRiscv for RV64 before SoC instantiation
# NOTE: with_fpu and with_rvc MUST be passed via CLI (--with-fpu --with-rvc)
# because LiteXArgumentParser.parse_args() overrides class-level settings.
NaxRiscv.xlen                 = 64
NaxRiscv.data_width           = 64
NaxRiscv.gcc_triple           = ("riscv64-unknown-elf", "riscv64-linux-gnu", "riscv64-unknown-linux-gnu")
NaxRiscv.linker_output_format = "elf64-littleriscv"
NaxRiscv.no_netlist_cache     = False
NaxRiscv.with_fpu             = True
NaxRiscv.with_rvc             = True
# L2 OFF for DDR corruption diagnostic (Test B).
# NaxSoc.scala: `def withL2 = l2Bytes > 0` — 0 disables L2 entirely.
# Forces NaxRiscv to issue single-word accesses instead of 64-byte burst refills;
# if memtest errors disappear the burst path through 32-bit GP0 is the cause.
NaxRiscv.l2_bytes             = 0          # L2 disabled
NaxRiscv.l2_ways              = 4          # ignored when l2_bytes==0

# CRG ------------------------------------------------------------------------------------

class _CRG(LiteXModule):
    def __init__(self, platform, sys_clk_freq):
        self.rst    = Signal()
        self.cd_sys = ClockDomain()
        self.comb += ClockSignal("sys").eq(ClockSignal("ps7"))
        self.comb += ResetSignal("sys").eq(ResetSignal("ps7") | self.rst)

# BaseSoC ---------------------------------------------------------------------------------

class BaseSoC(SoCCore):
    def __init__(self, sys_clk_freq=100e6, with_atomik_adapter=True,
                 with_video_framebuffer=False,
                 with_video_phy_only=False,
                 with_video_framebuffer_hp=False,
                 **kwargs):
        platform = hamgeek_rk7020f.Platform()
        self.crg = _CRG(platform, sys_clk_freq)

        # Force NaxRiscv — parser default is VexRiscv, setdefault won't override.
        kwargs["cpu_type"] = "naxriscv"
        # Disable the default LiteX UART (LiteUART on PL pin V12, not host-
        # visible on this board).  We replace it below with PSUart0Bridge,
        # which exposes the same CSR layout but writes through to Zynq PS
        # UART0 on MIO10/11 → FT232 → /dev/ttyUSB1.
        kwargs["uart_name"] = "stub"
        SoCCore.__init__(self, platform, sys_clk_freq,
            ident       = "ATOMiK NaxRiscv RV64 SoC on HamGeek RK-ZYNQ7020-F",
            **kwargs,
        )

        # PS7 DDR3 via GP slave
        self.ps = ps = zynq7000.Zynq7000(platform=platform, variant="standard")
        ps.set_ps7(name="Zynq", config=platform.ps7_config)

        # Force USB0 MIO config via raw TCL — Vivado's PS7 IP rejects single-shot
        # dict assignments due to parameter dependency ordering: PCW_USB0_RESET_IO
        # only sticks if PCW_USB_RESET_ENABLE / PCW_USB_RESET_SELECT are already
        # set on the IP. Split into THREE sequential set_property calls so the
        # IP configurator settles dependencies between each batch:
        #   1. enable USB0 + assign data MIO pins + interrupt routing
        #   2. set umbrella USB reset config (must come before per-port reset)
        #   3. assign per-port USB0 reset to MIO 46
        # Build #6/#7 had everything in one dict and Vivado rejected
        # PCW_USB0_RESET_IO as `<Select> enabled:false` — meaning the PS7 USB
        # peripheral never drove MIO46, so the USB3320 PHY never got reset and
        # the controller's automatic ULPI bring-up sequence (which writes OTG
        # Control bit 6 = DrvVbusExternal) never ran.
        # NOTE: double braces {{ }} to escape Python .format()
        ps.ps7_tcl.append(
            'set_property -dict [list '
            'CONFIG.PCW_EN_USB0 {{1}} '
            'CONFIG.PCW_USB0_PERIPHERAL_ENABLE {{1}} '
            'CONFIG.PCW_USB0_USB0_IO {{MIO 28 .. 39}} '
            'CONFIG.PCW_USE_FABRIC_INTERRUPT {{1}} '
            'CONFIG.PCW_P2F_USB0_INTR {{1}} '
            '] [get_ips Zynq]'
        )
        ps.ps7_tcl.append(
            'set_property -dict [list '
            'CONFIG.PCW_USB_RESET_ENABLE {{1}} '
            'CONFIG.PCW_USB_RESET_POLARITY {{Active Low}} '
            'CONFIG.PCW_USB_RESET_SELECT {{Share reset pin}} '
            '] [get_ips Zynq]'
        )
        ps.ps7_tcl.append(
            'set_property -dict [list '
            'CONFIG.PCW_USB0_RESET_ENABLE {{1}} '
            'CONFIG.PCW_USB0_RESET_IO {{MIO 46}} '
            '] [get_ips Zynq]'
        )

        axi_gp0 = ps.add_axi_gp_slave(clock_domain="sys")

        # DDR region: NaxRiscv 0x40000000 → PS DDR 0x00100000
        axi_gp0_ddr_dst = SoCRegion(origin=0x100000, size=0x2000_0000)
        main_ram_region = SoCRegion(
            origin=self.cpu.mem_map["main_ram"],
            size=axi_gp0_ddr_dst.size,
            mode="rwx",
        )

        wb = self.bus.add_adapter("axi_gp0", axi_gp0, direction="s2m")
        remap_module = wishbone.Interface(data_width=32, address_width=32)
        self.submodules += wishbone.Remapper(
            master=remap_module, slave=wb,
            src_regions=[main_ram_region], dst_regions=[axi_gp0_ddr_dst],
        )
        self.bus.add_slave("main_ram", remap_module, main_ram_region)

        # PS I/O Peripherals via GP1: NaxRiscv 0x80000000 → PS IOP 0xE0000000
        # Exposes PS USB0, GEM0, SDIO, GPIO to NaxRiscv Linux
        axi_gp1 = ps.add_axi_gp_slave(clock_domain="sys")
        wb_iop = self.bus.add_adapter("axi_gp1", axi_gp1, direction="s2m")
        iop_dst = SoCRegion(origin=0xE000_0000, size=0x0030_0000)
        iop_src = SoCRegion(origin=0x8000_0000, size=0x0030_0000, mode="rw", cached=False)
        remap_iop = wishbone.Interface(data_width=32, address_width=32)
        self.submodules += wishbone.Remapper(
            master=remap_iop, slave=wb_iop,
            src_regions=[iop_src], dst_regions=[iop_dst],
        )
        self.bus.add_slave("ps_iop", remap_iop, iop_src)

        # PS UART0 bridge — replaces the disabled LiteX UART.  Presents the
        # standard LiteUART CSR layout at the "uart" CSR slot so the BIOS,
        # Linux liteuart driver, and OpenSBI litex_console work unchanged,
        # but internally proxies writes/reads to Zynq PS UART0 (MIO10/11 →
        # FT232 → /dev/ttyUSB1) via a wishbone master on the SoC bus.
        # Wishbone accesses target NaxRiscv 0x8000_002C / 0x8000_0030 which
        # the ps_iop remapper above translates to ARM 0xE000_002C / 0x30.
        self.uart = PSUart0Bridge()
        self.csr.add("uart", use_loc_if_exists=True)
        self.bus.add_master("psuart_bridge", master=self.uart.bus)
        # UART interrupt — keep the slot so software that references
        # CONFIG_UART_INTERRUPT (LiteX BIOS, Linux liteuart driver) still
        # builds.  The bridge drives ev.tx / ev.rx triggers based on the
        # cached PS-side status.
        self.irq.add("uart", use_loc_if_exists=True)

        # USB0 interrupt from PS to NaxRiscv PLIC
        # PS7 IRQ_P2F_USB0 output mirrors the USB0 interrupt to PL fabric.
        # Wire it to PLIC input 3 (input 1 = liteuart).
        usb0_irq = ps.get_irq_p2f_usb0()
        self.irq.add("usb0", use_loc_if_exists=True)
        self.comb += self.cpu.interrupt[3].eq(usb0_irq)

        # LCD ST7789V 320x172 — GPIO bitbang SPI
        # Pins confirmed from RK-ZYNQ7020-F Schematics.pdf page 5, Bank 33.
        # V19 (PHY2_RXCTL) was the JTAG crash culprit, NOT these LCD pins.
        # Names prefixed "z" so they sort AFTER uart/timer/video in CSR space,
        # preserving the baseline CSR layout that OpenSBI was compiled against.
        from litex.soc.cores.gpio import GPIOOut
        lcd_mosi = platform.request("lcd_mosi")
        lcd_clk  = platform.request("lcd_clk")
        lcd_dc   = platform.request("lcd_dc")
        lcd_cs   = platform.request("lcd_cs")
        lcd_rst  = platform.request("lcd_rst")
        lcd_led  = platform.request("lcd_led")
        self.submodules.zlcd_mosi = GPIOOut(lcd_mosi)
        self.submodules.zlcd_clk  = GPIOOut(lcd_clk)
        self.submodules.zlcd_dc   = GPIOOut(lcd_dc)
        self.submodules.zlcd_cs   = GPIOOut(lcd_cs)
        self.submodules.zlcd_rst  = GPIOOut(lcd_rst)
        self.submodules.zlcd_led  = GPIOOut(lcd_led)

        # ATOMiK CFU Adapter (Wishbone-mapped)
        if with_atomik_adapter:
            zynq_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            adapter_v = os.path.join(zynq_dir, "rtl", "atomik_cfu_adapter.v")
            wrapper_v = os.path.join(zynq_dir, "rtl", "atomik_cfu_wishbone.v")
            platform.add_source(adapter_v)
            platform.add_source(wrapper_v)

            adapter_bus = wishbone.Interface(data_width=32, address_width=32, addressing="word")
            self.specials += Instance("atomik_cfu_wishbone",
                i_clk   = ClockSignal("sys"),
                i_rst   = ResetSignal("sys"),
                i_adr   = adapter_bus.adr[:3],
                i_dat_w = adapter_bus.dat_w,
                o_dat_r = adapter_bus.dat_r,
                i_we    = adapter_bus.we,
                i_cyc   = adapter_bus.cyc,
                i_stb   = adapter_bus.stb,
                o_ack   = adapter_bus.ack,
            )

            self.bus.add_slave("atomik_adapter", adapter_bus,
                region=SoCRegion(origin=0xf0020000, size=0x20, cached=False))

        # ------------------------------------------------------------------
        # Phase 9.2 "cheap confirmation" variant: --with-video-phy-only
        #
        # Enables MMCM + HDMI PHY + VTG exactly like the FB build, but SKIPS
        # the framebuffer DMA and its wishbone master on the SoC bus. The
        # VTG's sync signals drive the PHY directly with r/g/b tied low
        # (blank black frame). This isolates the "added bus master / arbiter"
        # variable from all other HDMI-side changes — if Linux boots in this
        # config, the shared-bus arbiter is confirmed as the v5 hang cause.
        # ------------------------------------------------------------------
        if with_video_phy_only:
            self.cd_hdmi   = ClockDomain()
            self.cd_hdmi5x = ClockDomain()
            self.video_pll = video_pll = S7MMCM(speedgrade=-2)
            self.comb += video_pll.reset.eq(ResetSignal("sys"))
            video_pll.register_clkin(ClockSignal("sys"), sys_clk_freq)
            video_pll.create_clkout(self.cd_hdmi,   25.175e6)
            video_pll.create_clkout(self.cd_hdmi5x, 5*25.175e6)

            self.videophy = VideoS7HDMIPHY(platform.request("hdmi_out"),
                                           clock_domain="hdmi")

            video_vtg = VideoTimingGenerator(default_video_timings="640x480@60Hz")
            video_vtg = ClockDomainsRenamer("hdmi")(video_vtg)
            self.add_module(name="video_vtg", module=video_vtg)

            # Connect VTG → PHY via stream.connect(), auto-mapping shared
            # fields (valid, ready, last, de, hsync, vsync). PHY's r/g/b
            # default to 0 (black) since VTG doesn't produce pixel data.
            self.comb += video_vtg.source.connect(self.videophy.sink,
                keep={"valid", "ready", "last", "de", "hsync", "vsync"})

            # Same XDC pre-placement false-path fix as the FB variant.
            platform.toolchain.pre_placement_commands.add(
                "set_clock_groups -asynchronous "
                "-group [get_clocks clk_fpga_0] "
                "-group [get_clocks -include_generated_clocks -of_objects [get_pins -hierarchical -filter {{NAME =~ */MMCME2_ADV/CLKOUT*}}]]"
            )
            # Intentionally no bus.add_master call. No wishbone arbiter.

        # ------------------------------------------------------------------
        # Phase 9.2 Path B: DDR-backed framebuffer via AXI HP0
        #
        # Confirmed necessary 2026-04-15: phy-only boots fine, but adding a
        # second wishbone bus master (Path A) inserts an Arbiter that breaks
        # CSR access → kernel hangs at LiteX SoC Controller driver probe.
        #
        # Path B avoids the SoC bus entirely:
        #   FB DMA → private wishbone → Wishbone2AXI → S_AXI_HP0 → PS DDR
        # NaxRiscv stays single-master on the SoC bus. No arbiter inserted.
        #
        # FB base: PS DDR 0x08100000 (= NaxRiscv 0x48000000 via GP0 remapper).
        # Linux userspace writes via /dev/mem at 0x48000000 (NaxRiscv view);
        # FB hardware reads via HP0 at 0x08100000 (PS DDR physical).
        # ------------------------------------------------------------------
        if with_video_framebuffer_hp:
            from litex.soc.interconnect import axi

            # HDMI clock domains + PHY (same as phy-only and Path A)
            self.cd_hdmi   = ClockDomain()
            self.cd_hdmi5x = ClockDomain()
            self.video_pll = video_pll = S7MMCM(speedgrade=-2)
            self.comb += video_pll.reset.eq(ResetSignal("sys"))
            video_pll.register_clkin(ClockSignal("sys"), sys_clk_freq)
            # 1920x1080@30Hz: pixel clock = 74.25 MHz (same as 720p@60 — proven),
            # 5x serializer = 371.25 MHz (proven safe for OSERDES2 -2).
            # Uses standard 1080p total timing (2200×1125) at half refresh.
            video_pll.create_clkout(self.cd_hdmi,   74.25e6)
            video_pll.create_clkout(self.cd_hdmi5x, 5*74.25e6)

            self.videophy = VideoS7HDMIPHY(platform.request("hdmi_out"),
                                           clock_domain="hdmi")

            # Custom 1080p@30Hz timing: standard 1080p pixel layout but at
            # 74.25 MHz pixel clock → 74.25e6 / (2200*1125) = 30 Hz.
            _1080p30_timings = {
                "pix_clk"       : 74.25e6,
                "h_active"      : 1920,
                "h_blanking"    : 280,
                "h_sync_offset" : 88,
                "h_sync_width"  : 44,
                "v_active"      : 1080,
                "v_blanking"    : 45,
                "v_sync_offset" : 4,
                "v_sync_width"  : 5,
            }
            video_vtg = VideoTimingGenerator(default_video_timings=_1080p30_timings)
            video_vtg = ClockDomainsRenamer("hdmi")(video_vtg)
            self.add_module(name="video_framebuffer_vtg", module=video_vtg)

            # PS AXI HP0 slave
            # 64-bit HP for higher bandwidth (2 pixels per beat)
            axi_hp0 = ps.add_axi_hp_slave(clock_domain="sys", data_width=64)

            FRAMEBUFFER_BASE_PS = 0x08100000

            # Pipelined AXI reader — drives HP0 AR/R directly, no wishbone.
            self.video_framebuffer = WishboneVideoFrameBuffer(
                axi            = axi_hp0,
                hres           = FRAMEBUFFER_HRES,
                vres           = FRAMEBUFFER_VRES,
                base           = FRAMEBUFFER_BASE_PS,
                clock_domain   = "hdmi",
                default_enable = 0,
            )

            # VTG → FB → PHY
            self.comb += video_vtg.source.connect(self.video_framebuffer.vtg_sink)
            self.comb += self.video_framebuffer.source.connect(self.videophy.sink)

            # False-path between sys (clk_fpga_0) and hdmi (clkout0/1) domains
            platform.toolchain.pre_placement_commands.add(
                "set_clock_groups -asynchronous "
                "-group [get_clocks clk_fpga_0] "
                "-group [get_clocks {{{{clkout0 clkout1}}}}]"
            )

        # HDMI Video Framebuffer — Phase 9.2 experimental path (PATH A, BROKEN)
        #
        # STATUS (2026-04-15): `--with-video-framebuffer` is OPT-IN and
        # currently broken for boot: adding the FB as a second wishbone bus
        # master causes Linux to hang after `riscv-plic ... mapped 32
        # interrupts`, just before the LiteX SoC Controller driver's first
        # CSR access at 0xF0000000. Timing is CLEAN (sys_clk WNS +0.291 ns
        # after the XDC fix, see below). The strongest remaining suspect is
        # the shared-bus master / arbitration path: inserting a second
        # master makes LiteX wrap the interconnect with wishbone.Arbiter,
        # and that is the change most correlated with the new hang.
        #
        # Do NOT ship this config as the default until either:
        #   (a) the FB moves to a dedicated AXI HP master (Plan C / Path B),
        #       bypassing the SoC wishbone entirely, or
        #   (b) the bus arbiter behaviour is root-caused.
        #
        # Known-good baseline = the non-FB bitstream. See
        # `hardware/zynq/ps_loader/PHASE_9_1_MILESTONE.md`.
        if with_video_framebuffer:
            # Video clock domains from MMCM (driven by PS7 100 MHz FCLK)
            self.cd_hdmi   = ClockDomain()
            self.cd_hdmi5x = ClockDomain()

            self.video_pll = video_pll = S7MMCM(speedgrade=-2)
            self.comb += video_pll.reset.eq(ResetSignal("sys"))
            video_pll.register_clkin(ClockSignal("sys"), sys_clk_freq)
            # 640x480@60Hz: pixel clock = 25.175 MHz, 5x ≈ 125.875 MHz
            video_pll.create_clkout(self.cd_hdmi,   25.175e6)
            video_pll.create_clkout(self.cd_hdmi5x, 5*25.175e6)

            # HDMI PHY
            self.videophy = VideoS7HDMIPHY(platform.request("hdmi_out"),
                                           clock_domain="hdmi")

            # Phase 9.2: DDR-backed framebuffer. Replaces add_video_terminal.
            #
            # VTG (in hdmi clock domain) generates the 640x480@60Hz timing.
            # WishboneVideoFrameBuffer DMA-reads pixels from main_ram via a
            # wishbone master attached to the SoC bus, crosses sys→hdmi, and
            # streams {hsync,vsync,de,r,g,b} into the HDMI PHY sink.
            #
            # Framebuffer memory: 640 * 480 * 4 = 1,228,800 bytes at
            # NaxRiscv 0x48000000 (PS DDR 0x08100000 via GP0 remapper).
            # LiteX BIOS (main.c) expects the VTG to be registered as
            # `video_framebuffer_vtg` so it can poke the enable CSR by that
            # name. We match that convention.
            video_vtg = VideoTimingGenerator(default_video_timings="640x480@60Hz")
            video_vtg = ClockDomainsRenamer("hdmi")(video_vtg)
            self.add_module(name="video_framebuffer_vtg", module=video_vtg)

            # Wishbone master interface the framebuffer drives. 32-bit data /
            # 32-bit address matches our main_ram bus.
            fb_bus = wishbone.Interface(data_width=32, address_width=32)
            self.bus.add_master(name="video_fb", master=fb_bus)

            # fifo_depth 256 (1 KiB @ 32 bits) keeps LUT pressure modest so
            # timing closes at 100 MHz sys_clk. At 37 MB/s pixel rate that
            # covers ~27 µs of bus stalls — plenty for short GP0 arbitration
            # bubbles. default_enable=0 so the DMA master stays silent until
            # Linux explicitly enables it (avoids BIOS/kernel boot contention).
            self.video_framebuffer = WishboneVideoFrameBuffer(
                bus            = fb_bus,
                hres           = FRAMEBUFFER_HRES,
                vres           = FRAMEBUFFER_VRES,
                base           = FRAMEBUFFER_BASE,
                fifo_depth     = 256,
                clock_domain   = "hdmi",
                default_enable = 0,
            )

            # VTG → FB → PHY
            self.comb += video_vtg.source.connect(self.video_framebuffer.vtg_sink)
            self.comb += self.video_framebuffer.source.connect(self.videophy.sink)

            # NOTE: don't add `VIDEO_FRAMEBUFFER_*` constants — LiteX's CSR
            # generator already emits `video_framebuffer_base_read()` etc.
            # in csr.h from the module's CSRs, and duplicate constants would
            # clash (soc.h vs csr.h). The CSRs carry the same info at runtime.

            # False-path constraints between sys and hdmi clock domains.
            #
            # Putting set_clock_groups in the main XDC doesn't work on this
            # Zynq build: LiteX marks that XDC as PROCESSING_ORDER EARLY so
            # IO LOC/IOSTANDARD constraints apply pre-synthesis — but that
            # also forces the clock-group constraint to evaluate before the
            # Zynq IP generates `clk_fpga_0` or the MMCM elaborates its
            # output clocks, so every `get_clocks` lookup finds an empty set
            # and Vivado emits CRITICAL WARNING 12-4739. Result: the
            # constraint is silently ignored and the sys↔hdmi cross-domain
            # paths stay reported as synchronous timing violations.
            #
            # Fix: inject the set_clock_groups via the Vivado `pre_placement`
            # hook, which runs after synthesis + opt, at which point the
            # generated clocks exist.
            # Actual post-synth clock names (from synth timing report):
            #   clk_fpga_0            -> Zynq PS FCLK0 (sys, 100 MHz)
            #   clkout0               -> HDMI pixel (25.175 MHz)
            #   clkout1               -> HDMI 5x serializer (125.875 MHz)
            # Brace-escape is 4× because this string is format()'d once
            # by LiteX (unescapes {{..}} to {..}) then passed as TCL code,
            # which requires a single brace pair.
            platform.toolchain.pre_placement_commands.add(
                "set_clock_groups -asynchronous "
                "-group [get_clocks clk_fpga_0] "
                "-group [get_clocks -include_generated_clocks -of_objects [get_pins -hierarchical -filter {{NAME =~ */MMCME2_ADV/CLKOUT*}}]]"
            )

def main():
    from litex.build.parser import LiteXArgumentParser

    parser = LiteXArgumentParser(platform=hamgeek_rk7020f.Platform,
        description="ATOMiK NaxRiscv RV64 SoC on HamGeek RK-ZYNQ7020-F")
    parser.add_target_argument("--sys-clk-freq", default=100e6, type=float)
    parser.add_target_argument("--with-video-framebuffer", action="store_true",
                               help="Enable HDMI framebuffer output")
    parser.add_target_argument("--with-video-phy-only", action="store_true",
                               help="Enable HDMI PLL/PHY/VTG but skip the FB bus master "
                                    "(cheap confirmation build for arbiter hypothesis)")
    parser.add_target_argument("--with-video-framebuffer-hp", action="store_true",
                               help="Enable HDMI framebuffer via AXI HP0 (Path B — "
                                    "dedicated DMA master, no SoC bus arbiter)")
    args = parser.parse_args()

    # Force NaxRiscv config AFTER parse_args (which resets class attrs).
    # All attributes that NaxRiscv.args() normally sets must be replicated
    # here because the parser doesn't register NaxRiscv's args (it defaults
    # to VexRiscv until kwargs["cpu_type"] overrides in BaseSoC.__init__).
    NaxRiscv.xlen                 = 64
    NaxRiscv.data_width           = 64
    NaxRiscv.no_netlist_cache     = True   # force regen — FPU/RVC not in cache hash
    NaxRiscv.update_repo          = "no"  # don't git pull NaxRiscv repo
    NaxRiscv.with_fpu             = True
    NaxRiscv.with_rvc             = True
    NaxRiscv.jtag_tap             = False
    NaxRiscv.jtag_instruction     = False
    NaxRiscv.with_dma             = False
    NaxRiscv.l2_bytes             = 0           # L2 disabled (DDR diagnostic)
    NaxRiscv.l2_ways              = 4           # ignored when l2_bytes==0

    soc = BaseSoC(
        sys_clk_freq=args.sys_clk_freq,
        with_video_framebuffer=args.with_video_framebuffer,
        with_video_phy_only=args.with_video_phy_only,
        with_video_framebuffer_hp=args.with_video_framebuffer_hp,
        **parser.soc_argdict,
    )
    builder = Builder(soc, **parser.builder_argdict)
    if args.build:
        builder.build(**parser.toolchain_argdict)

if __name__ == "__main__":
    main()
