# Two failure modes — need help breaking the deadlock

I've been alternating between two failure modes for ~10 Vivado builds. Each fix for one mode causes the other. I need a fresh perspective on the correct architecture.

## The two modes

### Mode A: "Coherent text, scrolling too fast" (v13, v13d)
- Text IS readable (user confirmed "I can definitely see coherent text")
- Tux penguin clear, boot messages recognizable
- But the image scrolls vertically in a rapid loop
- fetch_line is driven by the FSM: increments after each line fetch completes
- The fetch runs at its own pace (~64µs per line due to slow single-beat wishbone reads through Wishbone2AXI → AXI HP0)
- The fetch completes ~240 lines per 16.7ms frame, then wraps to 0 → the displayed content cycles through the framebuffer repeatedly → scrolling

### Mode B: "Illegible blocks, stable" (v10f, v12, v13b, v13c)
- Image is stable (not scrolling)
- Tux penguin recognizable but blocky
- Text is NOT readable — appears as garbled blocks
- fetch_line is driven by new_line events (hsync CDC) — paced to display
- But each fetch takes ~2 line periods → fetch can't keep up → lines are skipped or addresses shift mid-fetch → garbled

## Why I keep alternating

- **FSM-driven counter (Mode A)**: fetch reads lines 0, 1, 2, 3... sequentially at its own pace. Text is coherent because each line is fully and correctly fetched. But the counter wraps every ~240 lines while the display expects 480 → scrolling.

- **Display-paced counter (Mode B)**: fetch_line tracks the display via new_line (hsync). But the fetch takes 2× the line period → during one fetch, fetch_line advances by 2 → either the address shifts mid-fetch (v13b) or lines are skipped (blocky).

Adding vsync reset to Mode A (v13d) should stop the scrolling by resetting fetch_line to 0 each frame. But the scrolling persists — possibly because the vsync crossing delay means the reset fires too late, or the FSM is in FETCH when the reset should fire and misses it.

## Hardware constraints

- sys_clk = 100 MHz
- pixel_clk = 25.175 MHz (640x480@60Hz)
- Line period = 800 pixels × (1/25.175MHz) = 31.78µs = 3178 sys cycles
- Single-beat wishbone read through Wishbone2AXI → AXI HP0: ~10 sys cycles per word
- 640 words per line × 10 cycles = 6400 cycles = 64µs = **2× the line period**
- AXI burst reads (v11): ARREADY never accepted → black screen (separate bug, not solved)

## Scanline architecture (current)

```
HDMI domain (25.175 MHz):
  VTG → hcount → Mux(rd_page, hcount+640, hcount) → BRAM rd_port
  rd_port.dat_r → 1-cycle delay → r/g/b (gated by de_d1)
  Page swap: on line_ready_toggle change AND de rising edge (ownership handshake)

Sys domain (100 MHz):
  Fetch FSM: WAIT → FETCH 640 words → DONE → toggle line_ready → WAIT
  BRAM write: Mux(wr_page, word_idx+640, word_idx)
  fetch_line_latched snapshots fetch_line at WAIT→FETCH

Line buffer: Memory(32, 2*640) — 2 pages ping-pong
```

## What I think the real problem is

The fetch takes 2× the line period. No matter how I pace the counter:
- FSM-paced: coherent but scrolls (wraps too fast)
- Display-paced: stable but garbled (skips lines)

The fundamental issue is that I cannot fetch every line every frame at single-beat bandwidth. I need either:
1. Faster reads (burst AXI — but that's broken)
2. Accept half the vertical resolution (fetch every other line, double each)
3. A completely different approach

## What I need from ChatGPT

1. **Is there a counter/pacing scheme that gives coherent + stable text at half bandwidth?** E.g., fetch line 0, display it for 2 scan lines, then fetch line 1, display for 2 scan lines... effectively 640x240 rendered as 640x480 with line doubling. Would this produce readable text?

2. **Why does the vsync reset not stop the scrolling in v13d?** The fetch_line should reset to 0 on vsync. Is the vsync CDC crossing too slow? Is the FSM missing the reset because it's in FETCH state? Is there a race between the FSM incrementing and the vsync clearing?

3. **Should I just accept Mode A (scrolling) and find a way to slow it down to ~1 frame per wrap?** If the fetch did exactly 480 lines per frame period and then stopped until the next vsync, it would be frame-locked. The fetch does ~240 lines per frame → it wraps ~2× per frame → scrolls at 2× the frame rate. Could I add a "done for this frame, wait for vsync" state after fetch_line reaches 479?

4. **Is the burst AXI path salvageable?** If I could get 16-beat bursts working, each line takes ~800 cycles = 8µs — well under the 31.78µs line period. That eliminates the bandwidth problem entirely. The v11 attempt had ARREADY stuck — is that likely a signal I'm not driving, or a PS7 HP configuration issue?

## Files
- `hardware/zynq/litex/wb_framebuffer.py` — current module
- `hardware/zynq/litex/soc_nax64_atomik.py` — SoC integration
