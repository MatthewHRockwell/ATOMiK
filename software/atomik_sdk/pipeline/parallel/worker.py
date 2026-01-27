"""
Worker Process Wrapper

Wraps individual task execution with timeout, cancellation,
and structured result reporting.
"""

from __future__ import annotations

import threading
import time
from dataclasses import dataclass
from enum import Enum
from typing import Any, Callable


class WorkerState(Enum):
    """Worker execution state."""
    IDLE = "idle"
    RUNNING = "running"
    COMPLETED = "completed"
    FAILED = "failed"
    TIMED_OUT = "timed_out"
    CANCELLED = "cancelled"


@dataclass
class WorkerResult:
    """Result from a worker execution."""
    worker_id: str
    state: WorkerState
    output: Any = None
    error: str = ""
    duration_ms: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return {
            "worker_id": self.worker_id,
            "state": self.state.value,
            "error": self.error,
            "duration_ms": round(self.duration_ms, 1),
        }


class Worker:
    """
    Worker wrapper with timeout and cancellation support.

    Executes a callable with a timeout and returns a structured
    result. Supports cancellation via an event flag.

    Example:
        >>> worker = Worker("gen_python")
        >>> result = worker.run(lambda: generate("python"), timeout=30.0)
        >>> assert result.state == WorkerState.COMPLETED
    """

    def __init__(self, worker_id: str) -> None:
        self.worker_id = worker_id
        self._cancel_event = threading.Event()
        self._state = WorkerState.IDLE

    @property
    def state(self) -> WorkerState:
        return self._state

    def run(
        self,
        fn: Callable[[], Any],
        timeout: float = 60.0,
    ) -> WorkerResult:
        """
        Execute a function with timeout.

        Args:
            fn: Function to execute.
            timeout: Maximum execution time in seconds.

        Returns:
            WorkerResult with execution state and output.
        """
        self._state = WorkerState.RUNNING
        start = time.perf_counter()

        result_container: list[Any] = [None]
        error_container: list[str] = [""]

        def target() -> None:
            try:
                result_container[0] = fn()
            except Exception as e:
                error_container[0] = str(e)

        thread = threading.Thread(target=target, daemon=True)
        thread.start()
        thread.join(timeout=timeout)

        duration = (time.perf_counter() - start) * 1000

        if self._cancel_event.is_set():
            self._state = WorkerState.CANCELLED
            return WorkerResult(
                worker_id=self.worker_id,
                state=WorkerState.CANCELLED,
                duration_ms=duration,
            )

        if thread.is_alive():
            self._state = WorkerState.TIMED_OUT
            return WorkerResult(
                worker_id=self.worker_id,
                state=WorkerState.TIMED_OUT,
                error=f"Timed out after {timeout}s",
                duration_ms=duration,
            )

        if error_container[0]:
            self._state = WorkerState.FAILED
            return WorkerResult(
                worker_id=self.worker_id,
                state=WorkerState.FAILED,
                error=error_container[0],
                duration_ms=duration,
            )

        self._state = WorkerState.COMPLETED
        return WorkerResult(
            worker_id=self.worker_id,
            state=WorkerState.COMPLETED,
            output=result_container[0],
            duration_ms=duration,
        )

    def cancel(self) -> None:
        """Signal cancellation to the worker."""
        self._cancel_event.set()

    @property
    def is_cancelled(self) -> bool:
        return self._cancel_event.is_set()
