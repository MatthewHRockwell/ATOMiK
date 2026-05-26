# ATOMiK - Investor Data Room

> Current diligence draft: 2026-05-26. Do not share externally without a final
> review against `../../results/claims_registry.yaml` and
> `../../docs/evidence-labels.md`.

This data room supports the current ATOMiK investor narrative: customer value
first, evidence-bounded proof second, and silicon/IP readiness as the next gate.
Older historical materials may still exist elsewhere in `business/`; this folder
should be treated as the current diligence packet only when files carry a current
2026-05-26 status or have been explicitly reviewed.

## Current Pitch Narrative

ATOMiK makes change the unit of compute. The lead commercial path is an
evaluation-first motion for edge and embedded teams with one state-heavy
workload, one current baseline, and one painful constraint.

Direct first-order evaluation pains:

- battery or power budget
- heat in sealed or fanless systems
- bandwidth pressure on intermittent or expensive links
- update/reconstruction latency
- reliability, field-service, size, weight, or hardware-footprint pressure

Derived or expansion pains:

- data-center power bills
- cooling and water pressure
- sustainability reporting
- rack density and infrastructure overbuild

Power, thermal, water, battery, cooling, and footprint improvements are
evaluation targets until measured on a specific workload.

## Table Of Contents

| Section | Directory | Contents | Current use |
|---|---|---|---|
| Financial | [01_financial/](01_financial/) | Financial model, revenue scenario worksheet, development cost | Use `financial_model.md` as the current source-backed planning draft; CFO review required. |
| Legal | [02_legal/](02_legal/) | Entity status, IP assignment template, license summary | Confirm legal status before investor distribution. |
| Intellectual Property | [03_intellectual_property/](03_intellectual_property/) | Patent status, provisional patent PDF, formal proof work inventory | Use current patent status doc; counsel review required. |
| Team | [04_team/](04_team/) | Founder profile, advisory / fractional support plan | Use current advisor plan for CFO / ASIC mentor / customer advisor story. |
| Technical | [05_technical/](05_technical/) | Technical explainers and ASIC feasibility guidance | Evidence review required before quoting metrics externally. `asic_economics_clarification.md` is the current tape-out boundary. |
| Market / pipeline | [05_customers/](05_customers/) | Target-buyer pipeline | Use only as internal planning unless updated. |
| Hiring | [06_team/](06_team/) | First hires and fractional roles | Use current first-hires doc for sequencing. |

## Current Key Documents

| Document | Why it matters |
|---|---|
| [Patent Status](03_intellectual_property/patent_status.md) | Current IP status, conversion gate, and counsel-review needs. |
| [Financial Model](01_financial/financial_model.md) | Source-backed planning model for pre-seed use of funds; CFO review required. |
| [Pre-Seed Financing Plan](01_financial/preseed_financing_plan.md) | $2.0M target ask with minimum/target/stretch budgets and 18-month milestones. |
| [Valuation And Terms Notes](01_financial/valuation_terms_notes.md) | Internal term-discipline notes; do not share as investor copy. |
| [Revenue Model Scenario Draft](01_financial/revenue_model_revised.md) | Internal scenario worksheet; no revenue forecast or valuation claim. |
| [ASIC Strategy Clarification](05_technical/asic_economics_clarification.md) | Feasibility-first boundary: pre-seed does not claim to fund tape-out. |
| [Advisory Board And Fractional Support Plan](04_team/advisory_board_plan.md) | Fractional CFO, ASIC mentor, and customer advisor plan. |
| [First Hires And Fractional Roles](06_team/first_hires.md) | Hiring sequence tied to proof gates. |
| [Formal Proof Work Inventory](03_intellectual_property/formal_proofs_inventory.md) | Proof-work inventory; do not quote counts until audited. |

## Current Technical Proof Outside Data Room

| Artifact | Label | Location |
|---|---|---|
| Zynq Desk v0.39-K live UI proof | `HARDWARE_VALIDATED` | `../../website/public/09-current-live-atomik-desk-v039k.png` |
| Linux userspace to FPGA validation | `HARDWARE_VALIDATED` | `../../docs/LINUX_USERSPACE_PROOF.md` |
| AX7020 board run matrix | `LIVE_MEASURED` | `../../results/perf_matrix_ax7020_20260509.txt` |
| Hardware synthesis and bank scaling | `SYNTHESIS_VALIDATED` | `../../docs/HARDWARE_SYNTHESIS.md` |
| Standalone SD boot artifacts | `BUILD_ARTIFACT` | `../../hardware/zynq/fsbl_build/BOOT.bin` plus local probe notes |
| Visual asset rules | N/A | `../../docs/visual-asset-manifest.md` |

## Sharing Rules

- Share the updated pitch deck and one-pager first.
- Share data-room files only after claim review.
- Do not quote historical memory-traffic, power, thermal, market-size, customer,
  or revenue numbers unless the source artifact and evidence label are attached.
- Never share credentials, private keys, raw config files, or unreviewed build
  artifacts.
