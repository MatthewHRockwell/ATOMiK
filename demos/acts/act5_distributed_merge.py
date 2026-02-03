"""Act 5: Distributed Merge — 3-node XOR merge, commutativity proof.

Grand Finale: three FPGAs each process a subset of deltas. The host
XOR-merges the results, proving lock-free distributed computing via
commutativity.

Math:
    Node1 accumulates {d1, d2}:  acc1 = d1 XOR d2
    Node2 accumulates {d3, d4}:  acc2 = d3 XOR d4
    Node3 accumulates {d5, d6}:  acc3 = d5 XOR d6
    Host merge: final = S0 XOR acc1 XOR acc2 XOR acc3
    Verify: single-node with all 6 deltas produces identical result.
"""

from __future__ import annotations

import random

from demos.acts.base import ActBase, ActResult
from demos.node import NodeController

_RNG = random.Random(0xD157)


class Act5DistributedMerge(ActBase):
    number = 5
    title = "Distributed Merge"
    narration = (
        "Grand finale: three nodes each process a different subset of deltas. "
        "The host XOR-merges the partial accumulators — and the result is "
        "identical to processing all deltas on a single node. This proves "
        "lock-free distributed computing via commutativity."
    )

    def run(self, nodes: list[NodeController]) -> ActResult:
        details: list[str] = []
        all_passed = True

        # Shared initial state
        S0 = 0xFEDCBA9876543210

        # Generate 6 random deltas, split into 3 pairs
        all_deltas = [_RNG.getrandbits(64) for _ in range(6)]
        partitions = [
            all_deltas[0:2],  # Node 1
            all_deltas[2:4],  # Node 2
            all_deltas[4:6],  # Node 3
        ]

        details.append(f"Initial state S0 = 0x{S0:016X}")
        details.append("6 deltas partitioned across 3 nodes:")
        for i, part in enumerate(partitions):
            details.append(f"  {nodes[i].name}: {[f'0x{d:016X}' for d in part]}")
        details.append("")

        # ── Phase 1: Each node accumulates its partition ──────────────
        details.append("Phase 1: Parallel accumulation")
        accumulators: list[int] = []

        for node, partition in zip(nodes, partitions):
            node.load(0)  # accumulate deltas only (no initial state in acc)
            node.accumulate_batch(partition)
            acc = node.read()  # read() returns initial XOR accumulator = 0 XOR acc = acc
            accumulators.append(acc)
            details.append(f"  {node.name} accumulator = 0x{acc:016X}")

        # ── Phase 2: Host XOR-merge ───────────────────────────────────
        details.append("")
        details.append("Phase 2: Host XOR-merge")

        merged = S0
        for acc in accumulators:
            merged ^= acc
        details.append(f"  S0 XOR acc1 XOR acc2 XOR acc3 = 0x{merged:016X}")

        # ── Phase 3: Single-node verification ─────────────────────────
        details.append("")
        details.append("Phase 3: Single-node verification")

        verify_node = nodes[0]
        verify_node.load(S0)
        for d in all_deltas:
            verify_node.accumulate(d)
        single_result = verify_node.read()
        details.append(f"  Single node with all 6 deltas = 0x{single_result:016X}")

        ok = merged == single_result
        details.append("")
        details.append(f"Distributed merge {'==' if ok else '!='} single-node: {'PASS' if ok else 'FAIL'}")
        all_passed &= ok

        # ── Phase 4: Order-independence (commutativity) ───────────────
        details.append("")
        details.append("Phase 4: Order-independence (commutativity)")

        # Reverse the partition assignment and re-merge
        accumulators_rev: list[int] = []
        for node, partition in zip(nodes, reversed(partitions)):
            node.load(0)
            node.accumulate_batch(partition)
            acc = node.read()
            accumulators_rev.append(acc)

        merged_rev = S0
        for acc in accumulators_rev:
            merged_rev ^= acc

        ok_commutative = merged_rev == single_result
        details.append(f"  Reversed partition merge = 0x{merged_rev:016X}")
        details.append(f"  Commutativity holds: {'PASS' if ok_commutative else 'FAIL'}")
        all_passed &= ok_commutative

        return self._result(
            passed=all_passed,
            summary=(
                "Distributed merge verified — lock-free computation proven"
                if all_passed
                else "Distributed merge failed"
            ),
            details=details,
        )
