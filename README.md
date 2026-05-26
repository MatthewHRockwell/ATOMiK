# ATOMiK

ATOMiK is a state-aware compute architecture for systems that spend too much
work rediscovering what changed.

The current public materials document the live hardware prototype direction,
technical proof artifacts, developer adoption path, and roadmap toward ATOMiK
Desk and Resource Fabric.

![ATOMiK Desk v0.39-K prototype UI running on live Zynq hardware](website/public/09-current-live-atomik-desk-v039k.png)

**HARDWARE_VALIDATED:** ATOMiK Desk v0.39-K prototype UI running on live Zynq
hardware. This screenshot is proof of current prototype progress, not a
polished commercial product, benchmark, uptime claim, or power/thermal result.

## Current Status

| Area | Status |
|---|---|
| Live hardware prototype | Yes. Current and historical proof screenshots are available and labeled above. |
| Public benchmark artifacts | Present where artifact packages exist. See `results/` and `docs/perf/`. |
| Concept UI | Design target and roadmap material, not commercial product UI. |
| Evaluation access | Request-based. No conventional free tier is presented publicly. |

## What Is Live

- Hardware-backed ATOMiK prototype work is present in the repository and proof
  docs.
- ATOMiK Desk has live prototype screenshots from running hardware, including
  the v0.39-K UI/UX proof image now closest to the current concept direction.
- Linux userspace validation, hardware synthesis documentation, and board-run
  performance artifacts are tracked with evidence labels.
- Public software, formal proof, and SDK material remain available for technical
  inspection.

## What Is In This Repo

- `website/` - Next.js public website.
- `docs/` - public documentation, proof notes, evidence labels, and concept docs.
- `results/` - recorded artifacts and claims registry.
- `software/`, `sdk/`, `math/` - public software, SDK, and proof work.
- `hardware/` and `atomik_os/` - active hardware, embedded, and live prototype
  implementation areas.
- `business/` - public-safe business, pitch, outreach, and evaluation drafts.

## What Is Not In This Repo

- Final commercial product commitments.
- Customer traction claims unless explicitly documented and approved.
- Private CRM data, private customer workloads, or confidential partner material.
- Unlabeled performance, power, uptime, savings, or production-readiness claims.

## Quick Links

- [Evidence labels](docs/evidence-labels.md)
- [Docs publication notes](docs/PUBLICATION_NOTES.md)
- [Business material publication notes](business/PUBLICATION_NOTES.md)
- [Claims registry](results/claims_registry.yaml)
- [Technical proof](docs/technical-proof.md)
- [Hardware validation](docs/hardware-validation.md)
- [Roadmap](docs/roadmap.md)
- [Design partner guide](docs/design-partners.md)
- [ATOMiK Desk concept](docs/concepts/atomik-desk.md)
- [Visual asset manifest](docs/visual-asset-manifest.md)
- [Resource Fabric concept](docs/concepts/resource-fabric.md)
- [Compiler adoption lane](docs/concepts/compiler-adoption-lane.md)
- [Replica Flow](docs/concepts/replica-flow.md)

## Start Here

If you are evaluating ATOMiK, start with one real workload or state-heavy path:

1. Identify where your system repeatedly moves, replays, rescans, or reconstructs
   state.
2. Review the proof labels and claims registry.
3. Decide whether you want a proof review, technical demo, or scoped evaluation
   conversation.

Public CTAs:

- [Request Evaluation Access](https://atomik.tech/contact?intent=evaluation)
- [Review Proof](https://atomik.tech/proof)
- [Discuss Design Partnership](https://atomik.tech/contact?intent=design-partner)

## Evidence Hygiene

Live screenshots show current prototypes. Concept visuals show product direction
and are not represented as current commercial functionality. Performance claims
are only stated when backed by measured artifacts.

Every public claim should use one of these labels:

- `LIVE_MEASURED`
- `HARDWARE_VALIDATED`
- `SOFTWARE_VALIDATED`
- `SYNTHESIS_VALIDATED`
- `BUILD_ARTIFACT`
- `PROJECTED`
- `CONCEPTUAL`
- `ROADMAP`

See [docs/evidence-labels.md](docs/evidence-labels.md) for definitions.

## License and IP Notice

Source code license and commercial-use terms should be reviewed in the relevant
package, directory, or agreement before use. Public materials avoid making final
commercial licensing claims until they are reconciled with counsel-reviewed
language.
