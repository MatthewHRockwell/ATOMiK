"""
Pipeline-Level Regression Detection

Integrates the analysis-level regression detector with baseline
management to provide a pipeline gate that can block completion
on critical regressions.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from ..analysis.regression_detector import (
    RegressionDetector,
    RegressionReport,
    RegressionSeverity,
)
from ..event_bus import EventBus, Event, EventType
from .baseline import BaselineManager, BaselineSnapshot


@dataclass
class GateResult:
    """Result of a regression gate check."""
    passed: bool = True
    blocked: bool = False
    report: RegressionReport | None = None
    baseline_created: bool = False
    message: str = ""

    def to_dict(self) -> dict[str, Any]:
        return {
            "passed": self.passed,
            "blocked": self.blocked,
            "baseline_created": self.baseline_created,
            "message": self.message,
            "report": self.report.to_dict() if self.report else None,
        }


class RegressionGate:
    """
    Pipeline gate that blocks on critical regressions.

    Wraps the analysis-level RegressionDetector with baseline
    management. After each run, checks for regressions against
    the baseline. If `fail_on_regression` is True, critical
    regressions block pipeline completion.

    Example:
        >>> gate = RegressionGate(fail_on_regression=True)
        >>> result = gate.check("trade_packet", current_metrics, history)
        >>> if result.blocked:
        ...     print(f"Pipeline blocked: {result.message}")
    """

    def __init__(
        self,
        baseline_manager: BaselineManager | None = None,
        detector: RegressionDetector | None = None,
        event_bus: EventBus | None = None,
        fail_on_regression: bool = False,
    ) -> None:
        self.baselines = baseline_manager or BaselineManager()
        self.detector = detector or RegressionDetector()
        self.event_bus = event_bus
        self.fail_on_regression = fail_on_regression

    def check(
        self,
        schema_name: str,
        current_metrics: dict[str, Any],
        history: list[dict[str, Any]] | None = None,
    ) -> GateResult:
        """
        Check for regressions against baseline and history.

        If no baseline exists, creates one and passes. Otherwise
        runs regression detection and optionally blocks.

        Args:
            schema_name: Schema being checked.
            current_metrics: Metrics from the current run.
            history: Optional run history for trend analysis.

        Returns:
            GateResult with pass/block status and report.
        """
        result = GateResult()

        # Auto-create baseline if missing
        baseline, created = self.baselines.create_if_missing(
            schema_name, current_metrics
        )
        if created:
            result.baseline_created = True
            result.message = "Baseline created (first run)"
            return result

        # Build history from baseline if not provided
        if history is None:
            history = [baseline.metrics]

        # Run regression detection
        report = self.detector.detect(history, current_metrics)
        result.report = report

        if report.count == 0:
            result.message = "No regressions detected"
            # Update baseline with successful metrics
            self.baselines.update_baseline(schema_name, current_metrics)
            return result

        result.passed = not report.has_critical
        result.message = (
            f"{report.count} regression(s) detected"
            f" ({sum(1 for r in report.regressions if r.severity == RegressionSeverity.CRITICAL)} critical)"
        )

        if report.has_critical and self.fail_on_regression:
            result.blocked = True
            result.message += " -- pipeline blocked"

            if self.event_bus:
                self.event_bus.emit(Event(
                    EventType.REGRESSION_ALERT,
                    {
                        "schema": schema_name,
                        "critical_count": sum(
                            1 for r in report.regressions
                            if r.severity == RegressionSeverity.CRITICAL
                        ),
                        "blocked": True,
                    },
                    source="regression_gate",
                ))
        else:
            # Update baseline even with non-critical regressions
            if not report.has_critical:
                self.baselines.update_baseline(schema_name, current_metrics)

        return result


class PipelineRegressionDetector:
    """
    High-level regression detector for pipeline integration.

    Combines baseline management, regression detection, and
    gate checking into a single interface used by the pipeline
    controller.

    Example:
        >>> prd = PipelineRegressionDetector()
        >>> gate_result = prd.check_and_gate("trade_packet", metrics)
    """

    def __init__(
        self,
        baseline_dir: str = ".atomik/baselines",
        fail_on_regression: bool = False,
        event_bus: EventBus | None = None,
    ) -> None:
        self.baseline_manager = BaselineManager(baseline_dir)
        self.gate = RegressionGate(
            baseline_manager=self.baseline_manager,
            event_bus=event_bus,
            fail_on_regression=fail_on_regression,
        )
        self._history: dict[str, list[dict[str, Any]]] = {}

    def record_run(self, schema_name: str, metrics: dict[str, Any]) -> None:
        """Record a run's metrics for history tracking."""
        if schema_name not in self._history:
            self._history[schema_name] = []
        self._history[schema_name].append(dict(metrics))

    def check_and_gate(
        self,
        schema_name: str,
        current_metrics: dict[str, Any],
    ) -> GateResult:
        """
        Record metrics, check for regressions, and apply gate.

        Args:
            schema_name: Schema being checked.
            current_metrics: Metrics from the current run.

        Returns:
            GateResult with pass/block status.
        """
        history = self._history.get(schema_name, [])
        result = self.gate.check(schema_name, current_metrics, history or None)
        self.record_run(schema_name, current_metrics)
        return result

    def get_baseline(self, schema_name: str) -> BaselineSnapshot | None:
        """Get the current baseline for a schema."""
        return self.baseline_manager.get_baseline(schema_name)

    def reset_baseline(self, schema_name: str) -> bool:
        """Delete the baseline for a schema (next run creates fresh one)."""
        return self.baseline_manager.delete_baseline(schema_name)
