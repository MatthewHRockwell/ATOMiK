# Claim Hygiene Audit

Date: 2026-05-10

Updated: 2026-05-14

Follow-up: 2026-05-15

Second follow-up: 2026-05-15

## Scope Reviewed

Risky claim search was run across:

```text
README.md website/src docs business results
```

with generated website dependency/build folders excluded.

Search terms:

```text
proven guaranteed production-ready shipped customers benchmarked measured faster
saves reduces autonomous AI-powered world's first revolutionary production
enterprise-ready
```

## Tightened In This Update

- Homepage: removed free-tier SaaS packaging, production-style customer claims,
  and unlabeled metric claims.
- Pricing/evaluation pages: replaced four-tier public pricing with two
  request-based evaluation paths.
- Contact form: changed generic sales form into evaluation/demo/design-partner
  lead capture.
- Case studies page: removed unapproved named customer stories and replaced
  them with workload hypotheses.
- Benchmarks page: replaced headline metrics with an evidence-labeled artifact
  map.
- Docs index and hardware docs: routed visitors to evidence labels, technical
  proof, hardware validation, roadmap, and claims registry.
- Compliance and whitepaper pages: removed enterprise-ready / certification /
  production-result framing and replaced it with evidence posture.
- README, one-pager, deck source, and outreach materials: aligned with evidence
  labels and current live proof.

## Tightened In 2026-05-14 Update

- Homepage and README: promoted the v0.38-G live UI screenshot as the current
  public proof image while preserving the prototype/not-shipped label.
- Visual manifest, ATOMiK Desk concept doc, technical proof doc, claims
  registry, and pitch visual map: added the v0.38-G UI proof artifact and kept
  v0.38-H pending until a recorded screenshot or demo artifact exists.
- FAQ, register, terms, dashboard, ROI, kernel-module docs, changelog, and
  selected blog/solution CTAs: removed stale self-serve free-trial / Pro-plan
  language and aligned public conversion to request-based evaluation access.
- AI demo route: replaced projected AI power/cost comparison content with a
  concept/evaluation page that requires measured artifacts before AI-specific
  performance or power claims.
- Whitepaper and OpenGraph metadata: removed unlabeled headline metrics and
  reframed them around evidence-labeled technical proof.

The 2026-05-14 high-risk public-site scan found no remaining matches for:

```text
90-day free trial
90-day free Pro trial
free Pro
Pro trial
Start Pro Trial
Start Your 90-Day Free Trial
free 90-day trial
Free 90-day trial
916,000x
7,670x
220W
12W
18x
less power
same AI output
production results
world's first
revolutionary
```

## Tightened In 2026-05-15 Follow-Up

- Blog and docs routes: added a reusable evidence banner warning that older
  articles and technical notes may contain exploratory examples, synthesis
  outputs, or modeled comparisons.
- Blog index and claim-dense articles: softened CRDT, event-sourcing,
  distributed-cache, kernel-module, and FPGA-story copy so examples are framed
  as educational/model-based unless backed by artifacts.
- Migration and examples docs: replaced production-ready, no-conflict,
  no-consensus, and guarantee-style language with scoped evaluation language.
- Static `docs/site/` dashboards: added visible legacy-warning banners and
  classified them as historical demo/dashboard material, not current public
  landing pages for `ATOMiK.tech`.
- Legacy business collateral: added inline publication-status banners to
  funding, diligence, data-room, score-package, launch-post, competitive memo,
  and generator/template files that still contain historical metrics or strong
  market claims.
- v0.38-H claims remain intentionally unpromoted until a screenshot or recorded
  artifact is saved and classified.

## Tightened In 2026-05-15 Second Follow-Up

- Replaced `shipped product` / `shipped functionality` disclaimer wording in
  current public copy with `commercial product` / `commercial functionality` to
  reduce noisy claim-scan matches without weakening the prototype disclaimer.
- Marked the legacy SDK user manual as review-required and softened its intro
  bullets so memory, latency, and validation claims point back to evidence
  labels.
- Updated funding/outreach generators and shared Q&A defaults so generated
  accelerator, grant, VC, and video copy defaults to artifact-linked language
  instead of old hard metrics.
- Added review-required warnings to NVIDIA, Alliance SoCal, YC, and HAX
  application drafts.
- Added `docs/public-claim-scan.md` as the repeatable scan checklist for current
  public surfaces versus quarantined legacy material.

## Remaining Legacy Risk

Older claims still exist in historical or specialized materials, including:

- `docs/landing/index.html`
- `docs/PRODUCTION_DEPLOYMENT.md`
- older technical/reference files under `docs/reference/`
- generated/static dashboards under `docs/site/`
- `business/cosmos-cookoff/**`
- `business/data_room/**`
- `business/funding_strategy/**`
- `business/outreach/**` application drafts and generated VC emails
- `business/score_package/**`
- `business/pitch_deck/investor_deck_full.md`
- `business/pitch_deck/generate_deck.py`
- `business/pitch_deck/update_deck*.py`
- older investor, diligence, comparison, and launch-post drafts under
  `business/`

Those areas are now explicitly marked as review-required by:

- `docs/PUBLICATION_NOTES.md`
- `business/PUBLICATION_NOTES.md`
- `business/data_room/README.md`
- `business/score_package/00_START_HERE.md`
- visible banners on `docs/site/*.html`
- inline publication-status banners on high-risk legacy business files
- `docs/public-claim-scan.md` for repeatable current/legacy claim scans

Do not send or publish review-required material without either backing the
claims with artifacts, adding evidence labels, or rewriting it against the
current public positioning.

Residual risk: some legacy files still contain old numbers by design so past
work can be audited. Their current mitigation is quarantine, review-required
labeling, and generator default softening, not full historical rewrite.

## Current Rule

Live screenshots show current prototypes. Concept visuals show product direction
and are not represented as current commercial functionality. Performance claims
are only stated when backed by measured artifacts.
