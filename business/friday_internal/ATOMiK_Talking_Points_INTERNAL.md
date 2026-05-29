# ATOMiK Investor Pitch - Talking Points & Time Boxes

Role: expanded prep and Q&A backup. Primary live script: `ATOMiK_Friday_Talking_Points.md`.

## Friday Meeting Prep | Pre-Seed $5M Ask

Purpose: keep the room focused on the investable claim. ATOMiK is not asking investors to accept a broad compute thesis. ATOMiK is asking them to fund the next evidence gates: measured workload proof, IP diligence, ASIC feasibility, and design-partner evaluation.


## Aggie Ask Reconciliation

Aggie Angel Network received earlier framing around a `$5M` raise. Use `$5M target pre-seed` in the room so the live ask matches their prior expectation. Final SAFE mechanics, valuation cap, discount, pro-rata, staged close structure, and entity conversion remain CFO/counsel pending.

If asked about older email language such as `97% energy efficiency gains`, `87% average performance`, or `+100x speedup`, do not repeat it as a broad current customer-outcome claim. Say: earlier technical updates used preliminary or artifact-specific language; the current Friday packet is deliberately tighter and evidence-bound. Current proof should be quoted with artifact, context, and caveat. Battery, energy, heat, cooling, water, footprint, and customer ROI remain workload-specific evaluation targets until measured end to end.

## Allison Feedback Integration

Allison's live role is commercial translation, not technical deck delivery. Matt should present the deck itself: opening frame, mechanism, proof posture, finance/ask, and technical Q&A. Allison should jump in around buyer pain, customer language, commercialization, and the AAN feedback segment.

Use Allison's customer-language frame when the room gets too technical:

> Potential ATOMiK customers are teams in robotics, edge AI, industrial systems, remote systems like drones and satellites, and defense-adjacent environments. What they have in common is expensive pressure from battery life, heat, size, weight, hardware limits, performance, or rising cost. Before they spend more money on bigger hardware, batteries, cooling, bandwidth, or redesigns, ATOMiK evaluates whether we can help them get more out of the systems they already have. They show us the constrained state path, we measure whether ATOMiK can improve it against the current baseline, and we show the measured opportunity or tell them it is no-fit.

If time is short, cut technical detail first. Keep the buyer pain, proof boundary, financial model, and ask.

Best ask from Keaton Savoie: design-partner introductions and investor feedback, especially access to people actively dealing with battery, heat, size, weight, hardware-limit, latency, or performance problems.

## Core Sentence

> ATOMiK makes change the unit of compute. We help constrained edge and embedded teams find wasted state movement, measure it against a real baseline, and decide whether state-aware execution belongs in their architecture.

If time gets cut, repeat this sentence and move to proof plus ask.

## The Three Things The Room Must Remember

1. **Pain:** constrained systems waste battery, bandwidth, time, and thermal margin moving or rebuilding state they already know.
2. **Proof:** ATOMiK has hardware-backed primitive proof, Linux userspace-to-FPGA validation, and live-measured AX7020 artifacts with honest caveats.
3. **Business:** $5M funds measured customer workload proof, IP diligence, ASIC feasibility, licensing-ready materials, and a planning path from paid evaluations to design partners to licensing if proof holds. It does not fund tape-out.

## 5-Minute Version

Use this if the meeting is rushed or you are interrupted early. Skip use-case detail and most market context.

| Time | Slide | Say |
|---:|---|---|
| 0:00-0:30 | 1 - Cover | ATOMiK makes change the unit of compute. We start with one constrained workload, one baseline, and one painful metric. |
| 0:30-1:10 | 2 - Problem | The hidden tax is repeated state work: scans, syncs, replay, reconstruction, and full-state movement when only a compact change matters. |
| 1:10-1:50 | 3 - Mechanism | The primitive is reference state plus accumulated delta. LOAD, ACCUM, READ, SWAP. Plain English: track meaningful change and reconstruct only when needed. |
| 1:50-2:50 | 4 - Proof Today | Proof today is evidence-labeled: Zynq UI, Linux userspace-to-FPGA path, AX7020 measured matrix, formal proof work, SD boot build artifacts. We do not claim universal speedups or battery/heat savings. |
| 2:50-3:40 | 5 - Commercial Path | Commercial path: proof review, technical evaluation, design partner, then licensing/IP diligence if proof supports it. |
| 3:40-4:35 | 6 - Ask | We are raising $5M. The round funds one measured workload artifact, IP packet, ASIC feasibility, and design-partner access. No tape-out. |
| 4:35-5:00 | Close | The ask is capital plus introductions to qualified workloads and design partners. |

5-minute close:

> We are not claiming ATOMiK solves every compute problem. We are funding the evidence step: bring one constrained state path, measure the waste, and decide with proof whether state-aware compute belongs there.

## 10-Minute Version

This is the default investor pitch if you have a tight room. Speak to the slide headline, not every bullet.

