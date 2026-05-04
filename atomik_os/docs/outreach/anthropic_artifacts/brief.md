# One-pager — ATOMiK OS × Claude Artifacts

## Who I am

Matthew Rockwell. Solo founder. Built ATOMiK — a delta-state CPU
architecture (108 Lean4 theorems, abelian-group XOR algebra) and a
companion OS, both running on FPGA today.

## What ATOMiK OS is

A 72 KB C-only desktop OS rendering directly to /dev/fb0 from a
100 MHz soft RISC-V on a $200 FPGA. No browser, no React, no GPU.
Apps are field-delta streams over a shared compiled UI frame —
`visible = invariant_frame ⊕ Σ deltas`. Same architecture that makes
the OS tiny makes adaptive UI cheap.

## Why this matters to Anthropic

Claude Artifacts today render inside the web client. But an artifact
is structurally a self-contained UI description, not a tightly-bound
DOM tree. Pair the artifact format with ATOMiK as the renderer and
the artifact can run *anywhere* — control surfaces, dashboards,
in-vehicle, industrial HMIs, dev boards.

The story: **AI-rendered UI running on real hardware, not in a
browser tab.** That's a positioning move nobody else can make.
Anthropic ships Claude; we ship the surface.

## What I'm asking for

20 minutes. Goals:
1. Understand whether the Artifact format has a stable enough schema
   for an adapter commit.
2. Validate that "Claude on hardware" is a positioning Anthropic
   wants to back.
3. Discuss open-sourcing the adapter as a reference implementation.

## What I'm offering

A working renderer, MIT-licensed, on a $200 of hardware. Free
demonstrations. A talk at a developer event if it ever helps. Zero
ask for funding or distribution rights.

## Proof

- Repo: github.com/MatthewHRockwell/ATOMiK
- Verified screenshot: `atomik_os/docs/screenshots/deploy_20260504_162651_v0.18.png`
- AdaptiveCards adapter (proves the pattern):
  `atomik_os/adapters/adaptivecards_to_atomik.py`
- Architecture truth-source: `atomik_os/docs/ARCHITECTURE.md`
