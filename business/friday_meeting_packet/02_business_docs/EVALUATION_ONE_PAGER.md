# Evaluation Offer One-Pager

## Give us X
One state-heavy workload, the current baseline, and the constraint that already hurts.

## We evaluate Y
Where state movement creates waste: repeated scans, full-state transfers, sync loops, replay, reconstruction, repeated writes, or state paths that move more than the meaningful change.

## You receive Z
A workload map, baseline comparison, evidence map, fit/no-fit recommendation, risks and caveats, and a next-step plan.

## Success looks like A
Measured improvement against one agreed metric while preserving correctness and connecting the result to a business constraint worth pursuing.

## What to bring
- Representative workload
- Current implementation or pseudocode
- State model, state size, and update cadence
- Current state movement path
- Current baseline measurements
- Painful constraint: battery, power budget, heat, bandwidth, latency, footprint, weight, reliability, cost, or hardware overbuild
- Target hardware or deployment environment
- Decision metric and threshold
- Available traces, logs, counters, power data, latency data, bandwidth data, or thermal data

Customers do not need to expose the entire product. They need to provide enough about one constrained state path to evaluate relevance.

## What we measure
Primary metrics include bytes moved, bytes avoided, full-state transfers avoided, scans avoided, operations coalesced, unique-region ratio, cycles per update, update latency, reconstruction cost, memory/state footprint, and correctness preservation. Power and thermal are measured only when instrumentation and method are responsible.

## Step-by-step process
1. First response and qualification
2. Intake call
3. Evaluation package
4. State-movement map
5. Success criteria
6. Evaluation path selection
7. Readout
8. Final outcome: no fit, proof review complete, technical evaluation recommended, design partnership recommended, licensing/IP diligence recommended, or investor diligence recommended

## Fit signals
State-heavy path, repeated changes to the same regions, sparse updates, sync/replay/reconstruction cost, measurable baseline, clear constraint, and a continuation threshold.

## No-fit signals
Uniform raw compute, every region truly needs to move, no baseline, no correctness oracle, interface overhead dominates, or the value depends on unsupported downstream claims.

## What happens after evaluation
The next step may be no-fit, proof review, scoped technical evaluation, design-partner proposal, licensing/IP diligence, or investor diligence.

## What ATOMiK does not claim yet
No universal speedup, guaranteed battery extension, guaranteed heat reduction, guaranteed cooling reduction, water savings, guaranteed smaller hardware, production-ready ASIC, or proof from unrelated workloads.
