# ATOMiK OS

The first delta-state desktop operating system. One compiled UI frame.
Every app is a stream of typed field deltas. The desktop IS the
canonical ATOMiK workload.

> **One window. Type *"load calendar"* — it becomes a calendar. Type
> *"load tasks"* — same window, now a task list. Then *"set primitive
> feed"* — it becomes a feed. No app installs. No app store. Just
> intent → field deltas → invariant frame.**

---

## What this is

ATOMiK OS reduces the entire desktop computing stack to one equation:

```
visible_state = invariant_frame ⊕ Σ field_deltas
```

which is exactly the ATOMiK delta-state algebra implemented in
hardware:

```
current_state = initial_state ⊕ accumulator
```

The OS is the product. The hardware is the substrate. Every other
desktop computing platform asks developers to ship native UIs per
platform. ATOMiK OS asks for a manifest + a stream of typed deltas and
generates the rest.

---

## Status

| Area | State |
|------|-------|
| Compositor (`/dev/fb0` mmap, double buffer, VTG/DMA) | ✅ shipped |
| Drawing primitives (rect / gradient / rounded-rect-AA / blend) | ✅ shipped |
| Font rendering (8×16 VGA, scaled) | ✅ shipped (Unicode pass = v0.7+) |
| Window manager (stack / focus / fade-in tween / close button) | ✅ shipped |
| Adaptive dock (frequency × recency × Markov) | ✅ shipped |
| Animation runtime (60 Hz frame loop, ease-out, pulse) | ✅ shipped |
| Toast notifications | ✅ shipped |
| Live status bar (CPU + uptime + agent prediction) | ✅ shipped |
| System apps: About, Monitor, Terminal, Files, Notes | ✅ shipped |
| Invariant-frame primitives: LIST / CARD / GRID / FEED / CONVO | ✅ shipped |
| 5 reference edge apps (Calendar, Tasks, Code, Brief, Chat) | ✅ shipped |
| **Document app — chat-driven UI morphing (the pitch)** | ✅ shipped (v0.10) |
| Document persistence | ✅ shipped (v0.10.1) |
| Field-delta wire format | ⏳ v0.11 |
| LLM-backed parser + token meter | ⏳ v0.12 |
| Multi-document workspace | ⏳ v0.13 |
| Speech input | ⏳ v0.14 |
| Token wallet UI | ⏳ v0.15 |
| Cross-device sync | ⏳ v1.0 |

Full roadmap: [`docs/TODO.md`](docs/TODO.md).

---

## Quick tour

| Key | Opens |
|-----|-------|
| `D` | **Document** — universal chat-driven UI |
| `A` | About |
| `M` | Monitor (live ATOMiK adapter slots) |
| `T` | Terminal (`/bin/sh` over forkpty) |
| `F` | Files (read-only directory browser) |
| `N` | Notes (persistent text editor) |
| `C` `K` `G` `B` `H` | Reference edge apps: Calendar / Tasks / Code / Brief / Chat |
| `1`–`6` | Launch whichever app is at that adaptive-dock slot |
| `Tab` | Cycle window focus |
| `Esc` | Close focused window |
| `Q` / Ctrl-C | Quit OS |

Inside the Document, the chat panel takes commands:

```
load calendar
load tasks
set primitive grid
set accent cyan
set header "Inbox"
add "buy milk"
clear list
help
save
reset
```

Every command is a field-delta on the underlying `edge_app_t`
accumulator. Documents are persisted between launches.

---

## Source layout

| Path | Role |
|------|------|
| `Makefile`              | cross-compile to `riscv64-linux-gnu-` |
| `deploy.py`             | base64-over-UART transfer + nohup launch |
| `include/atomik_os.h`   | public types + APIs (single header) |
| `src/main.c`            | event loop + app dispatch + frame poller |
| `src/fb.c`              | `/dev/fb0` mmap, present, scan-out enable |
| `src/draw.c`            | rect / gradient / rounded rect (corner-AA) / blend |
| `src/font.c`            | 8×16 VGA font, scaled |
| `src/wallpaper.c`       | gradient + accent vignette + wordmark, cached |
| `src/wm.c`              | window stacking, focus, fade-in |
| `src/dock.c`            | adaptive dock (agent-driven sort + pulse) |
| `src/agent.c`           | freq + recency + Markov, persisted |
| `src/anim.c`            | monotonic clock, ease-out, frame liveness |
| `src/notify.c`          | toast notifications |
| `src/status.c`          | top status bar |
| `src/about.c`           | About app |
| `src/monitor.c`         | Live ATOMiK slot reader |
| `src/terminal.c`        | forkpty terminal |
| `src/files.c`           | Files browser |
| `src/notes.c`           | Notes editor |
| `src/atomik.c`          | `/dev/mem` ATOMiK adapter access |
| `src/eapp.c`            | edge_app_t + delta-apply primitives |
| `src/eapp_render.c`     | invariant-frame primitive renderer |
| `src/edge_demo.c`       | 5 reference edge apps as data only |
| `src/document.c`        | the **Document** app |
| `docs/TODO.md`          | canonical roadmap (every commit ticks a box) |
| `docs/ARCHITECTURE.md`  | technical architecture spec |
| `docs/BUSINESS_MODEL.md`| token-pay AI-app economics |
| `docs/SCREENSHOT_PLAN.md` | screenshot capture plan |

---

## Build + deploy

```sh
# from atomik_os/
make                              # cross-compile -> build/atomik_os
python3 deploy.py                 # base64-over-UART -> /tmp/atomik_os, launch
python3 deploy.py --no-launch     # transfer only
```

The board UART is `/dev/ttyUSB2` at 115200 baud. A 50 KB binary takes
~9 minutes to transfer with the v0.10 fast deploy (1024-char chunks,
0.8 ms per char). Real hardware (the planned ATOMiK laptop build) gets
a normal distribution path — UART is for the FPGA prototype only.

---

## Why this matters

**Apps don't ship UI code.** Microsoft asks for Win32. Apple asks for
AppKit. The web asks for HTML+JS. ATOMiK OS asks for a manifest and a
stream of typed deltas — and the algebra of that delta is the same
algebra the silicon implements. Nobody else can do this because nobody
else has the substrate.

**The compute model collapses.** Drawing a frame is a memcpy.
Animating is a pulse on a per-pixel ease curve. Streaming an app to a
new device = sending the manifest delta. Bandwidth is bytes/s, not
MB/s.

**The economics flip.** AI-mediated commands are token-priced. You pay
for the work the agent did, not for a monthly seat-license that
amortizes the heavy users onto you. See
[BUSINESS_MODEL.md](docs/BUSINESS_MODEL.md).

**Privacy is the cheapest path.** Local primitives cost zero tokens.
Going to a cloud LLM is opt-in, audited, and previewed before commit.

This is the pitch. The desktop UI is the proof.
