"""Unit tests for metrics framework."""

import unittest
import os
import sys
import tempfile
import shutil

sys.path.insert(0, os.path.dirname(__file__))

from metrics import (
    BenchmarkResult,
    StatisticalSummary,
    MetricsCollector,
    BenchmarkRunner,
    StatisticalAnalyzer,
    OutlierDetector,
    generate_comparison_table,
)


class DummyWorkload:
    """Dummy workload for testing."""

    def __init__(self, size: int = 10):
        self.size = size

    def run(self, iterations: int = 10):
        # Simulate some work
        total = 0
        for i in range(iterations * self.size):
            total += i
        return {
            'workload': 'dummy',
            'total_reads': iterations * 10,
            'total_writes': iterations * 5,
            'memory_bytes': 1000,
        }


class TestMetricsCollector(unittest.TestCase):
    """Test MetricsCollector."""

    def test_time_measurement(self):
        """Test execution time measurement."""
        collector = MetricsCollector()
        collector.start(track_memory=False)

        # Do some work
        total = sum(range(1000))

        exec_time, peak_mem = collector.stop()

        self.assertGreater(exec_time, 0.0)
        self.assertEqual(peak_mem, 0)

    def test_memory_tracking(self):
        """Test memory tracking."""
        collector = MetricsCollector()
        collector.start(track_memory=True)

        # Allocate some memory
        data = [0] * 10000

        exec_time, peak_mem = collector.stop()

        self.assertGreater(exec_time, 0.0)
        self.assertGreater(peak_mem, 0)


class TestStatisticalAnalyzer(unittest.TestCase):
    """Test StatisticalAnalyzer."""

    def test_compute_summary(self):
        """Test statistical summary computation."""
        values = [10.0, 12.0, 11.0, 13.0, 10.5]
        summary = StatisticalAnalyzer.compute_summary(values, 'test_metric')

        self.assertEqual(summary.metric_name, 'test_metric')
        self.assertAlmostEqual(summary.mean, 11.3, places=1)
        self.assertEqual(summary.sample_count, 5)
        self.assertGreater(summary.confidence_interval_95, 0.0)

    def test_empty_summary(self):
        """Test summary with empty data."""
        summary = StatisticalAnalyzer.compute_summary([], 'empty')
        self.assertEqual(summary.mean, 0.0)
        self.assertEqual(summary.sample_count, 0)

    def test_welch_t_test(self):
        """Test Welch's t-test."""
        # Two clearly different samples
        sample1 = [10.0, 11.0, 12.0, 13.0, 14.0]
        sample2 = [20.0, 21.0, 22.0, 23.0, 24.0]

        t_stat, p_value = StatisticalAnalyzer.welch_t_test(sample1, sample2)

        # Should be statistically significant
        self.assertLess(p_value, 0.05)
        self.assertNotEqual(t_stat, 0.0)

    def test_t_test_identical_samples(self):
        """Test t-test with identical samples."""
        sample = [10.0, 10.0, 10.0, 10.0]
        t_stat, p_value = StatisticalAnalyzer.welch_t_test(sample, sample)

        # Should NOT be significant
        self.assertGreater(p_value, 0.05)

    def test_compare_variants(self):
        """Test variant comparison."""
        baseline_results = [
            BenchmarkResult('test', 'baseline', 100.0, 1000, 10, 5000, {}),
            BenchmarkResult('test', 'baseline', 110.0, 1100, 10, 5500, {}),
            BenchmarkResult('test', 'baseline', 105.0, 1050, 10, 5250, {}),
        ]

        atomik_results = [
            BenchmarkResult('test', 'atomik', 80.0, 800, 10, 4000, {}),
            BenchmarkResult('test', 'atomik', 85.0, 850, 10, 4250, {}),
            BenchmarkResult('test', 'atomik', 82.0, 820, 10, 4100, {}),
        ]

        comparison = StatisticalAnalyzer.compare_variants(
            baseline_results, atomik_results, 'execution_time_ms'
        )

        self.assertEqual(comparison['metric'], 'execution_time_ms')
        self.assertGreater(comparison['improvement_ratio'], 0.0)
        self.assertTrue(comparison['atomik_faster'])


class TestOutlierDetector(unittest.TestCase):
    """Test OutlierDetector."""

    def test_outlier_detection(self):
        """Test outlier detection with modified Z-score."""
        values = [10.0, 11.0, 12.0, 11.5, 10.5, 100.0]  # 100.0 is outlier
        outliers = OutlierDetector.modified_z_score(values)

        # Last value should be detected as outlier
        self.assertTrue(outliers[-1])
        self.assertFalse(any(outliers[:-1]))

    def test_no_outliers(self):
        """Test detection with no outliers."""
        values = [10.0, 11.0, 12.0, 11.5, 10.5]
        outliers = OutlierDetector.modified_z_score(values)

        self.assertFalse(any(outliers))

    def test_remove_outliers(self):
        """Test outlier removal from results."""
        results = [
            BenchmarkResult('test', 'baseline', 10.0, 1000, 10, 5000, {}),
            BenchmarkResult('test', 'baseline', 11.0, 1100, 10, 5500, {}),
            BenchmarkResult('test', 'baseline', 12.0, 1200, 10, 6000, {}),
            BenchmarkResult('test', 'baseline', 100.0, 10000, 10, 50000, {}),  # Outlier
        ]

        filtered = OutlierDetector.remove_outliers(results, 'execution_time_ms')

        # Should remove the outlier
        self.assertEqual(len(filtered), 3)


class TestBenchmarkRunner(unittest.TestCase):
    """Test BenchmarkRunner."""

    def setUp(self):
        """Create temporary directory for test outputs."""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up temporary directory."""
        shutil.rmtree(self.temp_dir)

    def test_run_workload(self):
        """Test running a workload."""
        runner = BenchmarkRunner(output_dir=self.temp_dir)

        results = runner.run_workload(
            DummyWorkload,
            {'size': 10},
            'baseline',
            iterations=5
        )

        self.assertEqual(len(results), 5)
        self.assertEqual(results[0].variant, 'baseline')
        self.assertGreater(results[0].execution_time_ms, 0.0)

    def test_save_results(self):
        """Test saving results to CSV."""
        runner = BenchmarkRunner(output_dir=self.temp_dir)

        runner.run_workload(
            DummyWorkload,
            {'size': 10},
            'baseline',
            iterations=3
        )

        runner.save_results('test_results.csv')

        # Check file exists
        filepath = os.path.join(self.temp_dir, 'test_results.csv')
        self.assertTrue(os.path.exists(filepath))


class TestComparisonTable(unittest.TestCase):
    """Test comparison table generation."""

    def test_generate_table(self):
        """Test markdown table generation."""
        baseline_results = [
            BenchmarkResult('test', 'baseline', 100.0, 1000, 10, 5000, {}),
            BenchmarkResult('test', 'baseline', 110.0, 1100, 10, 5500, {}),
        ]

        atomik_results = [
            BenchmarkResult('test', 'atomik', 80.0, 800, 10, 4000, {}),
            BenchmarkResult('test', 'atomik', 85.0, 850, 10, 4250, {}),
        ]

        table = generate_comparison_table(baseline_results, atomik_results)

        # Check table contains expected elements
        self.assertIn('Metric', table)
        self.assertIn('Baseline', table)
        self.assertIn('ATOMiK', table)
        self.assertIn('execution_time_ms', table)
        self.assertIn('p-value', table)


if __name__ == '__main__':
    unittest.main()