| Time | Slide | Required point |
|---:|---|---|
| 0:00-0:35 | 1 | Category and ask: change-first compute, $5M pre-seed. |
| 0:35-1:20 | 2 | Repeated state work creates paid pain in battery, heat, bandwidth, latency, and reliability. |
| 1:20-2:00 | 3 | Customers buy measurable outcomes: bytes avoided, latency, operations coalesced, correctness. Battery/thermal remain evaluation targets. |
| 2:00-2:45 | 4 | ATOMiK mechanism: reference state plus accumulated delta. |
| 2:45-3:20 | 5 | Fit discipline: strongest where updates, sparse changes, sync, replay, or context movement dominate. Not a CPU replacement. |
| 3:20-3:55 | 6 | Lead wedge: edge/embedded first; data center/infrastructure later as measured expansion path. |
| 3:55-5:30 | 7-8 | Proof stack: show what is real today and what each proof does not prove. Be explicit about labels. |
| 5:30-6:45 | 9 | Commercial path: proof review -> technical evaluation -> design partner -> licensing/IP diligence if proof holds. No promised acquisition. |
| 6:45-7:45 | 10 | Financial model: founder-prepared planning ranges plus $5M use of funds. Stress not a forecast and not booked revenue. |
| 7:45-8:25 | 11 | Risks: customer validation, ASIC economics, downstream outcome measurement, incumbents. Show discipline. |
| 8:25-8:55 | 12 | Team: Matt owns proof/ask; Allison owns customer translation, commercialization, and feedback capture. |
| 8:55-9:40 | 13 | Ask: $5M target, SAFE pending counsel/CFO, design-partner introductions, workload access. |
| 9:40-10:00 | Close | Repeat the evidence-first wedge. |

10-minute close:

> The next milestone is not a bigger claim. It is a measured customer workload artifact with correctness preserved. That is what this round buys.

## Friday Default Run-of-Show

| Time | Segment | Goal |
|---:|---|---|
| 0:00-0:30 | Frame the meeting | Set the agenda: 10-minute pitch, Q&A, then AAN background and feedback. |
| 0:30-10:00 | 10-minute pitch | Use the repaired 13-slide investor deck and speak to the headline, not every detail. |
| 10:00-[time available] | Q&A | Pressure-test proof, fit, commercialization, funding plan, and claim boundaries. |
| Final segment | AAN Background & Feedback | Capture reactions, proof gaps, and specific intro opportunities. |

Transition to Q&A:

> That's the 10-minute version. I'd like to use Q&A to pressure-test proof, fit, and commercialization.

Transition to AAN feedback:

> We'd like to use the remaining time to get your reaction to three things: whether the buyer framing lands, whether the proof package feels credible, and who you think the first design-partner audience should be.

## Backup Expanded Run-of-Show - Use Only If They Ask for More Detail

Use this only if the room explicitly asks for a deeper walkthrough or extends the pitch portion. If questions start early, answer directly, then return to slide 8 or slide 12.

| Time | Segment | Slides | Goal |
|---:|---|---|---|
| 0:00-1:00 | Hook | 1 | Establish category, ICP, and ask. |
| 1:00-3:00 | Pain | 2-3 | Make the buyer pain concrete and measurable. |
| 3:00-5:00 | Mechanism | 4-5 | Explain how ATOMiK works without drowning the room in algebra. |
| 5:00-6:30 | Wedge | 6 | Show edge/embedded first, data center later. |
| 6:30-10:00 | Proof | 7-8 | Show evidence labels and claim boundaries. This is the trust section. |
| 10:00-12:30 | Business path | 9 | Connect proof gates to IP value and strategic options. |
| 12:30-14:30 | Financial model | 10 | Show the planning ranges and milestone-driven budget without presenting them as a forecast. |
| 14:30-16:00 | Risk discipline | 11 | Show what is unproven and how the round attacks it. |
| 16:00-16:45 | Team | 12 | Clarify Matt/Allison roles and why the room gets both proof and customer translation. |
| 16:45-18:00 | Ask | 13 | Ask for money, design-partner intros, workload access, and diligence help. |
| 18:00-30:00 | Q&A | Appendix/data room | Lead with proof boundaries; do not improvise unsupported claims. |

## Slide-By-Slide Speaker Notes

### Slide 1 - Make change the unit of compute

Say:
"ATOMiK is for constrained edge and embedded teams. They bring one workload, one baseline, and one painful constraint. We measure whether state-aware execution can reduce wasted state movement while preserving correctness."

Skip if rushed:
Detailed explanation of all proof labels. Save it for slide 8.

### Slide 2 - The hidden tax is repeated state work

Say:
"The paid pain is not abstract. It is battery budget, enclosure heat, bandwidth, latency, reliability, and hardware margin. We are looking for the state path behind that pain."

Do not say:
"ATOMiK reduces data-center power" or "ATOMiK saves water." Those are market context and evaluation targets.

### Slide 3 - Customers do not buy delta-state algebra

Say:
"The evaluation starts with a metric: bytes moved, operations coalesced, update latency, reconstruction cost, or correctness preservation. Downstream power and thermal claims require responsible measurement."

### Slide 4 - The primitive

