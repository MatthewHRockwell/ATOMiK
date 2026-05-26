# ATOMiK Investor Pitch Deck

> Internal long-form reference: 2026-05-23. This is not the generated 12-slide
> PPTX source. Use `slides.md` plus `generate_deck.py` for the current generated
> Aggie Angel deck. Do not share this long-form reference externally without a
> separate review against `docs/evidence-labels.md` and
> `results/claims_registry.yaml`.

## Long-Form Partner Meeting Reference

### Slide 1 - ATOMiK

**Make change the unit of compute.**

State-aware compute evaluation for edge and embedded teams constrained by battery, heat, bandwidth, latency, reliability, or hardware footprint.

Evidence: `HARDWARE_VALIDATED` ATOMiK Desk v0.39-K prototype UI running on live
Zynq hardware.

Speaker frame: ATOMiK is not a traditional desktop and not another generic
processor pitch. It is a delta-state compute architecture: make change the unit
of work, then evaluate where avoiding redundant state movement creates customer
value.

### Slide 2 - The Customer Problem

Many constrained systems waste energy, bandwidth, and time rediscovering what changed.

- Edge products lose battery to scans, copies, sync, and repeated reconstruction.
- Sealed or fanless systems turn redundant state work into heat.
- AI and agent systems move context even when only a small delta matters.
- Remote and defense systems are constrained by every watt, ounce, packet, and
  minute of field runtime.
- Data-center power, cooling, water, and sustainability are strategic expansion themes that still require workload-specific measurement.

Evidence boundary: this is a market/problem framing, not an ATOMiK savings claim.

### Slide 3 - Why Now

Compute growth is colliding with power, cooling, and deployment constraints.

- IEA estimates data centers used about 415 TWh in 2024 and projects roughly
  945 TWh by 2030 in its base case.
- IEA reports cooling can range from about 7% of electricity use in efficient
  hyperscale sites to more than 30% in less-efficient enterprise sites.
- LBNL estimates U.S. data centers used 176 TWh in 2023 and projects roughly
  325-580 TWh in 2028; its water model estimates direct U.S. data-center water
  consumption reached 66 billion liters in 2023.
- Edge and AI systems increasingly need to keep useful state local instead of
  moving everything through constrained links.

Sources: IEA, *Energy and AI: Energy demand from AI*; LBNL, *2024 United States
Data Center Energy Usage Report*.

### Slide 4 - What The Customer Gets

Customers do not buy delta-state algebra. They buy evidence that a constrained state path may improve.

| Outcome | Customer value |
|---|---|
| Bytes moved or avoided | Less state movement against the current baseline |
| Update/reconstruction latency | Faster local state paths where the workload fits |
| Bandwidth pressure | Fewer full-state transfers when measured |
| Operations coalesced | Less repeated work in change-heavy paths |
| Power or thermal proxy | Follow-on measurement when instrumentation exists |
| Correctness preservation | No improvement matters if correctness breaks |

Battery, cooling, water, and smaller hardware outcomes remain evaluation targets until measured on a specific workload.

### Slide 5 - The Primitive

ATOMiK makes change the unit of compute:

```text
state = reference_state XOR accumulated_delta
```

Operationally: load a reference, accumulate deltas, read/reconstruct when
needed, and commit clean epoch boundaries.

Why XOR matters: it is compact, self-inverse, and maps naturally to hardware.
Do not over-explain this slide in the first pass; the investor story is the
customer consequence.

### Slide 6 - Where It Fits

ATOMiK is designed for sparse-change, update-heavy, bandwidth-constrained,
latency-sensitive workloads.

It is **not** positioned as a general-purpose CPU replacement.

Best-fit signals:

- many writes or updates before reads
- repeated change detection
- expensive full-state sync
- rollback / undo / replay overhead
- context retention where the hot delta matters more than full movement

### Slide 7 - Customer Use Cases

| Segment | Pain | Evaluation target |
|---|---|---|
| Edge / embedded | battery, enclosure heat, intermittent links, reliability | bytes moved, update latency, bandwidth, power proxy |
| AI at the edge | context/state movement, memory pressure | context retained, transfer avoided, response time |
| Remote / industrial / robotics | weight, wattage, packet budget, field runtime | runtime proxy, packet budget, update cost |
| Data center / infrastructure | power bill, cooling, water pressure, rack density | measured bytes moved, power/thermal path |

The sales motion should start with one workload, one constraint, and one success
metric.

### Slide 8 - Live Proof Today

Use `website/public/09-current-live-atomik-desk-v039k.png`.

Caption: `HARDWARE_VALIDATED` ATOMiK Desk v0.39-K prototype UI running on live
Zynq hardware.

Do not claim that the screenshot proves performance, power savings, commercial
maturity, or production readiness. It proves the current live demo surface.

### Slide 9 - Proof Stack

| Proof | Label | Status |
|---|---|---|
| Zynq Desk v0.39-K | `HARDWARE_VALIDATED` | current public proof image |
| Linux userspace to FPGA path | `HARDWARE_VALIDATED` | documented OS-to-bus validation path |
| AX7020 board run matrix | `LIVE_MEASURED` | raw artifact with caveats |
| Formal proof work | `SOFTWARE_VALIDATED` | algebraic proof work in repo |
| Standalone SD boot artifacts | `BUILD_ARTIFACT` | local build output exists; public power-on artifact still gated |

The strongest investor posture is: compelling proof, clean boundaries, obvious
next gates.

### Slide 10 - Business Path

Near term:

1. Paid technical evaluations and design-partner work.
2. IP strengthening and diligence packaging.
3. ASIC feasibility review before any tape-out commitment.
4. Strategic licensing / acquisition conversations once proof and workload fit
   are stronger.

