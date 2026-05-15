# Public Site Truth Update - 2026-05-15

## Scope

This note documents the ATOMiK.tech update that aligns the public website with
the current first-silicon / design-partner narrative.

## Unsupported Claims Removed

- Production-style customer and case-study framing.
- Conventional free-tier / Community-tier public offer language.
- Public dashboard mock metrics, fake usage counts, uptime, saved-bandwidth
  claims, and mock API-key surfaces.
- Exact benchmark and hardware numbers from public summary pages when the page
  did not link directly to the source artifact and caveats.
- Productized Kubernetes, database, observability, and CI/CD integration claims
  that were not scoped as evaluation examples.
- Legacy proof-count, coordination-removal, automatic-convergence, and
  free-registration language on secondary docs/blog/community surfaces.

## Claims Relabeled

- Target workloads are now illustrative target applications, not customer
  deployments.
- ROI output is now a projected scenario model, not a savings claim.
- Zynq material is split into synthesis-validated ceiling characterization,
  hardware-validation artifacts, live prototype progress, and first-silicon
  roadmap work.
- ATOMiK Desk v0.38-I is labeled as live prototype proof, not commercial
  product functionality.
- Migration docs and technical articles now describe model-fit criteria and
  proof artifacts instead of implying production traffic guarantees.

## Live / Measured Claims That Remain

- `LIVE_MEASURED`: AX7020 board-run artifacts only where the raw output and
  interpretation notes are linked.
- `HARDWARE_VALIDATED`: ATOMiK Desk v0.38-I screenshot and hardware validation
  paths where physical hardware artifacts exist.
- `SYNTHESIS_VALIDATED`: Zynq ceiling characterization and toolchain outputs
  when kept separate from live-board measurements.
- `SOFTWARE_VALIDATED`: public SDK and formal proof work.

## Projected / Roadmap Claims That Remain

- First-silicon evaluation chip path.
- Resource Fabric productization.
- Enterprise / IP / SDK licensing discussions.
- Workload-specific ROI models until replaced by measured workload artifacts.

## Next-Pass TODOs

- GitHub README: keep v0.38-I as current proof, and cross-link this note.
- Investor deck: replace legacy metrics with evidence-labeled proof slides.
- Customer one-pager: keep target workloads scenario-framed until references
  exist.
- Design partner outreach: use the two-offer model and avoid free-tier language.
- Benchmark packaging: consolidate live AX7020/Zynq demo logs into reproducible
  artifact bundles before publishing new numeric claims.
