# ATOMiK Evaluation One-Pager Source

## Headline

Bring one constrained state path. Leave with evidence.

## The Simple Offer

Give us: one state-heavy workload, your current baseline, and the constraint that already hurts.

We evaluate: where state movement creates waste and whether ATOMiK can improve the path.

You receive: a workload map, baseline comparison, evidence map, fit/no-fit recommendation, and next-step plan.

Success looks like: measured improvement against one agreed metric while preserving correctness and showing enough economic or technical value to justify a design-partner evaluation.

## What To Bring

The best first evaluation starts with one real workload, one baseline, and one constraint.

Bring as many as possible:

- Representative workload.
- Current implementation or pseudocode.
- State model, state size, and update frequency.
- Current data or state movement path.
- Current baseline measurements.
- Painful constraint: battery, heat, bandwidth, latency, footprint, weight, reliability, cost, or compute density.
- Target hardware or deployment environment.
- Decision metric and threshold for continuing.
- Available traces, logs, counters, power data, latency data, bandwidth data, or thermal data.

Customers do not need to expose their entire product. They need to provide enough about one constrained state path to evaluate whether ATOMiK is relevant.

## What We Measure

Possible primary metrics include bytes moved, bytes avoided, full-state transfers avoided, state scans avoided, replay/reconstruction cost, operations coalesced, unique-region ratio, cycles per update, update latency, response latency, duty cycle, wake-up frequency, memory/state footprint, power proxy, thermal proxy, bandwidth pressure, correctness preservation, and target business outcome.

## Process

1. First response and qualification.
2. Intake call.
3. Customer evaluation package.
4. ATOMiK state-movement map.
5. Success criteria.
6. Evaluation path.
7. Readout.
8. Final outcome.

## Fit Signals

- The workload repeatedly moves, scans, syncs, replays, or rebuilds state.
- The team can provide a current baseline and decision threshold.
- Battery, heat, bandwidth, latency, footprint, weight, reliability, or cost is already painful.
- Correctness can be checked against an expected state, invariant, or output.
- The value is large enough to justify evaluation work.

## No-Fit Signals

- There is no measurable constraint.
- The workload is mostly stateless compute.
- The customer cannot share representative state behavior.
- The baseline is unknown and cannot be measured.
- Correctness cannot be verified.
- The pain is not expensive enough to support evaluation.

## Next Steps

- Proof review.
- Technical evaluation.
- Design-partner evaluation.
- Licensing/IP diligence.
- Investor diligence.

## What ATOMiK Does Not Promise Yet

No universal speedup, guaranteed battery extension, guaranteed heat reduction, guaranteed water savings, guaranteed smaller hardware, production-ready ASIC claim, or proof from unrelated workloads.
