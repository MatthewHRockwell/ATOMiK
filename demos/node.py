"""NodeController — high-level wrapper around a NodeInterface backend.

Tracks state history, delta counts, and emits events for the dashboard.
"""

from __future__ import annotations

import time
from dataclasses import dataclass
from typing import Any, Callable

from demos.config import NodeConfig
from demos.hardware_interface import NodeInterface

# Type alias for event callbacks: (event_name, payload_dict)
EventCallback = Callable[[str, dict[str, Any]], None]


@dataclass
class NodeSnapshot:
    """Point-in-time snapshot of node state (for the dashboard)."""
    node_name: str
    domain: str
    n_banks: int
    freq_mhz: float
    throughput_mops: float
    is_hardware: bool
    state_hex: str
    accumulator_zero: bool
    delta_count: int
    last_delta_hex: str
    elapsed_ms: float


class NodeController:
    """Manages one demo node (hardware or simulated)."""

    def __init__(
        self,
        config: NodeConfig,
        backend: NodeInterface,
        on_event: EventCallback | None = None,
    ) -> None:
        self.config = config
        self.backend = backend
        self._on_event = on_event
        self._delta_count: int = 0
        self._last_delta: int = 0
        self._start_time: float = 0.0

    # -- Public API ---------------------------------------------------------

    @property
    def name(self) -> str:
        return self.config.name

    @property
    def is_hardware(self) -> bool:
        return self.backend.is_hardware

    @property
    def badge(self) -> str:
        return "[HW]" if self.is_hardware else "[SIM]"

    def load(self, initial_state: int) -> None:
        """Load initial state and reset counters."""
        self._delta_count = 0
        self._last_delta = 0
        self._start_time = time.perf_counter()
        self.backend.load(initial_state)
        self._emit("node_load", {"initial_state": f"0x{initial_state:016X}"})

    def accumulate(self, delta: int) -> None:
        """Accumulate a single delta."""
        self.backend.accumulate(delta)
        self._delta_count += 1
        self._last_delta = delta
        self._emit("node_accumulate", {
            "delta": f"0x{delta:016X}",
            "count": self._delta_count,
        })

    def accumulate_batch(self, deltas: list[int]) -> None:
        """Accumulate a batch of deltas."""
        for d in deltas:
            self.accumulate(d)

    def read(self) -> int:
        """Read current reconstructed state."""
        value = self.backend.read()
        self._emit("node_read", {"state": f"0x{value:016X}"})
        return value

    def status(self) -> bool:
        """Return True if accumulator is zero."""
        return self.backend.status()

    def snapshot(self) -> NodeSnapshot:
        """Capture the current node state for display."""
        state = self.backend.read()
        elapsed = (time.perf_counter() - self._start_time) * 1000 if self._start_time else 0.0
        return NodeSnapshot(
            node_name=self.config.name,
            domain=self.config.domain,
            n_banks=self.config.n_banks,
            freq_mhz=self.config.freq_mhz,
            throughput_mops=self.config.throughput_mops,
            is_hardware=self.backend.is_hardware,
            state_hex=f"0x{state:016X}",
            accumulator_zero=self.backend.status(),
            delta_count=self._delta_count,
            last_delta_hex=f"0x{self._last_delta:016X}",
            elapsed_ms=elapsed,
        )

    def reset(self) -> None:
        """Convenience: load zero state."""
        self.load(0)

    # -- Internals ----------------------------------------------------------

    def _emit(self, event: str, data: dict[str, Any]) -> None:
        if self._on_event:
            payload = {"node": self.config.name, **data}
            self._on_event(event, payload)
