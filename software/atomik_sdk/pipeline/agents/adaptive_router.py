"""
Adaptive Model Router

Multi-signal model selection considering task complexity, error history,
budget pressure, and prompt cache state. Extends the Phase 4C static
router with adaptive scoring.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from .router import ModelTier, ModelRouter, TaskClass, TOKEN_ESTIMATES
from .complexity_scorer import ComplexityScorer, ComplexityScore


@dataclass
class RoutingDecision:
    """A recorded routing decision with reasoning."""
    stage: str
    selected_tier: ModelTier
    complexity_score: float = 0.0
    complexity_class: str = ""
    budget_pressure: float = 0.0   # 0.0 to 1.0
    has_prior_failure: bool = False
    cache_hit: bool = False
    reason: str = ""

    def to_dict(self) -> dict[str, Any]:
        return {
            "stage": self.stage,
            "selected_tier": self.selected_tier.value,
            "complexity_score": round(self.complexity_score, 2),
            "complexity_class": self.complexity_class,
            "budget_pressure": round(self.budget_pressure, 2),
            "has_prior_failure": self.has_prior_failure,
            "cache_hit": self.cache_hit,
            "reason": self.reason,
        }


class AdaptiveRouter:
    """
    Adaptive model router that considers multiple signals.

    Extends the static ModelRouter with signals:
    - Schema complexity (field count, operation count, data width)
    - Error history (prior failures on this schema)
    - Budget pressure (consumed / limit)
    - Prompt cache availability

    Falls back to static routing when adaptive signals are unavailable.

    Example:
        >>> router = AdaptiveRouter()
        >>> tier = router.route("generate", schema=schema, budget_pressure=0.3)
        >>> print(f"Selected: {tier.value}")
    """

    def __init__(
        self,
        static_router: ModelRouter | None = None,
        complexity_scorer: ComplexityScorer | None = None,
    ) -> None:
        self._static = static_router or ModelRouter()
        self._scorer = complexity_scorer or ComplexityScorer()
        self._failure_history: dict[str, int] = {}  # schema_hash -> failure count
        self._decisions: list[RoutingDecision] = []

    def route(
        self,
        stage_name: str,
        schema: dict[str, Any] | None = None,
        schema_hash: str = "",
        budget_pressure: float = 0.0,
        cache_hit: bool = False,
    ) -> ModelTier:
        """
        Route a task to the optimal model tier.

        Args:
            stage_name: Pipeline stage or subtask name.
            schema: Parsed schema dict (for complexity scoring).
            schema_hash: Schema content hash (for error history lookup).
            budget_pressure: Fraction of budget consumed (0.0 to 1.0).
            cache_hit: Whether a prompt cache hit is available.

        Returns:
            Selected ModelTier.
        """
        # Start with static routing
        base_tier = self._static.route(stage_name)

        # If stage is deterministic, no adaptation needed
        if base_tier == ModelTier.LOCAL:
            decision = RoutingDecision(
                stage=stage_name,
                selected_tier=ModelTier.LOCAL,
                reason="deterministic_stage",
            )
            self._decisions.append(decision)
            return ModelTier.LOCAL

        # Compute adaptive signals
        complexity_score = 0.0
        complexity_class = ""
        if schema:
            score = self._scorer.score_schema(schema)
            complexity_score = score.total
            complexity_class = self._scorer.classify(score)

        has_prior_failure = self._failure_history.get(schema_hash, 0) > 0

        # Adaptive selection
        selected = self._select_tier(
            base_tier, complexity_class, has_prior_failure,
            budget_pressure, cache_hit,
        )

        reason = self._build_reason(
            base_tier, selected, complexity_class,
            has_prior_failure, budget_pressure, cache_hit,
        )

        decision = RoutingDecision(
            stage=stage_name,
            selected_tier=selected,
            complexity_score=complexity_score,
            complexity_class=complexity_class,
            budget_pressure=budget_pressure,
            has_prior_failure=has_prior_failure,
            cache_hit=cache_hit,
            reason=reason,
        )
        self._decisions.append(decision)
        return selected

    def record_failure(self, schema_hash: str) -> None:
        """Record a failure for a schema (increases escalation priority)."""
        self._failure_history[schema_hash] = (
            self._failure_history.get(schema_hash, 0) + 1
        )

    def record_success(self, schema_hash: str) -> None:
        """Record a success (decreases escalation priority over time)."""
        if schema_hash in self._failure_history:
            self._failure_history[schema_hash] = max(
                0, self._failure_history[schema_hash] - 1
            )

    def get_decisions(self) -> list[dict[str, Any]]:
        """Get all routing decisions for analysis."""
        return [d.to_dict() for d in self._decisions]

    def clear_decisions(self) -> None:
        """Clear routing decision log."""
        self._decisions.clear()

    def _select_tier(
        self,
        base_tier: ModelTier,
        complexity_class: str,
        has_prior_failure: bool,
        budget_pressure: float,
        cache_hit: bool,
    ) -> ModelTier:
        """Select model tier based on adaptive signals."""
        tiers = list(ModelTier)
        base_idx = tiers.index(base_tier)

        # Budget pressure > 80%: downgrade to cheaper tier
        if budget_pressure > 0.8 and base_idx > 0:
            return tiers[base_idx - 1]

        # Prior failure: escalate one tier
        if has_prior_failure and base_idx < len(tiers) - 1:
            return tiers[base_idx + 1]

        # Low complexity: downgrade one tier
        if complexity_class == "low" and base_idx > 0:
            return tiers[base_idx - 1]

        # High complexity: escalate one tier
        if complexity_class == "high" and base_idx < len(tiers) - 1:
            return tiers[base_idx + 1]

        return base_tier

    def _build_reason(
        self,
        base_tier: ModelTier,
        selected: ModelTier,
        complexity_class: str,
        has_prior_failure: bool,
        budget_pressure: float,
        cache_hit: bool,
    ) -> str:
        """Build a human-readable reason for the routing decision."""
        if selected == base_tier:
            return "static_default"

        reasons = []
        if budget_pressure > 0.8:
            reasons.append(f"budget_pressure({budget_pressure:.0%})")
        if has_prior_failure:
            reasons.append("prior_failure_escalation")
        if complexity_class == "low":
            reasons.append("low_complexity_downgrade")
        elif complexity_class == "high":
            reasons.append("high_complexity_escalation")
        if cache_hit:
            reasons.append("cache_hit")

        return "+".join(reasons) if reasons else "adaptive"
