# Zynq Results Artifact Structure

Status: planned structure. Do not cite result paths as proof until files exist and correctness passes.

```text
results/zynq_validation/YYYYMMDD/
  board_info.json
  build_info.json
  sd_boot_validation.log
  dirty_state_sync/
    raw_runs.csv
    summary.json
    correctness.log
    README.md
  register_coalescing/
    raw_runs.csv
    summary.json
    correctness.log
    README.md
  checkpoint_reconstruction/
    raw_runs.csv
    summary.json
    correctness.log
    README.md
  bandwidth_state_transfer/
    raw_runs.csv
    summary.json
    correctness.log
    README.md
  edge_context_state_update/
    raw_runs.csv
    summary.json
    correctness.log
    README.md
  proof_cards.md
```

## Required summary fields
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

## Raw data requirements
- Include all runs, including losing or failed runs.
- Separate end-to-end timing from component-level timing if both are measured.
- Include host/device transfer overhead unless clearly separated.
- Record measurement method and clock/counter source.
- Do not publish performance numbers if correctness fails.
