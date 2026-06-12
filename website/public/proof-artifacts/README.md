# Proof Artifacts Index

These artifacts support controlled proof review and diligence. Quote any performance or proof claim only with artifact, context, and caveat.

Customer outcomes are not proven by this folder. Battery, heat, cooling, water, power-bill, footprint, customer savings, production readiness, and universal workload outcomes require workload-specific measurement.

Proof story: ATOMiK's current proof stack is not a universal performance claim. It is an artifact-bound stack showing: (1) a current Zynq UI/demo surface captured live from the framebuffer, (2) a Linux userspace-to-FPGA primitive path with algebraic checks passing, (3) a live-measured AX7020 workload matrix with wins, losses, and caveats, (4) a live-measured parallel-bank throughput substrate that scales 1/2/4/8x with allocated banks on silicon, (5) the first customer-facing Workloads demo (telemetry aggregation) proven end-to-end on hardware with live measured throughput, and (6) a version map and claims registry that prevent overclaiming. The next evidence gate is customer-representative Zynq workload validation.

Integrity note: this folder includes `CHECKSUMS.md`. Packet-level checksums are also regenerated in `business/friday_send/CHECKSUMS.md` and `business/friday_meeting_packet/CHECKSUMS.md`.

## Proof Index

| Artifact | Evidence label | Use | Safe public sentence | Do not claim |
|---|---|---|---|---|
| `PROOF_CARDS.md` | Digest | Human-readable proof-card summary | ATOMiK proof artifacts are evidence-labeled and workload-bound. | That a digest replaces raw artifacts. |
| `VERSION_MAP.md` | Governance | Explains v0.40-A, v0.39-K, v0.33-D, zynq-linux-v1, and SD boot status | ATOMiK separates UI artifacts, board-run matrices, frozen Linux validation, and build artifacts by version. | That all versions prove the same thing. |
| `10-current-live-atomik-desk-v040a.png` | HARDWARE_VALIDATED | Current Zynq demo surface screenshot | ATOMiK Desk v0.40-A is the current Zynq hardware UI artifact, captured live from the framebuffer. | Customer workload performance, uptime, production maturity, battery, heat, water, power, or footprint outcomes. |
| `10-current-live-atomik-desk-v040a.caption.md` | Caption / guardrail | Safe screenshot caption | ATOMiK Desk v0.40-A shows live prototype/demo surface and UI direction with real measured on-board metrics. | That screenshot metrics are customer or production metrics. |
| `PARALLEL_BANKS_HARDWARE_VALIDATED.md` (repo) | LIVE_MEASURED | AX7020 parallel-bank throughput | On the AX7020, an 8-bank ATOMiK accumulator scaled 1/2/4/8x with allocated banks, byte-identical to software. | A customer-workload speedup, power, thermal, or production outcome. |
| `12-workloads-live-ax7020.png` | LIVE_MEASURED | Live Workloads surface on AX7020 | The Workloads surface runs on AX7020 HDMI showing live measured parallel-bank throughput (800 Mevents/s at 8 banks, byte-identical across configs). | An interactive session, USB input, or any on-screen value beyond the throughput figures. |
| `12-workloads-live-ax7020.caption.md` | Caption / guardrail | Safe Workloads-surface caption | First customer-facing workload demo proven on hardware; throughput is measured. | Interactive demo, USB input, or non-throughput on-screen values. |
| `13-workloads-telemetry-verified-ax7020.png` | LIVE_MEASURED | Verified telemetry-sync savings on AX7020 | Redesigned conventional-vs-ATOMiK surface: 2,048 B vs 160 B per tick (92% less data moved), every delta adapter-verified with a falsification-tested harness. | A universal multiplier (ratio is pattern/density-dependent), an interactive session, USB input, or non-comparison on-screen values. |
| `13-workloads-telemetry-verified-ax7020.caption.md` | Caption / guardrail | Safe verified-telemetry caption | Measured + adapter-verified savings, tightly scoped. | Universal multipliers, interactivity, USB input. |
| `14-workloads-coalescing-selfdriving-ax7020.png` | LIVE_MEASURED | Verified coalescing savings + self-driving demo | 256 updates coalesce to 32 net writes (87% fewer), adapter-verified; frame captured while the untethered board auto-cycled scenarios (AUTO). | Interactive input (self-driving = auto-cycling), universal ratios, power/thermal/customer outcomes. |
| `14-workloads-coalescing-selfdriving-ax7020.caption.md` | Caption / guardrail | Safe coalescing/self-driving caption | Verified write reduction + honest self-driving framing. | Interactivity or USB input claims. |
| `LINUX_USERSPACE_PROOF.md` | HARDWARE_VALIDATED | Linux userspace-to-FPGA path and algebraic property checks | ATOMiK algebraic property checks passed through a Linux userspace-to-FPGA path on the documented Zynq configuration. | Customer workload value, downstream outcomes, production readiness. |
| `LINUX_USERSPACE_PROOF_SUMMARY.md` | HARDWARE_VALIDATED summary | One-page proof summary | The documented Linux userspace path reached the ATOMiK FPGA core and passed 16/16 checks. | Battery, thermal, cooling, water, footprint, or power-bill outcomes. |
| `ZYNQ_BASELINE.md` | HARDWARE_VALIDATED / baseline context | Frozen Zynq Linux/userspace validation baseline | zynq-linux-v1 is the frozen Linux userspace-to-FPGA validation baseline. | That it is the v0.39-K UI, v0.33-D matrix, or future SD boot workload proof. |
| `perf_matrix_ax7020_20260509.txt` | LIVE_MEASURED | AX7020 raw board-run matrix | The AX7020 matrix shows workload-specific wins and losses across software, direct, batched, and profiled paths. | Universal speedup or a customer workload result. |
| `perf_matrix_ax7020_20260509.csv` | LIVE_MEASURED export | Machine-readable matrix export | The CSV exports the raw AX7020 matrix for review. | That derived exports replace raw artifacts. |
| `perf_matrix_ax7020_20260509.summary.json` | LIVE_MEASURED summary | Machine-readable context and caveats | The JSON records board/version/context and known limitations. | That summary context is a new measurement. |
| `20260509_matrix_interpretation.md` | LIVE_MEASURED interpretation | Honest AX7020 interpretation | ATOMiK wins when batching/coalescing/personality rules apply and can lose when used naively or on non-fit workloads. | Cherry-picked best number without workload and caveat. |
| `AX7020_SUMMARY.md` | LIVE_MEASURED summary | Compact matrix readout | The small STATE row improved when profiled/coalesced, while larger or non-fit rows show losses or caveats. | That AX7020 proves broad commercial outcomes. |
| `claims_registry_snapshot.yaml` | Claims registry | Public-safe claim map snapshot | The claims registry maps each claim to label, artifact, and caveat. | That registry entries are proof without the artifacts. |
| `evidence-labels.md` | Evidence framework | Defines labels and claim rules | ATOMiK uses labels to separate measured, validated, projected, conceptual, and roadmap claims. | Mixing projected or conceptual claims with measured proof. |
| `README_EXTERNAL_REFERENCES.md` | Reference index | Lists repo artifacts referenced but not packaged here | Some registry items point to external repo artifacts that require source access. | That unbundled artifacts are present in this folder. |
| `CHECKSUMS.md` | Integrity | SHA-256 hashes for proof artifacts | Proof artifact hashes are recorded for packet hygiene. | That checksums prove technical correctness. |

