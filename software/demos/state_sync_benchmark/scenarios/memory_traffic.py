"""Scenario 4: Memory traffic — bytes moved comparison.

Compares total bytes written for ATOMiK sparse deltas vs conventional
dense full-state updates across N state updates.
"""

from __future__ import annotations

import random
import time
from dataclasses import dataclass

from ..atomik_delta import AtomikAccumulator, AtomikParallelBank
from ..conventional import FullStateCopy


@dataclass
class MemoryTrafficResult:
    name: str = "Memory Traffic"
    atomik_bytes_written: int = 0
    conventional_bytes_written: int = 0
    atomik_parallel_bytes: int = 0
    reduction_pct: float = 0.0
    n_updates: int = 100_000
    state_width: int = 64
    sparsity: float = 0.95


class MemoryTrafficScenario:
    """Total bytes written: sparse deltas vs dense state copies."""

    name = "Memory Traffic"
    description = (
        "Measure total bytes written across N state updates. "
        "ATOMiK writes only non-zero delta bytes (sparse). "
        "Conventional writes full state on every update."
    )

    @staticmethod
    def run(
        n_updates: int = 100_000,
        state_width: int = 64,
        sparsity: float = 0.95,
    ) -> MemoryTrafficResult:
        result = MemoryTrafficResult(
            n_updates=n_updates,
            state_width=state_width,
            sparsity=sparsity,
        )
        rng = random.Random(42)
        width_bytes = state_width // 8

        # Generate sparse deltas (most bits zero)
        deltas = []
        for _ in range(n_updates):
            if rng.random() < sparsity:
                # Sparse: only 1-2 non-zero bytes
                d = rng.getrandbits(8) << (rng.randint(0, width_bytes - 1) * 8)
            else:
                d = rng.getrandbits(state_width)
            deltas.append(d)

        # --- ATOMiK: only the delta bytes matter ---
        acc = AtomikAccumulator(width=state_width)
        acc.load(rng.getrandbits(state_width))

        atomik_bytes = 0
        for delta in deltas:
            acc.accumulate(delta)
            # Count non-zero bytes in delta
            d = delta
            nz_bytes = 0
            while d:
                if d & 0xFF:
                    nz_bytes += 1
                d >>= 8
            atomik_bytes += max(nz_bytes, 1)  # At least 1 byte per op

        result.atomik_bytes_written = atomik_bytes

        # --- ATOMiK parallel (16 banks) ---
        bank = AtomikParallelBank(n_banks=16, width=state_width)
        bank.load(rng.getrandbits(state_width))
        for delta in deltas:
            bank.accumulate(delta)
        # Same delta bytes, distributed across banks
        result.atomik_parallel_bytes = atomik_bytes

        # --- Conventional: full state copy every update ---
        store = FullStateCopy(width=state_width)
        store.load(rng.getrandbits(state_width))
        conventional_bytes = 0
        state = store.read()
        for delta in deltas:
            state ^= delta  # Simulate applying delta to get new state
            store.update(state)
            conventional_bytes += width_bytes  # Full state written every time

        result.conventional_bytes_written = conventional_bytes
        result.reduction_pct = (
            (1 - result.atomik_bytes_written / max(conventional_bytes, 1))
            * 100
        )

        return result
