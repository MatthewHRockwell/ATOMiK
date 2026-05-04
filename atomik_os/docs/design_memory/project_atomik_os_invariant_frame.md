---
name: ATOMiK OS — invariant frame + field-delta apps
description: Every app shares ONE compiled UI frame. Apps stream as small field-delta blobs, not pixels or native binaries. This is the killer ATOMiK workload.
type: project
originSessionId: 0b1da619-fb37-4461-99b3-200ab2dbce84
---
# The architecture that makes ATOMiK OS uniquely possible

User insight (2026-05-03): every running app draws the same conserved structure — window frame, title bar, color scheme, typography, scroll affordances, button styles — and only a small handful of FIELDS differ (text strings, list items, geometry slots, primitive choice). So an "app" doesn't need to be a binary or a remote framebuffer. It's just **the deltas to those fields**, applied on top of the universal frame the OS already renders locally.

## The reduction

```
app_visible_state  =  invariant_frame  ⊕  accumulated_field_deltas
```

This is *literally* the ATOMiK state-reconstruction equation:

```
current_state     =  initial_state    ⊕  accumulator
```

The OS itself becomes the canonical ATOMiK workload. Not a benchmark, not a demo — the actual product runs on the algebra it was built for.

## What it collapses

1. **App-streaming bandwidth** drops from "render a framebuffer" (MB/s) to "send a few field deltas" (bytes/s). No VNC, no remote-desktop, no GPU pipeline. The OS already has the frame — the remote service just says `list[3] = {12 items}`, `header = "Inbox"`, `accent = blue`.

2. **App-build size** drops from "native binary per platform" to "a manifest of which primitives + which fields." Tens of KB max per app. No per-platform compile.

3. **Per-device compute** drops to ~zero per app. Drawing the frame is once-per-OS-instance. Drawing field deltas is cache-friendly. Even a 100 MHz soft CPU can host hundreds of edge apps simultaneously because they share the compiled frame.

4. **Agent prediction** has perfect leverage: predicting "next likely field state" is a much smaller, more-typed problem than "next likely binary action." The local agent can pre-fetch / pre-render predicted states before the user asks.

## How it maps to ATOMiK hardware

Each app's UI state IS an ATOMiK accumulator. Fields ARE deltas. Caching the previous state and detecting changes IS what the adapter does in hardware (the change-detection benchmark we already cite at 399–500x). The OS isn't simulating ATOMiK — the desktop's per-frame work IS ATOMiK ops.

When we add the dynamic-virtual-processor allocator later, each running app gets one or more delta cells that hold its field accumulator. The OS scheduler is literally an ATOMiK accumulator manager.

## Architecture sketch

```
+--------------------------------------------------------+
| Invariant Frame (compiled ONCE per OS install)         |
|   - color tokens, typography, palette                  |
|   - window chrome: title bar, close button, shadow     |
|   - primitive renderers: list, card, grid, feed, etc.  |
|   - scroll/input affordances                           |
+--------------------------------------------------------+
                         ^
                         | XOR (delta apply)
                         |
+--------------------------------------------------------+
| Field-delta stream per app                             |
|   field_id -> typed value (string/int/list/geometry)   |
|   sent over UART / network / local IPC                 |
|   tiny: typically <1 KB / s active                     |
+--------------------------------------------------------+
                         ^
                         | dispatch
                         |
+--------------------------------------------------------+
| App manifest (the entire "app")                        |
|   - which primitive (list / card / grid / feed)        |
|   - field schema (typed slots)                         |
|   - capabilities (which API verbs)                     |
|   - auth (later)                                       |
+--------------------------------------------------------+
```

The manifest is what a developer "ships." It's a few hundred bytes of typed schema. ATOMiK OS does the rest.

## Implications

- **App "streaming" between devices**: sync the field-delta accumulator over the network. Receiving device's local OS reconstructs the UI. Bandwidth ≈ keystrokes worth of bytes.
- **Cross-device app state**: my Calendar on the AX7020 and on the future ATOMiK laptop have the same accumulator. Switching devices is XOR-equivalent — instant.
- **Privacy**: only field deltas leave the device. Never pixels, never raw data. Compute stays at the edge.
- **Snapshotting / undo**: free, because XOR is self-inverse. Roll back any app to any state by re-applying the delta in reverse.

## Roadmap update

This *replaces* the previous v0.9 plan ("edge-app manifest schema") with the more correct v0.9: **invariant-frame compiler + field-delta runtime**. Every previous app written so far (About, Monitor, Terminal, Files, Notes) becomes a *reference implementation* of the renderer primitives that the invariant frame uses; subsequent apps are pure manifests.

| Version | Scope |
|---------|-------|
| v0.9  | Invariant frame: lock the frame template (chrome + 5 primitives) and write the manifest+field-delta runtime. Existing system apps refactor to use it. |
| v0.10 | First field-delta-only apps: Calendar, Tasks, Code, each shipped as a single manifest file. No new C per app. |
| v0.11 | Field-delta encoding format (binary, ATOMiK-aligned), so apps can stream over UART or network. |
| v0.12 | Agent capability matcher routes natural-language intent to field-delta updates. |
| v1.0  | Cross-device app sync: same accumulator on AX7020 and laptop. |

## Why this is the pitch

Every other OS asks developers to ship native UIs. Microsoft asks for Win32. Apple asks for AppKit/UIKit. Even the web asks for HTML+JS, which is still per-app code. ATOMiK OS asks for **a manifest and a stream of typed deltas** — and the algebra of that delta is the same algebra the silicon implements. Nobody else can do this because nobody else has the substrate.

**This is *the* product. The desktop UI is the demo.**
