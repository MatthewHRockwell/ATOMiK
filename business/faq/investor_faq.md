# ATOMiK - Investor FAQ

> Current external-use draft: 2026-05-26. Answers are written for the Aggie
> Angel Network pitch narrative and must stay aligned with
> `docs/evidence-labels.md`.

## What does the customer get out of ATOMiK?

A way to evaluate whether a constrained state path is wasting work. The first
customer-facing value is evidence around bytes moved, full-state transfers
avoided, operations coalesced, latency, bandwidth pressure, power/thermal proxy,
and correctness preservation. Battery, heat, cooling, water, and smaller
hardware outcomes are not claimed as measured results until a workload artifact
proves them.

## What is ATOMiK in one sentence?

ATOMiK is a state-aware compute architecture that helps constrained edge and
embedded teams do less unnecessary state work by tracking meaningful changes
instead of repeatedly moving, scanning, syncing, replaying, or rebuilding full
state.

## Why does that matter commercially?

Many systems repeatedly move, scan, replay, and synchronize full state even when
only a small delta matters. That waste can become battery drain, heat, latency,
bandwidth cost, reliability risk, and hardware complexity. ATOMiK targets
workloads where avoiding that state movement can create measurable customer
value.

## Is ATOMiK a CPU replacement?

No. ATOMiK is not pitched as a general-purpose CPU replacement. It is a
state-aware architecture / accelerator path for sparse-change, update-heavy,
bandwidth-constrained, latency-sensitive workloads.

## Why XOR?

XOR is compact, self-inverse, and maps cleanly to hardware. It supports a simple
state equation, cheap accumulation, clean undo behavior, and parallel merge
patterns. The investor explanation should stop there unless the room asks for
the technical detail.

## What is live today?

ATOMiK Desk v0.39-K is a framebuffer-native prototype UI running on live Zynq
hardware. It is `HARDWARE_VALIDATED` proof of the current demo surface. The repo
also contains Linux userspace-to-FPGA validation, AX7020 board-run artifacts,
formal proof work, synthesis artifacts, and standalone SD boot build artifacts.

## What should we not claim yet?

Do not claim measured heat reduction, water reduction, battery-life extension,
commercial product maturity, production readiness, or end-to-end standalone SD
boot until the corresponding artifact exists. Those are evaluation targets or
next gates.

## What are the first customer segments?

- Edge / embedded devices: battery life, enclosure heat, intermittent links, local latency, reliability.
- AI at the edge: context movement, state pressure, memory pressure, response time.
- Remote / industrial / robotics / defense-adjacent systems: weight, wattage, packet budget, field runtime.
- Data center / infrastructure: power bill, cooling, water pressure, rack density as a strategic expansion path.

## How will customers evaluate ATOMiK?

Start with one workload, one current baseline, one painful constraint, and one
success metric. Examples: bytes moved, full-state transfers avoided, operations
coalesced, cycles per update, update latency, bandwidth avoided, power/thermal
proxy, field runtime, or packet budget. If ATOMiK does not move the agreed
metric while preserving correctness, it is not the right wedge.

## What is the business model?

Near term: paid technical evaluations and design-partner engagements.

Mid term: IP licensing, integration partnerships, and support for customers who
have a validated workload fit.

Long term: strategic licensing, partnership, or acquisition by a chip or
platform company that can scale the architecture.

## Why not compete directly with Intel, NVIDIA, AMD, or ARM?

The current strategic goal is not to outspend incumbents. The better path is to
build proof, protect IP, validate customer workloads, and become a strategic
asset that a major chip or platform company wants to license, partner with, or
acquire.

## How much are you raising?

The current working ask is a $2.0M target pre-seed. The minimum viable close is
$1.25M; the stretch plan is $2.75M. The recommended default instrument is a
post-money SAFE, with valuation cap, discount, pro-rata rights, and close
mechanics to be finalized by the fractional CFO and counsel.

## What does pre-seed capital fund?

The $2.0M target budget funds:

- $600K engineering and demo hardening.
- $400K customer workload proof.
- $300K IP and legal.
- $300K ASIC feasibility.
- $250K finance, GTM, and operations.
- $150K reserve.

The round funds feasibility and measured proof, not tape-out.

## What financial benchmarks are safe to discuss?

Use benchmarks as context, not as ATOMiK-specific claims. Current source-backed
references include IEA/LBNL for data-center energy, cooling, and water pressure;
SIA/WSTS for semiconductor market backdrop; Carta and PitchBook/NVCA for current
pre-seed/seed market context; and YC/Techstars for accelerator-term dilution
context. The working ask is now $2.0M; the CFO still needs to approve final SAFE
terms, valuation cap, discount, and closing mechanics.

## Does pre-seed fund a chip tape-out?

No. The current pre-seed story funds customer proof, IP strengthening, demo
hardening, and ASIC feasibility. Tape-out should remain a later, quote-backed
milestone after workload value and ASIC economics are validated.

## Is the patent filed?

The data room records provisional IP protection and patent-pending positioning.
The near-term funding goal is to strengthen and convert that coverage on the
right schedule with legal support.

## What are the biggest risks?

- Battery, power, thermal, cooling, water, and footprint savings still need workload-specific measurement.
- Customer validation is pending.
- ASIC feasibility needs expert review before tape-out economics are promoted.
- Incumbents could respond quickly if the architecture proves valuable.

## Why is this fundable now?

Because there is enough proof to justify the next diligence step, but the
company is still early enough that pre-seed capital can materially change the
trajectory. The strongest next milestone is measured customer-value proof.
