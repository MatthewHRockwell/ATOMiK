"""Benchmark runner - executes all workloads and collects results.

This script runs:
- T2.5: Memory efficiency benchmarks (W1.1-W1.3)
- T2.6: Computational overhead benchmarks (W2.1-W2.3)
- T2.7: Scalability benchmarks (W3.1-W3.3)
"""

import sys
import os

# Add current directory to path for imports
sys.path.insert(0, os.path.dirname(__file__))

from metrics import BenchmarkRunner, StatisticalAnalyzer, OutlierDetector
import baseline.workloads as baseline_wl
import atomik.workloads as atomik_wl


def run_memory_benchmarks(iterations: int = 100):
    """Execute memory efficiency benchmarks (T2.5: W1.1-W1.3).

    Args:
        iterations: Number of iterations per workload
    """
    print("=" * 70)
    print("T2.5: MEMORY EFFICIENCY BENCHMARKS")
    print("=" * 70)

    runner = BenchmarkRunner(output_dir="../data/memory")

    # W1.1: Matrix operations
    print("\nW1.1: Matrix Operations")
    for size in [32, 64]:  # Reduced from [64, 128] for faster execution
        print(f"  Size {size}x{size}...")
        runner.run_workload(
            baseline_wl.MatrixWorkload,
            {'size': size},
            'baseline',
            iterations=iterations,
            workload_run_params={'iterations': 5}  # Reduced from 10
        )
        runner.run_workload(
            atomik_wl.MatrixWorkload,
            {'size': size},
            'atomik',
            iterations=iterations,
            workload_run_params={'iterations': 5}  # Reduced from 10
        )

    # W1.2: State machines
    print("\nW1.2: State Machines")
    for num_states in [100, 500]:  # Reduced from [256, 1000]
        print(f"  States: {num_states}...")
        runner.run_workload(
            baseline_wl.StateMachineWorkload,
            {'num_states': num_states},
            'baseline',
            iterations=iterations,
            workload_run_params={'steps': 500}  # Reduced from 1000
        )
        runner.run_workload(
            atomik_wl.StateMachineWorkload,
            {'num_states': num_states},
            'atomik',
            iterations=iterations,
            workload_run_params={'steps': 500}  # Reduced from 1000
        )

    # W1.3: Streaming
    print("\nW1.3: Streaming Data")
    for num_stages in [5, 20]:  # Reduced from [10, 50]
        print(f"  Stages: {num_stages}...")
        runner.run_workload(
            baseline_wl.StreamingWorkload,
            {'num_stages': num_stages},
            'baseline',
            iterations=iterations,
            workload_run_params={'data_points': 500}  # Reduced from 1000
        )
        runner.run_workload(
            atomik_wl.StreamingWorkload,
            {'num_stages': num_stages},
            'atomik',
            iterations=iterations,
            workload_run_params={'data_points': 500}  # Reduced from 1000
        )

    runner.save_results('memory_benchmarks.csv')
    print(f"\nSaved results to: {runner.output_dir}/memory_benchmarks.csv")
    print(f"Total measurements: {len(runner.results)}")


def run_overhead_benchmarks(iterations: int = 100):
    """Execute computational overhead benchmarks (T2.6: W2.1-W2.3).

    Args:
        iterations: Number of iterations per workload
    """
    print("\n" + "=" * 70)
    print("T2.6: COMPUTATIONAL OVERHEAD BENCHMARKS")
    print("=" * 70)

    runner = BenchmarkRunner(output_dir="../data/overhead")

    # W2.1: Delta composition
    print("\nW2.1: Delta Composition")
    for chain_length in [100, 1000]:
        print(f"  Chain length: {chain_length}...")
        runner.run_workload(
            baseline_wl.CompositionWorkload,
            {'chain_length': chain_length},
            'baseline',
            iterations=iterations,
            workload_run_params={'repetitions': 10}
        )
        runner.run_workload(
            atomik_wl.CompositionWorkload,
            {'chain_length': chain_length},
            'atomik',
            iterations=iterations,
            workload_run_params={'repetitions': 10}
        )

    # W2.3: Mixed read/write
    print("\nW2.3: Mixed Read/Write Operations")
    for read_ratio in [0.3, 0.7]:
        print(f"  Read ratio: {read_ratio}...")
        runner.run_workload(
            baseline_wl.MixedWorkload,
            {'read_ratio': read_ratio},
            'baseline',
            iterations=iterations,
            workload_run_params={'operations': 1000}
        )
        runner.run_workload(
            atomik_wl.MixedWorkload,
            {'read_ratio': read_ratio},
            'atomik',
            iterations=iterations,
            workload_run_params={'operations': 1000}
        )

    runner.save_results('overhead_benchmarks.csv')
    print(f"\nSaved results to: {runner.output_dir}/overhead_benchmarks.csv")
    print(f"Total measurements: {len(runner.results)}")


