# ATOMiK Evidence Labels

This framework is the public source of truth for how ATOMiK labels claims,
metrics, screenshots, concept visuals, and roadmap material.

## Global Disclaimer

Live screenshots show current prototypes. Concept visuals show product direction
and are not represented as current shipped functionality. Performance claims
are only stated when backed by measured artifacts.

## Label Definitions

| Label | Definition | Public wording rule |
|---|---|---|
| `LIVE_MEASURED` | Observed on a running system with recorded measurement artifacts. | May be phrased as an observed result only when the artifact is linked. |
| `HARDWARE_VALIDATED` | Demonstrated on physical hardware, but not necessarily accompanied by full benchmark artifacts. | May say hardware-backed or hardware-validated. Do not imply full production readiness. |
| `SOFTWARE_VALIDATED` | Shown in a software prototype, simulation, local runtime, or non-hardware environment. | May say software-validated. Do not imply board execution. |
| `SYNTHESIS_VALIDATED` | Validated through synthesis, build, compile, or toolchain output. | Must be separated from live-board results. |
| `PROJECTED` | A model, estimate, or expected economic/performance outcome. | Must not be phrased as a result. |
| `CONCEPTUAL` | Used to explain product direction, UX, architecture, or design intent. Not current proof. | Must be labeled as concept visual or design target. |
| `ROADMAP` | Planned work that may change. | Must be phrased as planned or future work. |

## Claim Rules

- Every public performance number needs a label and an artifact link.
- Concept visuals must never appear as proof of current product maturity.
- Synthesis ceilings and modeled economics must not be blended with live
  measurements.
- Public customer, production, uptime, power, thermal, savings, and traction
  claims require artifact-backed approval before publication.
- If a claim cannot be traced to an artifact, soften it, label it as roadmap or
  conceptual, or remove it.

## Public Asset Labels

See [visual-asset-manifest.md](visual-asset-manifest.md) for approved captions,
placement rules, and overclaim risks for the current public image set.

## Claims Registry

The current claims registry is maintained in
[results/claims_registry.yaml](../results/claims_registry.yaml). Public pages
should use that registry as the source of truth for proof labels and claim
status.
