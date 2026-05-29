# Financial Model - ATOMiK

> Current diligence draft: 2026-05-27. This is planning material, not an
> approved signed financing term sheet. The current working ask is $2.0M; final
> valuation, instrument details, runway model, and closing mechanics should be
> approved by the fractional CFO and counsel before signatures.

## Positioning

ATOMiK should fund the transition from working proof to diligence-grade silicon
IP. The near-term business path is not mass manufacturing and not tape-out. It is measured customer proof, IP strengthening, ASIC feasibility
review, and strategic licensing / partnership positioning.

The customer-value narrative is financially credible when it is framed as:

- reduce wasted state movement before it becomes heat, power draw, bandwidth,
  latency, battery drain, or larger hardware profiles;
- prove that value on one workload at a time;
- use the proof to justify paid evaluations, design-partner work, licensing, or
  strategic licensing or platform-partnership interest.

## Source-Backed Market Context

| Topic | Current public benchmark | How to use it |
|---|---|---|
| Data-center energy | IEA estimates data centers used about 415 TWh in 2024 and projects about 945 TWh by 2030 in its base case. | Supports "why now"; not an ATOMiK savings claim. |
| Cooling burden | IEA reports cooling can range from about 7% of data-center electricity in efficient hyperscale sites to more than 30% in less-efficient enterprise sites. | Supports the less-heat narrative; do not claim ATOMiK reduces cooling until measured. |
| U.S. data-center growth | LBNL estimates U.S. data centers used 176 TWh in 2023 and projects roughly 325-580 TWh in 2028. | Supports customer urgency and infrastructure pressure. |
| Water pressure | LBNL estimates direct U.S. data-center water consumption rose from 21.2 billion liters in 2014 to 66 billion liters in 2023, and hyperscale 2028 direct use could reach 60-124 billion liters. | Use carefully: lower heat may reduce cooling pressure, but water savings require site-specific measurement. |
| Semiconductor market | SIA reported 2025 global semiconductor sales of $791.7B and said Q1 2026 sales reached $298.5B, with 2026 still on track for roughly $1T. | Market backdrop only; do not turn into a TAM claim without a segmentation model. |
| Pre-seed market | Carta reports about 3,000 U.S. startups on Carta raised pre-seed funding in Q1 2026, totaling over $2.3B with an expected final total around $2.9B. | Shows active pre-seed market; not a valuation recommendation. |
| Seed valuation benchmark | PitchBook/NVCA Q1 2026 shows median U.S. VC seed pre-money valuation of $18.4M. | A reference point for CFO discussion, not ATOMiK's target valuation. |
| Seed deal-size benchmark | PitchBook/NVCA Q1 2026 shows median U.S. VC seed deal value of $3.0M. | Supports a disciplined $2.0M target ask as below median seed, not a valuation claim. |
| Accelerator terms | YC's standard deal is $500K: $125K for 7% plus $375K on an uncapped MFN SAFE. Techstars' 2025 standard offer is $220K with $20K for 5% common equity plus $200K uncapped MFN SAFE. | Useful dilution context; not directly comparable to an angel/pre-seed round. |
| ASIC cost discipline | AnySilicon describes ASIC cost as a stack of design, verification, IP, EDA, mask/MPW, packaging, test, yield, qualification, and production support. | Supports "feasibility first"; avoid false precision before expert quotes. |


## Recommended Financing Package

Investor-facing working ask, pending CFO/counsel approval:

| Plan | Amount | Runway | Use |
|---|---:|---|---|
| Minimum viable close | $1.25M | ~12 months | Finish IP conversion, harden demo, and run first measured workload evaluation. |
| Target raise | $2.0M | ~18 months | Recommended ask: customer proof, IP strengthening, ASIC feasibility, first technical capacity, and financing operations. |
| Stretch plan | $2.75M | ~18 months at accelerated spend | Faster hiring and deeper evaluation/ASIC feasibility support without claiming tape-out; longer only if spend is not accelerated. |