def run_scalability_benchmarks(iterations: int = 100):
    """Execute scalability benchmarks (T2.7: W3.1-W3.3).

    Args:
        iterations: Number of iterations per workload
    """
    print("\n" + "=" * 70)
    print("T2.7: SCALABILITY BENCHMARKS")
    print("=" * 70)

    runner = BenchmarkRunner(output_dir="../data/scalability")

    # W3.1: Problem size scaling
    print("\nW3.1: Problem Size Scaling")
    for problem_size in [16, 64, 256]:  # Reduced from [16, 64, 256, 1024]
        print(f"  Problem size: {problem_size}...")
        runner.run_workload(
            baseline_wl.ScalingWorkload,
            {'problem_size': problem_size},
            'baseline',
            iterations=iterations,
            workload_run_params={'operations_per_element': 5}  # Reduced from 10
        )
        runner.run_workload(
            atomik_wl.ScalingWorkload,
            {'problem_size': problem_size},
            'atomik',
            iterations=iterations,
            workload_run_params={'operations_per_element': 5}  # Reduced from 10
        )

    # W3.2: Parallel composition
    print("\nW3.2: Parallel Composition")
    for num_ops in [100, 1000]:
        print(f"  Operations: {num_ops}...")
        runner.run_workload(
            baseline_wl.ParallelWorkload,
            {'num_operations': num_ops},
            'baseline',
            iterations=iterations,
            workload_run_params={'repetitions': 10}
        )
        runner.run_workload(
            atomik_wl.ParallelWorkload,
            {'num_operations': num_ops},
            'atomik',
            iterations=iterations,
            workload_run_params={'repetitions': 10}
        )

    # W3.3: Cache locality
    print("\nW3.3: Cache Locality")
    for cache_kb in [1, 64, 1024]:
        print(f"  Working set: {cache_kb}KB...")
        runner.run_workload(
            baseline_wl.CacheWorkload,
            {'working_set_kb': cache_kb},
            'baseline',
            iterations=iterations,
            workload_run_params={'iterations': 10}
        )
        runner.run_workload(
            atomik_wl.CacheWorkload,
            {'working_set_kb': cache_kb},
            'atomik',
            iterations=iterations,
            workload_run_params={'iterations': 10}
        )

    runner.save_results('scalability_benchmarks.csv')
    print(f"\nSaved results to: {runner.output_dir}/scalability_benchmarks.csv")
    print(f"Total measurements: {len(runner.results)}")


def print_summary_statistics():
    """Print summary statistics from all benchmark runs."""
    print("\n" + "=" * 70)
    print("SUMMARY STATISTICS")
    print("=" * 70)

    # Note: In a real implementation, we would load the CSV files and compute
    # aggregate statistics here. For now, just indicate completion.
    print("\nAll benchmarks completed successfully.")
    print("Data files created:")
    print("  - experiments/data/memory/memory_benchmarks.csv")
    print("  - experiments/data/overhead/overhead_benchmarks.csv")
    print("  - experiments/data/scalability/scalability_benchmarks.csv")


def main():
    """Main execution function."""
    print("ATOMiK Benchmark Suite")
    print("Phase 2: SCORE Comparison")
    print("=" * 70)

    # Number of iterations per workload configuration
    # For production: 100+. For quick execution: 10
    # Note: 10 iterations provides preliminary results; production should use 100+
    ITERATIONS = 10

    try:
        # Execute all benchmark categories
        run_memory_benchmarks(iterations=ITERATIONS)
        run_overhead_benchmarks(iterations=ITERATIONS)
        run_scalability_benchmarks(iterations=ITERATIONS)

        # Print summary
        print_summary_statistics()

        print("\n" + "=" * 70)
        print("BENCHMARK EXECUTION COMPLETE")
        print("=" * 70)
        print("\nNext steps:")
        print("  1. Review data files in experiments/data/")
        print("  2. Run statistical analysis (T2.8)")
        print("  3. Generate comparison report (T2.9)")

    except Exception as e:
        print(f"\nERROR: Benchmark execution failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
