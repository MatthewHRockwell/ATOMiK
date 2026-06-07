Generated source copy. Do not edit as sole authority; use `../SOURCE_OF_TRUTH.md`, `../PITCH_DECK_FRIDAY.md`, and internal presenter docs for current routing.

# ATOMiK Friday Pitch Deck Source

## Friday 10-Minute Operating Deck

Use this as the controlling Friday pacing guide when time is constrained. The generated full investor deck currently has 16 slides; the live talk should still fit the 10-minute agenda by speaking to headlines, not every bullet.

| Slide | Time | Title | Headline | Main point | Evidence/caveat note |
|---:|---:|---|---|---|---|
| 1 | 0:00-0:30 | Cover | Make change the unit of compute. | Name ATOMiK, category, and `$5M` proof-round posture. | Planned SAFE; final terms CFO/counsel pending. |
| 2 | 0:30-1:05 | Problem | The hidden tax is repeated state work. | Buyers feel this as battery, heat, bandwidth, latency, reliability, size, weight, footprint, and hardware limits. | Pain categories are not all proven ATOMiK outcomes. |
| 3 | 1:05-1:35 | End-Game Value | Customers do not buy delta-state algebra. | They buy bytes avoided, update latency, operations coalesced, correctness, and measured impact. | Downstream ROI requires workload-specific measurement. |
| 4 | 1:35-2:10 | Market Opportunity | The wedge is narrow; the pressure is large. | `$1T+` TAM context, `$112B-$169B` SAM context, `$10M-$40M` SOM planning target. | TAM/SAM are adjacent spend pools; SOM is founder-prepared target, not forecast. |
| 5 | 2:10-2:45 | Focus Markets | Where growth pressure meets constrained state work. | Edge AI, robotics/industrial, drones/remote autonomy, and smallsat/remote sensing growth trajectories plus target-account examples. | Target examples are hypotheses only; no relationship implied. |
| 6 | 2:45-3:15 | Competitive Landscape | Buyers compare ATOMiK against named incumbents and status quo. | More silicon, processor/IP incumbents, and software/status quo alternatives. | Names are alternatives, not partnership or displacement claims. |
| 7 | 3:15-3:45 | Primitive | Make change the unit of compute. | Reference state plus accumulated change: LOAD, ACCUM, READ, SWAP. | Keep CEO-safe; do not over-teach algebra. |
| 8 | 3:45-4:15 | Fit | ATOMiK has a wedge, not a universal claim. | Strongest where state movement, sync, replay, sparse updates, or context movement dominate. | Not a CPU/GPU/NPU replacement. |
| 9 | 4:15-4:40 | Use Cases | Different industries feel the same waste in different budgets. | Robotics, edge AI, industrial, remote, and defense-adjacent buyers. | ICP is constraint-led, not industry-label-led. |
| 10 | 4:40-5:10 | Live Proof | Current demo surface running on Zynq hardware. | Show current Zynq UI artifact (v0.40-A, captured from `/dev/fb0`, driven by real measured on-board data). | Screenshot is not customer workload proof; it is not an interactive demo. |
| 11 | 5:10-5:55 | Proof Stack | Real, specific, evidence-bounded. | Zynq UI v0.40-A, parallel-bank throughput measured on AX7020 (1/2/4/8x, byte-identical to software), Linux-to-FPGA, AX7020 matrix, 1080p30 display, formal/synthesis context. | No universal speedup or downstream outcome claim; 1080p30 only (1080p60 impossible on this board). |
| 12 | 5:55-6:35 | Commercial Path | Start with paid evaluation, then earn strategic options. | Proof review, technical evaluation, design partner, licensing/IP diligence. | No promised acquisition or return multiple. |
| 13 | 6:35-7:25 | Financial Model | `$5M` funds proof, partner readiness, and licensing diligence. | Founder-prepared planning ranges plus use of funds. | Not a forecast, booked revenue, or committed pipeline. |
| 14 | 7:25-8:00 | Risks and Gates | Trust comes from saying what is unproven. | Customer validation, ASIC economics, downstream outcome measurement, incumbents. | Risks are why the round exists. |
| 15 | 8:00-8:30 | Team | Founder-led proof with commercial translation. | Matt owns deck/proof/ask; Allison owns buyer/commercial translation and feedback capture. | Allison is not expected to present the technical deck. |
| 16 | 8:30-9:35 | Ask | `$5M` target to reach measured workload proof. | Capital, qualified workloads, design-partner introductions, diligence help. | Final SAFE mechanics CFO/counsel pending. |
| Close | 9:35-10:00 | Close | Bring one constrained state path. | Measure the waste and decide with evidence. | Transition cleanly to Q&A and AAN feedback. |

## Source Notes For Market And Competition Slides

- TAM context: semiconductor industry context only, not all addressable ATOMiK revenue.
- SAM context: embedded systems and edge AI are adjacent spend pools where power, latency, and hardware limits matter.
- SOM target: founder-prepared 3-5 year annual revenue planning target if proof converts through paid evaluations, design partners, and licensing. It is not a forecast.
- Focus-market growth sources: Grand View Research edge AI, smart robots, commercial drone, and nanosatellite/microsatellite market reports.
- Target-account examples: design-partner/account hypotheses only. No customer relationship, traction, endorsement, pipeline, or revenue is implied.
- Competitor/status-quo examples: buyer alternatives and incumbents, not partnership or displacement claims.

## Core Positioning

ATOMiK helps constrained edge and embedded teams do less unnecessary state work by tracking meaningful changes instead of repeatedly moving, scanning, syncing, replaying, or rebuilding full state.

The strongest buyer story is not "new computing architecture" first. It is: before customers spend more on batteries, cooling, bigger hardware, more bandwidth, or redesign, ATOMiK tests whether the expensive state path can do less work.

## Claim Rule

Every performance or maturity claim needs an evidence label, artifact, context, and caveat. Do not claim universal speedups, guaranteed battery gains, guaranteed heat reduction, guaranteed water savings, production ASIC readiness, or replacement of CPUs, GPUs, accelerators, compression, caching, or sync protocols.

## Current Deck Controls

- PPTX source of truth: `business/pitch_deck/generate_deck.py`.
- Current full investor deck: 16 slides.
- Current rushed-room deck: 6 slides.
- Live delivery source: `06_internal_presenter/ATOMiK_Friday_Talking_Points.md` plus `ROOM_CHECKLIST.md`.
- Proof details source: `05_proof_artifacts/README.md`, `PROOF_CARDS.md`, `VERSION_MAP.md`, `claims_registry_snapshot.yaml`, and `README_EXTERNAL_REFERENCES.md`.

## Closing Line

ATOMiK is not asking the market to accept a broad compute claim. We are asking the right customers to bring one constrained state path, measure the waste, and decide with evidence whether state-aware compute belongs in their architecture.
