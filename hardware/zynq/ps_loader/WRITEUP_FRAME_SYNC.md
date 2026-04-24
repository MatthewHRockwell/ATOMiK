# Frame sync problem — DDR framebuffer shows scrambled pixels, not readable text

Asking for help. I've been iterating on this for multiple builds and keep getting the same result. The data path works (correct colors proven), fbcon is writing text to DDR, but the display is scrambled noise because the DMA scan and VTG scan aren't synchronized.

## What works (proven on hardware)

1. **Pixel data path end-to-end**: `fb_test red` fills DDR at 0x48000000 with 0x00FF0000, HDMI shows RED. Same for green, blue, white. Colors are correct. Pixel unpack R=data[16:24], G=data[8:16], B=data[0:8] with `endianness="big"` (no byte-swap) in WishboneDMAReader.

2. **simplefb + fbcon**: kernel probes simplefb from DTS, creates /dev/fb0, fbcon switches to "colour frame buffer device 80x30." Confirmed fbcon writes real pixel data: `devmem 0x4803E800` (row 100) reads `0x00AAAAAA` (gray text color). First rows are 0x00000000 (black background). Text IS in DDR.

3. **DMA running**: DMA offset CSR advances continuously. Not stuck.

4. **VTG + PHY**: HDMI monitor syncs at 640x480@60Hz. `stream.connect()` proven to produce valid sync (phy-only build showed synced black screen, baseline `add_video_terminal` shows readable text).

5. **Architecture**: DMA reads PS DDR 0x08100000 via AXI HP0 (dedicated, not on SoC wishbone bus). VTG timing flows unconditionally to PHY via `stream.connect(keep={"valid","ready","last","de","hsync","vsync"})`. Pixel data from CDC overrides r/g/b when `de_active & pixel_src.valid`.

## What fails

HDMI shows scrambled white/colored pixelation, not readable text.

I still think this is a frame-alignment bug, but the current code suggests a
more specific root cause than "MultiReg missed vsync":

1. The current module **removed LiteX's upstream frame-boundary FSM** and
   replaced it with unconditional VTG→PHY timing plus opportunistic r/g/b
   override when pixel data happens to be available.
2. The DMA address generator can be reset back to offset 0, but the already
   queued data in the DMA's internal FIFO and the sys→hdmi CDC FIFO is **not**
   reset by that action.
3. So the design currently has **no guaranteed relationship** between:
   - "first active pixel of a display frame" and
   - "word 0 of the framebuffer"
4. Because the DMA free-runs much faster than the pixel sink and only stalls
   when buffers fill, the pipeline naturally accumulates prefetched pixels and
   then resumes from wherever it stalled. Without explicit frame-boundary logic
   using `last` / `first` or explicit FIFO flush, the displayed image becomes a
   rotated slice of the real framebuffer.

So the stronger claim is:

> The bug is probably **not** "vsync level failed to cross domains." The bug is
> that the current port threw away the upstream frame-sync contract, and
> resetting only the DMA FSM does not reset the buffered pixel stream.

## What I've tried

### Attempt 1: vsync rising-edge pulse via MultiReg (HP v6)

```python
vsync_r = Signal()
vsync_rise = Signal()
hdmi_sync += vsync_r.eq(source.vsync)
self.comb += vsync_rise.eq(source.vsync & ~vsync_r)

vsync_sys = Signal()
self.specials += MultiReg(vsync_rise, vsync_sys, "sys")
self.comb += self.dma.fsm.reset.eq(vsync_sys)
```

**Result**: Same scrambled display.

That failure is expected. A single hdmi-cycle pulse is the wrong thing to feed
through `MultiReg`.

### Attempt 2: vsync level hold via MultiReg (HP v7)

```python
vsync_level_sys = Signal()
self.specials += MultiReg(source.vsync, vsync_level_sys, "sys")
self.comb += self.dma.fsm.reset.eq(vsync_level_sys)
```

Theory: vsync is HIGH for ~63µs (2 scanlines × 800 pixels × 39.7ns), so
crossing the LEVEL should be reliable.

**Result**: Same scrambled display.

That does **not** mean the vsync level failed to cross. More likely:

1. Resetting `self.dma.fsm` only resets the **address generator**, not the
   data already sitting in `WishboneDMAReader.fifo` and not the data already
   sitting in `stream.ClockDomainCrossing`.
2. The design still has no frame-boundary FSM in the hdmi domain, so there is
   nothing forcing the first active pixel of a frame to correspond to the first
   framebuffer word.
3. The reset wiring itself is suspect if multiple combinatorial drivers are
   assigned to the same signal.

## Current Migen code for pixel consumption

```python
# VTG timing flows straight through to PHY
self.comb += vtg_sink.connect(source,
    keep={"valid", "ready", "last", "de", "hsync", "vsync"})

# Consume pixel data from CDC during active video
de_active = Signal()
self.comb += de_active.eq(vtg_sink.valid & vtg_sink.ready & source.de)
self.comb += pixel_src.ready.eq(de_active & pixel_src.valid)

# Override r/g/b when data available
self.comb += [
    If(de_active & pixel_src.valid,
        source.r.eq(pixel_src.data[16:24]),
        source.g.eq(pixel_src.data[ 8:16]),
        source.b.eq(pixel_src.data[ 0: 8]),
    ),
]
```

