"""Scenario 1: Sensor fusion — N-stream merge.

Compares ATOMiK lock-free XOR merge vs mutex-protected full-state copy
for merging N independent sensor streams into a single fused state.
"""

from __future__ import annotations

import os
import random
import time
from dataclasses import dataclass

from ..atomik_delta import AtomikAccumulator
from ..conventional import FullStateCopy


@dataclass
class SensorFusionResult:
    name: str = "Sensor Fusion (N=16 streams)"
    atomik_ops_sec: float = 0.0
    conventional_ops_sec: float = 0.0
    atomik_bytes: int = 0
    conventional_bytes: int = 0
    speedup: float = 0.0
    n_streams: int = 16
    n_updates: int = 100_000


class SensorFusionScenario:
    """N-stream sensor fusion: lock-free XOR merge vs mutex full-copy."""

    name = "Sensor Fusion"
    description = (
        "Merge N independent sensor streams into one fused state. "
        "ATOMiK uses lock-free XOR accumulation; conventional uses "
        "mutex-protected full-state copy."
    )

    @staticmethod
    def run(n_streams: int = 16, n_updates: int = 100_000) -> SensorFusionResult:
        result = SensorFusionResult(n_streams=n_streams, n_updates=n_updates)
        rng = random.Random(42)
        deltas = [rng.getrandbits(64) for _ in range(n_updates)]

        # --- ATOMiK: N accumulators, merge via XOR ---
        accumulators = [AtomikAccumulator() for _ in range(n_streams)]
        for acc in accumulators:
            acc.load(0)

        t0 = time.perf_counter()
        for i, delta in enumerate(deltas):
            accumulators[i % n_streams].accumulate(delta)
        # Merge all accumulators
        merged = AtomikAccumulator()
        for acc in accumulators:
            merged.merge(acc)
        _ = merged.read()
        atomik_time = time.perf_counter() - t0

        # Bytes: each delta is 8 bytes, no state copies needed
        result.atomik_bytes = n_updates * 8
        result.atomik_ops_sec = n_updates / atomik_time

        # --- Conventional: mutex full-state copy ---
        stores = [FullStateCopy() for _ in range(n_streams)]

        t0 = time.perf_counter()
        for i, delta in enumerate(deltas):
            stores[i % n_streams].update(delta)
        # Merge by reading all and combining (last-writer-wins typical)
        for store in stores:
            _ = store.read()
        conventional_time = time.perf_counter() - t0

        # Bytes: each update copies full 8-byte state + lock overhead
        result.conventional_bytes = n_updates * 8 * 2  # read + write
        result.conventional_ops_sec = n_updates / conventional_time

        result.speedup = result.atomik_ops_sec / max(
            result.conventional_ops_sec, 1
        )
        return result
