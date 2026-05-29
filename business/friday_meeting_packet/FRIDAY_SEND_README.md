# ATOMiK Friday Send Package

Last updated: 2026-05-28

Version manifest: `FRIDAY_SEND_CHECKSUMS.md` records SHA-256 hashes for the clean external send package. `CHECKSUMS.md` covers the broader internal meeting packet. Regenerate the relevant manifest after any package change.

This folder is the clean Friday send package. Use this instead of `business/for_codex`, which is an internal working folder containing prompts, audit notes, source markdown, and generators.

## Front Of Room

Presenter notes live outside this send folder in `business/friday_internal/`. Do not include those files in an investor/customer send unless explicitly intended.


- `01_front_of_room/ATOMiK_Investor_Deck.pptx`
- `01_front_of_room/ATOMiK_Investor_Deck.pdf`
- `01_front_of_room/ATOMiK_5_Minute_Deck.pptx`
- `01_front_of_room/ATOMiK_5_Minute_Deck.pdf`
- `01_front_of_room/ATOMiK_One_Pager.pdf`
- `01_front_of_room/ATOMiK_Executive_Summary.pdf`

## Financial Diligence

- `02_financial/ATOMiK_Financial_Model.xlsx`
- `02_financial/ATOMiK_Financial_Model_CFO_View.xlsx` (same model, conventional finance-reader styling)
- `02_financial/ATOMiK_Financial_Due_Diligence_Memo.pdf`
- `02_financial/ATOMiK_Financial_Source_Register.pdf`
- `02_financial/ATOMiK_Financial_Static_Summary.pdf`
- `02_financial/static_exports/*.csv` including use-of-funds, runway, dilution sensitivity, and revenue planning scenario

Use financials as founder-prepared milestone and planning-scenario materials pending CFO/counsel. Do not state a valuation cap, booked revenue, committed pipeline, guaranteed ROI, or tape-out budget. Revenue planning ranges must be presented as planning ranges, not forecasts.

## Controlled Data Room

- `03_data_room/README_DATA_ROOM.md` explains what is controlled vs NDA/counsel-controlled.
- `03_data_room/business/ATOMiK_Business_Plan.pdf` (controlled diligence; do not include in a lightweight front-of-room send)
- Legal/entity summary, legal-status note, and legal/IP attachment index
- Product/technical overview and investor-safe patent summary; the full provisional patent PDF is counsel/NDA-controlled and should not be included in a lightweight investor send
- Customer pipeline tracker/target map, not traction
- Design-partner templates are included for controlled diligence, not as signed customer evidence
- Close-prerequisites schedule for counsel/CFO follow-up
- Team overview
- Proof artifacts and claims registry snapshot

## Proof Boundary

Proof artifacts are workload- and artifact-specific. Do not claim universal speedup, battery extension, heat reduction, cooling/water savings, production readiness, or customer savings unless a matching measured artifact exists.

## Old Package Warning

Legacy package was moved to `business/archive/DO_NOT_SEND_ATOMiK_Business_Package_legacy.zip`. Do not send it; `business/friday_send/` is the Friday package.
