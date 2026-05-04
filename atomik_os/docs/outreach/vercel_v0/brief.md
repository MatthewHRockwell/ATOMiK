# One-pager — ATOMiK OS × Vercel v0

## Who I am

Matthew Rockwell. Solo founder. Built ATOMiK — a delta-state CPU
architecture (108 Lean4 theorems, abelian-group XOR algebra) and a
companion OS that runs on it. Both shipping today on FPGA.

## What ATOMiK OS is

A 72 KB C-only desktop OS that renders 1080p HDMI from a 100 MHz
soft RISC-V on a $200 FPGA. No browser, no React runtime, no GPU.
The killer architectural choice: **apps are field-delta streams over
a shared compiled UI frame** — `visible = invariant_frame ⊕ Σ deltas`.
This makes adaptive UI extraordinarily cheap: a calendar, a kanban,
a chat, and a code feed all share the same compiled chrome.

## Why this matters to v0

v0 generates React. React needs a browser. Browsers need ~hundreds
of MB and a real CPU. There's a long tail of *displays without
browsers*: dev boards, in-car HMIs, factory HMIs, industrial control
panels, kiosks, embedded dashboards, e-paper, low-power IoT panels.
v0 cannot reach those today.

ATOMiK OS already reaches them. The question is just: which schemas
does our adapter understand? Today: AdaptiveCards. Adding v0 is one
adapter file (~300 LOC based on the AdaptiveCards adapter we
already shipped).

## What I'm asking for

A 20-minute conversation. I'd like to:
1. Show the OS running, live.
2. Walk through the v0-to-ATOMiK adapter shape.
3. Understand whether v0's output schema is stable enough to commit
   an adapter to.
4. Hear whether you have customers asking for "v0 on hardware" use
   cases that I'm not seeing.

## What I'm not asking for

Money. Co-development. A press release. Just the conversation.

## Proof

- Repo: github.com/MatthewHRockwell/ATOMiK
- Verified screenshot: `atomik_os/docs/screenshots/deploy_20260504_162651_v0.18.png`
- Adapter that already works:
  `atomik_os/adapters/adaptivecards_to_atomik.py` (320 bytes)
- Architecture: `atomik_os/docs/ARCHITECTURE.md`
