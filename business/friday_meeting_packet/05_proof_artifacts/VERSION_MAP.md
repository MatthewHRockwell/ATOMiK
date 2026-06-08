# ATOMiK Proof Version Map

This map prevents proof artifacts from being blended across versions. Each version proves a different thing.

## Live Workloads surface (2026-06-08)

First customer-facing workload demo proven end-to-end on hardware.

- Packaged artifact: `12-workloads-live-ax7020.png`
- Evidence label: LIVE_MEASURED
- Capture method: live `fb2png` of `/dev/fb0` on the AX7020 unified bitstream (HDMI + bench engine), 2026-06-07. The Workloads surface reads live throughput from the parallel-bank engine (0xF0021000) via a board-side daemon writing `/tmp/atomik_bench_live.txt`.
- What it supports: the Workloads / telemetry-aggregation surface runs on hardware showing live measured parallel-bank throughput — 800 Mevents/s at 8 banks, 1/2/4/8 scaling, byte-identical across configs. Pairs with `PARALLEL_BANKS_HARDWARE_VALIDATED.md`.
- What it does NOT support: an interactive session or USB keyboard/mouse input (both still in flight); any on-screen value other than the throughput figures (Fabric lane values and top-bar temp/efficiency/predictive are derived/scenario); customer workload, power, thermal, or production outcomes.

## v0.40-A

Current Zynq UI capture artifact (supersedes v0.39-K as the current-live image).

- Packaged artifact: `10-current-live-atomik-desk-v040a.png`
- Evidence label: HARDWARE_VALIDATED
- Capture method: live `fb2png` of `/dev/fb0` on the booted AX7020 (RV64 NaxRiscv, simplefb 1920x1080), 2026-05-29. Not a recomposited/edited mockup.
- What it supports: current live concept-aligned UI (ATOMiK Desk) running on Zynq hardware, with on-screen metrics from real on-board perf-bench.
- What it does not support: customer workload performance, production maturity, uptime, battery, heat, cooling, water, power, or footprint outcomes.

## v0.39-K

Prior Zynq UI screenshot artifact (archived; superseded by v0.40-A above).

- Packaged artifact: `09-current-live-atomik-desk-v039k.png`
- Evidence label: HARDWARE_VALIDATED
- What it supports: current live prototype/demo surface and UI direction running on Zynq hardware.
- What it does not support: customer workload performance, production maturity, uptime, battery, heat, cooling, water, power, or footprint outcomes.

## v0.33-D

AX7020 performance matrix version.

- Packaged artifacts: `perf_matrix_ax7020_20260509.txt`, `perf_matrix_ax7020_20260509.csv`, `perf_matrix_ax7020_20260509.summary.json`, `20260509_matrix_interpretation.md`, `AX7020_SUMMARY.md`
- Evidence label: LIVE_MEASURED
- What it supports: workload-specific board-run matrix with wins, losses, and caveats.
- What it does not support: universal speedup, customer savings, battery/thermal outcomes, or production readiness.

## zynq-linux-v1

Frozen Linux userspace-to-FPGA validation baseline.

- Packaged artifacts: `LINUX_USERSPACE_PROOF.md`, `LINUX_USERSPACE_PROOF_SUMMARY.md`, `ZYNQ_BASELINE.md`
- Evidence label: HARDWARE_VALIDATED
- What it supports: Linux userspace path to ATOMiK FPGA core and 16/16 algebraic checks on the documented configuration.
- What it does not support: v0.39-K UI proof, v0.33-D performance matrix results, or future SD-boot workload validation.

## SD Boot Artifacts

Build/bring-up context only until promoted.

- Evidence label: BUILD_ARTIFACT unless recorded power-on boot logs and workload artifacts exist.
- What it supports today: local build artifacts and planned validation path.
- What it does not support yet: completed standalone SD boot, Zynq workload results, or customer-environment proof.

## Formal Proof Work

Formal proof work is present, but public theorem-count claims should remain reconciled and audited before headline use.

- Evidence label: FORMAL_PROOF only for directly audited formal statements; otherwise use SOFTWARE_VALIDATED / proof work present.
- What it does not support: commercial outcome claims, workload performance, or production readiness.
