# ATOMiK Proof Version Map

This map prevents proof artifacts from being blended across versions. Each version proves a different thing.

## v0.39-K

Current Zynq UI screenshot artifact.

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
