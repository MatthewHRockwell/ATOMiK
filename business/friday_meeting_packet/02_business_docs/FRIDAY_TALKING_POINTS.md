# Friday Talking Points

## Meeting flow
1. 10-minute pitch
2. Q&A
3. AAN Background & Feedback

## Required run-of-show
| Time | Segment | Presenter action |
|---:|---|---|
| 0:00-0:30 | Greeting and frame | Thank the room, state the agenda, and frame ATOMiK as an evidence-bound evaluation story. |
| 0:30-10:00 | 10-minute pitch | Use the 10-slide operating flow. Do not walk the longer deck slide by slide. |
| 10:00-20:00 or as scheduled | Q&A | Prioritize how it works, proof, not-yet-proven claims, first customer, evaluation process, Zynq validation, business model, and ask. |
| Final section | AAN Background & Feedback | Capture reaction and next actions. |

## Q&A priority order
1. How it works
2. What is proven
3. What is not proven
4. First customer
5. Evaluation process
6. Board/Zynq validation
7. Business model
8. Ask

## Opening 30-second version
ATOMiK makes change the unit of compute. We help edge and embedded teams evaluate whether reducing wasted state movement can improve battery, power, bandwidth, latency, thermal, or footprint constraints.

## 10-minute speaker script
| Time | Slide | Point |
|---:|---|---|
| 0:00-0:45 | 1 - Cover | ATOMiK makes change the unit of compute. Name the company, category, and proof-bound posture. |
| 0:45-1:45 | 2 - Buyer pain | Buyers feel the cost as battery, heat, bandwidth, latency, footprint, reliability, and hardware overbuild. These are pains, not claimed outcomes. |
| 1:45-2:45 | 3 - Hidden tax | In the right workloads, the hidden tax is wasted state movement: repeated scans, syncs, replays, rebuilds, and full-state transfers. |
| 2:45-4:00 | 4 - Insight/how | ATOMiK separates known state from meaningful change: reference, accumulate, combine, reconstruct, checkpoint. |
| 4:00-5:00 | 5 - First ICP | First customer is edge/embedded with one workload, one baseline, and one painful constraint. |
| 5:00-6:15 | 6 - Offer | Give us X, we evaluate Y, you receive Z, success looks like A. |
| 6:15-7:45 | 7 - Proof today | Proof is specific, evidence-labeled, and workload-bound. Show Linux-to-FPGA, AX7020 nuance, and Zynq validation plan. |
| 7:45-8:45 | 8 - Business model | Start with proof reviews and technical evaluations; expand to design partners and licensing/IP if evidence supports it. |
| 8:45-9:30 | 9 - Milestones | Next milestone is evaluated customer proof: SD boot, representative workloads, public proof card, design-partner path. |
| 9:30-10:00 | 10 - Ask | Ask for feedback, qualified workload introductions, proof reviewers, ASIC/IP diligence help, and investor follow-up. |

## CEO-level how answer
Traditional systems often work from the full-state view: read the state, move the state, compare the state, rebuild the state, sync the state, and repeat. ATOMiK separates the known state from the change layer. It starts with a reference state, accumulates meaningful changes, combines repeated changes when possible, and reconstructs the current state only when needed.

## Technical how answer
ATOMiK uses a reference state plus accumulated deltas. LOAD establishes the reference state. ACCUM adds meaningful changes. READ reconstructs current state from the reference plus accumulated changes. SWAP checkpoints the result and starts the next change cycle.

## Buyer outcome answer
ATOMiK directly evaluates reduced state movement, fewer emitted operations, lower update/reconstruction cost, lower latency, and correctness preservation. Battery, heat, cooling, water, power-bill, and footprint outcomes are downstream results that require workload-specific measurement.

## Proof answer
The current proof is specific, evidence-labeled, and workload-bound. We do not claim universal speedups. We show where ATOMiK helps, where it does not, and what needs customer-environment validation.

## Board/Zynq answer
We are moving the Zynq workflow to SD-card boot so workload changes can be made and rerun faster. After SD boot is stable, the first validation workloads are dirty-state telemetry sync and repeated register/control update coalescing.

## Q&A prep
Use answers from `website/business-docs/OBJECTION_HANDLING_FAQ.md`. Keep each answer under 90 seconds. If asked for battery, heat, cooling, water, or footprint proof, answer as downstream evaluation targets unless a matching artifact exists.

## AAN Background & Feedback transition
After Q&A: "We'd like to use the remaining time to get your reaction to three things: whether the buyer framing lands, whether the proof package feels credible, and who you think the first design-partner audience should be."

## AAN feedback capture
- What landed?
- What confused you?
- What proof is missing?
- Which buyer would care first?
- What claim should we sharpen or soften?
- Who should we talk to next?
- What would make this investable, partner-ready, or customer-ready?

## Closing ask
We want feedback on the buyer pain, the proof standard, the first ICP, the evaluation offer, and the specific intros or diligence steps that would make ATOMiK more partner-ready.

## Required close
ATOMiK is not asking the market to accept a broad compute claim. We are asking the right customers to bring one constrained state path, measure the waste, and decide with evidence whether state-aware compute belongs in their architecture.
