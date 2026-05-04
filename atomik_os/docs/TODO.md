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

### Field-delta wire format
- [x] **v0.11** — versioned binary encoding (DELT magic + opcodes:
      SET_PRIMITIVE / SET_FIELD_STR / LIST_APPEND / LIST_CLEAR /
      SET_ACCENT / SET_NAME / SET_SUBTITLE / RESET) (`e1cc680`)
- [x] Replaces full-struct fwrite with `delta_snapshot_to_file` (`e1cc680`)

### Real LLM provider abstraction + token meter
- [x] **v0.12** — `llm.c` provider registry, cost preview, audit log,
      stub backend with intent matching (`94bf226`)
- [x] **v0.12.1** — lifetime spend in status bar (`d4d7d17`)
- [x] **v0.12.2** — richer stub intent matching (`eec020e`)
- [x] `tools/atomik_ai.py` — laptop-side real-Claude relay over UART
      (`dad1616`)

### Multi-document workspace
- [x] **v0.13** — N independent Document instances, cascade-tiled,
      each with own state file (`5997ef3`)

### Speech input
- [x] **v0.14** — `tools/atomik_speech.py`: laptop mic →
      Whisper → atomik_ai.py → board (`2d0c761`)

### Token wallet
- [x] **v0.15** — `wallet.c`: balance, daily cap, audit-log ledger,
      Wallet app on `W` key, balance in status bar (`123da8d`)

### Shareable manifests
- [x] **v0.16** — `/export <path>` and `/import <path>` chat commands
      using delta-log wire format (`84a2431`)

### Schema interop
- [x] `adapters/adaptivecards_to_atomik.py` — AdaptiveCards JSON →
      ATOMiK delta-log (`7c1f491`)
- [x] `adapters/atomik_to_adaptivecards.py` — reverse: delta-log →
      AdaptiveCards JSON (`8945a24`); ATOMiK OS is now a two-way bridge
- [x] `docs/SCHEMA_SURVEY.md` — comparative scan, AdaptiveCards adopted
      as public interop format (`496037a`)

### v1.0 cross-device sync
- [x] `tools/atomik_view.py` — laptop-side Tk renderer of delta-log
      wire format (`78b7605`, `cd66ba1`)
- [x] `tools/atomik_pull.py` — board → laptop deltas bridge
      (`a193707`)
- [x] `atomik_view.py --edit` — laptop → board edits via UART
      injection. Bidirectional sync feature complete (`e36033c`)

---

## Pending / strategic

- [ ] **One design-partner conversation before public launch** — Vercel
      v0, Thesys, Anthropic Applied, or AdaptiveCards team. Validate
      the "we render your generated UI cheaply" pitch.
- [ ] **App distribution channel** — manifest registry, "install via
      URL" UX inside Files / Document.
- [ ] **First real cloud service** — Google Calendar OAuth wired
      through atomik_ai.py.
- [ ] **On-device LLM option** — for full offline operation. Probably
      a tiny intent-classifier model rather than a full LLM.
- [ ] **Bootable image for the ATOMiK laptop build** — bundle
      atomik_os + Linux + bridge into one shipping image.

## Quality / polish

- [ ] **Screenshots for README** — capture pipeline has a residual
      sentinel/encoding bug. Working DSLR-photo path is the fallback
      until that's fixed.
- [ ] Document the AdaptiveCards subset we support in
      `adapters/README.md`

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
