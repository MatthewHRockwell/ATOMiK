# ATOMiK — Investor Data Room

This data room contains due diligence materials for ATOMiK, a delta-state computing architecture achieving 1 billion operations/second on a $10 FPGA.

## Contents

| Section | Contents |
|---------|----------|
| [01_financial/](01_financial/) | Financial model, development cost breakdown |
| [02_legal/](02_legal/) | Entity status, IP assignment, license summary |
| [03_intellectual_property/](03_intellectual_property/) | Patent status, formal proofs inventory, trade secrets |
| [04_team/](04_team/) | Founder profile |
| [05_customers/](05_customers/) | Customer pipeline and target verticals |

## Regeneration

These documents are auto-generated from project data. To regenerate:

```bash
python business/data_room/_generate.py
```

## Sharing

For secure sharing with investors:
1. Use a private GitHub repository with collaborator access
2. Or export as PDF and share via encrypted link
3. Never share credentials or config files — use the `.template` versions

## Last Updated

This data room is regenerated after each pipeline action via the funding automation system.
