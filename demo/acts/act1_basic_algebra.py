"""Act 1: Basic Delta Algebra — Load + Accumulate + Read + Verify.

Demonstrates the fundamental XOR delta-state operations on Node 1.
"""

from __future__ import annotations

from demo.acts.base import ActBase, ActResult
from demo.node import NodeController


class Act1BasicAlgebra(ActBase):
    number = 1
    title = "Basic Delta Algebra"
    narration = (
        "We begin with the fundamental operation: load an initial state, "
        "accumulate XOR deltas, and read the result. Every operation completes "
        "in a single clock cycle — 10.6 nanoseconds."
    )

    def run(self, nodes: list[NodeController]) -> ActResult:
        node = nodes[0]  # Use Node 1 (Finance)
        details: list[str] = []
        all_passed = True

        # Step 1: Load a known initial state
        initial = 0x123456789ABCDEF0
        node.load(initial)
        result = node.read()
        ok = result == initial
        details.append(f"Load/Read roundtrip: 0x{result:016X} {'==' if ok else '!='} 0x{initial:016X}")
        all_passed &= ok

        # Step 2: Accumulate a single delta
        delta = 0xAAAAAAAAAAAAAAAA
        node.load(0)
        node.accumulate(delta)
        result = node.read()
        expected = delta  # 0 XOR delta = delta
        ok = result == expected
        details.append(f"Single delta: 0x{result:016X} {'==' if ok else '!='} 0x{expected:016X}")
        all_passed &= ok

        # Step 3: Multiple deltas (associativity)
        node.load(0)
        d1, d2, d3 = 0x1111111111111111, 0x2222222222222222, 0x4444444444444444
        node.accumulate(d1)
        node.accumulate(d2)
        node.accumulate(d3)
        result = node.read()
        expected = d1 ^ d2 ^ d3  # 0x7777...
        ok = result == expected
        details.append(f"Multi-delta (d1^d2^d3): 0x{result:016X} {'==' if ok else '!='} 0x{expected:016X}")
        all_passed &= ok

        # Step 4: State reconstruction (initial XOR delta)
        initial = 0xFF00FF00FF00FF00
        delta = 0x00FF00FF00FF00FF
        node.load(initial)
        node.accumulate(delta)
        result = node.read()
        expected = initial ^ delta  # 0xFFFFFFFFFFFFFFFF
        ok = result == expected
        details.append(f"Reconstruction S0^d: 0x{result:016X} {'==' if ok else '!='} 0x{expected:016X}")
        all_passed &= ok

        return self._result(
            passed=all_passed,
            summary="4/4 algebra checks passed" if all_passed else "Some algebra checks failed",
            details=details,
        )
