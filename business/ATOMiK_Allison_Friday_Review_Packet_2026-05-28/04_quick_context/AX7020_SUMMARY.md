# AX7020 Matrix Summary

Evidence label: LIVE_MEASURED

Artifacts:

- Raw matrix: `perf_matrix_ax7020_20260509.txt`
- CSV export: `perf_matrix_ax7020_20260509.csv`
- Summary JSON: `perf_matrix_ax7020_20260509.summary.json`
- Interpretation: `20260509_matrix_interpretation.md`

## Compact Readout

| Workload | Result | Meaning | Caveat |
|---|---|---|---|
| 8 regs / 64 ops / STATE | Profiled/coalesced path: 6,406 cycles vs software: 8,800 cycles; 1.37x faster than software | Coalescing plus batching can beat software in the right small state workload | Not universal; naive direct hardware was 48x slower in the same row |
| 32 regs / 256 ops / STATE | Profiled/coalesced path: 38,465 cycles vs software: 25,361 cycles; software wins | ATOMiK does not win uniformly as state/update patterns change | Need workload-fit evaluation |
| 64 regs / 1024 ops / STATE | Profiled/coalesced path: 145,766 cycles vs software: 74,600 cycles; software wins | Coalescing alone did not overcome MMIO cost in this row | Do not quote as a win |
| SYNC rows | Bytes avoided reported as 0 | Current matrix did not exercise cross-batch repeat/replay behavior | Needs replay/repeat validation before SYNC benefit claims |
| AGENT small rows | Slower than batched path | Relevance-sort overhead dominates small workloads | AGENT value appears only when overhead can amortize |
| AGENT 64 regs / 1024 ops | Profiled path: 165,209 cycles vs batched: 172,261 cycles; modest 1.04x improvement vs batched | Sorting/pruning begins to amortize at larger workload size | Still slower than software in this matrix |

## Defensible Interpretation

ATOMiK wins when the architectural advantage is used correctly and the workload fits: batching, coalescing, and personality rules must reduce meaningful work enough to overcome hardware/software interface overhead. Naive hardware access can lose.

## Known Limitations

- SYNC needs repeat/replay mode to exercise inter-batch unchanged-state skipping.
- AGENT overhead hurts small workloads.
- First-run lazy `/dev/mem` mmap overhead likely affects the smallest direct hardware row.
- This is an AX7020 board-run matrix, not customer workload validation.

## Public-Safe Claim

The AX7020 matrix shows workload-specific behavior: ATOMiK can win when batching/coalescing/personality rules reduce redundant work, and it can lose when hardware access overhead or workload shape dominates.
