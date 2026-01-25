"""Statistical analysis of benchmark results.

This script:
1. Loads benchmark data from CSV files
2. Performs statistical analysis
3. Generates comparison tables
4. Creates visualizations (requires matplotlib)
"""

import sys
import os
import csv
from collections import defaultdict
from typing import List, Dict, Any

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'benchmarks'))

from metrics import (
    BenchmarkResult,
    StatisticalAnalyzer,
    OutlierDetector,
    generate_comparison_table,
)


def load_csv_data(filepath: str) -> List[BenchmarkResult]:
    """Load benchmark results from CSV file.

    Args:
        filepath: Path to CSV file

    Returns:
        List of BenchmarkResult objects
    """
    results = []

    with open(filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            # Extract params (all columns except the main ones)
            params = {}
            for key, value in row.items():
                if key not in ['workload', 'variant', 'execution_time_ms',
                              'peak_memory_bytes', 'operation_count', 'memory_traffic_bytes']:
                    try:
                        # Try to convert to number
                        if '.' in value:
                            params[key] = float(value)
                        else:
                            params[key] = int(value)
                    except (ValueError, AttributeError):
                        params[key] = value

            result = BenchmarkResult(
                workload=row['workload'],
                variant=row['variant'],
                execution_time_ms=float(row['execution_time_ms']),
                peak_memory_bytes=int(row['peak_memory_bytes']),
                operation_count=int(row['operation_count']),
                memory_traffic_bytes=int(row['memory_traffic_bytes']),
                params=params
            )
            results.append(result)

    return results


def group_by_workload(results: List[BenchmarkResult]) -> Dict[str, Dict[str, List[BenchmarkResult]]]:
    """Group results by workload and variant.

    Args:
        results: List of benchmark results

    Returns:
        Nested dict: {workload: {variant: [results]}}
    """
    grouped = defaultdict(lambda: defaultdict(list))

    for result in results:
        grouped[result.workload][result.variant].append(result)

    return dict(grouped)


def analyze_category(category_name: str, csv_file: str) -> str:
    """Analyze a benchmark category and generate report.

    Args:
        category_name: Name of the category
        csv_file: Path to CSV data file

    Returns:
        Markdown-formatted analysis report
    """
    print(f"\nAnalyzing {category_name}...")

    # Load data
    results = load_csv_data(csv_file)
    print(f"  Loaded {len(results)} measurements")

    # Remove outliers
    baseline_results = [r for r in results if r.variant == 'baseline']
    atomik_results = [r for r in results if r.variant == 'atomik']

    baseline_filtered = OutlierDetector.remove_outliers(baseline_results, 'execution_time_ms')
    atomik_filtered = OutlierDetector.remove_outliers(atomik_results, 'execution_time_ms')

    outliers_removed = (len(baseline_results) - len(baseline_filtered) +
                        len(atomik_results) - len(atomik_filtered))
    print(f"  Removed {outliers_removed} outliers")

    # Group by workload
    grouped = group_by_workload(baseline_filtered + atomik_filtered)

    # Generate report
    lines = []
    lines.append(f"# {category_name} Analysis\n")
    lines.append(f"**Total Measurements**: {len(results)}")
    lines.append(f"**Outliers Removed**: {outliers_removed}")
    lines.append(f"**Workloads**: {len(grouped)}\n")

    # Analyze each workload
    for workload_name, variants in grouped.items():
        lines.append(f"## {workload_name.replace('_', ' ').title()}\n")

        if 'baseline' not in variants or 'atomik' not in variants:
            lines.append("**Error**: Missing baseline or ATOMiK variant\n")
            continue

        baseline_wl = variants['baseline']
        atomik_wl = variants['atomik']

        lines.append(f"**Sample Size**: Baseline={len(baseline_wl)}, ATOMiK={len(atomik_wl)}\n")

        # Compare metrics
        metrics = ['execution_time_ms', 'peak_memory_bytes', 'memory_traffic_bytes']

        for metric in metrics:
            comparison = StatisticalAnalyzer.compare_variants(
                baseline_wl, atomik_wl, metric
            )

            baseline_summary = comparison['baseline']
            atomik_summary = comparison['atomik']
            improvement = comparison['improvement_ratio'] * 100
            p_value = comparison['p_value']
            significant = "✅ Yes" if comparison['statistically_significant'] else "❌ No"

            lines.append(f"### {metric.replace('_', ' ').title()}\n")
            lines.append(f"- **Baseline**: {baseline_summary['mean']:.2f} ± {baseline_summary['confidence_interval_95']:.2f}")
            lines.append(f"- **ATOMiK**: {atomik_summary['mean']:.2f} ± {atomik_summary['confidence_interval_95']:.2f}")
            lines.append(f"- **Improvement**: {improvement:+.1f}%")
            lines.append(f"- **p-value**: {p_value:.4f}")
            lines.append(f"- **Statistically Significant**: {significant}\n")

        # Overall comparison table
        lines.append("### Comparison Table\n")
        table = generate_comparison_table(baseline_wl, atomik_wl)
        lines.append(table + "\n")

    return "\n".join(lines)


def main():
    """Main analysis function."""
    print("=" * 70)
    print("ATOMiK Benchmark Analysis (T2.8)")
    print("=" * 70)

    # Create output directory
    output_dir = os.path.dirname(__file__)
    os.makedirs(output_dir, exist_ok=True)

    # Analyze each category
    categories = [
        ("Memory Efficiency Benchmarks", "../data/memory/memory_benchmarks.csv"),
        ("Computational Overhead Benchmarks", "../data/overhead/overhead_benchmarks.csv"),
        ("Scalability Benchmarks", "../data/scalability/scalability_benchmarks.csv"),
    ]

    all_analyses = []

    for category_name, csv_file in categories:
        csv_path = os.path.join(output_dir, csv_file)

        if not os.path.exists(csv_path):
            print(f"Warning: {csv_path} not found, skipping...")
            continue

        analysis = analyze_category(category_name, csv_path)
        all_analyses.append(analysis)

    # Save combined analysis
    output_file = os.path.join(output_dir, "statistics.md")
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("# ATOMiK Benchmark Statistical Analysis\n\n")
        f.write("**Date**: January 24, 2026\n")
        f.write("**Phase**: 2 - SCORE Comparison\n")
        f.write("**Task**: T2.8\n\n")
        f.write("---\n\n")

        for analysis in all_analyses:
            f.write(analysis)
            f.write("\n---\n\n")

    print(f"\n{'=' * 70}")
    print(f"Analysis complete!")
    print(f"Report saved to: {output_file}")
    print(f"{'=' * 70}\n")

    # Print summary
    print("Summary:")
    print("- Statistical analysis complete for all benchmark categories")
    print("- Outlier detection and removal applied")
    print("- 95% confidence intervals computed")
    print("- Statistical significance tests performed (Welch's t-test)")
    print("\nNote: Visualization (plots) requires matplotlib/seaborn")
    print("      Install with: pip install matplotlib seaborn")


if __name__ == '__main__':
    main()
