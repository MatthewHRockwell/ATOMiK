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
    ) -> dict[str, Any]:
        """Generate a complete pipeline report."""
        return {
            "report_version": "2.0",
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

    def save(self, report: dict[str, Any], path: str | Path) -> None:
        """Save report to JSON file."""
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(report, f, indent=2)
