# ATOMiK OS — Master TODO Checklist

> Single source of truth for what's done, what's in flight, and what's
> next. Every commit ticks a box here. Every disconnect-recoverable.

---

## Done (committed + pushed to GitHub)

### Core OS shell
- [x] **v0** — `/dev/fb0` mmap compositor, gradient wallpaper, ATOMiK
      wordmark, dock skeleton, raw-mode UART input (`127eb21`)
- [x] **v0.1** — window manager, focus stack, close button, About app
      (`c6a5c5a`)
- [x] **v0.2** — Monitor app reading live ATOMiK adapter via `/dev/mem`
      (`50bbd9e`)
- [x] **v0.3** — agentic usage logger, adaptive dock, prediction surface
      (`7d4abb3`)
- [x] **v0.3.1** — clickable dock launches predicted-next app (`9ec6faf`)
- [x] **v0.4** — Markov-chain prediction + agent state persistence
      (`68bdc08`)
- [x] **v0.5** — Terminal app driving `/bin/sh` over `forkpty()`
      (`fd0bda5`)
- [x] **v0.6** — Files browser (read-only directory navigator)
      (`56ad1ae`)
- [x] **v0.6.1** — ASCII-only rendered strings, real version label
      (`6ad5c7d`)
- [x] **v0.7** — animation runtime, window fade-in, pulsing prediction
      dot, hybrid event/frame loop (`c123bd0`)
- [x] **v0.8** — Notes app, toast notifications, live status bar with
      CPU + uptime (`3df1163`)
- [x] **v0.8.1** — wallpaper cache (10× faster redraws) (`5dd6f16`)

### Invariant-frame architecture
- [x] **v0.9** — invariant-frame runtime, `edge_app_t` accumulator,
      `eapp_render` with 5 primitives (LIST/CARD/GRID/FEED/CONVO),
      Calendar/Tasks/Code reference edge apps as data only (`eb6564f`)
- [x] **v0.9.1** — Brief (CARD) + Chat (CONVO) edge apps complete the
      primitive set (`bc95b31`)

### Universal Document
- [x] **v0.10** — Document app: split-pane primitive renderer + chat
      panel + command grammar that morphs primitive/accent/fields
      (`d1c913e`)
- [x] **v0.10.1** — Document state persisted on every command
      (`14731a1`)

### Infrastructure
- [x] `deploy.py` — base64-over-UART transfer + nohup launch
- [x] `deploy.py` perf — chunk size 256→1024, per-char delay 3ms→0.8ms
      (`3e4bd90`); 22 min → ~9 min for ~50 KB binary
- [x] All commits pushed to `MatthewHRockwell/ATOMiK` `main`

---

## In flight

- [ ] **v0.10.1 deploy + visual confirm** — currently streaming the
      v0.9 binary; redeploy head-of-main once user is ready

---

## Next sprint (v0.11–v0.13) — agentic primitives

- [ ] **v0.11 — Field-delta wire format**
    - [ ] Define versioned binary encoding for an `edge_app_t` snapshot
    - [ ] Define an *incremental* delta encoding (op-codes:
          `SET_PRIMITIVE`, `SET_FIELD_STR`, `LIST_APPEND`, `LIST_CLEAR`,
          `SET_ACCENT`, `RESET`)
    - [ ] Replace the current full-struct serialization in
          `document.c` with the new delta log
    - [ ] Stream-over-UART path: pipe a delta stream from a host tool
          → board, watch the Document update live
    - [ ] Crash-recovery: replay the delta log on startup
- [ ] **v0.12 — Real LLM as the parser**
    - [ ] AI provider abstraction: pluggable backends (Anthropic,
          OpenAI, local). Each backend = (auth, base URL, model name,
          token-cost map)
    - [ ] One reference backend wired to a real API on the laptop side
          (the board still has no internet — laptop bridge proxies)
    - [ ] Token-meter: every request shows tokens-in / tokens-out and a
          running cost estimate
    - [ ] Same input → output contract as the v0.10 hand-rolled
          parser, so swapping is invisible to the rest of the OS
- [ ] **v0.13 — Multi-document workspace**
    - [ ] N concurrent Document instances; each has independent state
    - [ ] Side-by-side layout, save/load named documents
    - [ ] Quick-switch palette: type a doc name to focus

## Schema interop / partnership track (parallel to OS sprints)

- [ ] **Survey existing manifest schemas** — AdaptiveCards, Thesys, Vercel
      v0 output shape, Anthropic Artifacts, OpenAI structured outputs.
      Document coverage of our 5 primitives in each.
- [ ] **Pick the alignment target** — most likely AdaptiveCards (mature)
      or Thesys (LLM-native). Decision goes in
      `docs/ARCHITECTURE.md` §9.
- [ ] **Ship `adapters/from_adaptivecards.c`** — first reference adapter.
      Ingests JSON, emits `delta_emit_*` calls into an `edge_app_t`.
- [ ] **One design-partner conversation before v1.0 locks** — Vercel v0,
      Thesys, or Anthropic Applied. Even an exploratory chat informs
      whether our manifest schema should be theirs.

## Sprint after (v0.14–v0.16) — input + identity

- [ ] **v0.14 — Speech input**
    - [ ] Audio capture pipeline (laptop mic for now → bridge.py →
          UART text → document_handle_key)
    - [ ] On-device intent classifier stub (no audio on the board yet)
    - [ ] "Hold-to-talk" indicator in the chat panel
- [ ] **v0.15 — Per-user identity + token wallet**
    - [ ] Local user profile: name, default AI provider, token balance
    - [ ] Per-action cost preview before commit
    - [ ] Daily / monthly cap with a "review purchases" log
- [ ] **v0.16 — Shareable manifests**
    - [ ] Export a configured Document as a tiny shareable file
    - [ ] Import a manifest into a fresh Document → instant clone

## v1.0 — public-facing

- [ ] Cross-device sync: same accumulator on AX7020 + laptop
- [ ] App distribution: ship a manifest, not a binary
- [ ] First real cloud service (Google Calendar via Google API)
- [ ] On-device LLM option for fully offline operation
- [ ] Bootable image for the planned ATOMiK laptop build

---

## Hardware backlog

- [ ] HDMI: re-verify after each bitstream rebuild (`--with-video-framebuffer-hp`)
- [ ] USB host on AX7020: parked — see `feedback_usb_pause.md`
- [ ] LCD splash: confirmed working in pre-OS runs; needs reintegration
      into `atomik_os` boot sequence
- [ ] SD card boot: medium-term boot strategy

---

## Documentation

- [x] `atomik_os/README.md` — module overview + build instructions
- [x] `atomik_os/docs/TODO.md` — this file
- [x] `atomik_os/docs/ARCHITECTURE.md` — invariant frame + Document
- [x] `atomik_os/docs/BUSINESS_MODEL.md` — token-pay AI-app economics
- [x] `atomik_os/docs/SCREENSHOT_PLAN.md` — what to capture and how
- [x] Top-level `README.md` — landing page reflects current state

> **Discipline rule: every commit either ticks a box here or adds a new
> one. The TODO is canonical.**
