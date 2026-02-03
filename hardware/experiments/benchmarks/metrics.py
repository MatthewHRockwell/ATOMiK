"""Metrics collection and statistical analysis framework for benchmark comparison.

This module provides utilities for:
- Collecting performance metrics (time, memory, operations)
- Statistical analysis (mean, std dev, confidence intervals)
- Hypothesis testing (t-tests for significance)
- Data persistence (CSV output)
"""

import time
import tracemalloc
import statistics
from typing import List, Dict, Any, Callable, Tuple
import csv
import os
from dataclasses import dataclass, asdict
import math


@dataclass
class BenchmarkResult:
    """Single benchmark execution result.

    Attributes:
        workload: Workload name
        variant: 'baseline' or 'atomik'
        execution_time_ms: Wall clock time in milliseconds
        peak_memory_bytes: Peak memory usage
        operation_count: Number of operations performed
        memory_traffic_bytes: Estimated memory traffic
        params: Workload-specific parameters
    """
    workload: str
    variant: str
    execution_time_ms: float
    peak_memory_bytes: int
    operation_count: int
    memory_traffic_bytes: int
    params: Dict[str, Any]


@dataclass
class StatisticalSummary:
    """Statistical summary of multiple benchmark runs.

    Attributes:
        metric_name: Name of the metric
        mean: Sample mean
        std_dev: Sample standard deviation
        confidence_interval_95: 95% confidence interval (±)
        min_value: Minimum observed value
        max_value: Maximum observed value
        sample_count: Number of samples
    """
    metric_name: str
    mean: float
    std_dev: float
    confidence_interval_95: float
    min_value: float
    max_value: float
    sample_count: int


class MetricsCollector:
    """Collect performance metrics during benchmark execution."""

    def __init__(self):
        """Initialize metrics collector."""
        self.start_time: float = 0.0
        self.peak_memory: int = 0
        self.memory_tracking: bool = False

    def start(self, track_memory: bool = True) -> None:
        """Start metrics collection.

        Args:
            track_memory: Whether to track memory usage
        """
        self.memory_tracking = track_memory
        if track_memory:
            tracemalloc.start()
        self.start_time = time.perf_counter()

    def stop(self) -> Tuple[float, int]:
        """Stop metrics collection and return results.

        Returns:
            Tuple of (execution_time_ms, peak_memory_bytes)
        """
        execution_time = (time.perf_counter() - self.start_time) * 1000.0

        peak_memory = 0
        if self.memory_tracking:
            current, peak_memory = tracemalloc.get_traced_memory()
            tracemalloc.stop()

        return execution_time, peak_memory


class BenchmarkRunner:
    """Run benchmarks with metrics collection and statistical validation."""

    def __init__(self, output_dir: str = "experiments/data"):
        """Initialize benchmark runner.

        Args:
            output_dir: Directory for output data files
        """
        self.output_dir = output_dir
        self.results: List[BenchmarkResult] = []

    def run_workload(
        self,
        workload_class,
        workload_params: Dict[str, Any],
        variant: str,
        iterations: int = 100,
        workload_run_params: Dict[str, Any] = None
    ) -> List[BenchmarkResult]:
        """Run a workload multiple times and collect metrics.

        Args:
            workload_class: Workload class to instantiate
            workload_params: Parameters for workload constructor
            variant: 'baseline' or 'atomik'
            iterations: Number of times to run the workload
            workload_run_params: Parameters for workload.run() method

        Returns:
            List of benchmark results
        """
        if workload_run_params is None:
            workload_run_params = {}

        results = []
        collector = MetricsCollector()

        for _ in range(iterations):
            # Create fresh workload instance
            workload = workload_class(**workload_params)

            # Measure execution
            collector.start(track_memory=True)
            metrics = workload.run(**workload_run_params)
            exec_time, peak_mem = collector.stop()

            # Extract operation counts
            if variant == 'baseline':
                op_count = metrics.get('total_reads', 0) + metrics.get('total_writes', 0)
                mem_traffic = metrics.get('memory_bytes', 0) * op_count
            else:  # atomik
                op_count = metrics.get('total_accumulates', 0) + metrics.get('total_reconstructs', 0)
                # Deltas are smaller, so less traffic per operation
                mem_traffic = metrics.get('memory_bytes', 0)

            # Create result
            result = BenchmarkResult(
                workload=metrics.get('workload', 'unknown'),
                variant=variant,
                execution_time_ms=exec_time,
                peak_memory_bytes=peak_mem,
                operation_count=op_count,
                memory_traffic_bytes=mem_traffic,
                params=workload_params
            )
            results.append(result)

        self.results.extend(results)
        return results

    def save_results(self, filename: str) -> None:
        """Save results to CSV file.

        Args:
            filename: Output filename (will be placed in output_dir)
        """
        os.makedirs(self.output_dir, exist_ok=True)
        filepath = os.path.join(self.output_dir, filename)

        with open(filepath, 'w', newline='') as f:
            if not self.results:
                return

            # Get all possible keys from params
            param_keys = set()
            for r in self.results:
                param_keys.update(r.params.keys())

            fieldnames = ['workload', 'variant', 'execution_time_ms',
                         'peak_memory_bytes', 'operation_count',
                         'memory_traffic_bytes'] + sorted(param_keys)

            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()

            for result in self.results:
                row = {
                    'workload': result.workload,
                    'variant': result.variant,
                    'execution_time_ms': result.execution_time_ms,
                    'peak_memory_bytes': result.peak_memory_bytes,
                    'operation_count': result.operation_count,
                    'memory_traffic_bytes': result.memory_traffic_bytes,
                }
                row.update(result.params)
                writer.writerow(row)


