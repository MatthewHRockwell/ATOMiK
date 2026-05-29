# Customer Pipeline And Evaluation Targets

> **Publication status: INTERNAL PLANNING / NOT TRACTION.**
> This file is a target map, not evidence of customer commitments. Do not imply
> LOIs, pilots, revenue, or customer validation until artifacts exist.

## Current Customer Narrative

ATOMiK should lead with the current edge/embedded wedge: one state-heavy
workload, one current baseline, and one painful constraint. Buyer outcomes such
as longer battery life, lower heat, lower bandwidth pressure, faster local state
paths, and smaller hardware profiles remain evaluation targets until measured
for the specific workload.

## Priority Segments

| Segment | Buyer pain | First evaluation metric |
|---|---|---|
| Edge / embedded device teams | battery budget, enclosure heat, intermittent links, reliability | bytes moved, full-state transfers avoided, update latency, bandwidth pressure, power/thermal proxy if instrumented |
| AI at the edge | context/state movement, memory pressure, local response | context retained, transfers avoided, response latency |
| Remote / industrial / robotics / defense-adjacent | weight, wattage, packet budget, reliability | packet budget, update cost, runtime proxy, field constraint mapping |
| FPGA / embedded teams | state movement inside constrained hardware | resource use, update latency, operations coalesced, integration effort |
| Data centers / infrastructure | power bill, cooling, water pressure, rack density | measured bytes moved, power/thermal path, energy measurement only when instrumented |

## Outreach Status

| Target class | Status | Evidence boundary |
|---|---|---|
| Edge / embedded device teams | Targeting | No customer validation claim yet. |
| Data-center / infrastructure design partners | Strategic / later path | No customer validation claim yet. |
| AI-at-edge builders | Targeting | No customer validation claim yet. |
| Defense / remote operations advisors | Targeting | No customer validation claim yet. |
| Semiconductor / FPGA IP advisors | Targeting | No customer validation claim yet. |

## Evaluation Offer Shape

1. Identify one workload where state movement is visibly painful.
2. Define one metric before the evaluation starts.
3. Run ATOMiK against the baseline and collect artifacts.
4. Report results only at the evidence level supported by the artifact.
5. Convert the result into a paid evaluation, design-partner memo, or no-fit
   decision.

## Near-Term Work

- Pick the first edge/embedded workload example before the Aggie Angel meeting.
- Prepare one-page evaluation language for edge/embedded prospects first; keep data-center language as strategic expansion context.
- Keep heat, water, battery, power, and footprint claims as evaluation targets
  until measured.
