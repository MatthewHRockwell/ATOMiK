"""
Intelligent Context Manager

Manages the context window across multi-turn agent interactions.
Tracks segment relevance, evicts stale segments, injects KB entries,
and pre-loads schema sections based on task-type affinity.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from .segment_tracker import ContextSegment, SegmentTracker


@dataclass
class ContextBudget:
    """Token budget for context window management."""
    max_tokens: int = 128000      # Model's max context
    utilization_limit: float = 0.8  # Target < 80% to prevent truncation
    current_tokens: int = 0

    @property
    def available_tokens(self) -> int:
        limit = int(self.max_tokens * self.utilization_limit)
        return max(0, limit - self.current_tokens)

    @property
    def utilization(self) -> float:
        if self.max_tokens == 0:
            return 0.0
        return self.current_tokens / self.max_tokens

    def to_dict(self) -> dict[str, Any]:
        return {
            "max_tokens": self.max_tokens,
            "utilization_limit": self.utilization_limit,
            "current_tokens": self.current_tokens,
            "available_tokens": self.available_tokens,
            "utilization": round(self.utilization, 3),
        }


@dataclass
class ContextLoadResult:
    """Result of loading context for a task."""
    segments_loaded: list[str] = field(default_factory=list)
    segments_evicted: list[str] = field(default_factory=list)
    kb_entries_injected: int = 0
    total_tokens: int = 0
    within_budget: bool = True

    def to_dict(self) -> dict[str, Any]:
        return {
            "segments_loaded": self.segments_loaded,
            "segments_evicted": self.segments_evicted,
            "kb_entries_injected": self.kb_entries_injected,
            "total_tokens": self.total_tokens,
            "within_budget": self.within_budget,
        }


class IntelligentContextManager:
    """
    Intelligent context window manager.

    Prioritizes context segments by relevance to the current task,
    evicts stale segments, and manages token budget to stay within
    the model's context window limit.

    Example:
        >>> mgr = IntelligentContextManager(max_tokens=128000)
        >>> mgr.add_segment("schema_fields", schema_content, "schema", ["generate"])
        >>> result = mgr.load_for_task("generate")
        >>> context = mgr.build_context()
    """

    def __init__(
        self,
        max_tokens: int = 128000,
        utilization_limit: float = 0.8,
        stale_threshold: int = 3,
    ) -> None:
        self.tracker = SegmentTracker(stale_threshold_tasks=stale_threshold)
        self.budget = ContextBudget(
            max_tokens=max_tokens,
            utilization_limit=utilization_limit,
        )

    def add_segment(
        self,
        segment_id: str,
        content: str,
        segment_type: str,
        task_affinity: list[str] | None = None,
    ) -> ContextSegment:
        """Add a context segment for tracking."""
        seg = self.tracker.add(segment_id, content, segment_type, task_affinity)
        self._update_budget()
        return seg

    def load_for_task(
        self,
        task_type: str,
        kb_entries: list[dict[str, str]] | None = None,
    ) -> ContextLoadResult:
        """
        Prepare context for a new task.

        1. Advance the task counter
        2. Evict stale segments
        3. Inject relevant KB entries
        4. Rank remaining segments by relevance
        5. Trim to fit within budget

        Args:
            task_type: Type of the upcoming task (e.g., "generate", "verify").
            kb_entries: Optional KB entries to inject as context segments.

        Returns:
            ContextLoadResult with details of what was loaded/evicted.
        """
        result = ContextLoadResult()

        # Advance task counter
        self.tracker.advance_task()

        # Evict stale segments
        evicted = self.tracker.evict_stale()
        result.segments_evicted = evicted

        # Inject KB entries
        if kb_entries:
            for entry in kb_entries:
                eid = f"kb_{entry.get('error_class', 'unknown')}"
                content = entry.get("fix_description", "")
                if content:
                    self.tracker.add(
                        eid, content, "kb_entry",
                        task_affinity=[task_type],
                    )
                    result.kb_entries_injected += 1

        # Rank by relevance
        ranked = self.tracker.rank_by_relevance(task_type)

        # Trim to fit budget
        available = int(self.budget.max_tokens * self.budget.utilization_limit)
        running_total = 0

        for seg in ranked:
            if running_total + seg.token_count <= available:
                running_total += seg.token_count
                result.segments_loaded.append(seg.segment_id)
            else:
                # Remove segments that don't fit
                self.tracker.remove(seg.segment_id)
                result.segments_evicted.append(seg.segment_id)

        result.total_tokens = running_total
        self.budget.current_tokens = running_total
        result.within_budget = self.budget.utilization <= self.budget.utilization_limit

        return result

    def build_context(self, task_type: str = "") -> str:
        """
        Build the context string from all tracked segments.

        Segments are ordered by relevance (highest first).

        Args:
            task_type: Current task type for relevance ranking.

        Returns:
            Concatenated context string.
        """
        ranked = self.tracker.rank_by_relevance(task_type)
        parts = []
        for seg in ranked:
            parts.append(seg.content)
        return "\n\n".join(parts)

    def get_context_for_cold_start(self) -> str:
        """
        Get minimal context for cold-start loading (<1.5K tokens).

        Returns only schema-type segments, truncated to fit.
        """
        target_tokens = 1500
        ranked = self.tracker.rank_by_relevance()
        parts = []
        running = 0

        # Prioritize schema segments
        schema_segs = [s for s in ranked if s.segment_type == "schema"]
        other_segs = [s for s in ranked if s.segment_type != "schema"]

        for seg in schema_segs + other_segs:
            if running + seg.token_count <= target_tokens:
                parts.append(seg.content)
                running += seg.token_count
            elif running < target_tokens:
                # Truncate to fit
                remaining_chars = (target_tokens - running) * 4
                parts.append(seg.content[:remaining_chars])
                break

        return "\n".join(parts)

    def get_utilization(self) -> dict[str, Any]:
        """Get current context utilization metrics."""
        return {
            "budget": self.budget.to_dict(),
            "tracker": self.tracker.summary(),
        }

    def _update_budget(self) -> None:
        """Update the budget's current token count from tracker."""
        self.budget.current_tokens = self.tracker.total_tokens()
