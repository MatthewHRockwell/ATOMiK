# ATOMiK Proof Cards

This digest is for controlled proof review. It summarizes what each artifact proves, what it does not prove, and what language is safe to use.

## Proof Card: ATOMiK Desk v0.40-A UI Artifact

Evidence label: HARDWARE_VALIDATED

Artifact: `10-current-live-atomik-desk-v040a.png` (supersedes v0.39-K)

What it proves:

- The current ATOMiK Desk prototype/demo surface was captured live from `/dev/fb0` (fb2png) on the booted AX7020.
- The UI direction is visually inspectable, and on-screen metrics are driven by real measured on-board data.

What it does not prove:

- Customer workload performance.
- Production maturity or uptime.
- Battery, heat, cooling, water, power-bill, or footprint outcomes.
- Benchmark results.

Safe claim:

ATOMiK Desk v0.40-A is the current Zynq hardware UI artifact, captured live from the framebuffer.

Caveat:

Use with `10-current-live-atomik-desk-v040a.caption.md` and the label `HARDWARE_VALIDATED UI ARTIFACT - NOT A CUSTOMER WORKLOAD BENCHMARK`.

## Proof Card: Parallel-Bank Throughput on AX7020

Evidence label: LIVE_MEASURED

Artifact: `PARALLEL_BANKS_HARDWARE_VALIDATED.md` (repo: `hardware/zynq/results/`)

What it proves:

- An 8-bank ATOMiK XOR accumulator scaled linearly with allocated banks on the AX7020, measured with an on-chip cycle counter.
- 1/2/4/8 banks complete the same accumulation in 1/2/4/8x fewer cycles (about 100 to 800 Mdeltas/s at the 100 MHz sys clock).
- The result was byte-identical across every bank count and matched a software recompute: the order-independent, lock-free shared accumulator, on silicon.

What it does not prove:

- A customer-workload speedup.
- Battery, heat, cooling, water, power-bill, or footprint outcomes.
- Production readiness.

Safe claim:

On the AX7020, an 8-bank ATOMiK accumulator scaled 1/2/4/8x with allocated banks, with a byte-identical result that matched software.

Caveat:

This is the measurement bench engine (0xF0021000), not the production application adapter. Quote throughput figures at the 100 MHz sys clock.

## Proof Card: Live Workloads Surface on AX7020

Evidence label: LIVE_MEASURED

Artifact: `12-workloads-live-ax7020.png` (pairs with `PARALLEL_BANKS_HARDWARE_VALIDATED.md`)

What it proves:

- The Workloads surface (real-time telemetry aggregation) runs on the AX7020 over HDMI, captured live from `/dev/fb0`.
- The on-screen throughput is live measured from the parallel-bank engine: 800 Mevents/s at 8 banks, 1/2/4/8 throughput scaling, byte-identical result across all bank configurations.
- The first customer-facing workload demo proven end-to-end on hardware.

What it does not prove:

- Customer workload performance, production maturity, or downstream power/thermal outcomes.
- An interactive session or USB keyboard/mouse input (both still in flight; this is a static capture).
- That any on-screen value other than the throughput figures is measured (Resource Fabric lane values and top-bar temp/efficiency/predictive readouts are derived or scenario data).

Safe claim:

ATOMiK's Workloads surface runs on the AX7020 over HDMI showing live measured parallel-bank throughput (800 Mevents/s at 8 banks, byte-identical across configs).

Caveat:

Use with `12-workloads-live-ax7020.caption.md`. Scope the numbers to throughput only. Never present as an interactive demo or imply USB input.

## Proof Card: Verified Workload Savings + Self-Driving Demo (AX7020)

Artifact: `13-workloads-telemetry-verified-ax7020.png` + `14-workloads-coalescing-selfdriving-ax7020.png` (captions alongside)

What it proves:

- The redesigned conventional-vs-ATOMiK Workloads surface runs on the AX7020 over HDMI, captured live from `/dev/fb0`.
- Edge telemetry sync: 2,048 B conventional vs 160 B as deltas per tick — 92% less data moved — adapter-verified (exact 64-bit LOAD/ACCUM/READ round-trips at 0xF0020000, falsification-tested harness).
- Control coalescing: 256 updates coalesce to 32 net writes — 87% fewer — adapter-verified, final state identical.
- The demo SELF-DRIVES: the untethered board auto-cycles all three scenarios (two captures ~15s apart show different scenarios).

What it does NOT prove:

- Universal multipliers (92% is at 5% change density; 87% at this update locality — ratios scale with the pattern). Never blend with the retired 7,670x–916,000x range.
- An interactive session or USB keyboard/mouse input (both still in flight). Self-driving = auto-cycling, not input.
- Power, thermal, customer-outcome, or production-maturity claims. Fabric/top-bar values remain derived/scenario.

Use with `13-...caption.md` / `14-...caption.md`. Scope every number to its pattern.

## Proof Card: Zynq HDMI Display (1080p30)

Evidence label: HARDWARE_VALIDATED

Artifact: `HDMI_1080P60_IMPOSSIBLE.md` (repo: `hardware/zynq/results/`)

What it proves:

- The ATOMiK Desk UI renders at 1080p30 over HDMI on the AX7020.
- A fresh place-and-route shows 1080p60 fails minimum-pulse-width on the 737.5 MHz serializer.

What it does not prove:

- 1080p60 on this board (it is physically impossible on the soldered -2 part).

Safe claim:

ATOMiK renders at 1080p30 over HDMI on the AX7020.

Caveat:

Claim 1080p30 only. Never claim 1080p60. 720p60 is the achievable true-60Hz path.

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
