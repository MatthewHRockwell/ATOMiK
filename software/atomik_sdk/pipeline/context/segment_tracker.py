"""
Context Segment Tracker

Tracks context segments with usage statistics and relevance scores.
Each segment represents a portion of the context window (schema section,
KB entry, previous output) with metadata about its recency, task
affinity, and access frequency.
"""

from __future__ import annotations

import time
from dataclasses import dataclass, field
from typing import Any


@dataclass
class ContextSegment:
    """A tracked context segment with usage metadata."""
    segment_id: str
    content: str
    segment_type: str       # "schema", "kb_entry", "previous_output", "error_context"
    task_affinity: list[str] = field(default_factory=list)  # Task types this is relevant to
    token_count: int = 0
    created_at: float = 0.0
    last_accessed: float = 0.0
    access_count: int = 0
    relevance_score: float = 1.0

    def __post_init__(self) -> None:
        now = time.time()
        if self.created_at == 0.0:
            self.created_at = now
        if self.last_accessed == 0.0:
            self.last_accessed = now
        if self.token_count == 0:
            self.token_count = len(self.content) // 4  # Rough token estimate

    def touch(self) -> None:
        """Mark this segment as accessed."""
        self.last_accessed = time.time()
        self.access_count += 1

    @property
    def age_seconds(self) -> float:
        return time.time() - self.last_accessed

    @property
    def is_stale(self) -> bool:
        """A segment is stale if not accessed recently (heuristic)."""
        return self.access_count == 0 and self.age_seconds > 300

    def to_dict(self) -> dict[str, Any]:
        return {
            "segment_id": self.segment_id,
            "segment_type": self.segment_type,
            "token_count": self.token_count,
            "access_count": self.access_count,
            "relevance_score": round(self.relevance_score, 3),
            "age_seconds": round(self.age_seconds, 1),
            "task_affinity": self.task_affinity,
        }


class SegmentTracker:
    """
    Tracks context segments with usage statistics.

    Maintains a registry of context segments, computes relevance
    scores based on recency, task affinity, and access frequency,
    and identifies segments for eviction.

    Example:
        >>> tracker = SegmentTracker()
        >>> tracker.add("schema_fields", content, "schema", ["generate"])
        >>> ranked = tracker.rank_by_relevance("generate")
    """

    def __init__(self, stale_threshold_tasks: int = 3) -> None:
        self._segments: dict[str, ContextSegment] = {}
        self._task_counter: int = 0
        self._segment_task_last_used: dict[str, int] = {}
        self.stale_threshold = stale_threshold_tasks

    def add(
        self,
        segment_id: str,
        content: str,
        segment_type: str,
        task_affinity: list[str] | None = None,
    ) -> ContextSegment:
        """Add or update a context segment."""
        if segment_id in self._segments:
            seg = self._segments[segment_id]
            seg.content = content
            seg.token_count = len(content) // 4
            seg.touch()
        else:
            seg = ContextSegment(
                segment_id=segment_id,
                content=content,
                segment_type=segment_type,
                task_affinity=task_affinity or [],
            )
            self._segments[segment_id] = seg

        self._segment_task_last_used[segment_id] = self._task_counter
        return seg

    def get(self, segment_id: str) -> ContextSegment | None:
        """Get a segment by ID, marking it as accessed."""
        seg = self._segments.get(segment_id)
        if seg:
            seg.touch()
            self._segment_task_last_used[segment_id] = self._task_counter
        return seg

    def remove(self, segment_id: str) -> bool:
        """Remove a segment. Returns True if found."""
        if segment_id in self._segments:
            del self._segments[segment_id]
            self._segment_task_last_used.pop(segment_id, None)
            return True
        return False

    def advance_task(self) -> None:
        """Signal that a new task has started (for staleness tracking)."""
        self._task_counter += 1

    def rank_by_relevance(
        self,
        current_task_type: str = "",
    ) -> list[ContextSegment]:
        """
        Rank segments by relevance to the current task.

        Relevance is computed from:
        - Task affinity (2x weight if matching)
        - Recency (higher score for recently accessed)
        - Access frequency (log scale)

        Returns segments sorted by relevance (highest first).
        """
        for seg in self._segments.values():
            seg.relevance_score = self._compute_relevance(
                seg, current_task_type
            )

        return sorted(
            self._segments.values(),
            key=lambda s: s.relevance_score,
            reverse=True,
        )

    def get_stale_segments(self) -> list[ContextSegment]:
        """Get segments not accessed in the last N tasks."""
        stale = []
        for seg_id, seg in self._segments.items():
            last_used = self._segment_task_last_used.get(seg_id, 0)
            tasks_since = self._task_counter - last_used
            if tasks_since >= self.stale_threshold:
                stale.append(seg)
        return stale

    def evict_stale(self) -> list[str]:
        """Remove stale segments and return their IDs."""
        stale = self.get_stale_segments()
        evicted = []
        for seg in stale:
            self.remove(seg.segment_id)
            evicted.append(seg.segment_id)
        return evicted

    def total_tokens(self) -> int:
        """Total token count across all segments."""
        return sum(s.token_count for s in self._segments.values())

    @property
    def count(self) -> int:
        return len(self._segments)

    def summary(self) -> dict[str, Any]:
        """Summary of tracked segments."""
        return {
            "segment_count": self.count,
            "total_tokens": self.total_tokens(),
            "task_counter": self._task_counter,
            "segments": [s.to_dict() for s in self._segments.values()],
        }

    def _compute_relevance(
        self,
        segment: ContextSegment,
        current_task_type: str,
    ) -> float:
        """Compute relevance score for a segment."""
        score = 1.0

        # Task affinity bonus (2x)
        if current_task_type and current_task_type in segment.task_affinity:
            score *= 2.0

        # Recency: decay based on tasks since last use
        last_used = self._segment_task_last_used.get(segment.segment_id, 0)
        tasks_since = self._task_counter - last_used
        if tasks_since > 0:
            score *= 1.0 / (1.0 + 0.3 * tasks_since)

        # Frequency bonus (diminishing returns)
        if segment.access_count > 1:
            import math
            score *= 1.0 + 0.2 * math.log(segment.access_count)

        return score
