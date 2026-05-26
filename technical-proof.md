# Technical Proof

ATOMiK public proof is organized by evidence label so measured artifacts,
hardware validation, synthesis output, concepts, and roadmap work do not blur
together.

## Current Proof Map

| Claim area | Label | Artifact |
|---|---|---|
| Current ATOMiK Desk v0.39-K UI proof screenshot | `HARDWARE_VALIDATED` | `website/public/09-current-live-atomik-desk-v039k.png` |
| Earlier ATOMiK Desk v0.38-I UI proof screenshot | `HARDWARE_VALIDATED` | `website/public/08-current-live-atomik-desk-v038i.png` |
| Earlier ATOMiK Desk prototype screenshot | `HARDWARE_VALIDATED` | `website/public/01-current-live-atomik-desk.jpg` |
| AX7020 performance matrix board run | `LIVE_MEASURED` | `results/perf_matrix_ax7020_20260509.txt` |
| AX7020 matrix interpretation and caveats | `LIVE_MEASURED` | `docs/perf/20260509_matrix_interpretation.md` |
| Linux userspace validation path | `HARDWARE_VALIDATED` | `docs/LINUX_USERSPACE_PROOF.md` |
| Parallel accumulator bank hardware and synthesis notes | `HARDWARE_VALIDATED` / `SYNTHESIS_VALIDATED` | `docs/HARDWARE_SYNTHESIS.md` |
| Standalone Zynq SD boot build artifacts | `BUILD_ARTIFACT` | `hardware/zynq/fsbl_build/BOOT.bin`, `hardware/zynq/litex-build-nax64-sdboot/gateware/` |
| Formal proof work | `SOFTWARE_VALIDATED` | `math/proofs/` |
| Concept visuals | `CONCEPTUAL` / `ROADMAP` | `docs/visual-asset-manifest.md` |

## How To Read The Proof Stack

- `LIVE_MEASURED` means a running system produced a recorded artifact.
- `HARDWARE_VALIDATED` means physical hardware demonstrated the behavior, even
  if the complete benchmark package is not yet published.
- `SYNTHESIS_VALIDATED` means toolchain output supports the result, not live
  board execution.
- `BUILD_ARTIFACT` means the files or binaries exist locally, but the full
  hardware path is not yet promoted as a live result.
- `CONCEPTUAL` and `ROADMAP` explain product direction. They are not current
  commercial functionality.

## Current Live Proof

The current ATOMiK Desk v0.39-K screenshot is a live hardware prototype proof
image. It is appropriate for the homepage, investor brief, README, pitch deck,
technical docs, and one-pager when labeled as:

> ATOMiK Desk v0.39-K prototype UI running on live Zynq hardware.

It should not be described as a polished commercial product. The older
`01-current-live-atomik-desk.jpg`, v0.38-G, and v0.38-I artifacts remain useful
as historical live proof. Do not infer benchmark, uptime, allocation, maturity,
commercial product, power, thermal, water, battery, or performance claims from
any screenshot.

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

Do not fill gaps with estimated performance numbers. If a result is not backed
by a raw artifact and interpretation note, classify it as projected, conceptual,
or roadmap instead of phrasing it as an observed result.

## Source Of Truth

The public claims registry is
[results/claims_registry.yaml](../results/claims_registry.yaml).
