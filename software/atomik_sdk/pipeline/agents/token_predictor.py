"""
Token Predictor

Predicts token consumption before task execution based on
historical per-task averages. Enables proactive budget management.
"""

from __future__ import annotations

import time
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Any


@dataclass
class PredictionRecord:
    """A prediction vs. actual comparison."""
    task: str
    predicted: int
    actual: int = 0
    accuracy_pct: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return {
            "task": self.task,
            "predicted": self.predicted,
            "actual": self.actual,
            "accuracy_pct": round(self.accuracy_pct, 1),
        }


class TokenPredictor:
    """
    Predicts token consumption from historical data.

    Maintains per-task rolling averages and computes prediction
    accuracy over time. Used by the budget engine for pre-task
    estimation and proactive budget warnings.

    Example:
        >>> predictor = TokenPredictor()
        >>> predictor.record_actual("generate", 5000)
        >>> predictor.record_actual("generate", 4500)
        >>> estimate = predictor.predict("generate")
        >>> assert 4000 <= estimate <= 5000
    """

    def __init__(self, default_estimates: dict[str, int] | None = None) -> None:
        self._history: dict[str, list[int]] = defaultdict(list)
        self._defaults: dict[str, int] = default_estimates or {}
        self._predictions: list[PredictionRecord] = []
        self._window_size = 10  # Rolling average window

    def predict(self, task: str) -> int:
        """
        Predict token consumption for a task.

        Uses rolling average of historical data if available,
        otherwise falls back to default estimate.
        """
        history = self._history.get(task, [])
        if history:
            window = history[-self._window_size:]
            return int(sum(window) / len(window))
        return self._defaults.get(task, 0)

    def record_actual(self, task: str, tokens: int) -> None:
        """Record actual token consumption for a task."""
        self._history[task].append(tokens)

    def predict_and_track(self, task: str) -> int:
        """Predict and create a tracking record (call finalize_prediction later)."""
        predicted = self.predict(task)
        self._predictions.append(PredictionRecord(task=task, predicted=predicted))
        return predicted

    def finalize_prediction(self, task: str, actual: int) -> None:
        """Record actual tokens for the most recent prediction of this task."""
        self.record_actual(task, actual)
        for pred in reversed(self._predictions):
            if pred.task == task and pred.actual == 0:
                pred.actual = actual
                if pred.predicted > 0:
                    pred.accuracy_pct = 100.0 * (
                        1.0 - abs(pred.predicted - actual) / pred.predicted
                    )
                break

    def get_accuracy(self) -> float:
        """Get overall prediction accuracy percentage."""
        finalized = [p for p in self._predictions if p.actual > 0]
        if not finalized:
            return 0.0
        return sum(p.accuracy_pct for p in finalized) / len(finalized)

    def get_predictions(self) -> list[dict[str, Any]]:
        """Get all prediction records."""
        return [p.to_dict() for p in self._predictions]

    def predict_remaining(self, pending_tasks: list[str]) -> int:
        """Predict total tokens for all pending tasks."""
        return sum(self.predict(t) for t in pending_tasks)
