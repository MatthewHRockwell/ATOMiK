# ATOMiK Investor Package — Codex Review Folder

## Purpose
This folder contains all investor-facing documents for the Friday Aggie Angel pitch.
Codex: review each document and make improvements listed below.

## What to Review

### 01_pitch_materials/
- **ATOMiK_Investor_Deck.pptx** — 12-slide deck. Review slides for clarity, flow, VC-friendly language.
  - Ensure every slide gets to the point in <5 seconds of reading
  - Remove any tech jargon a non-engineer VC wouldn't immediately understand
  - Strengthen the ROI/return narrative on slide 9 (Return Path) and slide 12 (The Ask)
  - Verify all numbers match the financial model
- **ATOMiK_Executive_Summary.docx** — 1-2 page overview. Tighten to 1 page if possible.
- **ATOMiK_Business_Plan.docx** — Full business plan. Review for accuracy and VC-friendly tone.
- **ATOMiK_One_Pager.docx** — Single page overview. Must fit on one page. Trim aggressively.
- **ATOMiK_Talking_Points.md** — Pitch script. Review for natural speech, not corporate speak.

### 02_financial/
- **ATOMiK_Financial_Model.xlsx** — Review all 4 sheets:
  - Use of Funds: verify numbers sum correctly
  - Revenue Scenarios: flag any cells that need real data before investor meeting
  - Market Context: verify all source citations are accurate
  - Cap Table: clearly mark as DRAFT and ensure it doesn't create false impressions
- Flag any numbers that need CFO approval before sharing

### 03_data_room/
- **legal/ATOMiK_Legal_Formation_Summary.docx** — Ensure all PENDING items are clearly flagged
- **product_tech/ATOMiK_Product_Technical_Overview.docx** — Make accessible to non-engineers
- **customers/ATOMiK_Customer_Pipeline.docx** — Honest status of pipeline
- **team/ATOMiK_Team_Overview.docx** — Professional, factual

## Codex Instructions

1. **DO NOT** change any financial numbers without flagging them for human review
2. **DO NOT** remove evidence labels (HARDWARE_VALIDATED, LIVE_MEASURED, etc.)
3. **DO NOT** add performance claims that aren't in the claims_registry.yaml
4. **DO** improve prose clarity, grammar, flow, and VC-appropriate language
5. **DO** ensure consistent formatting within each document
6. **DO** strengthen the investor ROI narrative where appropriate
7. **DO** flag any inconsistencies between documents
8. **DO** note any PENDING items that need human action before Friday

## Key Claims Rules (DO NOT VIOLATE)
- Only claim battery/heat/water/cooling savings if there's a measured artifact
- Always pair performance numbers with evidence labels
- Never say "production ready" or "commercial product"
- Always say what IS measured vs what is an evaluation target

## Evidence Labels Currently Active
- HARDWARE_VALIDATED: v0.39-K UI, algebraic tests, Linux userspace path
- LIVE_MEASURED: AX7020 board run matrix
- SOFTWARE_VALIDATED: Formal algebraic proofs
- BUILD_ARTIFACT: SD boot artifacts

## Source Files
- 04_source_markdown/ contains the canonical markdown sources
- Website source is in /website/src/
- Claims registry is in /results/claims_registry.yaml

## Contact
matthew.h.rockwell@gmail.com
