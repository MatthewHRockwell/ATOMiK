# ATOMiK Pitch Materials Review Notes

Last updated: 2026-05-27

## Pass 1 Summary

Updated the Friday investor package to sharpen the proof-safe buyer narrative and reduce diligence friction.

### Material Changes

- Reframed broad "every constrained system" language into narrower "many constrained systems" and "where workload evidence supports it" language.
- Replaced "16/16 algebraic proofs pass" with "16/16 algebraic property checks pass" to match the Linux userspace proof artifact.
- Softened acquisition/return copy from implied outcome language to licensing, partnership, or acquisition optionality after workload proof and IP diligence.
- Updated semiconductor market context from the older WSTS forecast to SIA's current 2025 actual and Q1 2026 context.
- Corrected the technical overview state table size: 256x64 bits is 16,384 bits, or 2KB raw state storage, not 16KB.
- Removed unsafe exact current-build assumptions from the technical overview where the frozen baseline and SD-boot bring-up context differ.
- Clarified SD boot status as BUILD_ARTIFACT until a recorded power-on boot artifact exists.
- Regenerated Word, Excel, and PowerPoint materials in this review folder.

### Files Updated In This Review Folder

- `01_pitch_materials/ATOMiK_Investor_Deck.pptx`
- `01_pitch_materials/ATOMiK_Executive_Summary.docx`
- `01_pitch_materials/ATOMiK_Business_Plan.docx`
- `01_pitch_materials/ATOMiK_One_Pager.docx`
- `01_pitch_materials/ATOMiK_Talking_Points.md`
- `02_financial/ATOMiK_Financial_Model.xlsx`
- `03_data_room/legal/ATOMiK_Legal_Formation_Summary.docx`
- `03_data_room/product_tech/ATOMiK_Product_Technical_Overview.docx`
- `03_data_room/customers/ATOMiK_Customer_Pipeline.docx`
- `03_data_room/team/ATOMiK_Team_Overview.docx`
- `04_source_markdown/financial_model_source.md`
- `04_source_markdown/investor_faq_source.md`
- `04_source_markdown/one_pager_source.md`
- `04_source_markdown/revenue_model_source.md`
- `04_source_markdown/slides_source.md`
- `generate_all_docs.py`

## Human Review Gates Before Friday

- CFO/counsel must approve SAFE structure, valuation cap, discount, pro-rata rights, closing mechanics, and any cap table distribution.
- Counsel must verify entity status, founder IP assignment, patent conversion timing, and public patent-pending language.
- Matt/Allison must decide which proof artifacts are public, controlled, and NDA-only.
- SD boot must remain BUILD_ARTIFACT unless a recorded power-on boot artifact exists.
- Workload performance numbers from new Zynq runs must not enter the deck/homepage until correctness passes and `results/claims_registry.yaml` is updated.

## Claims Still Not Supported

Do not claim the following without new artifact-backed evidence:

- measured battery extension
- measured heat reduction
- cooling reduction
- water savings
- smaller hardware footprint
- production readiness
- universal speedup
- CPU/GPU/NPU replacement
- guaranteed customer savings
- signed customer traction, LOIs, or revenue

## Current Best Friday Story

ATOMiK is not asking investors or customers to accept a broad compute claim. The company is asking the right customers to bring one constrained state path, measure the waste, and decide with evidence whether state-aware compute belongs in their architecture.

The $2.0M pre-seed ask funds the move from hardware-backed primitive proof to measured customer workload proof, IP diligence, ASIC feasibility, and licensing-ready materials.

## Pass 2 VC / Design Audit - 2026-05-27

### Changes Made

- Reduced deck density on the most overloaded slides: 1, 2, 9, and 12.
- Rebuilt the talk track around strict 5-minute, 10-minute, and 30-minute meeting modes.
- Rewrote the one-pager so it actually exports to one page.
- Rewrote the executive summary so it exports to two pages instead of three.
- Added `DILIGENCE_APPENDIX.md` with validated/pending/proof-gate/status tables for VC follow-up.
- Added `VC_PITCH_AUDIT.md` with a VC-perspective content, design, timing, and diligence punch list.
- Preserved proof labels and kept SD boot as `BUILD_ARTIFACT`.

### Verification

- `ATOMiK_One_Pager.docx` exported to PDF: 1 page.
- `ATOMiK_Executive_Summary.docx` exported to PDF: 2 pages.
- `ATOMiK_Investor_Deck.pptx` exported to PDF: 12 slides.
- Deck word-count extraction after reduction:
  - Slide 1: 78 words
  - Slide 2: 92 words
  - Slide 9: 74 words
  - Slide 12: 94 words
- Financial totals were re-verified in Pass 3 after the model rebuild; continue re-checking after any future model edits.

### Still Not Perfect

