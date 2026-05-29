# External Or Referenced Proof Artifacts

This proof folder is self-contained for the Friday send packet, but some registry entries point to source-repository artifacts that are not duplicated here. Use this file to keep external artifacts from being treated as bundled proof.

| External artifact path | What it supports | Why it is not packaged here | Evidence label | Access | Safe language | Prohibited language |
|---|---|---|---|---|---|---|
| `docs/HARDWARE_SYNTHESIS.md` | Parallel-bank synthesis and Tang Nano 9K hardware-validation context | Full source artifact belongs in repo-controlled technical diligence | HARDWARE_VALIDATED / SYNTHESIS_VALIDATED depending on the exact claim | Controlled repo access or controlled technical export | Parallel-bank synthesis/hardware-validation context exists in the repo source artifact. | Production silicon proof, live Zynq workload proof, or broad throughput claim without exact artifact/context. |
| `hardware/zynq/fsbl_build/BOOT.bin` | SD boot build-artifact context | Local generated build artifact, not a diligence proof package by itself | BUILD_ARTIFACT | Controlled repo/local build access | SD boot build artifacts exist locally. | Standalone SD boot is validated, booted, or working end to end without recorded power-on logs. |
| `hardware/zynq/litex-build-nax64-sdboot/gateware/` | SD boot bitstream/gateware context | Generated build tree; too large and volatile for the proof digest | BUILD_ARTIFACT | Controlled repo/local build access | SD boot gateware artifacts exist locally. | Validated SD boot, validated workload path, or customer proof. |
| `math/proofs/` | Formal proof work context | Source-tree path, not packaged as an audited formal-proof export | SOFTWARE_VALIDATED / FORMAL_PROOF only where directly audited | Controlled repo access | Formal proof work is present; use FORMAL_PROOF only for exact audited statements. | Unreconciled theorem counts, commercial outcome proof, workload proof, or production readiness. |

If these artifacts become central to diligence, add them to a controlled technical-data-room export or provide repository access under NDA. Do not quote exact counts or performance ceilings from external artifacts unless the matching source artifact is available to the reviewer.
