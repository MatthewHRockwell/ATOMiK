"""Tests for atomik_core.benchmark — verifies each bench_* function and CLI."""

import contextlib
import io
import json
import sys

from atomik_core.benchmark import (
    bench_bandwidth,
    bench_change_detection,
    bench_convergence,
    bench_rollback,
    bench_throughput,
    main,
)

# =========================================================================
# Individual benchmark functions
# =========================================================================

ROLLBACK_KEYS = {
    "test", "n", "trad_time_ms", "atomik_time_ms",
    "speedup", "trad_mem_bytes", "atomik_mem_bytes", "mem_reduction",
}

CHANGE_DETECTION_KEYS = {
    "test", "buf_size", "n_checks", "trad_time_ms", "atomik_time_ms",
    "speedup", "incr_trad_time_ms", "incr_atomik_time_ms", "incr_speedup",
}

CONVERGENCE_KEYS = {
    "test", "n_nodes", "n_updates",
    "trad_time_ms", "atomik_time_ms", "speedup",
}

BANDWIDTH_KEYS = {"test", "entries"}

THROUGHPUT_KEYS = {
    "test", "n", "load_ops_sec", "accum_ops_sec", "read_ops_sec",
}


def _run_silent(fn):
    """Run a benchmark function with stdout suppressed, return its result."""
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        result = fn()
    return result


def test_bench_rollback_returns_dict():
    result = _run_silent(bench_rollback)
    assert isinstance(result, dict)
    assert result.keys() >= ROLLBACK_KEYS
    assert result["test"] == "rollback"
    assert result["speedup"] > 0


def test_bench_change_detection_returns_dict():
    result = _run_silent(bench_change_detection)
    assert isinstance(result, dict)
    assert result.keys() >= CHANGE_DETECTION_KEYS
    assert result["test"] == "change_detection"
    assert result["speedup"] > 0


def test_bench_convergence_returns_dict():
    result = _run_silent(bench_convergence)
    assert isinstance(result, dict)
    assert result.keys() >= CONVERGENCE_KEYS
    assert result["test"] == "convergence"
    assert result["speedup"] > 0


def test_bench_bandwidth_returns_dict():
    result = _run_silent(bench_bandwidth)
    assert isinstance(result, dict)
    assert result.keys() >= BANDWIDTH_KEYS
    assert result["test"] == "bandwidth"
    assert isinstance(result["entries"], list)
    assert len(result["entries"]) == 4  # 4 state sizes


def test_bench_throughput_returns_dict():
    result = _run_silent(bench_throughput)
    assert isinstance(result, dict)
    assert result.keys() >= THROUGHPUT_KEYS
    assert result["test"] == "throughput"
    assert result["load_ops_sec"] > 0
    assert result["accum_ops_sec"] > 0
    assert result["read_ops_sec"] > 0


# =========================================================================
# main() integration
# =========================================================================

def test_benchmark_main_returns_list():
    """main() returns a list of dicts (one per benchmark)."""
    buf = io.StringIO()
    saved_argv = sys.argv[:]
    try:
        sys.argv = ["atomik_core.benchmark"]  # no --json flag
        with contextlib.redirect_stdout(buf):
            results = main()
    finally:
        sys.argv = saved_argv

    assert isinstance(results, list)
    assert len(results) == 5
    for r in results:
        assert isinstance(r, dict)
        assert "test" in r


def test_benchmark_json_flag():
    """--json flag produces valid JSON on stdout."""
    buf = io.StringIO()
    saved_argv = sys.argv[:]
    try:
        sys.argv = ["atomik_core.benchmark", "--json"]
        with contextlib.redirect_stdout(buf):
            main()
    finally:
        sys.argv = saved_argv

    output = buf.getvalue()
    data = json.loads(output)
    assert "benchmarks" in data
    assert isinstance(data["benchmarks"], list)
    assert len(data["benchmarks"]) == 5
    test_names = {b["test"] for b in data["benchmarks"]}
    assert test_names == {"rollback", "change_detection", "convergence", "bandwidth", "throughput"}
