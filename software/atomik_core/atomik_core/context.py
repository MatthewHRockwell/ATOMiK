"""Single ATOMiK context — the fundamental unit of delta-state tracking.

A context holds one reference state and one accumulator. Deltas are XORed
into the accumulator; the current state is always reference ^ accumulator.

This mirrors the hardware exactly: one context = one address in the state table.
"""

from __future__ import annotations


class AtomikContext:
    """A single ATOMiK delta-state context.

    Implements the 4-operation algebra:
      load(value)  — Set reference, clear accumulator
      accum(delta) — XOR delta into accumulator
      read()       — Return reference ^ accumulator
      swap(value)  — Atomic snapshot + new epoch

    All operations are O(1) in time and space.
    """

    __slots__ = ("_reference", "_accumulator", "_width", "_mask", "_delta_count")

    def __init__(self, width: int = 64, initial_state: int = 0):
        """Create a new ATOMiK context.

        Args:
            width: Bit width (default 64). Supports 8, 16, 32, 64, 128, or any power of 2.
            initial_state: Initial reference state (default 0).
        """
        if width < 1:
            raise ValueError(f"width must be >= 1, got {width}")
        self._width = width
        self._mask = (1 << width) - 1
        self._reference = initial_state & self._mask
        self._accumulator = 0
        self._delta_count = 0

    def load(self, value: int) -> None:
        """LOAD: Set reference state, clear accumulator.

        After load, read() returns the loaded value.
        This starts a new epoch — all prior deltas are discarded.

        Args:
            value: New reference state.
        """
        self._reference = value & self._mask
        self._accumulator = 0
        self._delta_count = 0

    def accum(self, delta: int) -> None:
        """ACCUM: XOR delta into the accumulator.

        Order-independent: accum(a); accum(b) == accum(b); accum(a).
        Self-inverse: accum(d); accum(d) is a no-op.

        Args:
            delta: Value to XOR into accumulator.
        """
        self._accumulator = (self._accumulator ^ delta) & self._mask
        self._delta_count += 1

    def read(self) -> int:
        """READ: Reconstruct current state.

        Returns reference ^ accumulator. This is the state after applying
        all accumulated deltas to the reference.

        Returns:
            Current state value.
        """
        return (self._reference ^ self._accumulator) & self._mask

    def swap(self, new_reference: int | None = None) -> int:
        """SWAP: Atomic snapshot + new epoch.

        Returns the current state (reference ^ accumulator), then:
        - If new_reference is provided: sets it as the new reference
        - If new_reference is None: uses the current state as new reference
        Clears the accumulator in both cases.

        This is the hardware SWAP operation: read current state, start
        a new epoch from that state (or a specified state).

        Args:
            new_reference: Optional new reference state. If None, uses current state.

        Returns:
            The state at the moment of the swap.
        """
        current = self.read()
        if new_reference is not None:
            self._reference = new_reference & self._mask
        else:
            self._reference = current
        self._accumulator = 0
        self._delta_count = 0
        return current

    def rollback(self, delta: int) -> None:
        """Undo a previously applied delta.

        Because XOR is self-inverse, rollback(d) == accum(d).
        This is a convenience alias that makes intent clear.

        Args:
            delta: The delta to undo.
        """
        self.accum(delta)

    def merge(self, other: AtomikContext) -> None:
        """Merge another context's accumulator into this one.

        Because XOR is commutative and associative, merging is
        order-independent. Both contexts must track the same reference.

        Args:
            other: Context whose accumulator to merge.
        """
        self._accumulator = (self._accumulator ^ other._accumulator) & self._mask
        self._delta_count += other._delta_count

    def _set_accumulator(self, value: int) -> None:
        """Internal: set accumulator directly (for encapsulated friends like Fingerprint).

        Not part of the public 4-operation algebra. Avoids incrementing delta_count.
        """
        self._accumulator = value & self._mask
        self._delta_count = 0

    @property
    def accumulator(self) -> int:
        """Raw accumulator value (XOR of all deltas since last load/swap)."""
        return self._accumulator

    @property
    def reference(self) -> int:
        """Current reference state (set by load or swap)."""
        return self._reference

    @property
    def is_clean(self) -> bool:
        """True if no deltas have been applied (accumulator is zero)."""
        return self._accumulator == 0

    @property
    def delta_count(self) -> int:
        """Number of accum() calls since last load/swap."""
        return self._delta_count

    @property
    def width(self) -> int:
        """Bit width of this context."""
        return self._width

    def __repr__(self) -> str:
        hex_width = (self._width + 3) // 4
        return (
            f"AtomikContext(state=0x{self.read():0{hex_width}x}, "
            f"ref=0x{self._reference:0{hex_width}x}, "
            f"acc=0x{self._accumulator:0{hex_width}x}, "
            f"deltas={self._delta_count})"
        )

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, AtomikContext):
            return NotImplemented
        return self.read() == other.read() and self._width == other._width
