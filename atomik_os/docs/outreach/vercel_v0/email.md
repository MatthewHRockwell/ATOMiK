# Cold email — Vercel v0

**To:** community@vercel.com (also: @rauchg DM as a backup)
**Subject:** Rendering v0 output on a $200 FPGA without a browser

Hi v0 team,

I built an OS for an FPGA-based delta-state architecture (ATOMiK) that
renders adaptive UI directly to /dev/fb0 on a 100 MHz soft-RISC-V CPU
— no browser, no React runtime, no GPU. It already accepts
AdaptiveCards JSON and renders to 1080p HDMI in milliseconds.

I'd like to extend the renderer to consume v0's component output
verbatim. The pitch isn't competitive — it's reach. v0 generates a
React tree; we render it on devices that can't run React. Embedded
displays, kiosks, set-top boxes, dev boards — anywhere a browser is
overkill or impossible. Our job is to be the cheap edge for whatever
surface you ship.

Looking for: a 20-min call to see if there's a fit. Attached:
verified screenshot of the OS running, plus a one-page brief.

Matthew Rockwell
matthew.h.rockwell@gmail.com
github.com/MatthewHRockwell/ATOMiK
