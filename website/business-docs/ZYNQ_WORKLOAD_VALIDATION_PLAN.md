# Zynq Workload Validation Plan

## Required current status language
We are working on booting from SD card instead of relying on JTAG so workload updates can be made and rerun faster. After SD boot is stable, the first validation workloads will be integrated and measured.

Do not promote SD boot or workload results to hardware-validated claims until boot logs, build details, Linux/userspace hardware-path reachability, correctness-passing workload artifacts, and reproducible raw results exist.

## Evidence gates
- Tier 1: SD boot validation. Board boots from SD and produces a reproducible validation note.
- Tier 2: Linux userspace hardware-path validation. Userspace can reach the current ATOMiK hardware interface after SD boot.
- Tier 3: Workload validation. Baseline and ATOMiK paths run identical traces, correctness passes, and artifacts are written.
- Tier 4: Public proof card. Claim registry entry exists and the proof page can link to artifacts.

## Required SD boot validation note
Use `website/business-docs/SD_BOOT_VALIDATION_NOTE_TEMPLATE.md` until a real hardware note exists. The note must include board model, BOOT.BIN source, bitstream, Linux image, root filesystem, kernel version, device tree, ATOMiK hardware core version, git commit, JTAG fallback status, reproduction steps, logs, and caveats.

## Board model
Record the exact board model for each proof card. Existing Friday artifacts reference AX7020/Zynq context, but new workload proof must include the actual board used in the run.

## Build/bitstream info
Each run must capture BOOT.BIN source, bitstream, Linux image, root filesystem, kernel version, device tree, ATOMiK hardware core version, git commit, and whether JTAG remains available as fallback.

## P0 workloads
1. Dirty-state telemetry sync
2. Repeated register/control update coalescing

## P1 workloads
3. Checkpoint/reconstruction/replay cost
4. Bandwidth-constrained state transfer simulation

## P2 optional workload
5. Edge-context/local AI state update, only as a representative context-state update workload. Do not present this as full AI inference, faster LLM inference, model-quality improvement, or AI accelerator replacement.

## Required result fields
Every workload result must include:

- board model
- build/bitstream version
- workload name
- baseline path
- ATOMiK path
- trace seed
- number of runs
- correctness pass/fail
- bytes moved
- bytes avoided
- operations received
- operations emitted
- operations coalesced
- latency p50
- latency p95
- caveats
- artifact links

Recommended additional fields: state size, region count, update count, changed region count, unique-region ratio, baseline output hash, ATOMiK output hash, min/max latency, full-state transfers avoided, build commit, and measurement method.

## Correctness checks
No performance result is publishable unless correctness passes. Compare final state, expected invariant, output hash, and workload-specific accepted behavior.

## Results artifact structure
Use `website/business-docs/ZYNQ_RESULTS_ARTIFACT_STRUCTURE.md` as the source structure. Target path: `results/zynq_validation/YYYYMMDD/` with `board_info.json`, `build_info.json`, raw run CSVs, summaries, correctness logs, workload README files, and `proof_cards.md`.

## Proof-card template
Use `website/business-docs/ZYNQ_PROOF_CARD_TEMPLATE.md`. Proof cards must include workload, buyer pain, hardware, baseline, ATOMiK path, trace, measurement method, correctness result, measured result, evidence label, caveat, artifact links, public-safe claim, and claims not supported.

## Claims registry update plan
Use `website/business-docs/ZYNQ_CLAIMS_REGISTRY_UPDATE_PLAN.md`. Add or update registry entries only after correctness-passing artifacts exist. Do not add a homepage number before the proof page has the artifact-bound claim.

## Website proof-card plan
Update the proof page first with a `Zynq workload validation` section only after artifacts exist. Add conservative homepage language only after artifact-backed results exist.

## Claims that remain unsupported
Battery, heat, cooling, water, power-bill, smaller hardware, production readiness, customer savings, generalized AI inference improvement, and universal speedup remain unsupported until measured in the relevant environment.
