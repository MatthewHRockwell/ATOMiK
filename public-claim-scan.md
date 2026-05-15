# Public Claim Scan

Use this before publishing website, README, business, pitch, or outreach
changes. The goal is not to remove every technical number from the repository;
it is to keep current public copy separate from archived, review-required
material.

## Current Public Surfaces

Run this scan on surfaces that are intended to be current public copy:

```bash
rg -n "916,000x|7,670x|world.?s first|revolutionary|free Pro|90-day|Start Pro Trial|same AI output|220W|12W|automatic convergence|zero metadata|kernel speed|production-ready|enterprise-ready|guaranteed|No consensus|no leader|no conflict|shipped product|shipped functionality" README.md website/src docs/evidence-labels.md docs/technical-proof.md docs/hardware-validation.md docs/roadmap.md docs/design-partners.md docs/concepts docs/perf results business/one_pager business/design_partners business/faq/public_gtm_faq.md business/pitch_deck/deck-outline.md business/pitch_deck/slide-copy.md business/pitch_deck/visual-asset-map.md
```

Every match in current public copy needs one of these actions:

- remove the claim
- soften it to evaluation, concept, or roadmap language
- link it to an artifact and evidence label
- move it to a review-required legacy file

## Legacy / Review-Required Surfaces

Legacy areas may intentionally retain old numbers for auditability, but they
must carry publication-status or review-required warnings:

```bash
rg -n "Publication status|PUBLICATION STATUS|review-required|REVIEW REQUIRED|DO NOT PUBLISH|NOT CURRENT PUBLIC" business docs/site docs/landing docs/PRODUCTION_DEPLOYMENT.md docs/reference
```

If a legacy file contains performance, power, customer, production, pricing, or
market-size language and has no warning banner, add one before sharing or
generating derivative materials.

## Global Rule

Live screenshots show current prototypes. Concept visuals show product
direction and are not represented as current commercial functionality.
Performance claims are only stated when backed by measured artifacts.
