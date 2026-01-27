"""
Pipeline Self-Optimization Engine

Analyzes execution patterns across runs and automatically adjusts
configuration for optimal performance. Identifies bottleneck stages,
produces optimization reports, and applies tuning recommendations.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from .tuner import ConfigTuner, TuningResult


@dataclass
class Recommendation:
    """A specific optimization recommendation."""
    category: str       # "bottleneck", "workers", "retry", "routing", "kb"
    priority: str       # "high", "medium", "low"
    description: str
    tuning: TuningResult | None = None

    def to_dict(self) -> dict[str, Any]:
        return {
            "category": self.category,
            "priority": self.priority,
            "description": self.description,
            "tuning": self.tuning.to_dict() if self.tuning else None,
        }


@dataclass
class OptimizationReport:
    """Report from the self-optimization engine."""
    run_count_analyzed: int = 0
    bottleneck_stage: str = ""
    bottleneck_pct: float = 0.0
    recommendations: list[Recommendation] = field(default_factory=list)
    tuning_results: list[TuningResult] = field(default_factory=list)

    @property
    def recommendation_count(self) -> int:
        return len(self.recommendations)

    def to_dict(self) -> dict[str, Any]:
        return {
            "run_count_analyzed": self.run_count_analyzed,
            "bottleneck_stage": self.bottleneck_stage,
            "bottleneck_pct": round(self.bottleneck_pct, 1),
            "recommendation_count": self.recommendation_count,
            "recommendations": [r.to_dict() for r in self.recommendations],
            "tuning_results": [t.to_dict() for t in self.tuning_results],
        }


class SelfOptimizer:
    """
    Pipeline self-optimization engine.

    Analyzes run history to identify bottlenecks and produce
    actionable recommendations. Delegates parameter tuning to
    ConfigTuner and aggregates all findings into a structured report.

    Example:
        >>> optimizer = SelfOptimizer(report_every=5)
        >>> optimizer.record_run(run_metrics)
        >>> if optimizer.should_report():
        ...     report = optimizer.generate_report()
        ...     print(f"Bottleneck: {report.bottleneck_stage}")
    """

    def __init__(
        self,
        report_every: int = 5,
        tuner: ConfigTuner | None = None,
    ) -> None:
        self.report_every = report_every
        self.tuner = tuner or ConfigTuner()
        self._run_history: list[dict[str, Any]] = []
        self._reports_generated: int = 0

    def record_run(self, run_metrics: dict[str, Any]) -> None:
        """Record a pipeline run's metrics for analysis."""
        self._run_history.append(dict(run_metrics))

    @property
    def run_count(self) -> int:
        return len(self._run_history)

    def should_report(self) -> bool:
        """Check if enough runs have accumulated for a report."""
        return (
            self.run_count > 0 and
            self.run_count % self.report_every == 0
        )

    def generate_report(
        self,
        current_config: dict[str, Any] | None = None,
    ) -> OptimizationReport:
        """
        Generate an optimization report from accumulated history.

        Analyzes bottleneck stages, runs auto-tuning, and produces
        specific recommendations.

        Args:
            current_config: Current pipeline configuration.

        Returns:
            OptimizationReport with findings and recommendations.
        """
        report = OptimizationReport(run_count_analyzed=self.run_count)

        if not self._run_history:
            return report

        # 1. Identify bottleneck stage
        self._analyze_bottleneck(report)

        # 2. Run auto-tuning
        tuning_results = self.tuner.tune_all(
            self._run_history, current_config
        )
        report.tuning_results = tuning_results

        for tr in tuning_results:
            report.recommendations.append(Recommendation(
                category=self._categorize_tuning(tr.parameter),
                priority="medium",
                description=tr.reason,
                tuning=tr,
            ))

        # 3. Check for KB expansion opportunities
        self._analyze_kb_opportunities(report)

        # 4. Check for error pattern trends
        self._analyze_error_trends(report)

        self._reports_generated += 1
        return report

    def get_tuning_recommendations(
        self,
        current_config: dict[str, Any] | None = None,
    ) -> list[TuningResult]:
        """Get only the tuning results without full report."""
        return self.tuner.tune_all(self._run_history, current_config)

    def _analyze_bottleneck(self, report: OptimizationReport) -> None:
        """Identify the slowest pipeline stage across runs."""
        stage_times: dict[str, list[float]] = {}

        for run in self._run_history:
            stages = run.get("stage_durations", {})
            for stage, duration in stages.items():
                if stage not in stage_times:
                    stage_times[stage] = []
                try:
                    stage_times[stage].append(float(duration))
                except (ValueError, TypeError):
                    pass

        if not stage_times:
            return

        # Find the stage with highest average duration
        avg_times = {
            stage: sum(times) / len(times)
            for stage, times in stage_times.items()
            if times
        }

        if not avg_times:
            return

        total_avg = sum(avg_times.values())
        bottleneck = max(avg_times, key=avg_times.get)  # type: ignore[arg-type]
        bottleneck_time = avg_times[bottleneck]

        report.bottleneck_stage = bottleneck
        report.bottleneck_pct = (
            (bottleneck_time / total_avg * 100) if total_avg > 0 else 0
        )

        if report.bottleneck_pct > 50:
            report.recommendations.append(Recommendation(
                category="bottleneck",
                priority="high",
                description=(
                    f"Stage '{bottleneck}' consumes {report.bottleneck_pct:.0f}% "
                    f"of pipeline time (avg {bottleneck_time:.0f}ms)"
                ),
            ))

    def _analyze_kb_opportunities(self, report: OptimizationReport) -> None:
        """Check if knowledge base could benefit from expansion."""
        kb_hits = 0
        kb_misses = 0

        for run in self._run_history:
            feedback = run.get("feedback_summary", {})
            kb_hits += feedback.get("kb_hits", 0)
            kb_misses += feedback.get("kb_misses", 0)

        total = kb_hits + kb_misses
        if total > 5 and kb_misses > kb_hits:
            hit_rate = kb_hits / total if total > 0 else 0
            report.recommendations.append(Recommendation(
                category="kb",
                priority="medium",
                description=(
                    f"KB hit rate is {hit_rate:.0%} ({kb_hits}/{total}). "
                    f"Consider adding patterns for common misses."
                ),
            ))

    def _analyze_error_trends(self, report: OptimizationReport) -> None:
        """Check for recurring error patterns."""
        error_counts: dict[str, int] = {}

        for run in self._run_history:
            errors = run.get("errors", [])
            for error in errors:
                error_class = error if isinstance(error, str) else str(error)
                # Use first 50 chars as key
                key = error_class[:50]
                error_counts[key] = error_counts.get(key, 0) + 1

        # Flag errors that recur in >50% of runs
        threshold = max(2, self.run_count // 2)
        for error_key, count in error_counts.items():
            if count >= threshold:
                report.recommendations.append(Recommendation(
                    category="kb",
                    priority="high",
                    description=(
                        f"Recurring error in {count}/{self.run_count} runs: "
                        f"'{error_key}'. Add to knowledge base."
                    ),
                ))

    @staticmethod
    def _categorize_tuning(parameter: str) -> str:
        """Map a tuning parameter to a recommendation category."""
        if "worker" in parameter:
            return "workers"
        elif "retry" in parameter:
            return "retry"
        elif "routing" in parameter:
            return "routing"
        return "general"
