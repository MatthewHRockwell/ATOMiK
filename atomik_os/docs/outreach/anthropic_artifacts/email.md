# Cold email — Anthropic Artifacts

**To:** (no public address — try existing relationships, or
fan-mail-style to the Artifacts product manager via LinkedIn)
**Subject:** Running Claude Artifacts on a $200 FPGA, no browser

Hi,

I'm Matthew Rockwell. I build ATOMiK — a delta-state CPU architecture
and an OS that runs on it, both shipping today on a $200 FPGA dev
board. The OS is 72 KB, written in C, and renders directly to /dev/fb0
from a 100 MHz soft-RISC-V — no browser, no React, no GPU.

The pitch: I'd like to render Claude Artifacts on hardware. Today
Artifacts run inside the Claude web client (browser, sandboxed iframe).
But Artifacts could be a *substrate* — anywhere Claude runs, the
artifact could appear. A control surface in a car, an industrial
HMI, a dev board on someone's desk. ATOMiK is a renderer for those
surfaces; pair it with Claude as the artifact source and you have an
end-to-end "AI-rendered UI on real hardware" demo that nobody else
can show.

I'm not asking for product access or anything else commercial — just
20 minutes to walk through what the integration would look like. If
there's interest, I'd write the adapter myself and open-source it.

Matthew Rockwell
matthew.h.rockwell@gmail.com
github.com/MatthewHRockwell/ATOMiK
