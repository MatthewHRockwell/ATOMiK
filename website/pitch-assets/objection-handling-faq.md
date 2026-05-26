# ATOMiK Objection-Handling FAQ

## What is ATOMiK?

ATOMiK is a state-aware compute architecture that helps edge and embedded teams reduce wasted state movement by tracking meaningful change instead of repeatedly moving, scanning, syncing, or rebuilding full state.

## What is proven today?

The current evidence includes a public proof packet, evidence labels, claims registry, Linux userspace-to-FPGA validation, AX7020 board-run workload matrix, synthesis artifacts, and formal proof work. Each claim is bounded by artifact and label.

## Is ATOMiK always faster?

No. The AX7020 matrix is valuable because it shows both wins and losses. ATOMiK is strongest when redundant state movement, repeated scans, replay, reconstruction, or coalescing opportunities dominate the workload.

## Where does ATOMiK lose?

Naive hardware access can lose when MMIO or integration overhead dominates. Large uniform workloads with high unique-region ratios may favor software. Small AGENT-style paths can lose when sort/prune overhead exceeds skip savings.

## Why would a customer pay for evaluation?

Because the customer already has one painful constrained state path and needs evidence before changing architecture. The evaluation produces a workload map, baseline comparison, evidence map, fit/no-fit recommendation, and next-step plan.

## Who buys first?

Edge and embedded teams with one state-heavy workload, one baseline, and one expensive constraint: battery, heat, bandwidth, latency, footprint, weight, reliability, cost, or compute density.

## What is the moat?

The moat needs to be presented as a combination of architecture, proof discipline, implementation know-how, claims/evidence infrastructure, and potential IP/licensing path. Do not overstate patent or ASIC claims without current legal/IP artifacts.

## Is Atom AI a product?

Not unless explicitly productized. In the Friday materials, Atom AI is a guided evaluation concept and brand visual. It should be labeled conceptual when shown as a UI or assistant.

## What is the business model?

Proof review reservations, technical evaluation reservations, scoped design-partner evaluations, licensing/IP diligence, and commercial licensing or embedded IP paths.

## What is the ask?

The working ask is a $2.0M target pre-seed to convert proof into paid evaluations, design partners, ASIC/IP diligence, and commercial licensing readiness. Minimum viable close is $1.25M; stretch plan is $2.75M; final SAFE terms require CFO/counsel approval.
