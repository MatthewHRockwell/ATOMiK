# ATOMiK Desk — Claude Code Resume Prompt

**Use this file to resume work after a laptop restart.**
Paste the contents into a fresh Claude Code session, or just say:
"Read /home/mattrock/Projects/ATOMiK/atomik_os/docs/CLAUDE_RESUME.md and pick up where we left off."

---

## WHERE WE ARE

**Latest commit: `b6b8e92` — v0.39-K: Top Rail De-noise (screenshot)**

Working directory: `/home/mattrock/Projects/ATOMiK/atomik_os`
Branch: `main`

We are in the middle of a visual polish sprint (v0.38–v0.39 arc) for
ATOMiK Desk, a graphical OS shell running on a Xilinx Zynq XC7Z020
FPGA board (ALINX AX7020, $200 board — never say "cheap").  The OS
renders directly to /dev/fb0 (1920×1080 XRGB8888 simplefb) from a
single C process.  No compositor, no GPU, no X11.

---

## LAST SESSION SUMMARY

We have been doing ChatGPT-directed visual fidelity passes:
implement a slice → deploy to board → SIGSTOP+dd the framebuffer →
chunked-pull via UART → PIL convert to PNG → send to ChatGPT for
review → implement next slice.

**Slices shipped:**
- v0.39-C: Atom sprite states (mode-tinted aura halo)
- v0.39-D: Atom dirty-rect + SYNC clause + source-aware titles
- v0.39-E: SUCCESS emerald rim + global capture hotkeys Ctrl+E/Ctrl+G
- v0.39-F: Cap Rail line icons + per-cell labels (72×72→80×100 cells)
- v0.39-G: Cap Rail SYSTEM header + Atom divider
- v0.39-H: Pulse Bar unified metric + glow waveform (well 128→200 px)
- v0.39-I: Pulse Bar semantic alignment (perf_last_for vs perf_last_sample)
- v0.39-J: Hero Label Lift — 3-layer semantic halo + sublabel contrast

**FINAL scores — v0.39-K arc COMPLETE (2026-05-21):**

| Surface     | Score | Status |
|-------------|-------|--------|
| Pulse Bar   | 94    | DONE   |
| Cap Rail    | 93    | DONE   |
| Hero        | 93    | DONE   |
| Fabric      | 93    | DONE   |
| Background  | 94    | DONE   |
| Typography  | 94    | DONE   |
| Composition | 95    | DONE   |
| Atom        | 95    | DONE   |
| **Overall** | **95** | **TARGET HIT** |

**Visual polish arc v0.38→v0.39-K is CLOSED. Do not start v0.39-L.**

---

## KEY TECHNICAL CONTEXT

### ATOMiK architecture
Three personalities: STATE (coalesces writes), SYNC (propagates
deltas to replica), AGENT (retains hot context).  Not a traditional
desktop — a live demo of the hardware delta-state architecture.

### Color grammar (NEVER violate)
- Cyan `ATOMIK_SEM_HARDWARE` — hardware / STATE
- Green `ATOMIK_SEM_SAVINGS` — savings / SYNC
- Violet `ATOMIK_SEM_AGENT` — agent / AGENT
- Amber `ATOMIK_SEM_WASTE` — contention
- Dim slate `ATOMIK_FG_DIM` — idle / decorative

### Class discipline (HARD RULES)
- **Class A**: every visible number from a real producer (perf_last_for,
  metric_get, etc.)
- **Class B**: decorative chrome (waveforms, halos)
- **Class C FORBIDDEN**: fake numbers, placeholder stats

### Layered-stroke doctrine
No naked 1 px lines.  Every stroke carries an alpha halo (90 per side
for 1 px, 8/4/2 stack for heavier lines).

### Font atlas
- atomik_14 → `FONT_AA_LABEL`
- atomik_18 → `FONT_AA_UI`
- atomik_28 → `FONT_AA_DISPLAY`
- atomik_36 → `FONT_AA_BRAND`

### Key source files
- `src/status.c` — Pulse Bar (dev + investor paths)
- `src/dock.c` — Capability Rail
- `src/hero.c` — Hero (central triad + orbital core)
- `src/fabric.c` — Resource Fabric (5-lane panel)
- `src/assistant.c` — Atom assistant overlay
- `include/atomik_os.h` — version, constants, all public APIs

---

## DEPLOY PIPELINE

Board: ALINX AX7020 (XC7Z020-2CLG484-2), UART `/dev/ttyUSB2` @ 115200.
UART has known byte-duplication bug (~1 in 28k chars).

**Typical cycle (~30–50 min total):**
1. `make clean && make` in `atomik_os/` (cross-compiler: riscv64-linux-gnu-gcc)
2. Chunked gzip+base64 upload via `deploy_v039x.py` (2ms/char, 50 chunks × 2048)
3. md5sum verify on board with silence-detect (NOT cmd_capture — UART dup)
4. Launch via `/tmp/launch_aos.sh` written line-by-line
5. Verify `/tmp/atomik_os_version` == expected version
6. `kill -STOP $(pgrep atomik_os); dd if=/dev/fb0 of=/tmp/aos_rail.raw bs=8294400 count=1`
   (**Note: fb2png / mmap BROKEN on simplefb — always use dd**)
7. Chunked pull with control-byte sentinels (\x01..\x02) + per-chunk md5
8. Host-side: PIL converts BGRA raw → RGB PNG

**Capture hotkeys (always-global, bypass focused-window dispatch):**
- `printf '\x05' > /tmp/aos_keys` → Ctrl+E → summon Atom EXPLAIN (cyan)
- `printf '\x07' > /tmp/aos_keys` → Ctrl+G → force SUCCESS halo (emerald rim)
- `printf '\033' > /tmp/aos_keys` → Esc → dismiss Atom
- `printf 'I' > /tmp/aos_keys` → 'I' hotkey (only works when no app is focused)

**Scripts available in /tmp:**
- `/tmp/deploy_v039i.py` — template for next deploy (update version/raw-name)
- `/tmp/pull_v039i.py` — template for next pull
- `/tmp/pull_v039d.py` — full EXPLAIN+SUCCESS two-frame pull template

---

## WHAT TO DO NEXT

Visual polish arc is **closed at v0.39-K / Overall 95**.

Next work should be live demo substance — pick one:
- **Workload switching** — keyboard-driven personality cycling with visible hardware state change
- **Board-local control** — no laptop, standalone demo flow
- **New Class A surface** — a new panel or lane that proves the delta-state architecture in a way the current surfaces don't
- **SD/QSPI persistent boot** — eliminate the 3-5 min cold-boot recovery ritual

Do NOT start cosmetic v0.39-L work. ChatGPT directive: "additional micro-polish risks turning into churn."

## DEPLOY NOTES (post v0.38-D hardening)

Known issue: `deploy.py`'s bare `slow()` nohup launch line fails ~50% of the time due to UART byte duplication.
Workaround: use `/tmp/launch_v039j_fixed.py` (update EXPECTED version) + `/tmp/shot_v039j.py` (update output filename).
These use `sh /tmp/launch.sh` (short, safe) + deploy.py's proven `cmd_capture` for pulls.

---

## MEMORY FILES
Saved at: `/home/mattrock/.claude/projects/-home-mattrock-Projects-ATOMiK/memory/`
Key files: `MEMORY.md` (index), `project_v039_*_spec.md`, `project_v039_*_verdict.md`
