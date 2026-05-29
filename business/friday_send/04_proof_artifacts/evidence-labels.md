# ATOMiK Evidence Labels

This framework is the public source of truth for how ATOMiK labels claims,
metrics, screenshots, concept visuals, and roadmap material.

## Global Disclaimer

Live screenshots show current prototypes. Concept visuals show product direction
and are not represented as current commercial functionality. Performance claims
are only stated when backed by measured artifacts.

## Label Definitions

| Label | Definition | Public wording rule |
|---|---|---|
| `LIVE_MEASURED` | Observed on a running system with recorded measurement artifacts. | May be phrased as an observed result only when the artifact is linked. |
| `HARDWARE_VALIDATED` | Demonstrated on physical hardware, but not necessarily accompanied by full benchmark artifacts. | May say hardware-backed or hardware-validated. Do not imply full production readiness. |
| `SOFTWARE_VALIDATED` | Shown in a software prototype, simulation, local runtime, or non-hardware environment. | May say software-validated. Do not imply board execution. |
| `FORMAL_PROOF` | A directly audited formal statement or proof artifact. | May say formal proof only for the exact property proven. Do not imply workload, customer, or production outcomes. |
| `SYNTHESIS_VALIDATED` | Validated through synthesis, build, compile, or toolchain output. | Must be separated from live-board results. |
| `BUILD_ARTIFACT` | A concrete local build output exists, but the end-to-end hardware path has not yet been promoted as a live result. | May say built or assembled locally. Must not say booted, validated, or working end to end until a run artifact exists. |
| `PROJECTED` | A model, estimate, or expected economic/performance outcome. | Must not be phrased as a result. |
| `CONCEPTUAL` | Used to explain product direction, UX, architecture, or design intent. Not current proof. | Must be labeled as concept visual or design target. |
| `ROADMAP` | Planned work that may change. | Must be phrased as planned or future work. |

## Claim Rules

- Every public performance number needs a label and an artifact link.
- Use FORMAL_PROOF only for directly audited formal statements; otherwise use SOFTWARE_VALIDATED / proof work present.
- Concept visuals must never appear as proof of current product maturity.
- Synthesis ceilings and modeled economics must not be blended with live
  measurements.
- Public customer, production, uptime, power, thermal, savings, water, battery,
  and traction claims require artifact-backed approval before publication.
- If a claim cannot be traced to an artifact, soften it, label it as roadmap or
  conceptual, or remove it.

## Public Asset Labels

Packaged proof captions and overclaim risks are summarized in [README.md](README.md). The full repo may contain additional visual-asset manifests, but this send folder should be treated as the self-contained Friday artifact set.

## Claims Registry

The packaged claims-registry snapshot is [claims_registry_snapshot.yaml](claims_registry_snapshot.yaml). The repository source of truth remains `results/claims_registry.yaml`; update the package snapshot before quoting new workload claims.
