# ATOMiK Proof Packet Source

Source-copy note: this pitch-assets file is carried for website/pitch asset continuity. Current source-of-truth routing is in `../SOURCE_OF_TRUTH.md`; do not edit this copy as the sole authority unless regenerating packet assets intentionally.


## Proof Philosophy

Every ATOMiK claim needs a label, artifact, context, and caveat. Public performance claims must be quoted only with the artifact and interpretation that produced them.

ATOMiK is evaluated workload-by-workload. The architecture is strongest where state movement, repeated scans, full-state sync, replay, or reconstruction dominate the cost.

## Current Proof-Artifacts Source Of Truth

Use these packet-root files for current proof details before quoting or forwarding proof claims:

- `05_proof_artifacts/README.md`
- `05_proof_artifacts/PROOF_CARDS.md`
- `05_proof_artifacts/VERSION_MAP.md`
- `05_proof_artifacts/claims_registry_snapshot.yaml`
- `05_proof_artifacts/README_EXTERNAL_REFERENCES.md`
- `05_proof_artifacts/CHECKSUMS.md`

This source document is a website/pitch copy guide. The packaged proof folder controls artifact paths, captions, send policy, version governance, known limitations, external references, and checksums.

## Evidence Labels

| Label | Meaning | Public rule |
|---|---|---|
| LIVE_MEASURED | Observed on a running system with recorded measurement artifacts. | May state observed result only with artifact link. |
| HARDWARE_VALIDATED | Demonstrated on physical hardware. | May say hardware-validated. Do not imply production readiness. |
| SOFTWARE_VALIDATED | Shown in software, simulation, local runtime, or proof environment. | May say software-validated. Do not imply board execution. |
| SYNTHESIS_VALIDATED | Validated through synthesis, build, compile, or tool output. | Keep separate from live-board results. |
| BUILD_ARTIFACT | Local build output exists, but end-to-end run is not promoted. | May say built locally. Do not say validated. |
| FORMAL_PROOF | Directly audited formal statement or proof artifact. | Use only for the exact property proven; do not imply workload, customer, or production outcomes. |
| PROJECTED | Model or estimate. | Must not be phrased as a result. |
| CONCEPTUAL | Product direction or explanatory visual. | Must be labeled as concept/design target. |
| ROADMAP | Planned work. | Must be phrased as planned or future work. |

## Claims Registry Summary

Friday packet source: `05_proof_artifacts/claims_registry_snapshot.yaml`. Repo source: `results/claims_registry.yaml`.

Current public-safe proof families:

- Current ATOMiK Desk prototype screenshots: HARDWARE_VALIDATED, not commercial maturity proof.
- AX7020 board-run performance matrix: LIVE_MEASURED, workload-specific, caveated.
- Linux userspace-to-FPGA validation: HARDWARE_VALIDATED, 16 algebraic checks recorded as pass/fail in the source artifact.
- Parallel-bank hardware validation: HARDWARE_VALIDATED, exact counts only when quoting the artifact.
- Formal proof work: FORMAL_PROOF where directly audited; otherwise SOFTWARE_VALIDATED / proof work present. Avoid public proof counts until reconciled across repo, site, and deck.
- Concept visuals: CONCEPTUAL or ROADMAP, not proof of current functionality.

## Proof Card 1: Linux Userspace-to-FPGA Validation

Label: HARDWARE_VALIDATED

Artifact: `docs/LINUX_USERSPACE_PROOF.md`

What it shows: ATOMiK algebraic checks were exercised from Linux userspace through `/dev/mem`, Wishbone CSR bus, and the FPGA accelerator path. The artifact records 16 algebraic property checks passing with zero failures.

What it does not show: It does not prove production readiness, universal performance wins, battery impact, thermal impact, or customer workload value.

Approved language: `ATOMiK has hardware-validated Linux userspace-to-FPGA proof for the algebraic path.`

## Proof Card 2: AX7020 Matrix

Label: LIVE_MEASURED

Artifacts: `results/perf_matrix_ax7020_20260509.txt` and `docs/perf/20260509_matrix_interpretation.md`

What it shows: A board-run matrix compared software, direct hardware, batched ATOMiK, and profiled/coalesced ATOMiK paths. The interpretation shows a small STATE workload where profiled/coalesced ATOMiK beats the software baseline, and other rows where software wins.

What it does not show: It does not prove ATOMiK is always faster. It does not prove power, battery, thermal, water, or footprint improvement.

Approved language: `The AX7020 matrix shows ATOMiK can win in specific coalesced/batched scenarios and lose in others. Workload personality matters.`

## Proof Card 3: Hardware Synthesis and Bank Scaling

Label: HARDWARE_VALIDATED or SYNTHESIS_VALIDATED, depending on the quoted result.

Artifact: `docs/HARDWARE_SYNTHESIS.md`

What it shows: The artifact reports parallel accumulator bank scaling, synthesis results, and Tang Nano 9K validation records.

What it does not show: It is not production silicon proof and should not be blended with live AX7020 workload measurements.

Approved language: `ATOMiK has synthesis and hardware-validation artifacts for parallel accumulator bank scaling.`

## Proof Card 4: Formal Proof Work

Label: FORMAL_PROOF where directly audited; otherwise SOFTWARE_VALIDATED / proof work present.

Artifact: `math/proofs/`

What it shows: Formal proof work is present in the repository for the algebraic foundation.

What it does not show: It does not prove commercial workload outcomes, production readiness, or universal performance gains.

Approved language: `ATOMiK has formal proof work for the algebraic foundation. Implementation and workload claims remain evidence-labeled.`

## What Is Not Proven Yet

- Universal speedup.
- Guaranteed battery extension.
- Guaranteed heat reduction.
- Guaranteed water savings.
- Guaranteed smaller hardware.
- Production-ready ASIC or commercial silicon.
- Replacement of CPUs, GPUs, accelerators, compression, caching, or sync protocols.
- Customer-specific ROI without a customer workload, baseline, and measurement method.

## Approved Public Language

- `State-aware compute architecture.`
- `Makes meaningful change the unit of compute.`
- `Evaluation-first.`
- `One workload, one baseline, one constraint.`
- `Workload-specific evidence.`
- `Measured against the current baseline.`
- `Preserves correctness.`
- `Targets wasted state movement.`
- `May reduce pressure on power, heat, bandwidth, latency, or footprint when state movement is the constraint.`

## Language To Avoid

- `Universal speedup.`
- `Guaranteed battery improvement.`
- `Guaranteed heat reduction.`
- `Guaranteed water savings.`
- `Production-ready ASIC.`
- `Replaces CPUs/GPUs.`
- `Works for all compute.`
- `Proven smaller hardware` unless tied to a specific artifact.
- `Atom AI product` unless productized and labeled.
