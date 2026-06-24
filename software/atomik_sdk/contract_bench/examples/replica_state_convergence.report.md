# ATOMiK Sync Proof — Distributed State (archetype) / Replica state convergence

**VERDICT: CONVERGES** — coordination-free convergence + ~92% less data moved

## Convergence gate (correctness)
- Required: replicas reach identical state with no coordinator / consensus
- ATOMiK: byte-identical across 1/2/4/8 banks (0x17e9fe29b5dc0aad)
- PASS — any node, any order, after any partition -> identical merged state (the same byte-identical property proven on silicon)

## Data-moved gate (bandwidth)
- Required: 2048 B/tick full-state sync
- ATOMiK: 160 B/tick delta (measured)
- PASS — ~92% less at ~5% of state changed per tick — scales with change density, not a universal multiplier

## Provenance
- convergence_measured_on: 2026-06-21
- board: ALINX/HamGeek AX7020 (XC7Z020-2CLG484)
- reproduce_order_independence: abench 65537 0xDEADBEEF0BADF00D
- data_moved_note: Measured on the AX7020 ATOMiK adapter: an edge-telemetry sync tick moved 160 B vs 2,048 B of full state, with exact 64-bit LOAD/ACCUM/READ round-trips on a falsification-tested harness.

> Scope: The data-moved ratio is for ~5% of state changed per tick; it scales with how much of the state changes per tick — lower change density means a larger reduction, higher means smaller. It is not a universal multiplier.
