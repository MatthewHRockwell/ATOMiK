---
title: ATOMiK Investor Deck
subtitle: Make change the unit of compute
author: ATOMiK
date: May 2026
---

# ATOMiK

## Make change the unit of compute

State-aware compute evaluation for edge and embedded teams constrained by
battery, heat, bandwidth, latency, reliability, or hardware footprint.

`HARDWARE_VALIDATED` ATOMiK Desk v0.39-K runs as a framebuffer-native
prototype UI on Zynq hardware.

---

# The Hidden Tax

Many constrained systems waste energy, bandwidth, and time rediscovering what
changed.

- Edge devices spend limited battery on repeated scans, copies, sync, and replay.
- Sealed or fanless systems turn redundant state work into heat.
- AI and agent systems move context even when only a small delta matters.
- Remote systems are constrained by every watt, ounce, packet, and minute.
- Data-center power, cooling, water, and sustainability are expansion themes that still need workload-specific measurement.

Source context: IEA and LBNL support the broader energy, cooling, and water
pressure around modern compute. These are market-context facts, not ATOMiK
savings claims.

---

# The First Wedge

The first customer is not everyone.

It is the team with:

- one state-heavy workload
- one current baseline
- one painful constraint expensive enough to evaluate

Lead ICP: edge and embedded systems, AI at the edge, remote/field systems,
robotics, industrial control, IoT, and defense-adjacent hardware-constrained
teams.

---

# The Customer Value

Customers do not buy "delta-state algebra."

They buy evidence that a constrained state path may improve:

- bytes moved or avoided
- update/reconstruction latency
- bandwidth pressure
- operations coalesced
- power or thermal proxy where measured responsibly
- correctness preservation

Battery, cooling, water, and smaller hardware outcomes remain evaluation targets
until measured on the workload.

---

# The Primitive

ATOMiK makes change the unit of compute.

```text
state = reference_state XOR accumulated_delta
```

Load a known state. Accumulate compact changes. Reconstruct only when needed.
Commit clean epoch boundaries.

---

# Why It Can Matter

Traditional systems often move or rebuild full state to answer a smaller
question: "what changed?"

ATOMiK is designed for workloads where:

- writes or updates dominate reads
- state changes are sparse or coalescable
- bandwidth is expensive
- latency and power budgets are tight
- rollback, sync, replay, or context retention create overhead

It is not positioned as a general-purpose CPU replacement.

---

# Customer Use Cases

| Buyer | Pain | Evaluation target |
|---|---|---|
| Edge / embedded | battery, enclosure heat, intermittent links, reliability | bytes moved, update latency, bandwidth, power proxy |
| AI at the edge | context/state movement, memory pressure | context retained, transfer avoided, response time |
| Remote / industrial / robotics | weight, wattage, packet budget, field runtime | runtime proxy, packet budget, update cost |
| Data center / infrastructure | power bill, cooling, water pressure, rack density | measured bytes moved, power/thermal path |

Each use case needs one measured workload before ATOMiK claims customer outcomes.

---

# Hardware-Validated UI Proof

![ATOMiK Desk v0.39-K prototype UI running on live Zynq hardware](../../website/public/09-current-live-atomik-desk-v039k.png)

`HARDWARE_VALIDATED` ATOMiK Desk v0.39-K prototype UI running on live Zynq
hardware. This screenshot proves the current live demo surface, not commercial
product maturity or performance.

---

# Proof Stack

| Proof | Label | Status |
|---|---|---|
| v0.39-K Zynq Desk UI | `HARDWARE_VALIDATED` | current public proof image |
| Linux userspace to FPGA path | `HARDWARE_VALIDATED` | documented path through OS and bus |
| AX7020 board run matrix | `LIVE_MEASURED` | raw artifact with caveats and wins/losses |
| Lean4-checked formal algebra | `FORMAL_PROOF` where directly audited; otherwise `SOFTWARE_VALIDATED` | exact formal claims remain bounded to audited properties |
| Synthesis and bank scaling | `SYNTHESIS_VALIDATED` | toolchain/hardware validation, not production silicon |
| Zynq standalone boot artifacts | `BUILD_ARTIFACT` | local build output exists; public power-on artifact still gated |

Public claims stay tied to these labels.

---

# Business Path

The near-term plan is focused and capital-efficient.

1. Convert proof into measured customer evaluations.
2. Strengthen IP and diligence materials.
3. Use design partners to identify workloads where value is measurable.
4. De-risk ASIC feasibility before any tape-out commitment.
5. Position ATOMiK for strategic licensing or platform partnership readiness if proof supports it.

ATOMiK is not trying to outspend chip incumbents.

---

# Use Of Funds

Target ask: **$5M pre-seed**.

| Category | Amount | Proof gate |
|---|---:|---|
| Engineering + demo hardening | $1.5M | repeatable Zynq proof system |
| Customer proof | $1.0M | measured workload artifact |
| IP + legal | $750K | counsel-reviewed IP packet |
| ASIC feasibility | $750K | mentor-reviewed go/no-go path |
| Finance/GTM/ops + reserve | $1.0M | runway, reporting, partner pipeline |

Final SAFE mechanics, staged close structure, valuation cap, discount, and pro-rata require
CFO/counsel approval. This round does not fund tape-out.

---

# Risks And Gates

| Risk | Current answer |
|---|---|
| Battery, power, thermal, cooling, water, or footprint savings are not yet measured end-to-end | Treat as evaluation targets until artifacts exist |
| Customer validation is pending | Paid evaluations and design partners are the validation wedge |
| ASIC economics require expert review | Fund feasibility review before tape-out |
| Incumbents can move quickly | Protect IP, build proof, and pursue strategic conversations |

The pitch is intentionally evidence-bounded.

---

# The Ask

ATOMiK is raising a **$5M target pre-seed** to reach measured proof.

- 18-24 month proof round
- final SAFE mechanics and staged close structure by CFO/counsel
- 2-3 design-partner evaluations
- one sanitized measured workload artifact
- external ASIC/IP feasibility review
- licensing-ready package

The next milestone is measured customer-value proof, not another abstract demo.
