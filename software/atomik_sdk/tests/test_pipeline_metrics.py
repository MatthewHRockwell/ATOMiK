"""
Tests for ATOMiK Pipeline Metrics Framework

Tests cover:
- Metrics collector API
- Hardware benchmark parsing
- Pipeline benchmark recording
- Metrics reporter formatting
- CSV export
- Cross-schema comparison
"""

import json
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.metrics.collector import MetricsCollector
from pipeline.metrics.hardware_bench import HardwareBenchmark
from pipeline.metrics.pipeline_bench import PipelineBenchmark
from pipeline.metrics.reporter import MetricsReporter


class TestMetricsCollector:
    def test_create_collector(self):
        collector = MetricsCollector()
        assert collector.get_all() == []

    def test_record_metric(self):
        collector = MetricsCollector()
        collector.record("test_metric", 42, unit="count", category="pipeline")
        entries = collector.get_all()
        assert len(entries) == 1
        assert entries[0]["name"] == "test_metric"
        assert entries[0]["value"] == 42

    def test_record_pipeline(self):
        collector = MetricsCollector()
        collector.record_pipeline(tokens=0, time_ms=100)
        by_cat = collector.get_by_category("pipeline")
        assert len(by_cat) == 2

    def test_record_hardware(self):
        collector = MetricsCollector()
        collector.record_hardware(lut_pct=7, ff_pct=9, fmax=94.9)
        summary = collector.get_summary()
        assert "hardware" in summary
        assert summary["hardware"]["fmax"] == 94.9

    def test_record_runtime(self):
        collector = MetricsCollector()
        collector.record_runtime(ops_per_second=94_500_000)
        flat = collector.to_flat_dict()
        assert flat["ops_per_second"] == 94_500_000

    def test_record_quality(self):
        collector = MetricsCollector()
        collector.record_quality(tests_passed=10, tests_total=10)
        summary = collector.get_summary()
        assert summary["quality"]["tests_passed"] == 10

    def test_merge_collectors(self):
        c1 = MetricsCollector()
        c1.record("a", 1)
        c2 = MetricsCollector()
        c2.record("b", 2)
        c1.merge(c2)
        assert len(c1.get_all()) == 2

    def test_clear(self):
        collector = MetricsCollector()
        collector.record("a", 1)
        collector.clear()
        assert len(collector.get_all()) == 0


class TestHardwareBenchmark:
    def test_create_benchmark(self):
        bench = HardwareBenchmark()
        assert bench.DEVICE_CAPACITY["lut_total"] == 8640

    def test_compute_runtime_metrics(self):
        bench = HardwareBenchmark()
        metrics = bench.compute_runtime_metrics(fmax_mhz=94.9, data_width=64)
        assert metrics["ops_per_second"] == 94_900_000
        assert metrics["latency_ns"] > 0
        assert metrics["throughput_gbps"] > 0

    def test_phase3_comparison(self):
        bench = HardwareBenchmark()
        current = {"fmax_mhz": 95.0, "lut_pct": 8}
        comparison = bench.get_phase3_comparison(current)
        assert "fmax_mhz" in comparison
        assert comparison["fmax_mhz"]["baseline"] == 94.9
        assert comparison["fmax_mhz"]["current"] == 95.0
        assert comparison["fmax_mhz"]["delta"] == pytest.approx(0.1, abs=0.01)

    def test_runtime_zero_fmax(self):
        bench = HardwareBenchmark()
        metrics = bench.compute_runtime_metrics(fmax_mhz=0, data_width=64)
        assert metrics == {}


class TestPipelineBenchmark:
    def test_record_run(self):
        bench = PipelineBenchmark()
        metrics = bench.record_run(
            schema_name="video",
            total_time_ms=4500,
            tokens_consumed=0,
            tokens_saved=12000,
            files_generated=19,
            lines_generated=850,
            self_corrections=0,
            self_correction_successes=0,
        )
        assert metrics["token_efficiency_pct"] == 100.0
        assert metrics["cost_per_line"] == 0

    def test_nonzero_tokens(self):
        bench = PipelineBenchmark()
        metrics = bench.record_run(
            schema_name="test",
            total_time_ms=5000,
            tokens_consumed=2000,
            tokens_saved=10000,
            files_generated=19,
            lines_generated=850,
            self_corrections=1,
            self_correction_successes=1,
        )
        # efficiency = 10000 / (10000 + 2000) = 83.3%
        assert metrics["token_efficiency_pct"] == pytest.approx(83.3, abs=0.1)
        assert metrics["cost_per_line"] == pytest.approx(2.35, abs=0.01)

    def test_compare_schemas(self):
        bench = PipelineBenchmark()
        runs = [
            {"schema": "video", "pipeline_total_time_ms": 4500, "files_generated": 19},
            {"schema": "sensor", "pipeline_total_time_ms": 3800, "files_generated": 19},
        ]
        comparison = bench.compare_schemas(runs)
        assert "video" in comparison
        assert "sensor" in comparison
        assert comparison["video"]["time_ms"] == 4500


class TestMetricsReporter:
    def test_text_report(self):
        reporter = MetricsReporter()
        report = reporter.format_text_report("video-h264-delta", {
            "pipeline_total_time_ms": 4500,
            "tokens_consumed": 0,
            "token_efficiency_pct": 100,
            "files_generated": 19,
            "lines_generated": 850,
            "validation_level": "hw_validated",
            "sim_tests_passed": 10,
            "sim_tests_total": 10,
            "hw_tests_passed": 10,
            "hw_tests_total": 10,
            "lint_errors_found": 0,
            "self_correction_count": 0,
        })
        assert "video-h264-delta" in report
        assert "Pipeline Efficiency" in report
        assert "Hardware Validation" in report
        assert "HW_VALIDATED" in report

    def test_comparison_table(self):
        reporter = MetricsReporter()
        schemas = {
            "video": {"time_ms": 4500, "tokens": 0, "files": 19, "lines": 850, "efficiency": 100},
            "sensor": {"time_ms": 3800, "tokens": 0, "files": 19, "lines": 720, "efficiency": 100},
        }
        table = reporter.format_comparison_table(schemas)
        assert "video" in table
        assert "sensor" in table
        assert "Pipeline time" in table

    def test_empty_comparison(self):
        reporter = MetricsReporter()
        table = reporter.format_comparison_table({})
        assert "No data" in table

    def test_json_report(self, tmp_path):
        reporter = MetricsReporter()
        path = tmp_path / "report.json"
        reporter.write_json_report(path, {"schema": "test", "success": True})
        assert path.exists()
        data = json.loads(path.read_text())
        assert data["success"] is True

    def test_csv_history(self, tmp_path):
        import csv
        csv_path = tmp_path / "history.csv"
        with open(csv_path, "w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=["schema", "tokens"])
            writer.writeheader()
            writer.writerow({"schema": "video", "tokens": "0"})
            writer.writerow({"schema": "sensor", "tokens": "0"})

        reporter = MetricsReporter()
        history = reporter.read_csv_history(csv_path)
        assert len(history) == 2
        assert history[0]["schema"] == "video"
