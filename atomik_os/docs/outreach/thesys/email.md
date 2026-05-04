# Cold email — Thesys

**To:** hello@thesys.dev
**Subject:** Edge renderer for Thesys output, on a $200 FPGA

Hi Thesys team,

I'm building ATOMiK OS — a 72 KB OS for FPGA hardware that renders
adaptive UI directly to /dev/fb0, no browser, no GPU. The
architecture treats every app as field deltas over a shared
compiled UI frame; today it accepts AdaptiveCards JSON and renders
on 100 MHz soft-RISC-V.

Thesys is solving the same generative-UI problem I'm rendering, just
from the other end. You produce; we display. I'd like to add a
Thesys schema adapter so anything your platform emits can land on
hardware that's nowhere near a browser — embedded dashboards,
industrial HMIs, set-tops, dev boards.

Open to a 20-min conversation? Attached: verified screenshot, brief.

Matthew Rockwell
matthew.h.rockwell@gmail.com
github.com/MatthewHRockwell/ATOMiK
