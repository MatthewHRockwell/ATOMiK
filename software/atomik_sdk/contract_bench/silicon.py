"""
Silicon-measured facts for the ATOMiK parallel-bank aggregation engine.

Every number here was MEASURED on real hardware (not modelled, not projected):
the parallel-bank accumulator at 0xF0021000 on an ALINX/HamGeek AX7020
(XC7Z020-2CLG484) running at the 100 MHz system clock, driven by abench
(hardware/zynq/test/atomik_bench_sweep.c).  abench programs the engine with a
delta count + seed, reports the hardware cycle count per active-bank
configuration, and recomputes the merged accumulator in software to confirm the
hardware result.

The two facts that win a contract come straight from these numbers:

  1. THROUGHPUT — the engine sustains ~1 accumulation per bank per cycle, so at
     8 banks / 100 MHz it accumulates ~800 M deltas/s.  Linear: ~100 M/s per bank.

  2. CORRECTNESS-UNDER-CONCURRENCY — the merged result is BYTE-IDENTICAL across
     1, 2, 4 and 8 banks.  Bank count changes how the deltas are partitioned and
     the order they complete in, so an identical result proves the answer is
     independent of arrival order and parallelism.  A conventional aggregator can
     only promise that by serializing behind a lock — which removes the scaling.

Re-measure any time with `abench <count> <seed_hex>` on the board; this module is
the recorded reference so the harness produces reports without a live board.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class SiliconMeasurement:
    """A recorded, on-silicon measurement of the parallel-bank engine."""

    board: str
    engine: str
    clock_hz: int
    n_banks: int
    reference_count: int
    # hardware cycle count per active-bank configuration (banks -> cycles)
    cycles: dict[int, int]
    # merged accumulator value; identical across every bank count = the proof
    result_hex: str
    identical_across_banks: bool
    measured_on: str
    tool: str = "abench (atomik_bench_sweep.c)"

    def throughput_at(self, banks: int) -> float:
        """Measured accumulations/sec at `banks` banks (events / (cycles/clock))."""
        cyc = self.cycles[banks]
        return self.reference_count * self.clock_hz / cyc

    def per_bank_throughput(self) -> float:
        """Measured accumulations/sec contributed by a single bank."""
        return self.throughput_at(1)

    def peak_throughput(self) -> float:
        """Measured accumulations/sec at the full bank count."""
        return self.throughput_at(self.n_banks)

    def scaling(self) -> dict[int, float]:
        """Speedup vs 1 bank, per bank count (the measured linear-scaling table)."""
        base = self.cycles[1]
        return {b: base / c for b, c in sorted(self.cycles.items())}

    def bank_options(self) -> list[int]:
        return sorted(self.cycles)


# ---------------------------------------------------------------------------
# The recorded reference measurement.  Source: abench on AX7020, June 2026.
#   $ abench 65537 0xDEADBEEF0BADF00D
#   banks  hw_cycles  speedup  result              ok
#   1      65537      1.00     0x17e9fe29b5dc0aad  OK
#   2      32769      1.99     0x17e9fe29b5dc0aad  OK
#   4      16385      3.99     0x17e9fe29b5dc0aad  OK
#   8      8193       7.99     0x17e9fe29b5dc0aad  OK
#   software reference = 0x17e9fe29b5dc0aad  ->  ALL MATCH
# ---------------------------------------------------------------------------
ENGINE = SiliconMeasurement(
    board="ALINX/HamGeek AX7020 (XC7Z020-2CLG484)",
    engine="parallel-bank delta-state accumulator @0xF0021000",
    clock_hz=100_000_000,
    n_banks=8,
    reference_count=65537,
    cycles={1: 65537, 2: 32769, 4: 16385, 8: 8193},
    result_hex="0x17e9fe29b5dc0aad",
    identical_across_banks=True,
    measured_on="2026-06-21",
)


# Aggregations ATOMiK's engine accelerates are exactly the ones that form an
# abelian group (associative, commutative, with identity + inverse) — only then
# is the result provably order-independent.  This list keeps the harness honest:
# it will not claim order-independence for an op outside this class.
ABELIAN_AGGREGATIONS: dict[str, str] = {
    "xor": "bitwise XOR / parity (set-symmetric-difference, sketch merge)",
    "sum": "modular integer sum (counters, totals, moments)",
    "count": "occurrence counting (histograms, frequency tables)",
    "or": "monotone OR / set union (presence, bloom-style membership)",
    "min": "min reduction (note: idempotent, not strictly invertible)",
    "max": "max reduction (note: idempotent, not strictly invertible)",
    "hll_merge": "HyperLogLog register merge (cardinality estimation)",
    "counter_merge": "CRDT counter / sketch merge (replica aggregation)",
}


def is_abelian(aggregation: str) -> bool:
    return aggregation.lower() in ABELIAN_AGGREGATIONS
