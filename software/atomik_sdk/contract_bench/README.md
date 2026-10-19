# ATOMiK Benchmark Proof (`contract_bench`)

Turn a customer's aggregation workload + their commit bar into a **PASS/SURPASS
proof** backed by real silicon — the artifact you put in front of a prospect.

## What it proves (and why it wins)

A contract for an aggregation/reduction workload doesn't turn on "are you
faster." Our perf matrix is honest that ATOMiK has overhead. It turns on
**throughput-at-correctness**:

- **Throughput** — the parallel-bank engine accumulates ~1 delta/bank/cycle.
  Measured on an AX7020 (XC7Z020) at 100 MHz: **800 M events/s at 8 banks**,
  linear (100 M/s per bank).
- **Correctness under concurrency** — the merged result is **byte-identical
  across 1/2/4/8 banks**. Bank count changes how deltas are partitioned and the
  order they finish in, so an identical result *proves* the answer is independent
  of arrival order and parallelism. A conventional aggregator can only promise
  that by serializing behind a lock — which removes the scaling.

So the pitch is: **ATOMiK meets your rate *with* a byte-identical, lock-free
correctness guarantee a locked baseline can't match without giving up its
throughput.**

## Honesty rules (enforced in code, not slides)

- Throughput is the engine's **measured** accumulation rate (provenance-tagged,
  reproducible via `abench 65537 0xDEADBEEF0BADF00D`) — never a projection.
- Order-independence is claimed **only** for abelian aggregations
  (`sum/xor/count/or/min/max/hll_merge/counter_merge`). Anything else FAILS the
  correctness gate — `contract_bench` will not oversell.
- The verdict needs **both** gates. `SURPASS` is reserved for ≥2× headroom.
- The throughput figure is the **aggregation step** (the engine). End-to-end
  system throughput also depends on the ingest path — but ATOMiK's
  order-independence is exactly what removes the global ordering/locks that cap a
  conventional aggregator. The report states this scope explicitly.

## Use

```bash
# from a spec file (Markdown report)
python -m atomik_sdk.contract_bench.cli examples/telemetry_counter_rollup.json

# JSON for atomik.tech
python -m atomik_sdk.contract_bench.cli examples/telemetry_counter_rollup.json --json

# quick one-off
python -m atomik_sdk.contract_bench.cli --name "Rollup" --agg sum --required 50e6

# LIVE on a connected board — runs abench at the workload's scale on real silicon
python -m atomik_sdk.contract_bench.cli examples/telemetry_counter_rollup.json \
    --board --count 50000000
```

```python
from atomik_sdk.contract_bench import WorkloadSpec, evaluate
report = evaluate(WorkloadSpec("Counter rollup", aggregation="sum",
                               required_throughput=50e6, customer="Acme"))
print(report.verdict)        # SURPASS
print(report.to_markdown())  # hand to the prospect
report.to_json()             # feed atomik.tech
```

## Files

- `silicon.py` — the recorded, provenance-tagged silicon measurement + the
  abelian-aggregation allowlist (the honesty gate).
- `spec.py` — `WorkloadSpec`: a customer workload as they'd state it in procurement.
- `harness.py` — `evaluate(spec) -> BenchmarkReport` (the two gates + verdict).
- `report.py` — the report object + Markdown/JSON renderers.
- `board.py` — `measure_on_board(count)`: live on-silicon measurement at scale.
- `cli.py` — command line.
- `examples/` — worked customer specs + their generated reports.
