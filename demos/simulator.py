"""Pure-Python ATOMiK simulator satisfying NodeInterface.

Useful for demos when hardware is unavailable and for unit testing.
"""

from __future__ import annotations

import time

MASK_64 = 0xFFFF_FFFF_FFFF_FFFF


class ATOMiKSimulator:
    """Software-only ATOMiK accumulator (64-bit XOR algebra)."""

    def __init__(self, *, latency: float = 0.0) -> None:
        self._initial: int = 0
        self._accumulator: int = 0
        self._latency = latency  # optional artificial delay (seconds)

    # -- NodeInterface ------------------------------------------------------

    def load(self, initial_state: int) -> None:
        self._initial = initial_state & MASK_64
        self._accumulator = 0
        self._delay()

    def accumulate(self, delta: int) -> None:
        self._accumulator ^= delta & MASK_64
        self._delay()

    def read(self) -> int:
        self._delay()
        return (self._initial ^ self._accumulator) & MASK_64

    def status(self) -> bool:
        return self._accumulator == 0

    @property
    def is_hardware(self) -> bool:
        return False

    # -- Internals ----------------------------------------------------------

    def _delay(self) -> None:
        if self._latency > 0:
            time.sleep(self._latency)
