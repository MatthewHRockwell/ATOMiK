# Proof Packet Source

## Proof philosophy
Every claim needs an evidence label, artifact, context, and caveat. Performance claims are quoted only with workload and measurement context. Concept visuals explain direction; they are not proof.

## Evidence labels
- LIVE_MEASURED: measured result from a running system with raw artifacts.
- HARDWARE_VALIDATED: demonstrated on physical hardware with reproducible evidence.
- SOFTWARE_VALIDATED: demonstrated in software with tests, logs, or reproducible run evidence.
- SYNTHESIS_VALIDATED: validated by synthesis or implementation artifacts, not production silicon.
- BUILD_ARTIFACT: build exists, but the full live path is not promoted to measured proof.
- FORMAL_PROOF: mathematical proof artifact, limited to the property proven.
- PROJECTED: modeled or expected outcome requiring measurement.
- CONCEPTUAL: visual or product-direction material, not current functionality proof.
- ROADMAP: planned future work.

## Current proof artifacts
Use `business/friday_send/04_proof_artifacts/README.md` as the send-package proof index. It includes Linux userspace proof, AX7020 matrix artifacts, evidence labels, Zynq baseline context, current screenshot artifact, and claims registry snapshot.

## Linux userspace-to-FPGA validation
Evidence label: HARDWARE_VALIDATED. The artifact supports the claim that the primitive path has been exercised through Linux userspace to FPGA and that algebraic property checks passed in that context. It does not prove customer workload value or downstream battery, thermal, cooling, water, or footprint outcomes.

## AX7020 matrix / workload-specific benchmark story
Evidence label: LIVE_MEASURED for the board-run matrix and interpretation artifacts. The usable story is that ATOMiK is strongest where state movement, repeated scans, full-state sync, replay, reconstruction, batching, or coalescing dominate. Naive hardware access can lose, and some larger uniform workloads lose to software. Do not isolate the largest number without context and caveat.

## Zynq validation status or plan
Current Friday claim: We are working on booting from SD card instead of relying on JTAG so workload updates can be made and rerun faster. After SD boot is stable, the first validation workloads will be integrated and measured. Planned P0 workloads are dirty-state telemetry sync and repeated register/control update coalescing. Supporting source docs: `ZYNQ_WORKLOAD_VALIDATION_PLAN.md`, `SD_BOOT_VALIDATION_NOTE_TEMPLATE.md`, `ZYNQ_PROOF_CARD_TEMPLATE.md`, `ZYNQ_RESULTS_ARTIFACT_STRUCTURE.md`, and `ZYNQ_CLAIMS_REGISTRY_UPDATE_PLAN.md`. Do not present planned Zynq workload results as measured proof.

## Synthesis/build artifacts
Evidence label: SYNTHESIS_VALIDATED or BUILD_ARTIFACT depending on artifact quality. These support feasibility and build-path discussion, not production silicon performance or customer outcome claims.

## Formal proof foundation
Evidence label: FORMAL_PROOF where directly audited; otherwise SOFTWARE_VALIDATED / proof work present. Use FORMAL_PROOF only for the specific algebraic properties covered by the artifact. Avoid unaudited theorem-count hype until the site, README, deck, and proof packet use one reconciled count.

## What is not proven yet
No universal speedup, guaranteed battery extension, guaranteed heat reduction, guaranteed cooling reduction, water savings, production-ready ASIC, customer savings, signed traction, or generalized AI performance improvement.

## Approved public language
ATOMiK is evaluated workload by workload. The architecture is strongest where state movement, repeated scans, full-state sync, replay, or reconstruction dominate the cost. Downstream power, battery, thermal, cooling, water, and footprint outcomes require workload-specific measurement.

## Language to avoid
Universal speedup. Guaranteed battery improvement. Guaranteed heat reduction. Guaranteed water savings. Production-ready ASIC. Replaces CPUs, GPUs, NPUs, compression, caching, or sync protocols. Proven smaller hardware without a specific artifact.
