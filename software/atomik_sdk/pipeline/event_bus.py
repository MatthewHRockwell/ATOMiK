"""
Event Bus

Publish-subscribe event system for inter-stage communication.
Stages emit events on completion or failure; the orchestrator
and other listeners react to events to dispatch dependent work.
"""

from __future__ import annotations

import threading
import time
from collections import defaultdict
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Callable


class EventType(Enum):
    """Pipeline event types."""
    TASK_READY = "task_ready"
    TASK_STARTED = "task_started"
    TASK_COMPLETED = "task_completed"
    TASK_FAILED = "task_failed"
    FEEDBACK_START = "feedback_start"
    FEEDBACK_RESULT = "feedback_result"
    BUDGET_WARNING = "budget_warning"
    REGRESSION_ALERT = "regression_alert"
    PIPELINE_DONE = "pipeline_done"


@dataclass
class Event:
    """A typed event with payload."""
    event_type: EventType
    payload: dict[str, Any] = field(default_factory=dict)
    timestamp: str = ""
    source: str = ""

    def __post_init__(self) -> None:
        if not self.timestamp:
            self.timestamp = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())

    def to_dict(self) -> dict[str, Any]:
        return {
            "event_type": self.event_type.value,
            "payload": self.payload,
            "timestamp": self.timestamp,
            "source": self.source,
        }


# Type alias for event handler callbacks
EventHandler = Callable[[Event], None]


class EventBus:
    """
    Publish-subscribe event bus for pipeline orchestration.

    Handlers subscribe to specific event types and are invoked
    synchronously when events are emitted. Thread-safe for
    concurrent stage execution.

    Example:
        >>> bus = EventBus()
        >>> results = []
        >>> bus.subscribe(EventType.TASK_COMPLETED, lambda e: results.append(e))
        >>> bus.emit(Event(EventType.TASK_COMPLETED, {"task_id": "gen_python"}))
        >>> assert len(results) == 1
    """

    def __init__(self) -> None:
        self._handlers: dict[EventType, list[EventHandler]] = defaultdict(list)
        self._history: list[Event] = []
        self._lock = threading.Lock()

    def subscribe(self, event_type: EventType, handler: EventHandler) -> None:
        """Register a handler for an event type."""
        with self._lock:
            self._handlers[event_type].append(handler)

    def unsubscribe(self, event_type: EventType, handler: EventHandler) -> None:
        """Remove a handler for an event type."""
        with self._lock:
            handlers = self._handlers.get(event_type, [])
            if handler in handlers:
                handlers.remove(handler)

    def emit(self, event: Event) -> None:
        """
        Emit an event to all subscribed handlers.

        Handlers are invoked synchronously in subscription order.
        """
        with self._lock:
            self._history.append(event)
            handlers = list(self._handlers.get(event.event_type, []))

        # Invoke handlers outside lock to prevent deadlocks
        for handler in handlers:
            handler(event)

    def get_history(self, event_type: EventType | None = None) -> list[Event]:
        """Get event history, optionally filtered by type."""
        with self._lock:
            if event_type is None:
                return list(self._history)
            return [e for e in self._history if e.event_type == event_type]

    def clear_history(self) -> None:
        """Clear the event history."""
        with self._lock:
            self._history.clear()

    def clear_all(self) -> None:
        """Clear all handlers and history."""
        with self._lock:
            self._handlers.clear()
            self._history.clear()
