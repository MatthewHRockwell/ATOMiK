---
name: ATOMiK OS — agentic, adaptive desktop with dynamic vprocs
description: Design constraints for the ATOMiK OS desktop. NemoClaw-style agentic adaptation, plus future dynamic virtual CPUs/GPUs from delta cells.
type: project
originSessionId: 0b1da619-fb37-4461-99b3-200ab2dbce84
---
The ATOMiK OS is the centerpiece of the project — a Windows/macOS-competitive desktop UI/UX that runs on the AX7020 reference hardware short-term and the planned ATOMiK laptop long-term. Apps are downloadable + installable + accelerated by ATOMiK delta-state hardware.

## Core design pillars

**1. Agentic, adaptive at every layer.** Approach matches NemoClaw / OpenClaw direction: the OS itself is an agent, not a passive shell. The system learns the user's habits and adapts proactively — at the OS level (which apps to surface, when), at the workspace level (which workflow context the user is in right now), and down to individual application UIs (recommended actions, predicted inputs). Think Netflix-style recommendations but applied across the entire desktop experience, not just a single app.

**2. User-data-first.** Local user behavior data is a first-class input to every UI decision. Privacy stays local — adaptation happens on-device using ATOMiK delta-state primitives, not cloud-side. Delta-state is naturally suited to learning from user actions: every click, every keystroke, every workflow transition is a delta the OS can accumulate and reason over.

**3. Dynamic virtual processors (PARKED for now).** ATOMiK's architecture (8+ independent delta accumulator slots, each parameterizable) plausibly supports allocating "virtual CPUs," "virtual GPUs," "virtual NPUs" by reserving batches of delta cells per workload. e.g. a graphics-heavy workload requests a vGPU → OS allocates N cells with vector-friendly accumulator config; a compile job requests a vCPU → allocates cells with branching/control-flow profile. **User explicitly said this is "wait to start working on" — finish the OS shell first.** Parking it as a tracked design vector, not active work.

## Implementation status (2026-05-03)

- **v0 committed** (127eb21): bare desktop with wallpaper + dock + `/dev/fb0` compositor. 18 KB stripped riscv64 binary. Source in `atomik_os/`.
- Builds via `make -C atomik_os`. Cross-compiles with `riscv64-linux-gnu-gcc`.
- v0 has NO agentic adaptation yet — that comes in v0.2+.

## Roadmap

| Version | Scope |
|---------|-------|
| v0  | wallpaper + dock + `/dev/fb0` compositor (DONE 2026-05-03) |
| v0.1 | window manager: floating windows, focus, drag, close button |
| v0.2 | first apps: terminal, files, monitor — each in its own window |
| v0.3 | **agentic layer**: usage-pattern logging, dock reordering by frequency, app suggestions |
| v0.4 | predictive surfacing: "you usually open Files at 9am, here it is" |
| v0.5 | per-app adaptive inputs (recommended commands in terminal, file shortcuts in Files) |
| v0.9  | edge-app manifest schema + capability dispatcher + shared client core (mock) |
| v0.10 | 3 demo edge apps (Calendar, Tasks, Code) with fake data |
| v0.11 | adaptive renderer primitives (list/card/grid/feed/conversation) |
| v0.12 | agent capability matcher — NL intent → API call dispatch |
| v1.0  | real HTTP/auth backend + first real cloud service (e.g. Google Calendar) |
| later | dynamic virtual processors |

**Why:** the user's lost OS work was the centerpiece. Reconstruction needs a clear design north star so future Claude sessions don't drift back to investor-demo polish.

**How to apply:** every new module in `atomik_os/` should have a clear path to one of the design pillars. Don't add features that aren't on the v0→v1 ramp. Save design decisions (color tokens, animation timings, layout rules) as separate `project_atomik_os_*.md` memory files so the design language is preserved across sessions.
