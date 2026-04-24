# Scanline framebuffer: image visible but horizontally scrambled

Screenshot: `/home/mattrock/Projects/ATOMiK/reference-documents-tmp/processed-0798E799-1848-4B6C-88EB-85DD12A513FC.jpeg`

## What the screenshot shows

- Tux penguin in top-left corner — recognizable shape and colors but fuzzy
- Boot text fill the screen — white-on-black, correct vertical placement
- Every line is **horizontally displaced/sheared** — text is illegible
- The displacement appears to increase progressively down the screen, creating a diagonal shear effect
- Some colored artifacts at screen edges

## Architecture

Scanline-based framebuffer with dual-port BRAM ping-pong:

```
HDMI domain (25.175 MHz):
  VTG generates hcount/vcount/de/hsync/vsync
  rd_port.adr = Cat(hcount[0:10], rd_page)
  rd_port.dat_r → (1 clk delay) → source.r/g/b (gated by de_d1)
  Page swap: rd_page toggles on hsync rising edge

Sys domain (100 MHz):
  Fetch FSM: on new_line event → fetch vcount_sys+1 from DDR
    bus.adr = (base + fetch_line * 2560) / 4 + word_idx
    wr_port.adr = Cat(word_idx, wr_page)  where wr_page = ~rd_page_sys
  Pacing: waits for new_line_pending, fetches 640 words, clears pending

CDC:
  vcount_hdmi → MultiReg → vcount_sys
  hsync toggle → MultiReg → new_line pulse in sys
  rd_page → MultiReg → rd_page_sys → wr_page = ~rd_page_sys
```

BRAM: `Memory(32, 2 * 640)` — 2 pages of 640 words each.

## What I think is wrong

The **diagonal shear** (progressive horizontal offset per line) is the signature
of the fetch and display operating at slightly different line rates with no
per-line realignment of hcount↔word_idx.

Specifically: the fetch writes word_idx 0-639 to BRAM addresses 0-639 (+ page
offset). The VTG reads using hcount 0-639. These SHOULD align (word 0 = pixel 0).
But if hcount doesn't start at 0 at the same moment the fetch starts writing
word 0 — even by a few pixels — the entire line shifts horizontally.

The per-line page swap at hsync is supposed to synchronize them, but the CDC
delay between hsync (hdmi domain) and new_line (sys domain) means the fetch
starts writing the new line several sys cycles after hsync actually fired. During
those cycles, the VTG has already advanced past hcount=0 on the new line. The
VTG reads the first few pixels of the new line from the OLD page (which has the
previous line's data at those addresses). This creates a small horizontal shift.

Over 480 lines, if each line's fetch starts a few pixels later than ideal, the
accumulated shift creates the diagonal shear visible in the screenshot.

## What I need

1. Is the diagonal-shear diagnosis correct? (progressive per-line hcount↔word_idx misalignment due to CDC latency on the page swap)

2. What's the correct way to ensure hcount=0 reads word_idx=0 every line?

3. Should I abandon the hsync-triggered page swap entirely and instead:
   - Have the fetch FSM signal "line complete" to hdmi domain
   - Hdmi domain swaps the page at the start of horizontal blanking (de falling edge)
   - This guarantees all 640 active pixels were read from the correct page

4. Or is the whole ping-pong approach wrong for single-beat AXI HP reads, and
   I need a fundamentally different buffering strategy?

## Files

- `hardware/zynq/litex/wb_framebuffer.py` — current scanline FB module
- `hardware/zynq/litex/soc_nax64_atomik.py` — SoC integration
- Screenshot at path above
