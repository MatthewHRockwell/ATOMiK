"""
Metrics Collection Stage

Stage 6 of the pipeline: aggregates metrics from all previous stages,
computes derived metrics, appends to CSV history, and generates the
final pipeline report.
"""

from __future__ import annotations

import csv
import time
from pathlib import Path
from typing import Any

from ..context.checkpoint import Checkpoint
from . import BaseStage, StageManifest


class MetricsStage(BaseStage):
    """Pipeline stage for metrics aggregation and reporting."""

    name = "metrics"

    CSV_HEADERS = [
        "timestamp", "schema", "pipeline_time_ms", "tokens_consumed",
        "tokens_saved", "files_generated", "lines_generated",
        "lut_pct", "ff_pct", "fmax_mhz", "timing_slack_ns",
        "ops_per_sec", "latency_ns", "power_mw",
        "sim_pass", "hw_pass", "sw_pass", "validation_level",
    ]

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        # This stage receives ALL previous manifests through the pipeline.
        # The controller passes the last manifest, but we can reconstruct
        # the full picture from the config's pipeline result.
        checkpoint_dir = getattr(config, "checkpoint_dir", ".atomik")
        metrics_csv = getattr(config, "metrics_csv", ".atomik/metrics.csv")
        schema_name = Path(schema_path).stem

        # Aggregate metrics from all pipeline stages
        aggregated = self._aggregate(manifest, previous_manifest, schema_path)

        manifest.metrics.update(aggregated)

        # Compute derived metrics
        derived = self._compute_derived(aggregated)
        manifest.metrics.update(derived)

        # Append to CSV history
        self._append_csv(metrics_csv, schema_name, aggregated, derived)

        # Update checkpoint with results
        checkpoint = Checkpoint(checkpoint_dir)
        content_hash = aggregated.get("content_hash", "")
        if content_hash:
            checkpoint.update_schema(
                schema_name,
                content_hash,
                metrics={**aggregated, **derived},
            )

        manifest.next_stage = None  # Terminal stage
        manifest.tokens_consumed = 0

    def _aggregate(
        self,
        manifest: StageManifest,
        previous_manifest: StageManifest | None,
        schema_path: str,
    ) -> dict[str, Any]:
        """Aggregate metrics from previous stage manifests."""
        return {
            "schema": Path(schema_path).stem,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            # From previous manifests (propagated through pipeline)
            "content_hash": (
                previous_manifest.metrics.get("content_hash", "")
                if previous_manifest else ""
            ),
            "files_generated": (
                previous_manifest.metrics.get("files_generated", 0)
                if previous_manifest else 0
            ),
            "lines_generated": (
                previous_manifest.metrics.get("lines_generated", 0)
                if previous_manifest else 0
            ),
            "sim_tests_passed": (
                previous_manifest.metrics.get("sim_tests_passed", 0)
                if previous_manifest else 0
            ),
            "sim_tests_total": (
                previous_manifest.metrics.get("sim_tests_total", 0)
                if previous_manifest else 0
            ),
            "hw_tests_passed": (
                previous_manifest.metrics.get("hw_tests_passed", 0)
                if previous_manifest else 0
            ),
            "hw_tests_total": (
                previous_manifest.metrics.get("hw_tests_total", 0)
                if previous_manifest else 0
            ),
            "validation_level": (
                previous_manifest.validation_level
                if previous_manifest else "none"
            ),
        }

    def _compute_derived(self, aggregated: dict[str, Any]) -> dict[str, Any]:
        """Compute derived metrics from aggregated data."""
        derived: dict[str, Any] = {}

        sim_pass = aggregated.get("sim_tests_passed", 0)
        sim_total = aggregated.get("sim_tests_total", 0)
        derived["sim_pass_rate"] = (
            round(100 * sim_pass / sim_total, 1) if sim_total > 0 else 0
        )

        hw_pass = aggregated.get("hw_tests_passed", 0)
        hw_total = aggregated.get("hw_tests_total", 0)
        derived["hw_pass_rate"] = (
            round(100 * hw_pass / hw_total, 1) if hw_total > 0 else 0
        )

        return derived

    def _append_csv(
        self,
        csv_path: str,
        schema_name: str,
        aggregated: dict[str, Any],
        derived: dict[str, Any],
    ) -> None:
        """Append a metrics row to the CSV history file."""
        path = Path(csv_path)
        path.parent.mkdir(parents=True, exist_ok=True)

        write_header = not path.exists()

        row = {
            "timestamp": aggregated.get("timestamp", ""),
            "schema": schema_name,
            "pipeline_time_ms": aggregated.get("pipeline_time_ms", 0),
            "tokens_consumed": aggregated.get("tokens_consumed", 0),
            "tokens_saved": aggregated.get("tokens_saved", 0),
            "files_generated": aggregated.get("files_generated", 0),
            "lines_generated": aggregated.get("lines_generated", 0),
            "lut_pct": aggregated.get("lut_pct", ""),
            "ff_pct": aggregated.get("ff_pct", ""),
            "fmax_mhz": aggregated.get("fmax_mhz", ""),
            "timing_slack_ns": aggregated.get("timing_slack_ns", ""),
            "ops_per_sec": aggregated.get("ops_per_sec", ""),
            "latency_ns": aggregated.get("latency_ns", ""),
            "power_mw": aggregated.get("power_mw", ""),
            "sim_pass": f"{aggregated.get('sim_tests_passed', 0)}/{aggregated.get('sim_tests_total', 0)}",
            "hw_pass": f"{aggregated.get('hw_tests_passed', 0)}/{aggregated.get('hw_tests_total', 0)}",
            "sw_pass": aggregated.get("sw_pass", ""),
            "validation_level": aggregated.get("validation_level", ""),
        }

        with open(path, "a", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=self.CSV_HEADERS)
            if write_header:
                writer.writeheader()
            writer.writerow(row)