Say:
"ATOMiK keeps a reference state plus accumulated change. LOAD sets the reference. ACCUM adds changes. READ reconstructs. SWAP commits a boundary. That is the CEO-safe explanation."

### Slide 5 - Fit discipline

Say:
"ATOMiK has a wedge, not a universal claim. If state movement is not the cost, or if coalescing cannot beat interface overhead, the workload may be no-fit."

### Slide 6 - Use cases

Say:
"Edge and embedded are first because the pain is local, measurable, and expensive: battery, heat, links, latency, reliability. Infrastructure is an expansion narrative after measured workload proof."

### Slide 7 - Live proof

Say:
"This is proof of the current live Zynq demo surface. It is not a production-readiness or performance claim."

### Slide 8 - Proof stack

Say:
"Every proof has a label. Hardware-validated means demonstrated on physical hardware. Live-measured means raw measurement artifacts exist. Build artifact means a build exists but the end-to-end run is not yet promoted."

Must include:
"The AX7020 matrix has wins and losses. That is a diligence advantage because it shows workload-specific discipline."

### Slide 9 - Commercial path

Say:
"The commercial path is deliberately staged: proof review, technical evaluation, scoped design partner, then licensing or IP diligence if the evidence supports it. The investor return logic is evidence compounding, not a promised acquisition."

Do not say:
"We will be acquired" or cite unsourced transaction multiples.

### Slide 10 - Financial model

Say:
"This is a founder-prepared planning scenario, not a forecast. The business path is paid evaluations first, then design-partner SOWs, then licensing only if measured proof and partner diligence support it. The $5M use of funds is tied to proof, customer validation, IP/legal, ASIC feasibility, and operating readiness."

Do not say:
"We have booked this revenue," "this is guaranteed pipeline," or "customer ROI is proven."

### Slide 11 - Risks and gates

Say:
"The risks are exactly why the round exists. Battery, thermal, water, and footprint outcomes are not claimed until measured. Customer validation is pending. ASIC economics need expert review."

### Slide 12 - Team

Say:
"I will own the proof, roadmap, and ask. Allison is adding the commercial translation: buyer pain, positioning, and feedback capture. That split matters because the technical proof only becomes valuable if customers understand the expensive problem we can evaluate."

### Slide 13 - The ask

Say:
"We are raising a $5M target pre-seed. We also need design-partner introductions and constrained workloads where the pain is already expensive."

Close:
"ATOMiK is asking the right customers to bring one constrained state path, measure the waste, and decide with evidence whether state-aware compute belongs in their architecture."

## Q&A Answers To Keep Tight

**Where does ATOMiK lose?**
Where state movement is not the binding cost, where changes are not sparse or coalescable, or where interface overhead overwhelms the saved work. The AX7020 matrix already shows that naive hardware access can lose.

**What is proven today?**
Hardware-backed primitive checks through Linux userspace-to-FPGA, a live Zynq prototype UI, and live-measured AX7020 matrix artifacts with caveats. Formal proof work exists in the repo. SD boot remains build-artifact until promoted by a recorded run.

**What is not proven?**
Customer workload value, battery extension, heat reduction, cooling reduction, water savings, footprint reduction, production readiness, and universal speedup.

**What does $5M buy?**
One or more measured workload artifacts, a stronger IP packet, external ASIC/IP feasibility review, customer evaluation tooling, and licensing-ready diligence materials.

**What do you need besides capital?**
Qualified workloads, design-partner introductions, ASIC/IP diligence support, and reviewers who can pressure-test proof artifacts.

## If CFO Is Not In The Room

Use these guardrails. Do not improvise terms.

- **Round size:** $5M target pre-seed; staged close structure and final terms counsel/CFO pending.
- **Runway:** target plan is roughly 18-24 months; staged close mechanics remain CFO/counsel pending; extra capacity accelerates validation rather than funding tape-out.
- **Monthly budget:** target plan averages about $111K/month gross, or about $103K/month excluding reserve.
- **Valuation cap:** do not give one. Say final cap, discount, pro-rata, side letters, and close mechanics require CFO/counsel approval.
- **Dilution sensitivity:** at an illustrative $25M post-money SAFE cap, a $5M raise implies 20% SAFE ownership before option-pool and later-round effects. This is sensitivity math, not a cap recommendation.
- **Revenue:** founder-prepared planning ranges now exist for evaluations, design-partner SOWs, and licensing paths. Treat them as planning scenarios only; material revenue requires signed SOWs or licensing terms.

## Forbidden Phrases

- guaranteed savings
- universal speedup
- production-ready
- commercial product
- replaces CPU/GPU/NPU
- proven battery improvement
- proven heat reduction
- water savings
- data-center savings
- acquired at an IP premium

## Safe Phrases

- workload-specific
- measured against baseline
- correctness-preserving
- evidence-bound
- evaluation target
- hardware-validated for this artifact
- live-measured with raw artifacts
- build artifact until promoted by run evidence

*Last updated: 2026-05-28 | Do not distribute beyond controlled pitch meetings*
