# Technical Proof

ATOMiK public proof is organized by evidence label so measured artifacts,
hardware validation, synthesis output, concepts, and roadmap work do not blur
together.

## Current Proof Map

| Claim area | Label | Artifact |
|---|---|---|
| Current ATOMiK Desk prototype screenshot | `HARDWARE_VALIDATED` | `website/public/01-current-live-atomik-desk.jpg` |
| AX7020 performance matrix board run | `LIVE_MEASURED` | `results/perf_matrix_ax7020_20260509.txt` |
| AX7020 matrix interpretation and caveats | `LIVE_MEASURED` | `docs/perf/20260509_matrix_interpretation.md` |
| Linux userspace validation path | `HARDWARE_VALIDATED` | `docs/LINUX_USERSPACE_PROOF.md` |
| Parallel accumulator bank hardware and synthesis notes | `HARDWARE_VALIDATED` / `SYNTHESIS_VALIDATED` | `docs/HARDWARE_SYNTHESIS.md` |
| Formal proof work | `SOFTWARE_VALIDATED` | `math/proofs/` |
| Concept visuals | `CONCEPTUAL` / `ROADMAP` | `docs/visual-asset-manifest.md` |

## How To Read The Proof Stack

- `LIVE_MEASURED` means a running system produced a recorded artifact.
- `HARDWARE_VALIDATED` means physical hardware demonstrated the behavior, even
  if the complete benchmark package is not yet published.
- `SYNTHESIS_VALIDATED` means toolchain output supports the result, not live
  board execution.
- `CONCEPTUAL` and `ROADMAP` explain product direction. They are not current
  shipped functionality.

## Current Live Proof

The current ATOMiK Desk screenshot is a live hardware prototype proof image. It
is appropriate for the homepage, README, pitch deck, technical docs, and
one-pager when labeled as:

> Current ATOMiK Desk prototype running on live hardware.

It should not be described as a polished shipping product.

## Benchmark Artifact Guidance

New benchmark packages should include:

- workload
- platform
- hardware
- software version
- date
- command
- raw output
- interpretation
- evidence label
- commit hash when available

Measured results will be posted when artifact packages are ready. Do not fill
gaps with estimated performance numbers.

## Source Of Truth

The public claims registry is
[results/claims_registry.yaml](../results/claims_registry.yaml).
