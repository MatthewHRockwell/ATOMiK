# ATOMiK — Cost-of-Lock Value Memo (DRAFT)

- **Prepared for:** ________________________
- **Workload:** Per-entity counters
- **Operator / rate:** sum at 100 M/s
- **Today:** shard fan-out maintained to avoid contention

**VALUE** — ~$3,780/mo of shard fan-out maintained to avoid contention is what order-independence removes

## What the numbers say
- [MEASURED] **ATOMiK sustains your rate:** 100 M/s on 1 of 8 banks (8.0x headroom), lock-free, byte-identical
- [YOUR BASELINE] **Provisioned baseline:** $10,800/mo (12 x $900)
- [YOUR BASELINE] **Coordination tax (removed):** ~$3,780/mo — 35% of baseline you attribute to shard fan-out maintained to avoid contention
- [YOUR BASELINE] **Tail latency at peak:** The shard fan-out maintained to avoid contention component of your 40 ms p99 at peak is removed — ATOMiK's latency is deterministic (no lock or cache-coherency stall).

## The argument
For an abelian aggregation like `sum`, the merged result is byte-identical regardless of arrival order or parallelism (measured on the ALINX/HamGeek AX7020 (XC7Z020-2CLG484), result 0x17e9fe29b5dc0aad across 1/2/4/8 banks). That is exactly the property a shard fan-out maintained to avoid contention exists to guarantee — so it is no longer required. The tax above, from your stated baseline, is what that removes.

## Scope & honesty
- ATOMiK figures are MEASURED on silicon (the aggregation/merge step); savings are arithmetic on YOUR stated baseline.
- What is removed is the coordination tax for correctness under concurrency — an architectural property of abelian merges, not a throughput multiplier.
- End-to-end impact additionally depends on your ingest path; this is not a universal saving.

> Reproduce the silicon facts: `abench 65537 0xDEADBEEF0BADF00D`.
