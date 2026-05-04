# 2-min demo script — Anthropic Artifacts

**Frame:** "I want to render a Claude artifact on a piece of hardware
that can't run a browser."

**0:00 — board on camera.** AX7020, 1080p monitor showing ATOMiK OS
desktop. "100 MHz soft RISC-V, 72 KB OS, no browser."

**0:15 — context.** "Claude Artifacts today live in the web client.
But the artifact format is a self-contained UI description. So the
artifact could render anywhere — including hardware where Chromium
isn't an option."

**0:30 — show ATOMiK rendering an artifact-shaped payload.** Use a
synthetic artifact JSON for now (Claude calendar artifact format,
hand-written) piped through the adapter pattern. UI appears.

**0:50 — change a field via natural language on-device.** Press D for
Document, type "use amber accent". Local-intent classifier routes
on-device, accent flips to amber. "That natural-language layer is
already running on the board, no network."

**1:10 — the architectural pitch.** "Artifacts + ATOMiK is a story
nobody else is telling: AI-rendered UI on real hardware. Not a
hosted demo. Not a video. The board is right here."

**1:30 — the ask.** "20-minute conversation. I'd like to know whether
Artifacts has an output schema stable enough for an adapter, and
whether Anthropic sees value in a 'Claude on hardware' positioning."

**1:50 — close.** "Open source. Reproducible. I'll do the work."