## Key hardware details

- sys_clk = 100 MHz (Zynq PS FCLK0)
- pixel_clk = 25.175 MHz (MMCM output, hdmi domain)
- hdmi5x_clk = 125.875 MHz (TMDS serializer)
- DMA: WishboneDMAReader → private wishbone → Wishbone2AXI → S_AXI_HP0 → PS DDR
- CDC: `stream.ClockDomainCrossing([("data", 32)], cd_from="sys", cd_to="hdmi")`
- VTG: LiteX `VideoTimingGenerator` in hdmi domain, `source.valid` always 1 in RUN
- PHY: `VideoS7HDMIPHY`, `sink.ready` always 1, reads de/hsync/vsync/r/g/b every hdmi cycle
- DMA FSM reset / enable gating is currently implemented by multiple comb
  assignments across module layers; this should **not** be assumed to mean
  "implicit OR."

## Important code facts from LiteX and this port

1. `WishboneDMAReader` already creates:

   ```python
   self.comb += fsm.reset.eq(~self.enable)
   ```

   and `add_csr()` also does:

   ```python
   self.comb += self.enable.eq(self._enable.storage)
   ```

   So adding another plain `self.comb += fsm.reset.eq(...)` or
   `self.comb += self.enable.eq(...)` later is **not** some clean automatic OR.
   It is a second driver on the same signal and must be treated carefully.

2. `WishboneDMAReader` has an internal `SyncFIFO`, and the sys→hdmi path adds
   a separate `stream.ClockDomainCrossing` FIFO. Resetting the DMA FSM does not
   automatically flush either queued pixel stream.

3. The upstream LiteX `VideoFrameBuffer` does **not** just free-run the DMA and
   hope that `de` consumption stays aligned. It keeps an hdmi-domain FSM that:

   - waits in `SYNC`
   - starts `RUN` only at a frame boundary
   - consumes video data during active pixels
   - watches `video_pipe_source.last`
   - returns to `SYNC` at DMA frame end

   That upstream design keeps a real frame contract. My current port removed
   that contract and replaced it with:

   - unconditional VTG→PHY timing flow
   - no `first` / `last` tracking
   - no hdmi-domain sync FSM
   - no FIFO flush at frame boundary

## What I think is actually wrong now

1. **The main bug is architectural, not just CDC pulse capture.**
   The current module has no mechanism that says "top-left display pixel must
   come from framebuffer word 0."

2. **Resetting the DMA FSM is not sufficient.**
   Even if vsync crosses perfectly, stale words already buffered in:
   - `WishboneDMAReader.fifo`
   - `stream.ClockDomainCrossing`
   will still come out first.

3. **The current active-video consumer does not restore frame lock.**
   It only says "if data exists during `de`, consume it." That is compatible
   with a permanently phase-shifted stream.

4. **The upstream LiteX synchronization model is probably the correct fix.**
   The likely right port is:
   - keep `WishboneDMAReader`
   - keep HP0 transport
   - restore the upstream hdmi-domain `SYNC/RUN` FSM
   - use `video_pipe_source.last` as the end-of-frame marker
   - only add extra reset/flush if the two FIFOs still preserve stale data

## What I need help confirming

1. **Is the right fix simply to port LiteX's upstream `VideoFrameBuffer`
   sync FSM almost verbatim, replacing only the DMA backend?**
   That looks more plausible now than trying to bolt vsync-reset onto the
   simplified design.

2. **For Migen/LiteX: what is the cleanest way to combine DMA enable/reset with
   any extra frame-sync gating?**
   I do **not** want to rely on multiple comb drivers to the same signal.

3. **Do I need explicit FIFO flush/reset at frame boundary, or is restoring the
   upstream `SYNC/RUN` + `last` handling enough?**
   My suspicion is:
   - address-generator reset alone: not enough
   - upstream sync FSM: likely necessary
   - explicit FIFO flush: maybe only needed if stale data still leaks through

4. **Is there any reason `last` would fail to propagate correctly through this
   path?**
   Path is:
   - `WishboneDMAReader` internal FIFO
   - `stream.ClockDomainCrossing`
   - hdmi-domain video pipeline

5. **Is my current "scrambled noise = drift" interpretation too generous?**
   The simpler and more accurate statement may be:
   - the framebuffer contents are correct
   - the scan timing is correct
   - the pipeline currently lacks a deterministic frame-start contract
   - therefore displayed pixels are phase-rotated / stale-buffered, not
     necessarily "drifting because vsync reset failed"

## Files

- `hardware/zynq/litex/wb_framebuffer.py` — WishboneVideoFrameBuffer (the module)
- `hardware/zynq/litex/soc_nax64_atomik.py` — SoC integration (under `--with-video-framebuffer-hp`)
- `hardware/zynq/litex-build-nax64-fb-hp/` — latest bitstream + reports
