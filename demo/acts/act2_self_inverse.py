"""Act 2: Self-Inverse Property — Apply delta twice, state returns to original.

Demonstrates instant undo: delta XOR delta = 0, so any change is its own undo.
"""

from __future__ import annotations

from demo.acts.base import ActBase, ActResult
from demo.node import NodeController


class Act2SelfInverse(ActBase):
    number = 2
    title = "Self-Inverse (Instant Undo)"
    narration = (
        "XOR is self-inverse: applying the same delta twice cancels it out. "
        "This means every change is its own undo — no checkpoints, no logs, "
        "no rollback journals. Instant, zero-cost reversal."
    )

    def run(self, nodes: list[NodeController]) -> ActResult:
        node = nodes[0]
        details: list[str] = []
        all_passed = True

        # Test with several different initial states and deltas
        test_cases = [
            (0xCAFEBABE12345678, 0x5555555555555555, "pattern A"),
            (0xDEADBEEFDEADBEEF, 0xFF00FF00FF00FF00, "pattern B"),
            (0x0000000000000001, 0xFFFFFFFFFFFFFFFF, "edge: max delta"),
        ]

        for initial, delta, label in test_cases:
            node.load(initial)

            # Apply delta once — state changes
            node.accumulate(delta)
            after_first = node.read()
            changed = after_first != initial
            details.append(f"  {label}: after 1x delta -> 0x{after_first:016X} (changed={changed})")

            # Apply same delta again — state restored
            node.accumulate(delta)
            after_second = node.read()
            ok = after_second == initial
            details.append(
                f"  {label}: after 2x delta -> 0x{after_second:016X} "
                f"{'==' if ok else '!='} 0x{initial:016X}"
            )
            all_passed &= ok

            # Accumulator should be zero again
            acc_zero = node.status()
            details.append(f"  {label}: accumulator_zero = {acc_zero}")
            all_passed &= acc_zero

        return self._result(
            passed=all_passed,
            summary="3/3 self-inverse tests passed" if all_passed else "Self-inverse test failed",
            details=details,
        )