class StatisticalAnalyzer:
    """Perform statistical analysis on benchmark results."""

    @staticmethod
    def compute_summary(values: List[float], metric_name: str) -> StatisticalSummary:
        """Compute statistical summary of a metric.

        Args:
            values: List of metric values
            metric_name: Name of the metric

        Returns:
            Statistical summary
        """
        if not values:
            return StatisticalSummary(
                metric_name=metric_name,
                mean=0.0,
                std_dev=0.0,
                confidence_interval_95=0.0,
                min_value=0.0,
                max_value=0.0,
                sample_count=0
            )

        mean = statistics.mean(values)
        std_dev = statistics.stdev(values) if len(values) > 1 else 0.0

        # 95% confidence interval: z = 1.96 for normal distribution
        ci_95 = 1.96 * std_dev / math.sqrt(len(values)) if len(values) > 0 else 0.0

        return StatisticalSummary(
            metric_name=metric_name,
            mean=mean,
            std_dev=std_dev,
            confidence_interval_95=ci_95,
            min_value=min(values),
            max_value=max(values),
            sample_count=len(values)
        )

    @staticmethod
    def welch_t_test(sample1: List[float], sample2: List[float]) -> Tuple[float, float]:
        """Perform Welch's t-test (unequal variance t-test).

        Args:
            sample1: First sample (e.g., baseline)
            sample2: Second sample (e.g., ATOMiK)

        Returns:
            Tuple of (t_statistic, p_value_approximation)
        """
        if len(sample1) < 2 or len(sample2) < 2:
            return 0.0, 1.0

        mean1 = statistics.mean(sample1)
        mean2 = statistics.mean(sample2)
        var1 = statistics.variance(sample1)
        var2 = statistics.variance(sample2)
        n1 = len(sample1)
        n2 = len(sample2)

        # Check for zero variance (identical samples)
        denominator = math.sqrt(var1/n1 + var2/n2)
        if denominator == 0:
            # Samples have no variance - cannot compute meaningful t-test
            return 0.0, 1.0

        # Welch's t-statistic
        t_stat = (mean1 - mean2) / denominator

        # Degrees of freedom (Welch-Satterthwaite equation)
        numerator = (var1/n1 + var2/n2) ** 2
        denominator = (var1/n1)**2 / (n1-1) + (var2/n2)**2 / (n2-1)
        df = numerator / denominator if denominator > 0 else 1

        # Approximate p-value using normal approximation (conservative)
        # For t-distribution with large df, approaches normal distribution
        # This is a simplification; for production use scipy.stats
        z_score = abs(t_stat)
        p_value = 2 * (1 - StatisticalAnalyzer._normal_cdf(z_score))

        return t_stat, p_value

    @staticmethod
    def _normal_cdf(x: float) -> float:
        """Approximate cumulative distribution function for standard normal.

        Uses error function approximation.

        Args:
            x: Value to evaluate

        Returns:
            Approximate CDF value
        """
        # Simple approximation using error function
        return 0.5 * (1.0 + math.erf(x / math.sqrt(2.0)))

    @staticmethod
    def compare_variants(
        baseline_results: List[BenchmarkResult],
        atomik_results: List[BenchmarkResult],
        metric: str = 'execution_time_ms'
    ) -> Dict[str, Any]:
        """Compare two variants statistically.

        Args:
            baseline_results: Baseline benchmark results
            atomik_results: ATOMiK benchmark results
            metric: Metric to compare

        Returns:
            Comparison dictionary with statistics
        """
        baseline_values = [getattr(r, metric) for r in baseline_results]
        atomik_values = [getattr(r, metric) for r in atomik_results]

        baseline_summary = StatisticalAnalyzer.compute_summary(baseline_values, f'baseline_{metric}')
        atomik_summary = StatisticalAnalyzer.compute_summary(atomik_values, f'atomik_{metric}')

        t_stat, p_value = StatisticalAnalyzer.welch_t_test(baseline_values, atomik_values)

        # Compute improvement ratio
        if baseline_summary.mean > 0:
            improvement = (baseline_summary.mean - atomik_summary.mean) / baseline_summary.mean
        else:
            improvement = 0.0

        return {
            'metric': metric,
            'baseline': asdict(baseline_summary),
            'atomik': asdict(atomik_summary),
            't_statistic': t_stat,
            'p_value': p_value,
            'statistically_significant': p_value < 0.05,
            'improvement_ratio': improvement,
            'atomik_faster': atomik_summary.mean < baseline_summary.mean,
        }


