"""
Cross-Run Metrics Analyzer

Analyzes metrics trends across pipeline runs using moving averages
and statistical deviation thresholds for anomaly detection.
All computation is local (0 LLM tokens).
"""

from __future__ import annotations

import math
from dataclasses import dataclass, field
from typing import Any


@dataclass
class MetricTrend:
    """Trend analysis for a single metric."""
    name: str
    current_value: float
    moving_avg: float
    std_dev: float
    deviation: float       # How many std devs from moving avg
    direction: str         # "improving", "degrading", "stable", "insufficient_data"
    is_anomaly: bool       # Deviation > threshold

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "current_value": round(self.current_value, 4),
            "moving_avg": round(self.moving_avg, 4),
            "std_dev": round(self.std_dev, 4),
            "deviation": round(self.deviation, 2),
            "direction": self.direction,
            "is_anomaly": self.is_anomaly,
        }


@dataclass
class TrendReport:
    """Complete trend analysis report."""
    run_count: int
    window_size: int
    trends: list[MetricTrend] = field(default_factory=list)
    anomalies: list[MetricTrend] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return {
            "run_count": self.run_count,
            "window_size": self.window_size,
            "trends": [t.to_dict() for t in self.trends],
            "anomaly_count": len(self.anomalies),
            "anomalies": [a.to_dict() for a in self.anomalies],
        }


# Metrics where higher values are better
HIGHER_IS_BETTER = {
    "fmax_mhz", "ops_per_sec", "throughput_gbps",
    "sim_tests_passed", "hw_tests_passed", "sw_tests_passed",
    "token_efficiency_pct", "self_correction_success",
}

# Metrics where lower values are better
LOWER_IS_BETTER = {
    "tokens_consumed", "pipeline_time_ms", "latency_ns",
    "lut_pct", "ff_pct", "power_mw", "lint_errors_found",
    "self_correction_count",
}


class MetricsAnalyzer:
    """
    Cross-run metrics trend analysis.

    Computes moving averages, detects anomalies via statistical
    deviation, and classifies trends as improving, degrading, or
    stable.

    Example:
        >>> analyzer = MetricsAnalyzer(window=5)
        >>> report = analyzer.analyze(metrics_history, "fmax_mhz")
        >>> for trend in report.trends:
        ...     print(f"{trend.name}: {trend.direction}")
    """

    def __init__(
        self,
        window: int = 10,
        anomaly_threshold: float = 2.0,
    ) -> None:
        self.window = window
        self.anomaly_threshold = anomaly_threshold

    def analyze(
        self,
        history: list[dict[str, Any]],
        metrics: list[str] | None = None,
    ) -> TrendReport:
        """
        Analyze metric trends across pipeline runs.

        Args:
            history: List of metrics dicts from pipeline runs (chronological).
            metrics: Specific metric names to analyze (None = all numeric).

        Returns:
            TrendReport with trends and anomalies.
        """
        report = TrendReport(
            run_count=len(history),
            window_size=self.window,
        )

        if not history:
            return report

        # Discover metrics to analyze
        if metrics is None:
            metrics = self._discover_numeric_metrics(history)

        for metric_name in metrics:
            values = self._extract_values(history, metric_name)
            if len(values) < 2:
                report.trends.append(MetricTrend(
                    name=metric_name,
                    current_value=values[-1] if values else 0.0,
                    moving_avg=values[-1] if values else 0.0,
                    std_dev=0.0,
                    deviation=0.0,
                    direction="insufficient_data",
                    is_anomaly=False,
                ))
                continue

            trend = self._compute_trend(metric_name, values)
            report.trends.append(trend)
            if trend.is_anomaly:
                report.anomalies.append(trend)

        return report

    def compute_moving_average(
        self, values: list[float], window: int | None = None
    ) -> float:
        """Compute moving average of the last N values."""
        w = window or self.window
        recent = values[-w:]
        return sum(recent) / len(recent) if recent else 0.0

    def compute_std_dev(
        self, values: list[float], window: int | None = None
    ) -> float:
        """Compute standard deviation of the last N values."""
        w = window or self.window
        recent = values[-w:]
        if len(recent) < 2:
            return 0.0
        mean = sum(recent) / len(recent)
        variance = sum((x - mean) ** 2 for x in recent) / (len(recent) - 1)
        return math.sqrt(variance)

    def _compute_trend(self, name: str, values: list[float]) -> MetricTrend:
        """Compute trend for a single metric."""
        current = values[-1]
        window_values = values[-self.window:]
        avg = self.compute_moving_average(values)
        std = self.compute_std_dev(values)

        if std > 0:
            deviation = abs(current - avg) / std
        else:
            deviation = 0.0

        is_anomaly = deviation > self.anomaly_threshold

        direction = self._classify_direction(name, values)

        return MetricTrend(
            name=name,
            current_value=current,
            moving_avg=avg,
            std_dev=std,
            deviation=deviation,
            direction=direction,
            is_anomaly=is_anomaly,
        )

    def _classify_direction(self, name: str, values: list[float]) -> str:
        """Classify whether a metric is improving, degrading, or stable."""
        if len(values) < 3:
            return "insufficient_data"

        recent = values[-3:]
        slope = (recent[-1] - recent[0]) / len(recent)

        threshold = abs(recent[0]) * 0.05 if recent[0] != 0 else 0.1

        if abs(slope) < threshold:
            return "stable"

        if name in HIGHER_IS_BETTER:
            return "improving" if slope > 0 else "degrading"
        elif name in LOWER_IS_BETTER:
            return "improving" if slope < 0 else "degrading"
        else:
            return "stable"

    def _extract_values(
        self, history: list[dict[str, Any]], metric: str
    ) -> list[float]:
        """Extract numeric values for a metric across runs."""
        values = []
        for run in history:
            val = run.get(metric)
            if val is not None:
                try:
                    values.append(float(val))
                except (ValueError, TypeError):
                    pass
        return values

    def _discover_numeric_metrics(
        self, history: list[dict[str, Any]]
    ) -> list[str]:
        """Discover all numeric metric names from history."""
        metrics: set[str] = set()
        for run in history:
            for key, val in run.items():
                if isinstance(val, (int, float)):
                    metrics.add(key)
        return sorted(metrics)
