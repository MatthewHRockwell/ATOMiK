# ASIC Strategy Clarification - Feasibility First

> Current diligence draft: 2026-05-23. This is a planning document, not a
> tape-out budget. ASIC costs must be quote-backed by an ASIC mentor, design
> services partner, foundry shuttle program, or packaging/test partner before
> being used externally.

## Key Point

The pre-seed story should not be a tape-out round. The credible story is:

> Pre-seed funds customer proof, IP strengthening, and ASIC feasibility. A
> tape-out decision comes later, only if measured workload value and expert cost
> review justify it.

This is important because the customer benefit narrative depends on lower heat,
power, bandwidth, latency, and hardware pressure. Those benefits have to be
measured before the company can responsibly commit to custom silicon economics.

## What Pre-Seed Can Buy

| Workstream | Purpose | Output |
|---|---|---|
| Zynq demo hardening | Make the live proof repeatable and lower-friction. | Investor/design-partner demo package. |
| Customer workload evaluation | Measure one painful use case at a time. | Heat, power, bandwidth, latency, or footprint artifact. |
| RTL / architecture review | Identify what is reusable IP and what needs cleanup. | ASIC-readiness gap list. |
| ASIC feasibility study | Explore node, die area, power, timing, verification, packaging, and test strategy. | Go/no-go and next-budget recommendation. |
| IP and legal packaging | Align technical proof with patent and licensing posture. | Diligence-ready IP packet. |

## ASIC Cost Reality

ASIC cost is not one number. Public ASIC-cost references consistently treat it
as a stack of:

- architecture and RTL work;
- verification and testbench coverage;
- EDA tools and IP licensing;
- physical design, layout, and signoff;
- MPW shuttle or mask set;
- wafer fabrication, yield, packaging, test, qualification, and production
  support.

Because each component depends on node, die size, interfaces, package, volume,
and verification scope, ATOMiK should talk in phases rather than unsupported
dollar figures.

## Stage Gates

| Stage | Decision | Funding posture |
|---|---|---|
| Feasibility | Is a custom silicon path technically and economically rational? | Pre-seed can fund this. |
| MPW / shuttle prototype | Is first silicon worth pursuing after workload proof? | Later financing or strategic partner. |
| Full mask / production | Is volume, yield, packaging, and test economics justified? | Not a pre-seed claim. |
| Advanced-node implementation | Does the workload require it and can economics support it? | Future strategic decision only. |

## Why FPGA-First Is The Revenue Strategy

ATOMiK can pursue revenue before manufacturing a chip if a customer has an
FPGA, embedded, or system workload where the architecture measurably reduces a
painful cost. That path is more credible than claiming an ASIC is immediately
required.

FPGA-first means:

- faster external evaluation;
- less capital risk before customer pull is proven;
- clearer evidence for whether ASIC economics are worth pursuing;
- stronger strategic licensing posture if the proof is compelling.

## What ASIC Feasibility Must Answer

| Question | Why it matters |
|---|---|
| What is the minimal viable ASIC scope? | Prevents overbuilding. |
| What verification scope is required? | Verification is often the hidden cost driver. |
| Which process node fits the workload? | Mature nodes may be more practical than advanced nodes. |
| What die size, package, and test strategy are plausible? | Drives unit economics and yield risk. |
| What IP blocks are required? | IP can reduce schedule risk but adds licensing and integration cost. |
| Does FPGA-only or FPGA-plus-IP licensing remain better? | A valid outcome may be "do not tape out yet." |

## Public-Safe Wording

Use:

> "Pre-seed funds ASIC feasibility, not a tape-out. The next silicon milestone
> is an expert-reviewed feasibility study tied to measured customer workloads."

Avoid:

- exact tape-out cost claims without quotes;
- claims that seed/pre-seed funds production silicon;
- unqualified comparisons to ARM, Cadence, Synopsys, or CEVA revenue;
- power, thermal, or battery claims without workload measurements;
- "10x-100x ASIC performance" language until an expert-reviewed model exists.

## Source Register

- AnySilicon, *Semiconductor Manufacturing Cost Breakdown*, accessed
  2026-05-23:
  https://anysilicon.com/semiconductor-manufacturing-cost-breakdown/
- AnySilicon, *ASIC NRE Explained*, accessed 2026-05-23:
  https://anysilicon.com/asic-nre-explained/
