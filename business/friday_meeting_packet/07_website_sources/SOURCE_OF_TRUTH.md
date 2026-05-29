# Website Sources - Source Of Truth

This folder is a source library for website, pitch, and packet copy. It is not the final external send packet.

Final external and controlled materials are governed by the packet folders:

- `01_front_of_room/` - live decks, one-pager, executive summary, and presenter-facing front-room aids.
- `03_financial/` - financial model, static summary, source register, and diligence memo.
- `04_data_room/` - controlled business, legal/IP, customer, product/technical, and team diligence.
- `05_proof_artifacts/` - proof artifacts, proof cards, version map, claims registry snapshot, captions, and checksums.
- `06_internal_presenter/` - live script, Q&A support, AAN feedback sheet, and room checklist.

Current source routing:

| Area | Current source of truth | Notes |
|---|---|---|
| Website messaging | `MESSAGING_FRAMEWORK.md` | Buyer-first narrative: wasted state movement before broad compute thesis. |
| Brand/design | `BRAND_SYSTEM.md` | Use ATOMiK consistently; keep concept visuals labeled when used. |
| Evaluation offer | `EVALUATION_ONE_PAGER.md` | Controls the customer-facing `one workload, one baseline, one constraint` offer. |
| Proof source | `PROOF_PACKET.md` plus `05_proof_artifacts/README.md` | Packaged proof details live in `05_proof_artifacts/`, not in older repo paths alone. |
| Proof artifacts | `05_proof_artifacts/README.md`, `05_proof_artifacts/PROOF_CARDS.md`, `05_proof_artifacts/VERSION_MAP.md`, `05_proof_artifacts/claims_registry_snapshot.yaml` | Use these before quoting or forwarding proof claims. |
| Pitch source | Current front-of-room deck plus `06_internal_presenter/ATOMiK_Friday_Talking_Points.md` | Do not let older pitch-assets run-of-show files override the live script. |
| Data-room policy | `04_data_room/README_DATA_ROOM.md` and `02_business_docs/DATA_ROOM_INDEX.md` | Older pitch-assets data-room drafts are planning/source copies only. |
| Financials | `03_financial/ATOMiK_Financial_Due_Diligence_Memo.pdf`, `03_financial/ATOMiK_Financial_Model.xlsx`, `03_financial/ATOMiK_Financial_Static_Summary.pdf` | Founder-prepared materials; final terms and legal mechanics remain CFO/counsel pending. |
| Packet integrity | `MANIFEST.md` and `CHECKSUMS.md` at the packet root | Regenerate after packet edits. |

Pitch-assets guidance:

- `pitch-assets/*.md` files are source copies carried for continuity with website/pitch asset generation.
- Do not edit a pitch-assets copy as the sole current source unless regenerating packet assets intentionally.
- If a pitch-assets copy conflicts with this file, the final packet folders and current source routing above win.
