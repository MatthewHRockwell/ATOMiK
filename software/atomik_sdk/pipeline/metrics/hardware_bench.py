"""
Hardware Benchmark Runner

Collects FPGA synthesis metrics (utilization, timing) and runtime
performance data (throughput, latency) from hardware test runs.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .collector import MetricsCollector


class HardwareBenchmark:
    """
    Collects hardware metrics from synthesis reports and runtime
    benchmark output.
    """

    # Tang Nano 9K (GW1NR-9) device capacities
    DEVICE_CAPACITY = {
        "lut_total": 8640,
        "ff_total": 6693,
        "bsram_total": 26,
        "cls_total": 4320,
    }

    def __init__(self) -> None:
        self.collector = MetricsCollector()

    def parse_synthesis_report(self, report_path: str | Path) -> dict[str, Any]:
        """Parse a Gowin synthesis report for utilization and timing metrics."""
        path = Path(report_path)
        if not path.exists():
            return {}

        content = path.read_text(encoding="utf-8", errors="replace")
        metrics: dict[str, Any] = {}

        # Parse LUT usage
        lut_match = re.search(r"Total\s+Logic\s+Elements[:\s]+(\d+)/(\d+)", content)
        if lut_match:
            used, total = int(lut_match.group(1)), int(lut_match.group(2))
            metrics["lut_used"] = used
            metrics["lut_available"] = total
            metrics["lut_utilization_pct"] = round(100 * used / total, 1)
            self.collector.record_hardware(
                lut_used=used,
                lut_utilization=round(100 * used / total, 1),
            )

        # Parse FF usage
        ff_match = re.search(r"Total\s+Registers[:\s]+(\d+)/(\d+)", content)
        if ff_match:
            used, total = int(ff_match.group(1)), int(ff_match.group(2))
            metrics["ff_used"] = used
            metrics["ff_available"] = total
            metrics["ff_utilization_pct"] = round(100 * used / total, 1)
            self.collector.record_hardware(
                ff_used=used,
                ff_utilization=round(100 * used / total, 1),
            )

        # Parse Fmax
        fmax_match = re.search(r"Max\s+Frequency[:\s]+([\d.]+)\s*MHz", content)
        if fmax_match:
            fmax = float(fmax_match.group(1))
            metrics["fmax_mhz"] = fmax
            self.collector.record_hardware(fmax_achieved=fmax)

        # Parse timing slack
        slack_match = re.search(r"Slack[:\s]+([-\d.]+)\s*ns", content)
        if slack_match:
            slack = float(slack_match.group(1))
            metrics["timing_slack_ns"] = slack
            metrics["timing_met"] = slack >= 0
            self.collector.record_hardware(
                timing_slack_ns=slack,
                timing_met=slack >= 0,
            )

        return metrics

    def compute_runtime_metrics(
        self, fmax_mhz: float, data_width: int
    ) -> dict[str, Any]:
        """Compute runtime performance metrics from synthesis results."""
        metrics: dict[str, Any] = {}

        if fmax_mhz > 0:
            ops_per_sec = fmax_mhz * 1e6  # Single-cycle operation
            latency_ns = 1000 / fmax_mhz  # ns per operation
            throughput_gbps = data_width * fmax_mhz / 1000  # Gbps

            metrics["ops_per_second"] = int(ops_per_sec)
            metrics["latency_ns"] = round(latency_ns, 2)
            metrics["throughput_gbps"] = round(throughput_gbps, 2)

            self.collector.record_runtime(
                ops_per_second=int(ops_per_sec),
                latency_ns=round(latency_ns, 2),
                throughput_gbps=round(throughput_gbps, 2),
            )

        return metrics

    def get_phase3_comparison(self, current: dict[str, Any]) -> dict[str, Any]:
        """Compare current metrics against Phase 3 baseline."""
        baseline = {
            "fmax_mhz": 94.9,
            "lut_pct": 7,
            "ff_pct": 9,
            "ops_per_second": 94_500_000,
            "latency_ns": 10.6,
        }

        comparison: dict[str, Any] = {}
        for key, base_val in baseline.items():
            curr_val = current.get(key, current.get(f"{key}_achieved"))
            if curr_val is not None and isinstance(curr_val, (int, float)):
                diff = curr_val - base_val
                pct = round(100 * diff / base_val, 1) if base_val else 0
                comparison[key] = {
                    "baseline": base_val,
                    "current": curr_val,
                    "delta": round(diff, 2),
                    "delta_pct": pct,
                }

        return comparison

    def get_phase6_comparison(
        self, current: dict[str, Any], n_banks: int = 4
    ) -> dict[str, Any]:
        """Compare current metrics against Phase 6 parallel bank baseline.

        Phase 6 v2.0 baseline (syn_keep/syn_preserve optimized, GW1NR-9):
          - N=4 @ 81.0 MHz: 324 Mops/sec, 8.5% LUT, 0 ALU in accumulator
          - N=8 @ 67.5 MHz: 540 Mops/sec, 13.0% LUT, 0 ALU in accumulator
          - Latency: 1 cycle (constant, independent of N)
          - Hardware validated: 60/60 UART tests across 6 configs

        Args:
            current: Dict of current measured metrics.
            n_banks: Number of parallel banks in the configuration.

        Returns:
            Comparison dict with baseline, current, delta, and delta_pct
            for each metric.
        """
        # Per-bank scaling: throughput scales linearly with N
        # Max timing-met Fmax per bank count (v2.0 hardware-validated)
        fmax_per_n = {1: 94.5, 2: 94.5, 4: 81.0, 8: 67.5}
        fmax_baseline = fmax_per_n.get(n_banks, 94.5)
        baseline = {
            "fmax_mhz": fmax_baseline,
            "lut_pct": {1: 5.5, 2: 7.1, 4: 8.6, 8: 13.0}.get(n_banks, 8.6),
            "ops_per_second": int(fmax_baseline * 1e6 * n_banks),
            "throughput_mops": fmax_baseline * n_banks,
            "latency_cycles": 1,
            "n_banks": n_banks,
        }

        comparison: dict[str, Any] = {"n_banks": n_banks}
        for key, base_val in baseline.items():
            if key == "n_banks":
                continue
            curr_val = current.get(key, current.get(f"{key}_achieved"))
            if curr_val is not None and isinstance(curr_val, (int, float)):
                diff = curr_val - base_val
                pct = round(100 * diff / base_val, 1) if base_val else 0
                comparison[key] = {
                    "baseline": base_val,
                    "current": curr_val,
                    "delta": round(diff, 2),
                    "delta_pct": pct,
                }

        return comparison
