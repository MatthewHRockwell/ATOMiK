"""
Token Budget Accounting

Tracks per-pipeline and per-session token consumption. Enforces
budget limits and reports token efficiency metrics.
"""

from __future__ import annotations

import time
from dataclasses import dataclass, field
from typing import Any


@dataclass
class TokenEntry:
    """A single token consumption record."""
    stage: str
    model_tier: str
    tokens: int
    timestamp: str = ""
    description: str = ""

    def to_dict(self) -> dict[str, Any]:
        return {
            "stage": self.stage,
            "model_tier": self.model_tier,
            "tokens": self.tokens,
            "timestamp": self.timestamp,
            "description": self.description,
        }


class TokenBudget:
    """
    Token budget accounting for pipeline runs.

    Tracks actual vs. estimated consumption, enforces budget limits,
    and computes efficiency metrics.

    Example:
        >>> budget = TokenBudget(limit=15000)
        >>> budget.record("validate", "local", 0)
        >>> budget.record("generate", "local", 0)
        >>> print(f"Spent: {budget.total_consumed}")
    """

    def __init__(self, limit: int | None = None):
        self.limit = limit
        self._entries: list[TokenEntry] = []
        self._estimates: dict[str, int] = {}

    @property
    def total_consumed(self) -> int:
        return sum(e.tokens for e in self._entries)

    @property
    def remaining(self) -> int | None:
        if self.limit is None:
            return None
        return max(0, self.limit - self.total_consumed)

    def record(
        self,
        stage: str,
        model_tier: str,
        tokens: int,
        description: str = "",
    ) -> None:
        """Record actual token consumption for a stage."""
        self._entries.append(TokenEntry(
            stage=stage,
            model_tier=model_tier,
            tokens=tokens,
            timestamp=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            description=description,
        ))

    def set_estimate(self, stage: str, tokens: int) -> None:
        """Set estimated token consumption for a stage."""
        self._estimates[stage] = tokens

    def can_afford(self, estimated_tokens: int) -> bool:
        """Check if the budget can afford the estimated tokens."""
        if self.limit is None:
            return True
        return self.total_consumed + estimated_tokens <= self.limit

    def get_efficiency(self) -> dict[str, Any]:
        """Compute token efficiency metrics."""
        total = self.total_consumed
        total_estimated = sum(self._estimates.values())

        # Breakdown by model tier
        by_tier: dict[str, int] = {}
        for entry in self._entries:
            by_tier[entry.model_tier] = by_tier.get(entry.model_tier, 0) + entry.tokens

        local_tokens = by_tier.get("local", 0)
        llm_tokens = total - local_tokens
        local_stages = sum(1 for e in self._entries if e.model_tier == "local")
        total_stages = len(self._entries)

        return {
            "total_consumed": total,
            "total_estimated": total_estimated,
            "estimation_accuracy_pct": (
                round(100 * total / total_estimated, 1) if total_estimated > 0 else 0
            ),
            "budget_limit": self.limit,
            "budget_remaining": self.remaining,
            "local_execution_pct": (
                round(100 * local_stages / total_stages, 1) if total_stages > 0 else 0
            ),
            "by_tier": by_tier,
            "stages_recorded": total_stages,
        }

    def get_ledger(self) -> list[dict[str, Any]]:
        """Get the full token ledger."""
        return [e.to_dict() for e in self._entries]

    def reset(self) -> None:
        """Reset the budget for a new pipeline run."""
        self._entries.clear()
        self._estimates.clear()
