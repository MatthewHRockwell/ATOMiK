# One-pager — ATOMiK OS as an AdaptiveCards renderer

## Who I am

Matthew Rockwell. Solo founder. Built the ATOMiK delta-state CPU
architecture (108 Lean4 theorems) and a companion OS, both running
on FPGA today.

## What ATOMiK OS is

A 72 KB C-only desktop OS rendering directly to /dev/fb0 from a
100 MHz soft RISC-V on a $200 FPGA dev board. No browser, no React,
no GPU. v0.18 includes a working AdaptiveCards adapter
(`atomik_os/adapters/adaptivecards_to_atomik.py`) — any AC payload
becomes a stream of field deltas that lands on HDMI in milliseconds.

## Why this matters to AdaptiveCards

The current AC host ecosystem (Teams, Outlook, Bot Framework, Webex,
Cisco WebEx, etc.) is overwhelmingly browser- or web-view-based. The
schema is excellent and broadly adopted, but it can't reach surfaces
where Chromium is impractical — industrial HMIs, automotive
dashboards, kiosks, IoT control panels, dev boards, low-power
e-paper.

ATOMiK OS is the missing renderer for those surfaces. It's an
AdaptiveCards host you could point at without explaining what an
FPGA is.

## What I'm asking for

20 minutes. Goals:
1. Validate that an "ATOMiK OS AdaptiveCards renderer" fits the host
   ecosystem story.
2. Get feedback on what schema features to prioritize for embedded.
3. Introductions to AC community members building for embedded
   surfaces.

## What I'm offering

A reference implementation, MIT-licensed, in the open. Free
positioning material if you want to highlight a non-browser host as
an example of the schema's reach. A working AC renderer running on
$200 of hardware that I will demo on demand.

## Proof

- Repo: github.com/MatthewHRockwell/ATOMiK
- Verified screenshot: `atomik_os/docs/screenshots/deploy_20260504_162651_v0.18.png`
- Working AC adapter: `atomik_os/adapters/adaptivecards_to_atomik.py`
- Schema rationale: `atomik_os/docs/SCHEMA_SURVEY.md`
