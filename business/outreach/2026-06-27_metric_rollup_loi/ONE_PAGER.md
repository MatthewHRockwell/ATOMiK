# ATOMiK — Lock-Free, Byte-Identical Aggregation (One-Pager)

**For:** teams whose counters / rollups / sketches must stay exact under
concurrent, out-of-order load — and who pay for that exactness with a lock, a
sharded hot path, or a serialized reducer.

---

### The problem
Keeping per-series / per-key aggregates exact under concurrency forces
serialization: a lock, a hot-shard, or a single reducer. That cap bites exactly at
peak load, and it's the price of correctness — not a bug you can tune away.

### What ATOMiK does
ATOMiK is a delta-state engine that merges aggregates **order-independently and
lock-free**. For any **abelian** aggregation — sum, count, xor, or, min, max,
HyperLogLog / sketch merge — the merged result is **byte-identical regardless of
arrival order or parallelism**. The lock exists to guarantee exactly that
property, so it's no longer needed.

### Why you can believe it
- **Measured on silicon:** 800 M accumulations/s at 8 banks, byte-identical across
  1 / 2 / 4 / 8 banks, on an AX7020 (XC7Z020) at 100 MHz.
  Reproduce: `abench 65537 0xDEADBEEF0BADF00D`.
- **Formally proven:** the order-independence is machine-checked in Lean 4 — 108
  theorems, zero `sorry`, depending only on Lean's standard axioms.
  → atomik.tech/formal-verification
- **Two independent lines of evidence** for one property: proven in the math,
  observed in the hardware.

### What it's worth (computed from *your* baseline)
The lock / shard fan-out / reducer you run for correctness is the tax
order-independence removes. We quantify it from your own numbers — every figure
tagged measured (ours) vs. your-baseline (yours). → atomik.tech/cost-of-lock

### The offer — you commit only if it passes
1. **Scope** your operator, rate, and correctness bar → a silicon-measured verdict.
2. **Conditional LOI** — we draft the bar; you counter-sign, binding only if a live
   run meets it. Non-binding until executed.
3. **Live silicon run** — we run *your* workload on the AX7020 and hand you the
   dated transcript: sustained throughput + byte-identical result across banks.
4. **Pilot** on a pass. Honest **no-fit** if your operator isn't order-independent.
   → atomik.tech/design-partners

### Scope & honesty
Throughput is the **engine's accumulation/merge step**, not end-to-end system
throughput (that depends on your ingest path). Order-independence is claimed
**only for abelian operators** — medians, percentiles, and last-write-wins are
order-dependent, and we say so. No universal speedup claim. No named customers.

---

**Matthew Rockwell · ATOMiK · atomik.tech · matthew.h.rockwell@gmail.com**
*Browse your workload: atomik.tech/workloads · Test your bar: atomik.tech/benchmark-proof*
