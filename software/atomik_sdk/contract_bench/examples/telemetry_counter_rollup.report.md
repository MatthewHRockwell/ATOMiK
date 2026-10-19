# ATOMiK Benchmark Proof — Example Telemetry Co. / Fleet telemetry counter rollup

**VERDICT: SURPASS** — meets the required rate with x16.0 headroom AND a byte-identical, lock-free correctness guarantee

## Throughput gate
- Required: **50 M/s (events/s)**
- ATOMiK (measured on silicon): **800 M/s at 8 banks (linear: 100 M/s per bank)**
- PASS — x16.0 headroom

## Correctness gate (the part a lock would cost you)
- Required: byte-identical regardless of arrival order / parallelism
- ATOMiK: byte-identical across 1/2/4/8 banks (result 0x17e9fe29b5dc0aad)
- PASS — 'sum' (modular integer sum (counters, totals, moments)) is an abelian aggregation, so the merge is provably order-independent — lock-free, no serialization

## Measured linear scaling (silicon)
- Speedup vs 1 bank: 1->1.00x  2->1.99x  4->3.99x  8->7.99x
- To meet the bar you need **1 bank(s)**; the chip provides **8**.

## Provenance
- measured_on: 2026-06-21
- board: ALINX/HamGeek AX7020 (XC7Z020-2CLG484)
- engine: parallel-bank delta-state accumulator @0xF0021000
- clock: 100 MHz
- tool: abench (atomik_bench_sweep.c)
- reproduce: abench 65537 0xDEADBEEF0BADF00D

> Scope: Throughput is the engine's measured accumulation rate (the aggregation step) on silicon. End-to-end system throughput additionally depends on the data-ingest path; ATOMiK's order-independence means ingest needs no global ordering or locks, which is exactly what caps a conventional aggregator.
