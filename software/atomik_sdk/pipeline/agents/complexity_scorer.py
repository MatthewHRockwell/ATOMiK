"""
Complexity Scorer

Scores schema and task complexity to inform adaptive model routing.
Higher complexity scores suggest routing to higher-tier models.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass
class ComplexityScore:
    """Computed complexity score for a schema or task."""
    total: float
    field_count: int = 0
    operation_count: int = 0
    data_width: int = 0
    has_hardware: bool = False
    has_rollback: bool = False
    nested_depth: int = 0

    def to_dict(self) -> dict[str, Any]:
        return {
            "total": round(self.total, 2),
            "field_count": self.field_count,
            "operation_count": self.operation_count,
            "data_width": self.data_width,
            "has_hardware": self.has_hardware,
            "has_rollback": self.has_rollback,
            "nested_depth": self.nested_depth,
        }


# Weights for complexity scoring
WEIGHTS = {
    "field_count": 1.0,
    "operation_count": 2.0,
    "data_width_factor": 0.01,   # per bit
    "hardware_bonus": 5.0,
    "rollback_bonus": 3.0,
    "nested_depth": 1.5,
}

# Thresholds for routing decisions
COMPLEXITY_THRESHOLDS = {
    "low": 5.0,       # Route to Haiku or local
    "medium": 15.0,   # Route to Sonnet
    "high": 30.0,     # Route to Opus
}


class ComplexityScorer:
    """
    Scores schema complexity to inform adaptive model routing.

    Example:
        >>> scorer = ComplexityScorer()
        >>> score = scorer.score_schema(schema)
        >>> if score.total < 5.0:
        ...     print("Low complexity -- use Haiku")
    """

    def __init__(
        self,
        weights: dict[str, float] | None = None,
        thresholds: dict[str, float] | None = None,
    ) -> None:
        self._weights = dict(WEIGHTS)
        if weights:
            self._weights.update(weights)
        self._thresholds = dict(COMPLEXITY_THRESHOLDS)
        if thresholds:
            self._thresholds.update(thresholds)

    def score_schema(self, schema: dict[str, Any]) -> ComplexityScore:
        """
        Compute complexity score for a schema.

        Args:
            schema: Parsed ATOMiK schema dict.

        Returns:
            ComplexityScore with total and component scores.
        """
        fields = schema.get("delta_fields", {})
        ops = schema.get("operations", {})
        hw = schema.get("hardware", {})

        field_count = len(fields)
        op_count = len(ops)
        data_width = hw.get("data_width", 64)
        has_hardware = bool(hw)
        has_rollback = "rollback" in ops or bool(
            hw.get("rollback_history_depth", 0)
        )
        nested_depth = self._compute_nesting(schema)

        total = (
            field_count * self._weights["field_count"]
            + op_count * self._weights["operation_count"]
            + data_width * self._weights["data_width_factor"]
            + (self._weights["hardware_bonus"] if has_hardware else 0)
            + (self._weights["rollback_bonus"] if has_rollback else 0)
            + nested_depth * self._weights["nested_depth"]
        )

        return ComplexityScore(
            total=total,
            field_count=field_count,
            operation_count=op_count,
            data_width=data_width,
            has_hardware=has_hardware,
            has_rollback=has_rollback,
            nested_depth=nested_depth,
        )

    def classify(self, score: ComplexityScore) -> str:
        """Classify complexity as 'low', 'medium', or 'high'."""
        if score.total < self._thresholds["low"]:
            return "low"
        elif score.total < self._thresholds["medium"]:
            return "medium"
        else:
            return "high"

    def _compute_nesting(self, obj: Any, depth: int = 0) -> int:
        """Compute max nesting depth of a dict/list structure."""
        if isinstance(obj, dict):
            if not obj:
                return depth
            return max(
                self._compute_nesting(v, depth + 1)
                for v in obj.values()
            )
        elif isinstance(obj, list):
            if not obj:
                return depth
            return max(
                self._compute_nesting(v, depth + 1)
                for v in obj
            )
        return depth