Strategic thesis: ATOMiK should not compete head-on with major chip companies;
it should become the architecture or IP they want to own.

### Slide 11 - Use Of Funds

Target ask: **$2.0M pre-seed**.

| Category | Amount | Proof gate |
|---|---:|---|
| Engineering and demo hardening | $600K | repeatable Zynq proof system |
| Customer proof | $400K | measured workload artifact |
| IP and legal | $300K | counsel-reviewed IP packet |
| ASIC feasibility | $300K | mentor-reviewed go/no-go path |
| Finance/GTM/ops and reserve | $400K | runway, reporting, partner pipeline |

Minimum viable close: $1.25M. Stretch plan: $2.75M. Final SAFE terms require
CFO/counsel approval. This round does not fund tape-out.

### Slide 12 - Risks And Evidence Gates

| Risk | Current answer |
|---|---|
| Power / thermal savings are not measured end-to-end | Treat as evaluation targets until artifacts exist |
| Customer validation is pending | Paid evaluations and design partners are the validation wedge |
| ASIC economics require expert review | Fund feasibility review before tape-out |
| Incumbents can replicate | Protect IP, build proof, and pursue strategic conversations |

This slide builds trust. It shows the plan is ambitious but not careless.

### Long-Form Close - The Ask

ATOMiK is raising a $2.0M target pre-seed to fund the transition from working
proof to measured customer-value proof and diligence-grade silicon IP.

ATOMiK is looking for:

- $2.0M target pre-seed funding;
- design-partner introductions;
- ASIC and IP diligence support;
- customer workloads where heat, power, bandwidth, latency, or footprint are
  already painful.

Close with: the next milestone is measured proof, not another abstract demo.

## Diligence Appendix

### Evidence Labels

Use the repo evidence labels exactly:

- `LIVE_MEASURED`: observed on a running system with recorded measurement artifacts
- `HARDWARE_VALIDATED`: demonstrated on physical hardware
- `SOFTWARE_VALIDATED`: shown in software, simulation, or local runtime
- `SYNTHESIS_VALIDATED`: supported by toolchain output, not live board execution
- `BUILD_ARTIFACT`: local build output exists, end-to-end run pending
- `PROJECTED`: model or estimate, not a result
- `CONCEPTUAL` / `ROADMAP`: product direction or planned work

### Current Claim Boundaries

Safe external wording:

- ATOMiK targets wasted state movement so eligible workloads can be evaluated
  for lower heat, lower bandwidth, lower power, and faster state handling.
- ATOMiK Desk v0.39-K is a hardware-validated prototype UI running on Zynq.
- The standalone SD boot path has build artifacts; public power-on logs, FSBL exception handling, and final autonomous handoff are the next gate.
- Power, thermal, water, battery, and footprint improvements require measured
  workload artifacts before being stated as results.

Avoid:

- unqualified speedup, water savings, battery-life, or heat-reduction claims
- "production-ready" or "commercial desktop"
- implying concept visuals are live product functionality
- exact valuation or comparable-company claims without source notes

### Financial And Market Benchmarks

Use these only as context, not as ATOMiK-specific claims:

- Carta reports roughly 3,000 U.S. startups on Carta raised pre-seed funding in
  Q1 2026, totaling over $2.3B, with an expected final total around $2.9B.
- PitchBook/NVCA Q1 2026 reports a median U.S. VC seed pre-money valuation of
  $18.4M. Treat this as a benchmark for CFO discussion, not a target valuation.
- YC's standard deal is $500K: $125K for 7% plus $375K uncapped MFN SAFE.
  Techstars' 2025 offer is $220K with $20K for 5% common equity plus $200K
  uncapped MFN SAFE. These are accelerator structures, not direct angel-round
  comps.
- SIA/WSTS projects global semiconductor sales of $772.2B in 2025 and $975.4B
  in 2026. This is market backdrop only, not an ATOMiK TAM.
- ASIC feasibility should be quote-backed. Public ASIC-cost references describe
  a stack of design, verification, IP, EDA, MPW/mask, wafer, packaging, test,
  yield, qualification, and production support costs.

### Source Notes

- IEA, *Energy and AI: Energy demand from AI*, accessed 2026-05-23:
  https://www.iea.org/reports/energy-and-ai/energy-demand-from-ai
- Lawrence Berkeley National Laboratory, *2024 United States Data Center Energy
  Usage Report*, accessed 2026-05-23:
  https://eta-publications.lbl.gov/sites/default/files/2024-12/lbnl-2024-united-states-data-center-energy-usage-report_1.pdf
- Semiconductor Industry Association / WSTS, October 2025 sales and autumn 2025
  forecast release, accessed 2026-05-23:
  https://www.semiconductors.org/global-semiconductor-sales-increase-4-7-month-to-month-in-october/
- Carta, *State of Pre-Seed: Q1 2026*, accessed 2026-05-23:
  https://carta.com/sg/en/data/state-of-pre-seed-q1-2026/
- PitchBook / NVCA, *Q1 2026 Venture Monitor*, accessed 2026-05-23:
  https://nvca.org/wp-content/uploads/2026/04/Q1-2026-PitchBook-NVCA-Venture-Monitor.pdf
- Y Combinator, *The Y Combinator Deal*, accessed 2026-05-23:
  https://www.ycombinator.com/deal
- Techstars, *Investment Terms Update*, accessed 2026-05-23:
  https://www.techstars.com/newsroom/investment-terms
- AnySilicon, *Semiconductor Manufacturing Cost Breakdown* and *ASIC NRE
  Explained*, accessed 2026-05-23:
  https://anysilicon.com/semiconductor-manufacturing-cost-breakdown/
  https://anysilicon.com/asic-nre-explained/