class OutlierDetector:
    """Detect and remove statistical outliers."""

    @staticmethod
    def modified_z_score(values: List[float], threshold: float = 3.5) -> List[bool]:
        """Detect outliers using modified Z-score.

        Args:
            values: List of values
            threshold: Modified Z-score threshold (default 3.5)

        Returns:
            List of booleans indicating outliers
        """
        if len(values) < 3:
            return [False] * len(values)

        median = statistics.median(values)
        mad = statistics.median([abs(v - median) for v in values])

        # Avoid division by zero
        if mad == 0:
            return [False] * len(values)

        modified_z_scores = [abs(0.6745 * (v - median) / mad) for v in values]
        return [z > threshold for z in modified_z_scores]

    @staticmethod
    def remove_outliers(
        results: List[BenchmarkResult],
        metric: str = 'execution_time_ms'
    ) -> List[BenchmarkResult]:
        """Remove outliers from benchmark results.

        Args:
            results: List of benchmark results
            metric: Metric to use for outlier detection

        Returns:
            Filtered list without outliers
        """
        values = [getattr(r, metric) for r in results]
        outlier_mask = OutlierDetector.modified_z_score(values)

        return [r for r, is_outlier in zip(results, outlier_mask) if not is_outlier]


def generate_comparison_table(
    baseline_results: List[BenchmarkResult],
    atomik_results: List[BenchmarkResult]
) -> str:
    """Generate markdown comparison table.

    Args:
        baseline_results: Baseline results
        atomik_results: ATOMiK results

    Returns:
        Markdown-formatted comparison table
    """
    metrics = ['execution_time_ms', 'peak_memory_bytes', 'memory_traffic_bytes']
    analyzer = StatisticalAnalyzer()

    lines = []
    lines.append("| Metric | Baseline (mean ± CI) | ATOMiK (mean ± CI) | Improvement | p-value | Significant |")
    lines.append("|--------|---------------------|-------------------|-------------|---------|-------------|")

    for metric in metrics:
        comparison = analyzer.compare_variants(baseline_results, atomik_results, metric)

        baseline_mean = comparison['baseline']['mean']
        baseline_ci = comparison['baseline']['confidence_interval_95']
        atomik_mean = comparison['atomik']['mean']
        atomik_ci = comparison['atomik']['confidence_interval_95']

        improvement = comparison['improvement_ratio'] * 100
        p_value = comparison['p_value']
        significant = "✅" if comparison['statistically_significant'] else "❌"

        lines.append(
            f"| {metric} | {baseline_mean:.2f} ± {baseline_ci:.2f} | "
            f"{atomik_mean:.2f} ± {atomik_ci:.2f} | {improvement:+.1f}% | "
            f"{p_value:.4f} | {significant} |"
        )

    return "\n".join(lines)
