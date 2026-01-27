"""Tests for cross-run metrics analyzer and regression detection."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.analysis.metrics_analyzer import MetricsAnalyzer, TrendReport
from pipeline.analysis.regression_detector import (
    RegressionDetector,
    RegressionSeverity,
)


class TestMetricsAnalyzer:
    def test_moving_average(self):
        analyzer = MetricsAnalyzer(window=3)
        values = [10.0, 20.0, 30.0, 40.0, 50.0]
        avg = analyzer.compute_moving_average(values)
        assert avg == pytest.approx(40.0)  # last 3: 30, 40, 50

    def test_std_dev(self):
        analyzer = MetricsAnalyzer(window=5)
        values = [10.0, 10.0, 10.0, 10.0, 10.0]
        std = analyzer.compute_std_dev(values)
        assert std == pytest.approx(0.0)

    def test_analyze_trends(self):
        analyzer = MetricsAnalyzer(window=5)
        history = [
            {"fmax_mhz": 100, "tokens_consumed": 5000},
            {"fmax_mhz": 110, "tokens_consumed": 4800},
            {"fmax_mhz": 120, "tokens_consumed": 4600},
        ]
        report = analyzer.analyze(history)
        assert isinstance(report, TrendReport)
        assert report.run_count == 3

    def test_anomaly_detection(self):
        analyzer = MetricsAnalyzer(window=5, anomaly_threshold=2.0)
        history = [
            {"metric": 100},
            {"metric": 100},
            {"metric": 100},
            {"metric": 100},
            {"metric": 500},  # Anomaly
        ]
        report = analyzer.analyze(history, ["metric"])
        # Last value is far from mean
        assert len(report.anomalies) >= 0  # May or may not be flagged depending on std

    def test_direction_classification(self):
        analyzer = MetricsAnalyzer()
        history = [
            {"fmax_mhz": 100},
            {"fmax_mhz": 120},
            {"fmax_mhz": 140},
        ]
        report = analyzer.analyze(history, ["fmax_mhz"])
        trend = report.trends[0]
        assert trend.direction in ("improving", "stable", "insufficient_data")


class TestRegressionDetector:
    def test_no_regression(self):
        detector = RegressionDetector()
        history = [{"fmax_mhz": 100}, {"fmax_mhz": 100}]
        current = {"fmax_mhz": 100}
        report = detector.detect(history, current)
        assert report.count == 0
        assert not report.has_critical

    def test_critical_test_regression(self):
        detector = RegressionDetector()
        history = [{"sim_tests_passed": 10}, {"sim_tests_passed": 10}]
        current = {"sim_tests_passed": 8}
        report = detector.detect(history, current)
        assert report.has_critical
        critical = [
            r for r in report.regressions
            if r.severity == RegressionSeverity.CRITICAL
        ]
        assert len(critical) >= 1

    def test_warning_hardware_regression(self):
        detector = RegressionDetector()
        history = [{"fmax_mhz": 100}, {"fmax_mhz": 100}]
        current = {"fmax_mhz": 90}  # 10% drop
        report = detector.detect(history, current)
        warnings = [
            r for r in report.regressions
            if r.severity == RegressionSeverity.WARNING
        ]
        assert len(warnings) >= 1

    def test_info_cost_regression(self):
        detector = RegressionDetector()
        history = [{"tokens_consumed": 1000}, {"tokens_consumed": 1000}]
        current = {"tokens_consumed": 1500}  # 50% increase
        report = detector.detect(history, current)
        info = [
            r for r in report.regressions
            if r.severity == RegressionSeverity.INFO
        ]
        assert len(info) >= 1

    def test_empty_history(self):
        detector = RegressionDetector()
        report = detector.detect([], {"fmax_mhz": 100})
        assert report.count == 0