Suggested default instrument: post-money SAFE. Valuation cap, discount,
pro-rata rights, and close mechanics should be set by the fractional CFO and
counsel.

See [preseed_financing_plan.md](preseed_financing_plan.md) for the full budget,
minimum/target/stretch scenarios, and milestone plan.

## Revenue Model Candidates

| Stream | Timing | What must be true |
|---|---|---|
| Paid technical evaluations | Near term | One workload, one buyer pain, one success metric. |
| Design-partner engagements | Near term | Customer supplies workload access and agrees to measured proof gates. |
| Sponsored proof work | Near term | Customer or strategic partner funds integration work around a bounded evaluation. |
| IP licensing | Mid term | Legal packaging, workload proof, integration path, and diligence-ready claims. |
| Integration / support | Mid term | A validated workload fit creates recurring support demand. |
| Strategic licensing or platform partnership | Long term | Strategic outcome thesis; not a guaranteed exit or valuation claim. |

## Pre-Seed Use Of Funds

The ask should be built bottom-up from milestones, not from a generic market
round size. A credible pre-seed plan should buy 12-18 months of proof progress
and avoid spending as though the company is already ready for production
silicon.

| Category | Purpose | CFO / advisor output |
|---|---|---|
| IP / legal | Convert and strengthen provisional IP coverage; prepare diligence memo. | Patent conversion budget and counsel timeline. |
| Customer proof | Run measured evaluations around heat, power, bandwidth, latency, or footprint. | First evaluation package, pricing, and success metrics. |
| Hardware diligence | ASIC mentor, feasibility scope, synthesis/power/area review. | ASIC feasibility plan and quote-backed budget. |
| Demo packaging | Make the Zynq proof lower-friction for investor and partner meetings. | Demo reliability checklist and artifact plan. |
| Finance / ops | Fractional CFO, entity/readiness work, investor reporting. | Round size, instrument, valuation range, runway, and dilution model. |

## Financing Guardrails

- Publish the $2.0M target ask only as a CFO-ready working plan until final
  approval. Do not publish a valuation cap or priced-round valuation until
  CFO-reviewed.
- Do not treat the PitchBook/NVCA $18.4M median seed pre-money figure as
  ATOMiK's valuation. It is only one market benchmark.
- Do not imply accelerator terms are equivalent to angel or seed terms; they
  include program/network value and different equity structures.
- Do not include tape-out in the pre-seed budget. Pre-seed funds feasibility
  and measured proof, not tape-out.
- Do not forecast revenue as a result. Revenue models stay `PROJECTED` until
  signed customer agreements exist.

## What Not To Overstate

- Do not publish revenue projections as a forecast until CFO-reviewed.
- Do not present heat, water, battery, or footprint savings as measured results
  without linked artifacts.
- Do not imply a tape-out is funded, scheduled, or economical until expert
  feasibility work supports it.
- Do not turn the semiconductor market into TAM without a source-backed
  segmentation model.
- Do not use strategic-exit language as a promised outcome.

## Next CFO Deliverables

1. Approve or revise the $2.0M target ask and SAFE structure.
2. 12-18 month budget tied to proof gates.
3. Valuation rationale and dilution scenarios.
4. Minimum viable financing plan if the round is smaller than target.
5. Investor-ready financial appendix with assumptions separated from facts.
6. Quote-backed ASIC feasibility budget and explicit "no tape-out yet" boundary.

## Current Financial Positioning

The fundable claim is that a focused pre-seed round can move ATOMiK from
prototype proof to measured customer-value proof and silicon IP readiness.
The deck should sell the size of the problem, the clarity of the next proof
gates, and the strategic upside while staying disciplined about what is not yet
measured.

## Financial Diligence Addendum - 2026-05-27

The financial model has been upgraded from a static planning workbook into a formula-backed diligence workbook. It now includes use-of-funds totals, runway and burn sensitivity, evaluation-pricing signals, milestone gates, dilution sensitivity, market context, cap-table caveats, and financial claim boundaries.

