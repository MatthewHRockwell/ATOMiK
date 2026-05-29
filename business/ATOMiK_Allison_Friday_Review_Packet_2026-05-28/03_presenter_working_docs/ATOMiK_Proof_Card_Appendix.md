# ATOMiK Proof Card Appendix

Use this as a compact Q&A appendix. Quote claims only with artifact, context, and caveat.

Source of truth for proof details:
- `05_proof_artifacts/README.md`
- `05_proof_artifacts/PROOF_CARDS.md`
- `05_proof_artifacts/VERSION_MAP.md`
- `05_proof_artifacts/claims_registry_snapshot.yaml`
- `05_proof_artifacts/README_EXTERNAL_REFERENCES.md`

## Zynq Desk v0.39-K

- Evidence label: HARDWARE_VALIDATED
- Artifacts: `05_proof_artifacts/09-current-live-atomik-desk-v039k.png`, `05_proof_artifacts/09-current-live-atomik-desk-v039k.caption.md`
- What it supports: current Zynq demo surface / hardware artifact screenshot.
- What it does not support: production readiness, product maturity, battery/thermal outcomes, or customer workload value.
- Public-safe language: Current Zynq demo surface exists as a hardware-validated artifact.

## Linux Userspace-To-FPGA Validation

- Evidence label: HARDWARE_VALIDATED
- Artifacts: `05_proof_artifacts/LINUX_USERSPACE_PROOF_SUMMARY.md`, `05_proof_artifacts/LINUX_USERSPACE_PROOF.md`
- What it supports: the primitive path was exercised through Linux userspace to FPGA in the documented validation path, including 16/16 algebraic property checks.
- What it does not support: customer workload value, production readiness, or downstream battery/thermal/cooling outcomes.
- Public-safe language: ATOMiK has hardware-validated primitive-path evidence through Linux userspace-to-FPGA for the documented test context.

## AX7020 Board-Run Matrix

- Evidence label: LIVE_MEASURED
- Artifacts: `05_proof_artifacts/AX7020_SUMMARY.md`, `05_proof_artifacts/perf_matrix_ax7020_20260509.txt`, `05_proof_artifacts/perf_matrix_ax7020_20260509.csv`, `05_proof_artifacts/perf_matrix_ax7020_20260509.summary.json`, `05_proof_artifacts/20260509_matrix_interpretation.md`
- What it supports: workload-specific behavior with wins and losses; batching/coalescing matters.
- What it does not support: universal speedup, customer savings, or broad performance claims.
- Public-safe language: The AX7020 matrix shows workload-specific measured behavior and should be discussed only with artifact, workload, and caveat.

## SD Boot / Zynq Workload Validation

- Evidence label today: BUILD_ARTIFACT / validation plan, until promoted by run artifacts.
- Source docs: `02_business_docs/ZYNQ_WORKLOAD_VALIDATION_PLAN.md`, `02_business_docs/SD_BOOT_VALIDATION_NOTE_TEMPLATE.md`, `02_business_docs/ZYNQ_PROOF_CARD_TEMPLATE.md`
- Current status: We are working on booting from SD card instead of relying on JTAG so workload updates can be made and rerun faster. After SD boot is stable, the first validation workloads will be integrated and measured.
- P0 workloads: dirty-state telemetry sync; repeated register/control update coalescing.
- What it does not support yet: completed Zynq workload claims, battery gains, heat reduction, cooling/water savings, or production readiness.

## Formal Proof Foundation

- Evidence label: FORMAL_PROOF where directly audited; otherwise SOFTWARE_VALIDATED / proof work present.
- Artifact context: proof work exists in repo; avoid public theorem counts until reconciled across repo, site, and deck.
- What it supports: algebraic foundation and correctness discipline.
- What it does not support: commercial performance, workload value, or downstream physical outcomes.

## Claims Not Supported Today

- Universal speedup.
- Guaranteed battery-life improvement.
- Guaranteed heat, cooling, water, power-bill, or footprint reduction.
- Production-ready ASIC or product.
- Replacement of CPUs, GPUs, NPUs, compression, caching, or sync protocols.
- Customer traction, revenue, or signed design partners unless separately documented.
