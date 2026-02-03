"""Conventional baseline implementations for comparison.

Provides full-state copy, event-sourcing, and CRDT-style approaches
so benchmarks can measure ATOMiK's improvements against real alternatives.
"""

from __future__ import annotations

import copy
import threading
from dataclasses import dataclass, field


@dataclass
class FullStateCopy:
    """Baseline: mutex-protected full-state copy on every update."""

    width: int = 64
    state: int = 0
    _lock: threading.Lock = field(default_factory=threading.Lock)

    def load(self, value: int) -> None:
        with self._lock:
            self.state = value & ((1 << self.width) - 1)

    def update(self, new_state: int) -> None:
        with self._lock:
            self.state = new_state & ((1 << self.width) - 1)

    def read(self) -> int:
        with self._lock:
            return self.state

    def merge(self, other: FullStateCopy) -> None:
        """Merge by overwriting with other's state (last-writer-wins)."""
        with self._lock:
            self.state = other.read()


@dataclass
class EventSourcingStore:
    """Baseline: event log with periodic snapshots for rollback."""

    width: int = 64
    _events: list[int] = field(default_factory=list)
    _snapshots: dict[int, int] = field(default_factory=dict)
    _snapshot_interval: int = 100
    _state: int = 0

    def apply(self, event: int) -> None:
        self._events.append(event)
        self._state = event & ((1 << self.width) - 1)
        if len(self._events) % self._snapshot_interval == 0:
            self._snapshots[len(self._events)] = self._state

    def read(self) -> int:
        return self._state

    def rollback(self, steps: int) -> int:
        """Roll back by replaying from nearest snapshot. Returns ops count."""
        target = max(0, len(self._events) - steps)
        # Find nearest snapshot at or before target
        snap_idx = 0
        snap_state = 0
        for idx, state in sorted(self._snapshots.items()):
            if idx <= target:
                snap_idx = idx
                snap_state = state
        # Replay from snapshot
        ops = 0
        state = snap_state
        for i in range(snap_idx, target):
            state = self._events[i] & ((1 << self.width) - 1)
            ops += 1
        self._state = state
        self._events = self._events[:target]
        return ops + 1  # +1 for the snapshot lookup


@dataclass
class OrderedReplaySync:
    """Baseline: ordered message replay for distributed sync.

    Messages must be applied in causal order; out-of-order delivery
    requires buffering and re-sorting.
    """

    width: int = 64
    _state: int = 0
    _sequence: int = 0
    _buffer: list[tuple[int, int]] = field(default_factory=list)

    def apply_ordered(self, seq: int, value: int) -> int:
        """Apply a sequenced update. Returns number of messages processed."""
        self._buffer.append((seq, value))
        self._buffer.sort(key=lambda x: x[0])

        processed = 0
        while self._buffer and self._buffer[0][0] == self._sequence + 1:
            _, val = self._buffer.pop(0)
            self._state = val & ((1 << self.width) - 1)
            self._sequence += 1
            processed += 1

        return processed

    def read(self) -> int:
        return self._state

    @property
    def pending(self) -> int:
        return len(self._buffer)