## Known Limitations To Keep With The Proof

- SYNC rows in the AX7020 matrix show zero bytes avoided because the run did not exercise cross-batch repeat/replay behavior.
- AGENT can lose on small workloads because relevance sorting overhead can exceed skip savings.
- The first direct hardware row includes likely lazy `/dev/mem` mmap overhead; first samples should be treated carefully.
- Screenshot telemetry must not be interpreted as customer workload performance, production uptime, cost, battery, thermal, cooling, water, power, or footprint proof.
- SD boot remains build-artifact status until reproducible power-on boot logs and workload artifacts exist.

## Send Policy

Safe for light investor follow-up:

- `PROOF_CARDS.md`
- `VERSION_MAP.md`
- `10-current-live-atomik-desk-v040a.png` with `10-current-live-atomik-desk-v040a.caption.md`
- `evidence-labels.md`
- `AX7020_SUMMARY.md`

Safe for technical diligence:

- `LINUX_USERSPACE_PROOF.md`
- `LINUX_USERSPACE_PROOF_SUMMARY.md`
- `ZYNQ_BASELINE.md`
- `perf_matrix_ax7020_20260509.txt`
- `perf_matrix_ax7020_20260509.csv`
- `perf_matrix_ax7020_20260509.summary.json`
- `20260509_matrix_interpretation.md`
- `claims_registry_snapshot.yaml`

Do not send without context:

- Raw matrix alone.
- Claims registry alone.
- Screenshot without caption.
- Concept visuals without labels.
- Any artifact as proof of battery, heat, cooling, water, power-bill, footprint, customer savings, production readiness, or universal speedup.

Bottom line: this folder supports an evidence-bound proof story. It does not prove broad commercial outcomes or customer-environment results.
