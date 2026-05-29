# ATOMiK Pitch Materials - Independent Evaluator Findings

Last updated: 2026-05-27
Scope: `business/for_codex` investor package, cross-checked against `results/claims_registry.yaml`, `docs/evidence-labels.md`, `docs/LINUX_USERSPACE_PROOF.md`, `hardware/zynq/BASELINE.md`, and `docs/perf/20260509_matrix_interpretation.md`.

## Current Verdict

The package is now substantially safer and clearer than the starting version. The main story is coherent: ATOMiK makes change the unit of compute, starts with edge/embedded constrained-state workloads, and raises $2.0M to turn hardware proof into measured workload proof and IP readiness.

The materials still need human signoff before Friday on financing/legal terms, current SD-boot status, and any customer/pipeline statements.

## P0 Findings Addressed In This Pass

1. Overbroad buyer claim softened.
   - Previous risk: "Every constrained system" implied universal applicability.
   - Current treatment: "Many constrained systems" and "where the workload fits."
   - Files touched: `01_pitch_materials/ATOMiK_Investor_Deck.pptx`, `01_pitch_materials/ATOMiK_Talking_Points.md`, generated DOCX files, `04_source_markdown/*`.

2. Proof language corrected.
   - Previous risk: "16/16 algebraic proofs pass" blurred formal proof work with hardware property checks.
   - Current treatment: "16/16 algebraic checks pass" and "formal proof work present; avoid public counts until audited."
   - Evidence basis: `docs/LINUX_USERSPACE_PROOF.md`, `hardware/zynq/BASELINE.md`, `results/claims_registry.yaml`.

3. Strategic exit / ROI language softened.
   - Previous risk: "become a strategic acquisition target" and unsupported comparable acquisition range.
   - Current treatment: "licensing, partnership, or acquisition optionality if proof supports it" and "comparable transaction work remains a CFO/advisor diligence item."
   - Files touched: deck, executive summary, business plan, talking points.

4. Technology data-room overclaims corrected.
   - Previous risk: "NaxRiscv RV64GC (production)," hard-coded CSR base for newer builds, Ubuntu wording, utilization claims, and "workload harness ready."
   - Current treatment: frozen baseline separated from bring-up variants; utilization and CSR details require per-build artifacts; workload validation is the next evidence gate.
   - Files touched: `03_data_room/product_tech/ATOMiK_Product_Technical_Overview.docx`, `generate_all_docs.py`.

5. Market-context figures harmonized.
   - Previous risk: old SIA/WSTS forecast figures mixed with current 2026 SIA reports.
   - Current treatment: semiconductor market context now uses SIA-reported $791.7B 2025 sales and roughly $1T 2026 trajectory / Q1 context, while keeping market data separate from ATOMiK savings claims.
   - Files touched: financial model, business plan, `04_source_markdown/financial_model_source.md`.

6. Generator behavior improved.
   - Previous risk: running `generate_all_docs.py` overwrote the local deck, talking points, and source markdown from parent folders.
   - Current treatment: generated DOCX/XLSX rebuilds preserve local deck, talking points, and source markdown if present.
   - File touched: `generate_all_docs.py`.

## Remaining P0 Human Signoffs Before Friday

1. CFO/counsel must approve financing language.
   - Current ask: $2.0M target pre-seed, $1.25M minimum, $2.75M stretch, post-money SAFE.
   - Do not share final SAFE cap, discount, pro-rata, or close mechanics until approved.
   - Files: deck slide 10/12, financial model, executive summary, legal summary.

2. Legal formation items remain pending.
   - Formation docs, EIN, founder stock issuance, formal cap table, SAFE template review, employment/contractor agreements, and board approvals remain pending or counsel-gated.
   - File: `03_data_room/legal/ATOMiK_Legal_Formation_Summary.docx`.

3. Customer traction must stay conservative.
   - Current data-room language says no signed customer contracts or LOIs as of May 2026.
   - Do not verbally imply LOIs, design partners, revenue, or pipeline commitments unless status has changed and can be documented.
   - File: `03_data_room/customers/ATOMiK_Customer_Pipeline.docx`.

4. SD boot remains evidence-gated.
   - Current label should remain `BUILD_ARTIFACT` unless there is a recorded power-on boot artifact.
   - Do not say standalone SD boot is validated until the run artifact exists and claims registry is updated.
   - Evidence basis: `results/claims_registry.yaml`.

5. No Zynq workload-performance claims yet.
   - Until the SD-boot/workload sprint produces raw artifacts and correctness logs, Friday claims should stay limited to the Linux userspace proof, AX7020 matrix, and current UI/hardware artifacts.
   - Any new workload result must update the proof card and claims registry first.

## P1 Improvements For Next Iteration

1. Visual pass in PowerPoint.
   - Text is safer now, but the deck still needs a human visual check for slide density, font size, and image crop.
   - Priority slides: 1, 2, 8, 9, 12.

2. One-pager page-fit check.
   - The DOCX content is short enough conceptually, but Word pagination should be checked manually or exported to PDF to confirm it truly fits one page.

3. Add artifact links to proof/data-room docs.
   - Proof references are named, but a Friday data room should include direct paths or links to the exact artifacts where practical.

4. Add a dated open-items cover page.
   - Suggested front page: "What is validated / what is pending / what Friday capital buys."

5. ROI narrative still needs CFO/advisor support.
   - Current package has a defensible return logic, but no sourced semiconductor IP transaction comparables. Keep it that way until comps are sourced.

## Copy Rules To Preserve

Use:
- workload-specific
- measured against baseline
- correctness-preserving
- evidence-bound
- evaluation target
- hardware-validated for specific artifacts
- live measured only with raw artifacts

Avoid:
- universal speedup
- guaranteed battery/heat/water/cooling savings
- production-ready
- commercial product maturity
- replaces CPU/GPU/NPU
- acquisition target as a promise
- formal proof counts until audited
