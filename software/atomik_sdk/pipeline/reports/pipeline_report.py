"""
Structured Pipeline Report

Generates a comprehensive JSON report from a completed pipeline run,
combining per-stage results, metrics, and artifact manifests.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any


class PipelineReport:
    """Generates structured pipeline reports."""

    def generate(
        self,
        schema_name: str,
        stage_manifests: list[dict[str, Any]],
        aggregate_metrics: dict[str, Any],
        regression_report: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        """Generate a complete pipeline report."""
        report: dict[str, Any] = {
            "report_version": "2.1",
            "schema": schema_name,
            "summary": {
                "success": all(
                    s.get("status") in ("success", "skipped")
                    for s in stage_manifests
                ),
                "stages_run": len(stage_manifests),
                "validation_level": aggregate_metrics.get("validation_level", "none"),
            },
            "metrics": aggregate_metrics,
            "stages": stage_manifests,
        }

        if regression_report is not None:
            report["regressions"] = regression_report
            # Add severity badge to summary
            has_critical = regression_report.get("has_critical", False)
            count = regression_report.get("regression_count", 0)
            if has_critical:
                report["summary"]["regression_status"] = "CRITICAL"
            elif count > 0:
                report["summary"]["regression_status"] = "WARNING"
            else:
                report["summary"]["regression_status"] = "PASS"

        return report

    def save(self, report: dict[str, Any], path: str | Path) -> None:
        """Save report to JSON file."""
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(report, f, indent=2)