### Runway Arithmetic

| Scenario | Raise | Reserve | Spendable cash | Planning runway | Gross monthly budget | Ex-reserve monthly budget |
|---|---:|---:|---:|---:|---:|---:|
| Minimum | $1.25M | $100K | $1.15M | 12 months | ~$104K/mo | ~$96K/mo |
| Target | $2.0M | $150K | $1.85M | 18 months | ~$111K/mo | ~$103K/mo |
| Stretch | $2.75M | $150K | $2.60M | 18 months | ~$153K/mo | ~$144K/mo |

Stretch funding should be described as ~18 months at accelerated spend, not guaranteed longer runway. The spend should be staged by milestone and quote-backed vendor needs.

### Post-Money SAFE Dilution Sensitivity

This is not a valuation recommendation. It shows simple ownership sold under post-money SAFE math before option pool, prior instruments, side letters, and priced-round mechanics.

| Post-money cap | $1.25M min | $2.0M target | $2.75M stretch |
|---:|---:|---:|---:|
| $8M | 15.6% | 25.0% | 34.4% |
| $10M | 12.5% | 20.0% | 27.5% |
| $12M | 10.4% | 16.7% | 22.9% |
| $15M | 8.3% | 13.3% | 18.3% |
| $18M | 6.9% | 11.1% | 15.3% |

Do not offer a valuation cap in the room unless CFO/counsel has approved it. If asked, say the ask and use of funds are set, and the cap will be finalized around dilution, lead-check size, close structure, and counsel feedback.

### 18-Month Target Cash Plan

The workbook now includes a monthly cash-plan tab. It treats the $2.0M target as $1.85M spendable cash plus a $150K reserve. The monthly plan is a planning draft, not an approved hiring commitment. It allocates the target plan across engineering capacity, demo tooling, customer workload proof, IP/legal, ASIC feasibility, and finance/GTM/ops.

The monthly tab should be used to answer "how do you get 18 months?" It should not be used to promise exact hiring dates or vendor spend before CFO/accounting and vendor quotes are complete.

### Evaluation Pricing Treatment

Current public reservation prices are qualification mechanisms, not the full commercial model.

| Offer | Current public price | Financial treatment |
|---|---:|---|
| Proof review reservation | $750 | Small qualification signal; may be credited to larger scoped work. |
| Technical evaluation reservation | $2,500 | Scoping signal; final deliverables and terms require written scope. |
| Scoped design-partner evaluation | Request-based | First plausible material customer-funded proof path; no forecast until signed SOW. |
| Licensing / IP diligence | Request-based | Future/mid-term path after workload proof, IP packet, and integration path. |

The raise should not be underwritten by reservation revenue. Reservation activity validates interest and creates evaluation conversations; material revenue requires signed SOWs or licensing terms.

## Source Register

- IEA, *Energy and AI: Energy demand from AI*, accessed 2026-05-23:
  https://www.iea.org/reports/energy-and-ai/energy-demand-from-ai
- Lawrence Berkeley National Laboratory, *2024 United States Data Center Energy
  Usage Report*, accessed 2026-05-23:
  https://eta-publications.lbl.gov/sites/default/files/2024-12/lbnl-2024-united-states-data-center-energy-usage-report_1.pdf
- Semiconductor Industry Association, *Global Annual Semiconductor Sales Increase 25.6% to $791.7 Billion in 2025*, accessed 2026-05-27:
  https://www.semiconductors.org/global-annual-semiconductor-sales-increase-25-6-to-791-7-billion-in-2025/
- Semiconductor Industry Association, *Global Semiconductor Sales Increase 25% from Q4 2025 to Q1 2026*, accessed 2026-05-27:
  https://www.semiconductors.org/global-semiconductor-sales-increase-25-from-q4-2025-to-q1-2026/
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
