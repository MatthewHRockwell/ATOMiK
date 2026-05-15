# ATOMiK — Investor Data Room

> Publication status: historical / diligence draft. Do not share this data room
> externally without reconciling claims against `../../results/claims_registry.yaml`
> and the evidence definitions in `../../docs/evidence-labels.md`.

This data room contains due diligence materials for ATOMiK. Some documents in
this folder predate the current public-positioning pass and may include older
performance, production, customer, pricing, or roadmap language.

*Last updated: March 2026*

---

## Table of Contents

| Section | Directory | Contents | Description |
|---------|-----------|----------|-------------|
| **Financial** | [01_financial/](01_financial/) | Financial model, revenue model, development cost breakdown | Projections, unit economics, and the ~$225 total development cost history |
| **Legal** | [02_legal/](02_legal/) | Entity status, IP assignment template, license summary | Rockwell Industries LLC (CA) structure, Apache 2.0 evaluation license terms |
| **Intellectual Property** | [03_intellectual_property/](03_intellectual_property/) | Patent status, provisional patent PDF, formal proofs inventory, trade secrets | Patent Pending architecture, 108 Lean4 proofs, IP protection strategy |
| **Team** | [04_team/](04_team/) | Founder profile, advisory board plan | Solo founder background, post-funding hiring plan, advisory needs |
| **Technical** | [05_technical/](05_technical/) | XOR data type explainer, memory traffic analysis, ASIC economics | Deep-dive technical documents for engineering due diligence |
| **Market / pipeline** | [05_customers/](05_customers/) | Target-buyer pipeline | Target verticals (HFT, edge AI, streaming), engagement status |
| **Hiring** | [06_team/](06_team/) | First hires plan | Priority roles, compensation strategy, team scaling roadmap |

---

## Key Documents (Quick Links)

| Document | What It Proves |
|----------|---------------|
| [Provisional Patent PDF](03_intellectual_property/Provisional%20Patent%200.0.1.pdf) | Architecture is under IP protection |
| [Formal Proofs Inventory](03_intellectual_property/formal_proofs_inventory.md) | 108 machine-verified theorems, 0 sorry statements |
| [Development Cost](01_financial/development_cost.md) | ~$225 total spend — extreme capital efficiency |
| [Revenue Model](01_financial/revenue_model_revised.md) | ARM-style IP licensing projections |
| [Founder Profile](04_team/founder_profile.md) | Full-stack technical founder |
| [ASIC Economics](05_technical/asic_economics_clarification.md) | Path from FPGA to production silicon |

---

## Technical Validation (In Repository)

The following validation artifacts are in the main repository, not this data room:

| Artifact | Location | Description |
|----------|----------|-------------|
| Lean4 proofs | `math/proofs/ATOMiK/*.lean` | 108 theorems, 8 files, 0 sorry |
| v3 SoC RTL | `hardware/v3/` | Custom RV64I + ATOMiK + HDMI (1280×720) |
| v2 SoC RTL | `hardware/rtl/` | PicoRV32 + ATOMiK accelerator |
| Hardware tests | `hardware/sim/`, `hardware/v3/sim/` | 80/80 sweep + v3 validation suites |
| SDK source | `software/atomik_sdk/` | 353 tests, 5 languages |
| Zynq port | `hardware/zynq/` | AXI4-Lite wrapper, 52/52 sim tests |
| Benchmark data | `experiments/data/` | Raw CSV measurements |

---

## Sharing

For secure sharing with investors:
1. Use a private GitHub repository with collaborator access
2. Or export as PDF and share via encrypted link
3. Never share credentials or config files

## Regeneration

These documents can be regenerated from project data:

```bash
python business/data_room/_generate.py
```
