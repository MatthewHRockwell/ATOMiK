"""ATOMiK XOR delta-state implementation (software emulation).

Mirrors the hardware semantics: LOAD sets initial state,
ACCUMULATE XORs a delta into the accumulator, READ reconstructs
current state as initial_state XOR accumulator.
"""

from __future__ import annotations

from dataclasses import dataclass, field


@dataclass
class AtomikAccumulator:
    """Software emulation of a single ATOMiK delta accumulator."""

    width: int = 64
    initial_state: int = 0
    accumulator: int = 0
    delta_count: int = 0

    @property
    def mask(self) -> int:
        return (1 << self.width) - 1

    def load(self, value: int) -> None:
        """LOAD: set initial state, reset accumulator."""
        self.initial_state = value & self.mask
        self.accumulator = 0
        self.delta_count = 0

    def accumulate(self, delta: int) -> None:
        """ACCUMULATE: XOR delta into accumulator (single-cycle op)."""
        self.accumulator = (self.accumulator ^ delta) & self.mask
        self.delta_count += 1

    def read(self) -> int:
        """READ: reconstruct current state = initial XOR accumulator."""
        return (self.initial_state ^ self.accumulator) & self.mask

    def rollback(self, delta: int) -> None:
        """ROLLBACK: self-inverse — applying same delta undoes it."""
        self.accumulate(delta)  # XOR is its own inverse

    def merge(self, other: AtomikAccumulator) -> None:
        """Merge another accumulator's deltas (order-independent)."""
        self.accumulator = (self.accumulator ^ other.accumulator) & self.mask
        self.delta_count += other.delta_count


@dataclass
class AtomikParallelBank:
    """N parallel accumulators with XOR merge tree.

    Emulates Phase 6 hardware: round-robin distribution to N banks,
    combinational merge tree for read.
    """

    n_banks: int = 16
    width: int = 64
    initial_state: int = 0
    banks: list[int] = field(default_factory=list)
    _next_bank: int = 0
    delta_count: int = 0

    def __post_init__(self) -> None:
        if not self.banks:
            self.banks = [0] * self.n_banks

    @property
    def mask(self) -> int:
        return (1 << self.width) - 1

    def load(self, value: int) -> None:
        self.initial_state = value & self.mask
        self.banks = [0] * self.n_banks
        self._next_bank = 0
        self.delta_count = 0

    def accumulate(self, delta: int) -> None:
        """Round-robin distribute delta to next bank."""
        self.banks[self._next_bank] = (
            self.banks[self._next_bank] ^ delta
        ) & self.mask
        self._next_bank = (self._next_bank + 1) % self.n_banks
        self.delta_count += 1

    def read(self) -> int:
        """XOR merge tree: combine all banks then XOR with initial state."""
        merged = 0
        for bank in self.banks:
            merged ^= bank
        return (self.initial_state ^ merged) & self.mask
