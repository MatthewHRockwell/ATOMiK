---
name: ATOMiK OS edge-apps direction (NemoClaw-style universal shell)
description: Apps = API manifest + agent dispatcher + adaptive UI. Any service with an API becomes an OS app without a native port.
type: project
originSessionId: 0b1da619-fb37-4461-99b3-200ab2dbce84
---
The ATOMiK OS app model is intentionally NOT "ship a native binary per platform." It's a NemoClaw / OpenClaw-style universal shell:

**Each "edge app" is a thin manifest** (auth method, endpoints, capabilities, UI hints) — NOT a binary. The OS provides:
1. **Capability dispatcher** — agent matches user intent to app capabilities and invokes the right API call.
2. **Shared client core** — HTTP, auth (OAuth2 / API keys / MCP), rate-limit, response caching.
3. **Adaptive renderer** — primitives (list, card, calendar grid, feed, kanban, conversation) render any app's data per its UI hints, so all edge apps inherit a consistent look without per-app UI code.
4. **Local agent (Markov + freq, already shipped at v0.4)** — predicts next app/action per time-of-day, recent context, current focused window.

**Why this is the bet:** every other OS asks developers to ship a native UI. ATOMiK OS asks them to ship a spec. A one-person team can offer their service as a first-class desktop app instantly. The OS + agent generate the rest.

**Demo on AX7020 (no internet on the board):** ship 2–3 mock edge apps with fake-but-realistic data — Calendar, Tasks, GitHub-PRs — to prove the pattern. Same code architecture runs against real APIs the moment we move to networked hardware (the planned ATOMiK laptop).

**Roadmap insertion point:**
- v0.9: edge-app manifest schema + dispatcher + shared client core (mock backend)
- v0.10: 3 demo edge apps (Calendar, Tasks, Code) with fake data
- v0.11: adaptive renderer primitives (list/card/grid/feed) auto-applied per manifest
- v0.12: agent capability matcher — natural-language intent → API call dispatch
- v1.0: real HTTP backend + auth + first cloud service shipped (Calendar via Google API)

**Why this matters for ATOMiK specifically:** delta-state primitives are *naturally* suited to caching API responses (state = last seen, accumulator = changes since), to predicting user intent (every interaction is a delta), and to running on tiny edge devices. The "edge" in "edge apps" is literal — these run on the on-device agent, not in a cloud. That's the privacy + latency story.

**How to apply:** when planning OS work, treat "the next app" as a manifest + capability set rather than a new C file. The existing apps (About, Monitor, Terminal, Files, Notes) are *system apps*, not edge apps — they're the reference implementations of the renderer primitives. Edge apps come next and use the same primitives without writing C.
