"""Multi-context ATOMiK state table.

Maps addresses (0..N-1) to independent AtomikContext instances.
Mirrors the hardware state table: 256 entries x 64-bit in the ASIC/FPGA.
"""

from __future__ import annotations

from atomik_core.context import AtomikContext


class AtomikTable:
    """A table of independently addressable ATOMiK contexts.

    Each address holds its own reference + accumulator pair.
    This is the software equivalent of the hardware state table SRAM.

    Example:
        table = AtomikTable(num_contexts=256)
        table.load(addr=0, initial_state=0xCAFEBABE)
        table.load(addr=1, initial_state=0xDEADBEEF)
        table.accum(addr=0, delta=0x00000001)
        assert table.read(addr=0) == 0xCAFEBABF
        assert table.read(addr=1) == 0xDEADBEEF  # independent
    """

    __slots__ = ("_contexts", "_num_contexts", "_width")

    def __init__(self, num_contexts: int = 256, width: int = 64):
        """Create a state table.

        Args:
            num_contexts: Number of context slots (default 256, matching hardware).
            width: Bit width per context (default 64).
        """
        if num_contexts < 1:
            raise ValueError(f"num_contexts must be >= 1, got {num_contexts}")
        self._num_contexts = num_contexts
        self._width = width
        self._contexts = [AtomikContext(width=width) for _ in range(num_contexts)]

    def _ctx(self, addr: int) -> AtomikContext:
        if addr < 0 or addr >= self._num_contexts:
            raise IndexError(
                f"address {addr} out of range [0, {self._num_contexts - 1}]"
            )
        return self._contexts[addr]

    def load(self, addr: int, initial_state: int) -> None:
        """LOAD: Set initial state at address, clear its accumulator."""
        self._ctx(addr).load(initial_state)

    def accum(self, addr: int, delta: int) -> None:
        """ACCUM: XOR delta into the accumulator at address."""
        self._ctx(addr).accum(delta)

    def read(self, addr: int) -> int:
        """READ: Reconstruct current state at address."""
        return self._ctx(addr).read()

    def swap(self, addr: int, new_reference: int | None = None) -> int:
        """SWAP: Atomic snapshot + new epoch at address."""
        return self._ctx(addr).swap(new_reference)

    def context(self, addr: int) -> AtomikContext:
        """Get the raw AtomikContext at an address (for advanced use)."""
        return self._ctx(addr)

    def batch_accum(self, deltas: dict[int, int]) -> None:
        """Apply multiple deltas to multiple addresses in one call.

        Order-independent: the result is the same regardless of iteration order.

        Args:
            deltas: Mapping of {address: delta_value}.
        """
        for addr, delta in deltas.items():
            self._ctx(addr).accum(delta)

    def snapshot(self) -> dict[int, int]:
        """Read all non-zero contexts.

        Returns:
            Dict of {address: current_state} for all contexts with non-zero state.
        """
        result = {}
        for i, ctx in enumerate(self._contexts):
            state = ctx.read()
            if state != 0:
                result[i] = state
        return result

    @property
    def num_contexts(self) -> int:
        return self._num_contexts

    @property
    def width(self) -> int:
        return self._width

    def __repr__(self) -> str:
        active = sum(1 for ctx in self._contexts if not ctx.is_clean or ctx.reference != 0)
        return f"AtomikTable(contexts={self._num_contexts}, width={self._width}, active={active})"
