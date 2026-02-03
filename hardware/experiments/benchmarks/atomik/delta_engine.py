"""ATOMiK delta-state engine.

This implements the delta-state algebra proven in Phase 1:
- Delta composition via XOR
- Stateless transitions
- Parallel composition capability
"""

from typing import List, Callable


class DeltaEngine:
    """Delta-state architecture with compose/accumulate pattern.

    This represents the ATOMiK model where:
    - Deltas (XOR differences) are stored instead of full state
    - Composition is via XOR (associative, commutative)
    - State reconstruction happens on demand
    - Operations can be parallelized (order-independent)

    Proven mathematical properties (from Phase 1):
    - Associativity: (δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃)
    - Commutativity: δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁
    - Identity: δ ⊕ 0 = δ
    - Inverse: δ ⊕ δ = 0

    Attributes:
        initial_state: Starting state value (64-bit integer)
        delta_accumulator: XOR accumulation of all deltas
        delta_history: List of individual deltas (for analysis)
        accumulate_count: Number of delta accumulations
        reconstruct_count: Number of state reconstructions
    """

    def __init__(self, initial_state: int = 0, track_deltas: bool = False):
        """Initialize delta engine with initial state.

        Args:
            initial_state: Starting state value
            track_deltas: Whether to maintain delta history
        """
        self.initial_state: int = initial_state
        self.delta_accumulator: int = 0
        self.delta_history: List[int] = [] if track_deltas else None
        self.accumulate_count: int = 0
        self.reconstruct_count: int = 0
        self.track_deltas = track_deltas

    def accumulate(self, delta: int) -> None:
        """Accumulate a delta via XOR composition.

        This is the fundamental operation: δ_acc ← δ_acc ⊕ δ

        Args:
            delta: Delta value to accumulate
        """
        self.accumulate_count += 1
        self.delta_accumulator ^= delta
        if self.track_deltas and self.delta_history is not None:
            self.delta_history.append(delta)

    def reconstruct(self) -> int:
        """Reconstruct current state from initial state and deltas.

        State = S₀ ⊕ δ_accumulated

        Returns:
            Reconstructed state value
        """
        self.reconstruct_count += 1
        return self.initial_state ^ self.delta_accumulator

    def batch_accumulate(self, deltas: List[int]) -> None:
        """Accumulate multiple deltas sequentially.

        Args:
            deltas: List of delta values to accumulate
        """
        for delta in deltas:
            self.accumulate(delta)

    def parallel_accumulate(self, deltas: List[int]) -> int:
        """Compose deltas in parallel (order-independent).

        Due to commutativity: δ₁ ⊕ δ₂ ⊕ δ₃ = δ₃ ⊕ δ₁ ⊕ δ₂

        This method simulates parallel composition.
        In hardware, this would use a tree reduction.

        Args:
            deltas: List of deltas to compose

        Returns:
            Composed delta value
        """
        result = 0
        for delta in deltas:
            result ^= delta
        return result

    def compose_deltas(self, deltas: List[int]) -> int:
        """Compose a list of deltas into single delta.

        δ_result = δ₁ ⊕ δ₂ ⊕ ... ⊕ δₙ

        Args:
            deltas: List of deltas to compose

        Returns:
            Composed delta
        """
        return self.parallel_accumulate(deltas)

    def apply_function(self, func: Callable[[int], int]) -> None:
        """Apply a function by computing delta from state transformation.

        This converts a traditional state function into a delta:
        1. Reconstruct current state
        2. Apply function
        3. Compute delta: δ = S_old ⊕ S_new
        4. Accumulate delta

        Args:
            func: Function to apply to current state
        """
        old_state = self.reconstruct()
        new_state = func(old_state)
        delta = old_state ^ new_state
        self.accumulate(delta)

    def get_delta_at(self, index: int) -> int:
        """Retrieve specific delta from history.

        Args:
            index: Delta index (0 = first)

        Returns:
            Delta at given index, or 0 if not tracked
        """
        if self.delta_history is None:
            return 0
        if 0 <= index < len(self.delta_history):
            return self.delta_history[index]
        return 0

    def memory_footprint(self) -> int:
        """Calculate total memory used (bytes).

        Returns:
            Estimated memory usage in bytes
        """
        # Initial state: 8 bytes
        # Delta accumulator: 8 bytes
        # Counters: 16 bytes
        # Delta history: 8 bytes per entry (if tracked)
        base = 32
        history_size = 0 if self.delta_history is None else len(self.delta_history) * 8
        return base + history_size

    def reset(self, initial_state: int = 0) -> None:
        """Reset delta engine to initial conditions.

        Args:
            initial_state: New initial state value
        """
        self.initial_state = initial_state
        self.delta_accumulator = 0
        if self.delta_history is not None:
            self.delta_history.clear()
        self.accumulate_count = 0
        self.reconstruct_count = 0

    def snapshot(self) -> dict:
        """Create a snapshot of current state for validation.

        Returns:
            Dictionary with state, accumulator, counts
        """
        return {
            'state': self.reconstruct(),
            'delta_accumulator': self.delta_accumulator,
            'accumulate_count': self.accumulate_count,
            'reconstruct_count': self.reconstruct_count,
            'delta_count': 0 if self.delta_history is None else len(self.delta_history),
        }

    def verify_properties(self) -> bool:
        """Verify delta algebra properties hold.

        Tests the proven properties from Phase 1:
        - Identity: 0 ⊕ δ = δ
        - Inverse: δ ⊕ δ = 0
        - Commutativity: δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁

        Returns:
            True if all properties verified
        """
        test_delta = 0x123456789ABCDEF0

        # Identity
        if (0 ^ test_delta) != test_delta:
            return False

        # Inverse
        if (test_delta ^ test_delta) != 0:
            return False

        # Commutativity
        delta1 = 0xAAAAAAAAAAAAAAAA
        delta2 = 0x5555555555555555
        if (delta1 ^ delta2) != (delta2 ^ delta1):
            return False

        return True
