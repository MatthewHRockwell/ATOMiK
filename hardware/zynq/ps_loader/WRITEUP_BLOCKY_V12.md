# Scanline framebuffer v12: "blocky" text — Cat() fix confirmed, bandwidth suspected

## Current state

v12 shows the same "blocky" image as v10f — Tux penguin visible, text shapes recognizable, but characters appear doubled/stretched and are not legible. This is WITH the Mux() addressing fix (confirmed Cat() bug from earlier builds is gone — no more diagonal shear).

So: the Cat() fix eliminated the diagonal shear, but the remaining "blocky" is a separate issue. The image has correct structure (Tux in right place, text in right area) but insufficient detail to read.

## Architecture (v12)

```
HDMI domain (25.175 MHz):
  VTG → hcount → Mux(rd_page, hcount+640, hcount) → BRAM rd_port → 1clk → pixels
  Page swap: only at de rising edge AND line_ready_toggle changed (ownership handshake)

Sys domain (100 MHz):  
  Fetch FSM: WAIT → (new_line event) → FETCH vcount_sys+1 → DONE → toggle line_ready
  640 single-beat wishbone reads per line through Wishbone2AXI → AXI HP0 → PS DDR
```

## Bandwidth analysis

Each wishbone read goes through: WB → Wishbone2AXILite → AXILite2AXI → S_AXI_HP0.

From earlier observation (v10e DMA offset data), the effective single-beat read rate is approximately 5-10 sys cycles per word. At the pessimistic end:

- 640 words × 10 cycles = 6400 cycles = 64µs per line
- Line period = 31.78µs (800 pixels × 39.72ns)
- Fetch takes **2× the line period**

This means: the fetch FSM can only complete one line every ~2 line periods. With the ownership handshake, lines where the fetch hasn't completed yet repeat the previous line's data. Every other line is a duplicate → image appears vertically doubled → "blocky."

## What I tried that didn't work

**AXI burst reads (v11)**: Replaced Wishbone2AXI with a direct AXI3 burst reader (40 × 16-beat bursts per line). Should be ~800 cycles = 8µs per line. But the screen was completely BLACK — the burst reader FSM is stuck (AR probably never accepted by HP0). I couldn't debug this without AXI protocol visibility. The AXI signals looked correct in the generated Verilog.

## What I need help with

1. **Is the "blocky = line repetition from slow fetch" diagnosis correct?**

2. **Why would AXI HP0 reject burst read requests?** The AR channel had:
   - ARADDR = valid DDR address
   - ARBURST = 1 (INCR)
   - ARLEN = 15 (16 beats)
   - ARSIZE = 2 (4 bytes)
   - ARVALID = 1
   But ARREADY apparently never went high (FSM stuck in AR state → black screen).

3. **Is there a simpler way to get burst-like bandwidth through WishboneToAXI?** Could I pipeline the wishbone requests (assert STB before the previous ACK returns)?

4. **Alternative: could I use 64-bit HP (data_width=64) to halve the transaction count?** Would LiteX's Wishbone2AXI handle the 32→64 bit width mismatch automatically?

5. **Or should I just accept the blocky quality for Phase 9.3?** The text IS there — just doubled vertically. Is there a way to make it readable at this quality (e.g., use a larger font so the doubling doesn't obscure characters)?

## Key files

- `hardware/zynq/litex/wb_framebuffer.py` — current scanline FB (v12, wishbone single-beat)
- `hardware/zynq/litex/soc_nax64_atomik.py` — SoC integration
- Previous AXI burst attempt visible in git (v11 code)
