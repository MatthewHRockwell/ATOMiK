"""
Metrics Report Generation

Generates formatted pipeline reports in JSON and human-readable
text formats.
"""

from __future__ import annotations

import csv
import json
from pathlib import Path
from typing import Any


class MetricsReporter:
    """Generates pipeline metrics reports."""

    def format_text_report(
        self, schema_name: str, metrics: dict[str, Any]
    ) -> str:
        """Generate a formatted text report for stdout."""
        lines = []
        lines.append(f"ATOMiK Pipeline Report -- {schema_name}")
        lines.append("=" * (28 + len(schema_name)))
        lines.append("")

        # Pipeline Efficiency
        lines.append("Pipeline Efficiency")
        lines.append(f"  Total time:           {metrics.get('pipeline_total_time_ms', 0):,.0f} ms")
        lines.append(f"  Tokens consumed:      {metrics.get('tokens_consumed', 0):,}")
        tokens_saved = metrics.get("tokens_saved", 0)
        if tokens_saved:
            lines.append(f"  Tokens saved:         {tokens_saved:,} (differential)")
        lines.append(f"  Token efficiency:     {metrics.get('token_efficiency_pct', 100):.0f}%")
        lines.append(f"  Files generated:      {metrics.get('files_generated', 0)}")
        lines.append(f"  Lines of code:        {metrics.get('lines_generated', 0):,}")
        lines.append("")

        # Hardware Validation
        val_level = metrics.get("validation_level", "none")
        level_display = {
            "hw_validated": "HW_VALIDATED",
            "hw_programmed": "HW_PROGRAMMED",
            "synthesized": "SYNTHESIZED",
            "simulation_only": "SIM_ONLY",
            "sw_verified": "SW_VERIFIED",
            "none": "NONE",
        }
        lines.append(f"Hardware Validation     [{level_display.get(val_level, val_level)}]")
        sim_p = metrics.get("sim_tests_passed", 0)
        sim_t = metrics.get("sim_tests_total", 0)
        lines.append(f"  RTL simulation:       {sim_p}/{sim_t} tests passed")

        hw_p = metrics.get("hw_tests_passed", 0)
        hw_t = metrics.get("hw_tests_total", 0)
        if hw_t > 0:
            lines.append(f"  On-device tests:      {hw_p}/{hw_t} tests passed")
        lines.append("")

        # Synthesis Metrics
        if metrics.get("lut_pct") or metrics.get("lut_utilization_pct"):
            lines.append("Synthesis Metrics")
            lut = metrics.get("lut_utilization_pct", metrics.get("lut_pct", "N/A"))
            ff = metrics.get("ff_utilization_pct", metrics.get("ff_pct", "N/A"))
            fmax = metrics.get("fmax_mhz", metrics.get("fmax_achieved", "N/A"))
            slack = metrics.get("timing_slack_ns", "N/A")
            lines.append(f"  LUT utilization:      {lut}%")
            lines.append(f"  FF utilization:       {ff}%")
            lines.append(f"  Fmax achieved:        {fmax} MHz")
            lines.append(f"  Timing slack:         {slack} ns")
            lines.append("")

        # Runtime Performance
        if metrics.get("ops_per_second"):
            lines.append("Runtime Performance")
            lines.append(f"  Operations/second:    {metrics['ops_per_second']:,}")
            lines.append(f"  Latency per op:       {metrics.get('latency_ns', 'N/A')} ns")
            lines.append(f"  Throughput:           {metrics.get('throughput_gbps', 'N/A')} Gbps")
            lines.append("")

        # Quality
        lines.append("Quality")
        corrections = metrics.get("self_correction_count", 0)
        lines.append(f"  Lint errors:          {metrics.get('lint_errors_found', 0)}")
        lines.append(f"  Self-corrections:     {corrections}")
        lines.append("")

        return "\n".join(lines)

    def write_json_report(
        self, path: str | Path, metrics: dict[str, Any]
    ) -> None:
        """Write metrics report as JSON."""
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(metrics, f, indent=2)

    def read_csv_history(self, csv_path: str | Path) -> list[dict[str, Any]]:
        """Read metrics history from CSV."""
        path = Path(csv_path)
        if not path.exists():
            return []

        with open(path, encoding="utf-8") as f:
            reader = csv.DictReader(f)
            return list(reader)

    def format_comparison_table(
        self, schemas: dict[str, dict[str, Any]]
    ) -> str:
        """Format a cross-schema comparison table."""
        if not schemas:
            return "No data available."

        headers = ["Metric"] + list(schemas.keys())
        metric_keys = ["time_ms", "tokens", "files", "lines", "efficiency"]
        metric_labels = {
            "time_ms": "Pipeline time (ms)",
            "tokens": "Tokens consumed",
            "files": "Files generated",
            "lines": "Lines of code",
            "efficiency": "Token efficiency (%)",
        }

        # Compute column widths
        col_widths = [max(len(h), 20) for h in headers]
        lines = []

        # Header
        header_line = " | ".join(h.ljust(w) for h, w in zip(headers, col_widths))
        lines.append(header_line)
        lines.append("-+-".join("-" * w for w in col_widths))

        # Rows
        for key in metric_keys:
            row = [metric_labels.get(key, key)]
            for schema_name in schemas:
                val = schemas[schema_name].get(key, "N/A")
                row.append(str(val))
            row_line = " | ".join(v.ljust(w) for v, w in zip(row, col_widths))
            lines.append(row_line)

        return "\n".join(lines)
