"""Traditional stateful state manager.

This implements the SCORE model:
- Full state storage
- Read-modify-write pattern
- Serial dependencies
"""

from typing import Any, Callable, List, Optional


class StateManager:
    """Traditional stateful architecture with read/modify/write pattern.

    This represents conventional computing where:
    - State is stored explicitly in memory
    - Operations read current state, modify it, and write back
    - History requires additional memory allocation
    - Operations create data dependencies

    Attributes:
        state: Current state value (64-bit integer)
        history: Optional history buffer for state snapshots
        read_count: Number of state read operations
        write_count: Number of state write operations
    """

    def __init__(self, initial_state: int = 0, track_history: bool = False):
        """Initialize state manager with initial state.

        Args:
            initial_state: Starting state value
            track_history: Whether to maintain state history buffer
        """
        self.state: int = initial_state
        self.history: List[int] = [] if track_history else None
        self.read_count: int = 0
        self.write_count: int = 0
        self.track_history = track_history

    def read(self) -> int:
        """Read current state.

        Returns:
            Current state value
        """
        self.read_count += 1
        return self.state

    def write(self, new_state: int) -> None:
        """Write new state.

        Args:
            new_state: New state value to write
        """
        self.write_count += 1
        if self.track_history and self.history is not None:
            self.history.append(self.state)
        self.state = new_state

    def modify(self, operation: Callable[[int], int]) -> None:
        """Read-modify-write operation.

        This is the fundamental SCORE pattern:
        1. Read current state
        2. Apply operation
        3. Write result back

        Args:
            operation: Function to apply to current state
        """
        current = self.read()
        new_state = operation(current)
        self.write(new_state)

    def batch_modify(self, operations: List[Callable[[int], int]]) -> None:
        """Apply a sequence of operations serially.

        Each operation depends on the result of the previous one,
        creating a serial dependency chain.

        Args:
            operations: List of operations to apply in order
        """
        for op in operations:
            self.modify(op)

    def get_state_at(self, index: int) -> Optional[int]:
        """Retrieve historical state if tracking is enabled.

        Args:
            index: History index (0 = oldest)

        Returns:
            State at given index, or None if history not tracked
        """
        if self.history is None:
            return None
        if 0 <= index < len(self.history):
            return self.history[index]
        return None

    def memory_footprint(self) -> int:
        """Calculate total memory used (bytes).

        Returns:
            Estimated memory usage in bytes
        """
        # State: 8 bytes (64-bit integer)
        # History: 8 bytes per entry
        # Counters: 16 bytes
        base = 24
        history_size = 0 if self.history is None else len(self.history) * 8
        return base + history_size

    def reset(self, initial_state: int = 0) -> None:
        """Reset state manager to initial conditions.

        Args:
            initial_state: New initial state value
        """
        self.state = initial_state
        if self.history is not None:
            self.history.clear()
        self.read_count = 0
        self.write_count = 0

    def snapshot(self) -> dict:
        """Create a snapshot of current state for validation.

        Returns:
            Dictionary with state, read_count, write_count
        """
        return {
            'state': self.state,
            'read_count': self.read_count,
            'write_count': self.write_count,
            'history_length': 0 if self.history is None else len(self.history),
        }
