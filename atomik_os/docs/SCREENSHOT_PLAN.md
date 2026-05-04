# ATOMiK OS — Screenshot Plan

The HDMI output is on the board's 1920×1080 monitor. To put images in
the README we need to capture the framebuffer, transfer it to the
laptop, and convert to PNG.

---

## Approach A — board-side framebuffer dump

```sh
# On the board
dd if=/dev/fb0 of=/tmp/fb.raw bs=1M count=8        # 8.3 MB raw
gzip /tmp/fb.raw                                   # ~1.5 MB compressed
```

Then transfer `/tmp/fb.raw.gz` via base64-over-UART (~4 min at the
current deploy speed) and convert on the laptop:

```py
# On the laptop
import zlib, sys
from PIL import Image
raw = zlib.decompress(open('fb.raw.gz', 'rb').read(), 16+zlib.MAX_WBITS)
img = Image.frombytes('RGBA', (1920, 1080), raw, 'raw', 'BGRA')
img.save('atomik_os_v0_10.png')
```

This is the right approach for committed-to-repo screenshots.

---

## Approach B — DSLR / phone photograph of the monitor

Quick. Good enough for slides. Bad for README clarity (moiré, glare).

---

## Approach C — write a board-side tool: `fb2png`

A small RV64 binary on the board that mmap's `/dev/fb0` and writes a
PNG via libpng. Cleanest output, no transfer math. Adds libpng as a
build dep on the board.

Recommended sequence: ship `fb2png` in atomik_os v0.11.

---

## Shots to capture

### v0.10 hero shots (priority)

1. **`docs/screenshots/01_desktop.png`** — empty desktop with wallpaper
   + dock + status bar. Caption: *"ATOMiK OS — universal frame, before
   any app opens"*

2. **`docs/screenshots/02_about.png`** — About app open. Caption:
   *"System apps as reference primitives"*

3. **`docs/screenshots/03_monitor.png`** — Monitor app showing live
   ATOMiK adapter slot values with bars + change indicators. Caption:
   *"Live hardware introspection via /dev/mem"*

4. **`docs/screenshots/04_dock_predict.png`** — Dock with the predicted
   icon at left + pulsing cyan dot. Caption: *"Adaptive dock: agent
   sorts by frequency × recency × Markov transition"*

5. **`docs/screenshots/05_document_calendar.png`** — Document app, user
   has typed `load calendar` — left pane shows GRID primitive, right
   pane shows the chat history. Caption: *"One window. Type 'load
   calendar' → it becomes a calendar."*

6. **`docs/screenshots/06_document_tasks.png`** — Same Document, after
   `load tasks` — same window, now LIST primitive. Caption: *"Same
   window. Different intent."*

7. **`docs/screenshots/07_document_morph_3.png`** — A third
   transformation (FEED for code review, CARD for brief, or a custom
   one). Caption: *"All five primitives accessible from one app."*

8. **`docs/screenshots/08_terminal.png`** — Terminal app showing a
   real shell. Caption: *"Forkpty terminal — real /bin/sh"*

9. **`docs/screenshots/09_files.png`** — Files browser at `/`.
   Caption: *"Files — read-only navigator (v0.6)"*

10. **`docs/screenshots/10_notes.png`** — Notes app with some text +
    "saved" indicator. Caption: *"Notes — persistent local text"*

### v0.11+ aspirational

11. **`docs/screenshots/11_token_meter.png`** — Document with an LLM
    response and a visible token count + cost. Caption: *"Pay per
    token, not per month"*

12. **`docs/screenshots/12_speech.png`** — "Hold to talk" indicator on
    the chat panel. Caption: *"Speech input — same delta-stream
    underneath"*

---

## Where they go

Commit screenshots to `atomik_os/docs/screenshots/` (NEW directory).
Reference them from the top-level `README.md`. Resize to 1280px wide
for the README; keep originals at 1920×1080 in
`atomik_os/docs/screenshots/raw/` if disk allows.
