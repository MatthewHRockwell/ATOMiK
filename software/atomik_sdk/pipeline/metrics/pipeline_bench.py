"""
Pipeline Efficiency Benchmarks

Measures and reports pipeline execution efficiency: token consumption,
generation time, differential savings, and self-correction rates.
"""

from __future__ import annotations

from typing import Any

from .collector import MetricsCollector


class PipelineBenchmark:
    """Tracks and computes pipeline efficiency metrics."""

    def __init__(self) -> None:
        self.collector = MetricsCollector()

    def record_run(
        self,
        schema_name: str,
        total_time_ms: float,
        tokens_consumed: int,
        tokens_saved: int,
        files_generated: int,
        lines_generated: int,
        self_corrections: int,
        self_correction_successes: int,
        diff_type: str = "full",
    ) -> dict[str, Any]:
        """Record metrics for a complete pipeline run."""
        efficiency = (
            round(100 * tokens_saved / (tokens_saved + tokens_consumed), 1)
            if (tokens_saved + tokens_consumed) > 0
            else 100.0
        )

        cost_per_line = (
            round(tokens_consumed / lines_generated, 2)
            if lines_generated > 0
            else 0
        )

        metrics = {
            "schema": schema_name,
            "pipeline_total_time_ms": round(total_time_ms, 1),
            "tokens_consumed": tokens_consumed,
            "tokens_saved": tokens_saved,
            "token_efficiency_pct": efficiency,
            "files_generated": files_generated,
            "lines_generated": lines_generated,
            "cost_per_line": cost_per_line,
            "self_correction_count": self_corrections,
            "self_correction_success": self_correction_successes,
            "diff_type": diff_type,
        }

        self.collector.record_pipeline(**metrics)
        return metrics

    def compare_schemas(
        self, runs: list[dict[str, Any]]
    ) -> dict[str, dict[str, Any]]:
        """Compare pipeline metrics across schema runs."""
        comparison: dict[str, dict[str, Any]] = {}
        for run in runs:
            name = run.get("schema", "unknown")
            comparison[name] = {
                "time_ms": run.get("pipeline_total_time_ms", 0),
                "tokens": run.get("tokens_consumed", 0),
                "files": run.get("files_generated", 0),
                "lines": run.get("lines_generated", 0),
                "efficiency": run.get("token_efficiency_pct", 0),
            }
        return comparison
