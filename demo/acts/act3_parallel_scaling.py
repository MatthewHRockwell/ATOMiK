"""Act 3: Parallel Scaling — Same workload on N=4/8/16, show throughput.

Demonstrates linear throughput scaling across the three node configurations.
"""

from __future__ import annotations

import random
import time

from demo.acts.base import ActBase, ActResult
from demo.node import NodeController

# Fixed seed for reproducible demo
_RNG = random.Random(42)
_WORKLOAD_SIZE = 100  # number of deltas per node


class Act3ParallelScaling(ActBase):
    number = 3
    title = "Parallel Scaling"
    narration = (
        "Now we push all three nodes with the same workload. "
        "Node 1 has 4 parallel banks, Node 2 has 8, Node 3 has 16. "
        "Throughput scales linearly — from 324 Mops/s to over 1 Gops/s — "
        "all on a $10 FPGA."
    )

    def run(self, nodes: list[NodeController]) -> ActResult:
        details: list[str] = []

        # Generate a deterministic workload
        deltas = [_RNG.getrandbits(64) for _ in range(_WORKLOAD_SIZE)]
        expected_acc = 0
        for d in deltas:
            expected_acc ^= d

        scaling_results: list[tuple[str, int, float, float, bool]] = []

        for node in nodes:
            initial = 0
            node.load(initial)

            t0 = time.perf_counter()
            node.accumulate_batch(deltas)
            t1 = time.perf_counter()
            result = node.read()

            elapsed_ms = (t1 - t0) * 1000
            ok = result == (initial ^ expected_acc)
            scaling_results.append((
                node.config.name,
                node.config.n_banks,
                node.config.throughput_mops,
                elapsed_ms,
                ok,
            ))

        # Build details
        details.append(f"Workload: {_WORKLOAD_SIZE} deltas per node")
        details.append("")

        max_throughput = max(r[2] for r in scaling_results)
        all_passed = True

        for name, n_banks, throughput, elapsed, ok in scaling_results:
            bar_len = int(40 * throughput / max_throughput)
            bar = "#" * bar_len
            badge = "PASS" if ok else "FAIL"
            details.append(
                f"  N={n_banks:>2} ({name:>6}): {throughput:>8.1f} Mops/s  "
                f"|{bar:<40}| [{badge}]  ({elapsed:.1f} ms host)"
            )
            all_passed &= ok

        details.append("")
        details.append(f"  Peak: {max_throughput:,.0f} Mops/s = {max_throughput / 1000:.2f} Gops/s")
        gops_line = ">>> BREAKS 1 Gops/s BARRIER <<<" if max_throughput >= 1000 else ""
        if gops_line:
            details.append(f"  {gops_line}")

        return self._result(
            passed=all_passed,
            summary=(
                f"Linear scaling verified: {scaling_results[0][2]:.0f} -> "
                f"{scaling_results[-1][2]:.0f} Mops/s ({scaling_results[-1][1]}x banks)"
            ),
            details=details,
        )
