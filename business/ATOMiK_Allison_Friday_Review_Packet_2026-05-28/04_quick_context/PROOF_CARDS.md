# ATOMiK Proof Cards

This digest is for controlled proof review. It summarizes what each artifact proves, what it does not prove, and what language is safe to use.

## Proof Card: ATOMiK Desk v0.39-K UI Artifact

Evidence label: HARDWARE_VALIDATED

Artifact: `09-current-live-atomik-desk-v039k.png`

What it proves:

- A current ATOMiK Desk prototype/demo surface exists as a Zynq hardware UI artifact.
- The UI direction is visually inspectable.

What it does not prove:

- Customer workload performance.
- Production maturity or uptime.
- Battery, heat, cooling, water, power-bill, or footprint outcomes.
- Benchmark results.

Safe claim:

ATOMiK Desk v0.39-K is a current Zynq hardware UI artifact.

Caveat:

Use with `09-current-live-atomik-desk-v039k.caption.md` and the label `HARDWARE_VALIDATED UI ARTIFACT - NOT A CUSTOMER WORKLOAD BENCHMARK`.

## Proof Card: Linux Userspace-to-FPGA Validation

Evidence label: HARDWARE_VALIDATED

Artifacts: `LINUX_USERSPACE_PROOF.md`, `LINUX_USERSPACE_PROOF_SUMMARY.md`, `ZYNQ_BASELINE.md`

What it proves:

- ATOMiK primitive path was exercised through Linux userspace to FPGA.
- 16/16 algebraic property checks passed on the documented Zynq configuration.
- MMIO ordering requirement was discovered and documented.

What it does not prove:

- Customer workload value.
- Battery, thermal, cooling, water, footprint, or power-bill outcomes.
- Production readiness.
- Universal speedup.

Safe claim:

ATOMiK algebraic property checks passed through a Linux userspace-to-FPGA path on the documented Zynq configuration.

Caveat:

Use with environment details and the `fence iorw, iorw` plus dummy STATUS read ordering requirement.

## Proof Card: Zynq Frozen Baseline

Evidence label: HARDWARE_VALIDATED / baseline context

Artifact: `ZYNQ_BASELINE.md`

What it proves:

- The zynq-linux-v1 baseline records board, CPU, Linux, OpenSBI, rootfs, toolchain, boot configuration, checksums, and pass result.
- The documented baseline is the frozen Linux/userspace hardware-path validation baseline.

What it does not prove:

- v0.39-K UI screenshot metrics.
- v0.33-D AX7020 performance matrix results.
- Future SD-boot workload validation.

Safe claim:

zynq-linux-v1 is the frozen Linux userspace-to-FPGA validation baseline for the 16/16 algebraic checks.

Caveat:

Do not blend this baseline with later UI or SD-boot bring-up artifacts.

## Proof Card: AX7020 Raw Matrix

Evidence label: LIVE_MEASURED

Artifacts: `perf_matrix_ax7020_20260509.txt`, `perf_matrix_ax7020_20260509.csv`, `perf_matrix_ax7020_20260509.summary.json`

What it proves:

- A board-run four-way matrix exists for software, direct hardware, batched hardware, and profiled/coalesced paths.
- The matrix includes workload-specific wins and losses.

What it does not prove:

- Universal speedup.
- Customer workload results.
- Battery, heat, cooling, water, power-bill, footprint, or production outcomes.

Safe claim:

The AX7020 matrix shows workload-specific behavior across software, direct, batched, and profiled paths.

Caveat:

Do not isolate the biggest number without the row, workload, baseline, and caveat.

## Proof Card: AX7020 Interpretation

Evidence label: LIVE_MEASURED interpretation

Artifacts: `20260509_matrix_interpretation.md`, `AX7020_SUMMARY.md`

What it proves:

- The defensible story is workload-specific: ATOMiK can win when batching/coalescing/personality rules reduce redundant work.
- Naive hardware access can lose.
- Larger or non-fit rows can lose to software.

What it does not prove:

- That ATOMiK is always faster.
- That hardware alone is the value.
- That downstream customer outcomes are measured.

Safe claim:

ATOMiK wins when the workload lets the architecture compound; naive hardware access and non-fit workloads can lose.

Caveat:

Use with the raw matrix and the known limitations around SYNC repeat behavior, AGENT small-workload overhead, and first-run mmap overhead.

## Proof Card: Claims Registry Snapshot

Evidence label: Claims registry

Artifact: `claims_registry_snapshot.yaml`

What it proves:

- Current public-safe claims have labels, artifacts, caveats, and notes.
- v0.39-K, AX7020, Linux userspace validation, formal proof work, SD boot build artifacts, and concept visuals are separated by label.

What it does not prove:

- The technical claim itself without the referenced artifact.
- That external or future artifacts are included in this folder.

Safe claim:

The packaged claims registry maps each public-safe claim to an evidence label, artifact, and caveat.

Caveat:

Update the registry before adding any new Zynq workload claim.

## Proof Card: Evidence Labels

Evidence label: Evidence framework

Artifact: `evidence-labels.md`

What it proves:

- The packet has a governance system for separating live measured, hardware validated, software validated, synthesis validated, build artifact, formal proof, projected, conceptual, and roadmap claims.

What it does not prove:

- That any claim is true without its artifact.

Safe claim:

ATOMiK uses evidence labels to keep proof, projected outcomes, concept visuals, and roadmap work separate.

Caveat:

Labels must travel with the claim in decks, website copy, and follow-up materials.
