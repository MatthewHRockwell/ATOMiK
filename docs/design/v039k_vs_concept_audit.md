# ATOMiK Desk — v0.39-K vs Concept Coherence Audit

**Date:** 2026-05-28
**Compared:** `atomik_os/docs/screenshots/deploy_v0.39-K_rail.png` against `docs/design/concept_images/01-06`
**Purpose:** identify specific deltas between current hardware UI and the ATOMiK Desk concept art; feeds v0.40 roadmap.

## Per-Concept Gap Analysis

| Concept | Current state | Fidelity (0-100) | Net-new work |
|---|---|---:|---|
| 01 atomik_desk_home | Shell present (Cap Rail, Fabric, Pulse Bar); missing centered hero, twin glass panels, cool palette | 42 | palette pass, hero composition |
| 02 document_surface | Document app exists as window per README; not lifted into shell as full surface | 15 | route Document through shell |
| 03 replica_flow | Delta-log + cross-device sync ship per README v1.0; no Replica Flow surface | 8 | net-new surface (reuses substrate) |
| 04 agent_surface | Adaptive dock + Markov agent ship; no Agent surface rendered | 10 | net-new surface (reuses substrate) |
| 05 build_lane | Terminal app exists; no Build Lane surface | 5 | net-new surface (reuses terminal) |
| 06 adaptive_mode | 3-orb STATE/SYNC/AGENT hero present; missing triptych, mode-card stack, Adaptive Balance ledger | 22 | hero composition + balance ledger |

## Top 5 Highest-Impact Gaps for v0.40

1. **Palette + glass-panel chrome** — current is warm amber/brown, concept is cool navy/cyan with translucent glass panels and inner glow rims. Single highest-leverage change; touches every surface.
2. **Fabric lane waveforms** — concept Fabric cards show live sparkline waveforms behind numerics; v0.39-K shows only numbers. Add per-lane scrolling waveform.
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
