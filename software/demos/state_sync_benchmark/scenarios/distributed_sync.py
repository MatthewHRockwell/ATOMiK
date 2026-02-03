"""Scenario 3: Distributed sync — N-node merge.

Compares ATOMiK commutative (order-independent) convergence vs
ordered-replay synchronisation across N distributed nodes.
"""

from __future__ import annotations

import random
import time
from dataclasses import dataclass

from ..atomik_delta import AtomikAccumulator
from ..conventional import OrderedReplaySync


@dataclass
class DistributedSyncResult:
    name: str = "Distributed Sync (N nodes)"
    atomik_messages: int = 0
    conventional_messages: int = 0
    atomik_correct: bool = True
    conventional_correct: bool = True
    atomik_time_us: float = 0.0
    conventional_time_us: float = 0.0
    n_nodes: int = 8
    n_updates: int = 10_000


class DistributedSyncScenario:
    """N-node distributed sync: commutative merge vs ordered replay."""

    name = "Distributed Sync"
    description = (
        "N nodes each generate updates, then synchronise to a single "
        "consistent state. ATOMiK merges are order-independent "
        "(commutative XOR). Conventional requires ordered replay."
    )

    @staticmethod
    def run(
        n_nodes: int = 8, n_updates: int = 10_000
    ) -> DistributedSyncResult:
        result = DistributedSyncResult(
            n_nodes=n_nodes, n_updates=n_updates
        )
        rng = random.Random(42)
        initial = rng.getrandbits(64)

        # Generate per-node deltas
        node_deltas: list[list[int]] = [
            [rng.getrandbits(64) for _ in range(n_updates // n_nodes)]
            for _ in range(n_nodes)
        ]

        # --- ATOMiK: each node accumulates independently, then merge ---
        nodes = [AtomikAccumulator() for _ in range(n_nodes)]
        for node in nodes:
            node.load(initial)

        t0 = time.perf_counter()
        for i, node in enumerate(nodes):
            for delta in node_deltas[i]:
                node.accumulate(delta)

        # Merge all nodes (order doesn't matter — commutativity)
        merged = AtomikAccumulator()
        merged.load(initial)
        for node in nodes:
            merged.merge(node)
        atomik_final = merged.read()
        atomik_time = time.perf_counter() - t0

        # Merge in reverse order to verify order-independence
        merged_reverse = AtomikAccumulator()
        merged_reverse.load(initial)
        for node in reversed(nodes):
            merged_reverse.merge(node)

        result.atomik_correct = atomik_final == merged_reverse.read()
        result.atomik_messages = n_nodes  # One merge message per node
        result.atomik_time_us = atomik_time * 1_000_000

        # --- Conventional: ordered replay requires sequencing ---
        sync = OrderedReplaySync()
        all_updates = []
        seq = 1
        for i in range(n_nodes):
            for delta in node_deltas[i]:
                all_updates.append((seq, delta))
                seq += 1

        # Shuffle to simulate out-of-order delivery
        shuffled = list(all_updates)
        rng.shuffle(shuffled)

        t0 = time.perf_counter()
        total_messages = 0
        for s, val in shuffled:
            processed = sync.apply_ordered(s, val)
            total_messages += 1
        conventional_time = time.perf_counter() - t0

        result.conventional_correct = sync.pending == 0
        result.conventional_messages = total_messages
        result.conventional_time_us = conventional_time * 1_000_000

        return result
