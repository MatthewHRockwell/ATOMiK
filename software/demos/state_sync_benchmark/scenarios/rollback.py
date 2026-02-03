"""Scenario 2: Rollback — 10K deltas + undo.

Compares ATOMiK self-inverse (1 op) vs event-sourcing checkpoint replay
for rolling back state after 10K accumulated deltas.
"""

from __future__ import annotations

import random
import time
from dataclasses import dataclass

from ..atomik_delta import AtomikAccumulator
from ..conventional import EventSourcingStore


@dataclass
class RollbackResult:
    name: str = "Rollback (10K deltas)"
    atomik_undo_ops: int = 0
    conventional_undo_ops: int = 0
    atomik_latency_us: float = 0.0
    conventional_latency_us: float = 0.0
    n_deltas: int = 10_000
    rollback_steps: int = 1


class RollbackScenario:
    """Rollback after N deltas: self-inverse vs checkpoint replay."""

    name = "Rollback"
    description = (
        "Accumulate 10K deltas then undo the last one. "
        "ATOMiK uses self-inverse (XOR same delta = undo, 1 op). "
        "Conventional uses event-sourcing with periodic snapshots."
    )

    @staticmethod
    def run(
        n_deltas: int = 10_000, rollback_steps: int = 1
    ) -> RollbackResult:
        result = RollbackResult(
            n_deltas=n_deltas, rollback_steps=rollback_steps
        )
        rng = random.Random(42)
        deltas = [rng.getrandbits(64) for _ in range(n_deltas)]

        # --- ATOMiK: accumulate all, then rollback via self-inverse ---
        acc = AtomikAccumulator()
        acc.load(rng.getrandbits(64))
        for d in deltas:
            acc.accumulate(d)

        t0 = time.perf_counter()
        # Rollback last delta: just XOR it again
        for i in range(rollback_steps):
            acc.rollback(deltas[-(i + 1)])
        atomik_time = time.perf_counter() - t0

        result.atomik_undo_ops = rollback_steps
        result.atomik_latency_us = atomik_time * 1_000_000

        # --- Event sourcing: replay from last snapshot ---
        store = EventSourcingStore()
        for d in deltas:
            store.apply(d)

        t0 = time.perf_counter()
        replay_ops = store.rollback(rollback_steps)
        conventional_time = time.perf_counter() - t0

        result.conventional_undo_ops = replay_ops
        result.conventional_latency_us = conventional_time * 1_000_000

        return result
