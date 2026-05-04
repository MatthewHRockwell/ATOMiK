# One-pager — ATOMiK OS × Thesys

## Who I am

Matthew Rockwell. Solo founder. Built ATOMiK — a delta-state CPU
architecture and an OS that runs on it. Both shipping today on FPGA
(AX7020, 100 MHz soft RISC-V, 1080p HDMI).

## What ATOMiK OS is

A 72 KB C-only desktop OS rendering directly to /dev/fb0 with no
browser, no React, no GPU. Apps are field-delta streams over a shared
compiled UI frame — `visible = invariant_frame ⊕ Σ deltas`. The
arithmetic is XOR (abelian group, 108 Lean4 theorems proven), so
state is reconstructed on demand instead of stored.

## Why this matters to Thesys

Thesys produces adaptive UI. ATOMiK renders it on hardware where
React doesn't run. Today the OS consumes AdaptiveCards via an
adapter (`adapters/adaptivecards_to_atomik.py`). Adding Thesys is the
same shape: a JSON-to-delta-log translator.

The market opportunity is the same as for v0 — *displays without
browsers*. Industrial HMIs, automotive clusters, kiosks, dev kits.
Anywhere ARM + Linux + Chromium is overkill or unaffordable, our
renderer fits in 72 KB on a 100 MHz soft CPU.

## What I'm asking for

20 minutes. Just want to understand:
1. Is your output schema stable enough for an adapter commit?
2. Do you see partners asking for hardware-edge rendering?
3. Would you sponsor an open-source Thesys-to-ATOMiK adapter as a
   reference renderer?

## Proof

- Repo: github.com/MatthewHRockwell/ATOMiK
- Verified screenshot: `atomik_os/docs/screenshots/deploy_20260504_162651_v0.18.png`
- Working adapter pattern: `atomik_os/adapters/adaptivecards_to_atomik.py`
- Architecture: `atomik_os/docs/ARCHITECTURE.md`
