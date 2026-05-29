# ATOMiK Desk — v0.39-K vs Concept Coherence Audit

**Date:** 2026-05-28 (CORRECTED 2026-05-29)
**Compared:** `atomik_os/docs/screenshots/deploy_v0.39-K_rail.png` against `docs/design/concept_images/01-06`
**Purpose:** identify specific deltas between current hardware UI and the ATOMiK Desk concept art; feeds v0.40 roadmap.

> **2026-05-29 CORRECTION — the "warm palette" finding was an artifact, not a real gap.**
> This audit was performed against `deploy_v0.39-K_rail.png`, which had its **R and B
> channels swapped** by a broken manual screenshot export (verified: the rail PNG was the
> raw `deploy_20260521_031311_v0.39-K.png` with R↔B swapped, mean per-channel diff 0.20).
> The OS actually renders the **correct cool navy/cyan palette** — confirmed end-to-end:
> code palette is `0x00RRGGBB` (cyan `0x6EDDFF`), assets are cool, the LiteX framebuffer
> unpacks `r=word[16:24] g=[8:16] b=[0:8]` (XRGB), and the *raw* fb2png capture is cool.
> The swap made cyan→gold, violet→pink, navy→brown. The rail thumbnails and the published
> website image have been un-swapped. **Palette is NOT a v0.40 gap** — strike item #1 below.
> The remaining items (waveforms, hero, surface routing, pulse pills) stand.

## Per-Concept Gap Analysis

| Concept | Current state | Fidelity (0-100) | Net-new work |
|---|---|---:|---|
| 01 atomik_desk_home | Shell present (Cap Rail, Fabric, Pulse Bar), palette already cool/correct; missing centered hero + twin glass panels | 42 | hero composition (palette = DONE, was a capture artifact) |
| 02 document_surface | Document app exists as window per README; not lifted into shell as full surface | 15 | route Document through shell |
| 03 replica_flow | Delta-log + cross-device sync ship per README v1.0; no Replica Flow surface | 8 | net-new surface (reuses substrate) |
| 04 agent_surface | Adaptive dock + Markov agent ship; no Agent surface rendered | 10 | net-new surface (reuses substrate) |
| 05 build_lane | Terminal app exists; no Build Lane surface | 5 | net-new surface (reuses terminal) |
| 06 adaptive_mode | 3-orb STATE/SYNC/AGENT hero present; missing triptych, mode-card stack, Adaptive Balance ledger | 22 | hero composition + balance ledger |

## Top Highest-Impact Gaps for v0.40 (palette struck — see correction above)

1. ~~**Palette + glass-panel chrome**~~ — STRUCK. The OS palette is already cool/correct
   (`0x00RRGGBB`, cyan `0x6EDDFF`); the "warm" look was an R↔B swap in a broken screenshot
   export, now fixed. The **glass-panel chrome** half is still partly real: concept panels
   have translucent fills + inner-glow rims; current panels are flatter. Fold that into the
   hero/surface work rather than a palette pass.
2. **Fabric lane waveforms** — concept Fabric cards show live sparkline waveforms behind numerics; v0.39-K shows only numbers. Add per-lane scrolling waveform. (Sine-pulse source already exists — see "Do Not Regress".)
3. **Home-surface center hero** — concept anchors on "ATOMiK Desk / Idle-Ready / twin glass panels"; v0.39-K idle is a 3-orb scene. Reframe idle to introduce the system before showing activity.
4. **Surface routing for Document / Replica Flow / Build Lane** — three full-frame surfaces need to render inside the desk shell rather than as separate windows or not at all.
5. **Pulse Bar telemetry pills** — concept top bar has 4 distinctly compartmentalized + labeled glass pills; v0.39-K bar lacks the compartments + glow rims.

## What v0.39-K Has Right (Do Not Regress)

- Capability Rail left: icon-with-label cells + SYSTEM header + divider — score 93 from prior audit, on-target structurally
- Resource Fabric right: 5 lanes named correctly, AA chips, ACTIVE highlight — structural skeleton matches all 6 concepts
- Pulse Bar: semantic alignment to active personality, integrated-metric pulse well
- 3-orb personality hero: closer to concept 06 (Adaptive Mode) than concept 01; keep as Adaptive Mode surface
- Typography atlas (atomik_36 + glass pills)
- Sine pulse waveform: directly reusable as Fabric lane waveform source

## Pitch-Use Paragraph

> ATOMiK Desk today renders the shell of the target experience on real silicon: a 6-cell Capability Rail, a 5-lane Resource Fabric (STATE / SYNC / AGENT / EVENT / VISUAL), a semantic Pulse Bar, and a STATE/SYNC/AGENT personality hero — all driven by the same delta-state algebra the FPGA implements, captured directly from `/dev/fb0` on a Zynq AX7020. Internal scoring puts the shell at 95% fidelity against its V-phase target. Measured against the full ATOMiK Desk concept — Document, Replica Flow, Agent, Build Lane, and Adaptive Mode surfaces — we are roughly one-third of the way: the chrome is right, the palette and per-lane waveforms need a pass, and three application surfaces (Document already shipped as a window, Replica Flow and Build Lane net-new) need to be lifted into the desk shell. v0.40 closes that gap by routing the existing Document app, delta-log sync, and terminal substrate through the shell as first-class surfaces. The substrate exists; v0.40 is composition, not invention.
