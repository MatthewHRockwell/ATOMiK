"""
Regression Detector

Detects performance regressions by comparing current metrics
against moving averages using statistical deviation thresholds.
Used by MetricsAnalyzer for cross-run regression alerts.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from typing import Any

from .metrics_analyzer import HIGHER_IS_BETTER, LOWER_IS_BETTER, MetricsAnalyzer


class RegressionSeverity(Enum):
    """Severity classification for regressions."""
    CRITICAL = "critical"    # Test failure, hard metric violation
    WARNING = "warning"      # Metric degradation beyond threshold
    INFO = "info"            # Cost increase, minor degradation


@dataclass
class Regression:
    """A detected regression."""
    metric: str
    severity: RegressionSeverity
    expected_value: float
    actual_value: float
    deviation_pct: float
    message: str

    def to_dict(self) -> dict[str, Any]:
        return {
            "metric": self.metric,
            "severity": self.severity.value,
            "expected_value": round(self.expected_value, 4),
            "actual_value": round(self.actual_value, 4),
            "deviation_pct": round(self.deviation_pct, 1),
            "message": self.message,
        }


@dataclass
class RegressionReport:
    """Report of all detected regressions."""
    regressions: list[Regression] = field(default_factory=list)
    has_critical: bool = False

    @property
    def count(self) -> int:
        return len(self.regressions)

    def to_dict(self) -> dict[str, Any]:
        return {
            "regression_count": self.count,
            "has_critical": self.has_critical,
            "regressions": [r.to_dict() for r in self.regressions],
        }


# Regression thresholds per metric category
REGRESSION_THRESHOLDS: dict[str, dict[str, float]] = {
    # Test regressions (critical)
    "sim_tests_passed": {"threshold_pct": 0.0, "severity": 0},   # Any decrease
    "hw_tests_passed": {"threshold_pct": 0.0, "severity": 0},
    "sw_tests_passed": {"threshold_pct": 0.0, "severity": 0},
    # Hardware metric regressions (warning)
    "fmax_mhz": {"threshold_pct": 5.0, "severity": 1},
    "lut_pct": {"threshold_pct": 10.0, "severity": 1},
    "ff_pct": {"threshold_pct": 10.0, "severity": 1},
    # Cost regressions (info)
    "tokens_consumed": {"threshold_pct": 25.0, "severity": 2},
    "pipeline_time_ms": {"threshold_pct": 25.0, "severity": 2},
}

SEVERITY_MAP = {0: RegressionSeverity.CRITICAL, 1: RegressionSeverity.WARNING, 2: RegressionSeverity.INFO}


class RegressionDetector:
    """
    Detects regressions across pipeline runs.

    Compares current metrics against baseline or moving average.
    Classifies regressions by severity and produces a structured
    report.

    Example:
        >>> detector = RegressionDetector()
        >>> report = detector.detect(history, current_metrics)
        >>> if report.has_critical:
        ...     print("CRITICAL regression detected!")
    """

    def __init__(
        self,
        thresholds: dict[str, dict[str, float]] | None = None,
        analyzer: MetricsAnalyzer | None = None,
    ) -> None:
        self._thresholds = dict(REGRESSION_THRESHOLDS)
        if thresholds:
            self._thresholds.update(thresholds)
        self._analyzer = analyzer or MetricsAnalyzer()

    def detect(
        self,
        history: list[dict[str, Any]],
        current: dict[str, Any],
    ) -> RegressionReport:
        """
        Detect regressions in current metrics vs. history.

        Args:
            history: Previous run metrics (chronological order).
            current: Current run metrics.

        Returns:
            RegressionReport with all detected regressions.
        """
        report = RegressionReport()

        if not history:
            return report

        for metric, config in self._thresholds.items():
            current_val = current.get(metric)
            if current_val is None:
                continue

            try:
                current_val = float(current_val)
            except (ValueError, TypeError):
                continue

            # Compute baseline from history
            hist_values = []
            for run in history:
                v = run.get(metric)
                if v is not None:
                    try:
                        hist_values.append(float(v))
                    except (ValueError, TypeError):
                        pass

            if not hist_values:
                continue

            baseline = self._analyzer.compute_moving_average(hist_values)
            if baseline == 0:
                continue

            threshold_pct = config["threshold_pct"]
            severity_idx = int(config["severity"])

            # Compute deviation
            if metric in HIGHER_IS_BETTER:
                # Regression if current < baseline - threshold
                deviation_pct = 100.0 * (baseline - current_val) / abs(baseline)
                is_regression = deviation_pct > threshold_pct
            elif metric in LOWER_IS_BETTER:
                # Regression if current > baseline + threshold
                deviation_pct = 100.0 * (current_val - baseline) / abs(baseline)
                is_regression = deviation_pct > threshold_pct
            else:
                deviation_pct = 100.0 * abs(current_val - baseline) / abs(baseline)
                is_regression = deviation_pct > threshold_pct

            if is_regression:
                severity = SEVERITY_MAP.get(severity_idx, RegressionSeverity.INFO)
                regression = Regression(
                    metric=metric,
                    severity=severity,
                    expected_value=baseline,
                    actual_value=current_val,
                    deviation_pct=deviation_pct,
                    message=(
                        f"{metric}: expected ~{baseline:.2f}, "
                        f"got {current_val:.2f} "
                        f"({deviation_pct:+.1f}% deviation)"
                    ),
                )
                report.regressions.append(regression)
                if severity == RegressionSeverity.CRITICAL:
                    report.has_critical = True

        return report