- The deck is now cleaner but still not a truly cinematic 5-second deck. A final visual-design pass should consider removing header metadata from each slide, using larger type, and moving more proof detail into appendix.
- Superseded by Pass 3: the workbook now has formulas, runway, monthly cash plan, evaluation-pricing treatment, and dilution sensitivity. It remains founder-prepared and still needs CFO/counsel review, vendor quotes, tax/accounting assumptions, and formal cap-table records.
- Direct artifact links should be packaged into a controlled data-room folder before serious investor follow-up.

## Pass 3 Financial Diligence Audit - 2026-05-27

### Changes Made

- Rebuilt `ATOMiK_Financial_Model.xlsx` from a static 4-sheet workbook into a 9-sheet formula-backed diligence workbook.
- Added formula-backed use-of-funds totals, runway/burn sensitivity, 18-month target cash plan, evaluation-pricing signals, milestone gates, dilution sensitivity, market context, cap-table caveats, and financial caveats.
- Aligned minimum / target / stretch use-of-funds scenarios with the data-room financing plan.
- Added `FINANCIAL_DUE_DILIGENCE.md` in the Codex review folder and `financial_due_diligence_memo.md` in the data room.
- Added CFO-absence talking points to the Friday talk track: what to say on runway, valuation cap, dilution sensitivity, reservation pricing, and revenue.
- Expanded the paid technical evaluation proposal template with duration bands, fee treatment, payment timing, IP/confidentiality boundaries, and success criteria.

### Financial Verification

- Workbook sheets: 9.
- Workbook formulas: 119.
- Use-of-funds totals: $1.25M minimum, $2.0M target, $2.75M stretch.
- 18-month target cash plan: $1.85M planned spend plus $150K reserve.
- Target gross monthly budget: about $111K/month.
- Target ex-reserve monthly budget: about $103K/month.

### CFO-Substitute Guardrails

- Do not give a valuation cap in the room.
- Do not call reservations revenue traction.
- Do not forecast revenue without signed SOWs or licensing agreements.
- Do not treat market benchmarks as ATOMiK valuation or TAM proof.
- Do not imply this round funds tape-out.

### Still Needs Human / Advisor Review

- Final SAFE terms, valuation cap, discount, pro-rata, side letters, and closing mechanics.
- Formal cap table after incorporation, founder stock issuance, IP assignment, and prior-obligation review.
- Vendor-backed legal/IP and ASIC feasibility quotes.
- Tax/accounting treatment, benefits, payroll assumptions, and insurance costs.

## Pass 4 Packaging / Diligence Hygiene Audit - 2026-05-27

### Changes Made

- Created `business/friday_send/` as the clean investor/customer send folder. `business/for_codex/` remains internal working material.
- Added final front-of-room PDFs/PPTX/DOCX, financial docs, controlled data-room docs, and proof artifacts into the send folder.
- Added `00_READ_ME_FIRST.md` and `04_proof_artifacts/README.md` to explain proof boundaries and what not to send.
- Packaged proof artifacts: current Zynq screenshot, Linux userspace proof, Zynq baseline, AX7020 matrix, AX7020 interpretation, claims registry snapshot, and evidence labels.
- Added `legal_status_current.md` and regenerated the legal summary to clarify current LLC status, intended Delaware C-Corp path, IP assignment gate, and before-close checklist.
- Normalized stretch-runway wording to `~18 months at accelerated spend; longer only if spend is not accelerated`.
- Fixed the financial memo Markdown table parser and regenerated the financial due-diligence memo.
- Updated customer pipeline language to lead with edge/embedded and treat data center as strategic/later path.
- Created a separate `ATOMiK_5_Minute_Deck.pptx` / PDF for rushed rooms.
- Tightened slide 1 proof shorthand to `Zynq UI proof`, `Linux-to-FPGA path`, and `16/16 property checks`.
- Synced parent investor FAQ with safer acquisition-optionality language.

### Verification

- Friday send deck PDF exports: 12 pages for main deck, 6 pages for 5-minute deck.
- One-pager PDF exports to 1 page.
- Executive summary PDF exports to 2 pages.
- Financial due-diligence memo PDF exports to 4 pages.
- Financial workbook still has 9 sheets and 119 formulas.
- Use-of-funds totals remain $1.25M / $2.0M / $2.75M.
- Target cash plan remains $1.85M planned spend plus $150K reserve.

### Remaining Risks

- Old `business/ATOMiK_Business_Package.zip` still exists; it is explicitly marked in `business/friday_send/00_READ_ME_FIRST.md` as not the Friday send package.
- Final SAFE terms, cap, discount, pro-rata, entity conversion, cap table, and IP assignment chain still require counsel/CFO review.
- Proof artifacts are packaged, but new Zynq workload claims still require correctness-passing artifacts and claims-registry updates.
