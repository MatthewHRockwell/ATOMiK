# ATOMiK Business Plan

> Current planning draft: 2026-05-26. Use with the pitch deck, one-pager,
> evaluation offer, proof packet, and data-room index. Financial terms require
> CFO/counsel review.

## Company Thesis

ATOMiK makes change the unit of compute. Many constrained systems waste effort
moving, scanning, syncing, replaying, or rebuilding state they already know.
ATOMiK evaluates whether tracking meaningful change can reduce that waste in one
real workload.

## Positioning

ATOMiK is a state-aware compute architecture that helps constrained edge and
embedded teams do less unnecessary state work by tracking meaningful changes
instead of repeatedly moving, scanning, syncing, replaying, or rebuilding full
state.

## Lead Customer

The first customer is an edge or embedded systems team with:

- one representative state-heavy workload
- one current baseline
- one painful constraint expensive enough to evaluate

Primary segments include AI at the edge, remote/field systems, robotics,
industrial control, IoT, defense-adjacent systems, and other
hardware-constrained products. Data center and infrastructure partners are
strategic expansion paths once workload-specific power/thermal evidence exists.

## Buyer Pain Hierarchy

Direct first-order pains:

- battery or power budget
- heat in sealed or fanless systems
- bandwidth pressure
- update/reconstruction latency
- reliability, field-service, size, weight, or hardware footprint

Derived or expansion pains:

- cooling cost
- water use
- sustainability reporting
- data-center power bills
- rack density

Derived pains should be discussed as evaluation targets unless measured for the
specific workload.

## Evaluation Offer

Give us one state-heavy workload, your current baseline, and the constraint that
already hurts. We evaluate where state movement creates waste and whether ATOMiK
can improve the path. You receive a workload map, baseline comparison, evidence
map, fit/no-fit recommendation, and next-step plan. Success looks like measured
improvement against one agreed metric while preserving correctness.

## Commercial Path

1. Proof review reservations qualify interest and route diligence.
2. Technical evaluations map one workload and one metric.
3. Design-partner evaluations produce deeper customer-specific evidence.
4. Licensing/IP diligence opens chip, embedded, infrastructure, and strategic partner paths.
5. Commercial licensing or acquisition discussions become credible only after measured workload proof.

## Business Model

Near term revenue is service-like: paid proof reviews, technical evaluation
reservations, scoped evaluation SOWs, and design-partner work. Long-term revenue
can expand into embedded IP licensing, commercial support, strategic integration,
or acquisition by a chip/platform company.

Public reservation prices, if shown, are qualification mechanisms and do not
represent the full commercial model.

## Proof Strategy

Every claim needs a label, artifact, context, and caveat.

Evidence labels:

- `LIVE_MEASURED`
- `HARDWARE_VALIDATED`
- `SOFTWARE_VALIDATED`
- `SYNTHESIS_VALIDATED`
- `BUILD_ARTIFACT`
- `PROJECTED`
- `CONCEPTUAL`
- `ROADMAP`

Current proof stack includes the v0.40-A Zynq Desk UI proof image (captured
from /dev/fb0, driven by real measured on-board data), live-measured
parallel-bank throughput on the AX7020 (1/2/4/8x, byte-identical to software),
Linux userspace-to-FPGA validation, AX7020 board-run matrix, hardware synthesis
artifacts, formal proof work, and build artifacts for the standalone SD boot
path. Do not isolate the biggest performance number without artifact, context,
and caveat.

## Status Quo Alternatives

Customers currently buy margin through more compute, bigger batteries, more
cooling, more bandwidth, compression, caching, deduplication, sync protocols,
specialized accelerators, cloud offload, overbuilt hardware, manual optimization,
or reducing feature scope. ATOMiK differs by evaluating whether less state needs
to move in the first place.

## Go-To-Market

Near-term GTM is founder-led and evidence-led:

- identify teams with constrained state paths
- ask for one workload, one baseline, and one constraint
- run proof review or workload mapping
- convert fit cases into scoped technical evaluation or design partnership
- build a sanitized public workload artifact when allowed

## 12-18 Month Milestones

- 2-3 design-partner evaluations
- one sanitized measured workload artifact
- hardened proof packet and claims registry
- external ASIC/IP feasibility review
- repeatable evaluation tooling
- licensing-ready partner package

## Financing Plan

Working ask: $2.0M target pre-seed.

- Minimum viable close: $1.25M
- Stretch plan: $2.75M
- Planned runway: about 18 months
- Default structure: post-money SAFE, subject to CFO/counsel approval

Use of funds:

- $600K engineering and demo hardening
- $400K customer workload proof
- $300K IP and legal
- $300K ASIC feasibility
- $250K finance, GTM, and operations
- $150K reserve

This round funds measured proof and feasibility, not tape-out.

## Claim Boundaries

Do not claim universal speedup, guaranteed battery extension, guaranteed heat
reduction, guaranteed cooling or water savings, guaranteed smaller hardware,
production-ready ASIC, or replacement of CPUs/GPUs/accelerators/compression/
caching/sync protocols. Those can become measured workload outcomes only when a
specific artifact supports the exact claim.
