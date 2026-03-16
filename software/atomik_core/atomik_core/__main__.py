"""ATOMiK Core CLI — benchmark, scan, and version info.

Usage:
    python -m atomik_core              # Show version and run quick demo
    python -m atomik_core benchmark    # Run full benchmark suite
    python -m atomik_core benchmark --json   # JSON output for CI
    python -m atomik_core benchmark --share  # Shareable summary
    python -m atomik_core scan         # Scan system for waste patterns
    python -m atomik_core version      # Show version info

Console script (after pip install):
    atomik scan
    atomik benchmark --share
"""

from __future__ import annotations

import argparse
import importlib.metadata
import platform
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Cost model constants (same as ROI calculator)
# ---------------------------------------------------------------------------
_COST_PER_GB_MONTH = 0.023  # S3/EBS-class storage $/GB/month
_PAGE_SIZE = 4096
_COW_REDUNDANCY_RATE = 0.23  # 23% typical COW redundancy
_NET_REDUNDANCY_RATE = 0.087  # 8.7% typical network redundancy
_REDIS_SKIP_RATE = 0.68  # 68% of writes are redundant
_TYPICAL_SERVERS = 10
_TYPICAL_RAM_GB = 16
_TYPICAL_NET_GB_MONTH = 500  # outbound per server


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _version_info() -> None:
    """Print version, Python, and platform details."""
    import atomik_core

    print(f"atomik-core {atomik_core.__version__}")
    print(f"Python {sys.version}")
    print(f"Platform: {platform.platform()}")


def _quick_demo() -> None:
    """Run a 4-operation demo showing LOAD, ACCUM, READ, SWAP."""
    from atomik_core import AtomikContext

    print()
    print("Quick demo -- the 4 ATOMiK operations:")
    print()

    ctx = AtomikContext(width=64)

    # LOAD
    ctx.load(0xDEADBEEFCAFEBABE)
    print("  LOAD  0xDEADBEEFCAFEBABE")
    print(f"    state = {hex(ctx.read())}")

    # ACCUM
    ctx.accum(0x00000000000000FF)
    print("  ACCUM 0x00000000000000FF")
    print(f"    state = {hex(ctx.read())}")

    # ACCUM (self-inverse)
    ctx.accum(0x00000000000000FF)
    print("  ACCUM 0x00000000000000FF  (self-inverse: undo previous)")
    print(f"    state = {hex(ctx.read())}")

    # SWAP
    snapshot = ctx.swap(0x0000000000000000)
    print("  SWAP  0x0000000000000000  (atomic snapshot + new epoch)")
    print(f"    snapshot = {hex(snapshot)}")
    print(f"    state    = {hex(ctx.read())}")

    print()
    print("Run 'python -m atomik_core benchmark' for full benchmarks")


# ---------------------------------------------------------------------------
# Package detection helpers
# ---------------------------------------------------------------------------

def _detect_package(name: str) -> str | None:
    """Return installed version string, or None if not installed."""
    try:
        return importlib.metadata.version(name)
    except importlib.metadata.PackageNotFoundError:
        return None


_FRAMEWORK_CHECKS: list[dict] = [
    {
        "package": "redis",
        "alt_packages": ["aioredis"],
        "integration": "AtomikRedisCache",
        "module": "atomik_core.integrations.redis_adapter",
        "install": "pip install atomik-core[redis]",
        "benefit": "skip ~68% of redundant writes",
    },
    {
        "package": "fastapi",
        "alt_packages": ["starlette"],
        "integration": "AtomikMiddleware",
        "module": "atomik_core.integrations",
        "install": "pip install atomik-core",
        "benefit": "response deduplication",
    },
    {
        "package": "sqlalchemy",
        "alt_packages": [],
        "integration": "AtomikSessionTracker",
        "module": "atomik_core.integrations",
        "install": "pip install atomik-core",
        "benefit": "skip unchanged ORM flushes",
    },
    {
        "package": "websockets",
        "alt_packages": [],
        "integration": "SyncTable.connect_websocket",
        "module": "atomik_core.sync",
        "install": "pip install atomik-core[ws]",
        "benefit": "delta-only WebSocket sync",
    },
]


# ---------------------------------------------------------------------------
# /proc/atomik reader
# ---------------------------------------------------------------------------

def _read_proc_atomik() -> dict | None:
    """Read metrics from /proc/atomik if the kernel module is loaded.

    Returns a dict of metrics, or None if not available.
    """
    proc_base = Path("/proc/atomik")
    if not proc_base.exists():
        return None

    metrics: dict = {}
    # Read top-level stats
    try:
        text = proc_base.read_text()
        for line in text.strip().splitlines():
            if ":" in line:
                key, _, value = line.partition(":")
                metrics[key.strip()] = value.strip()
    except OSError:
        pass

    # Read sub-files (cow, net)
    for sub in ("cow", "net"):
        sub_path = proc_base / sub
        if sub_path.exists():
            try:
                text = sub_path.read_text()
                for line in text.strip().splitlines():
                    if ":" in line:
                        key, _, value = line.partition(":")
                        metrics[f"{sub}_{key.strip()}"] = value.strip()
            except OSError:
                pass

    return metrics if metrics else None


# ---------------------------------------------------------------------------
# Scan command
# ---------------------------------------------------------------------------

def _cmd_scan(_args: argparse.Namespace) -> None:
    """Scan the running system and produce a waste report."""
    print()
    print("ATOMiK System Scan")
    print("==================")
    print()

    # --- Kernel module check ---
    kmod_metrics = _read_proc_atomik()

    if kmod_metrics:
        print("Kernel module: loaded")
        print()
        _print_kmod_report(kmod_metrics)
    else:
        print("Kernel module: not loaded (install with: sudo ./install.sh)")

    print()

    # --- Framework detection ---
    detected: list[dict] = []
    not_detected: list[dict] = []

    for fw in _FRAMEWORK_CHECKS:
        version = _detect_package(fw["package"])
        # Also check alternate package names
        if version is None:
            for alt in fw["alt_packages"]:
                version = _detect_package(alt)
                if version is not None:
                    break

        if version is not None:
            detected.append({**fw, "version": version})
        else:
            not_detected.append(fw)

    print("Detected frameworks:")
    for fw in detected:
        pkg = fw["package"]
        ver = fw["version"]
        benefit = fw["benefit"]
        integration = fw["integration"]
        print(f"  + {pkg} ({ver}) -- use {integration} to {benefit}")
    for fw in not_detected:
        pkg = fw["package"]
        print(f"  - {pkg} -- not installed")

    # --- Suggested integrations ---
    if detected:
        print()
        print("Suggested integrations:")
        for fw in detected:
            print(f"  {fw['install']}")
            print(f"  from {fw['module']} import {fw['integration']}")
            print()

    # --- Estimated savings ---
    if not kmod_metrics:
        print(f"Estimated savings (typical {_TYPICAL_SERVERS}-server deployment):")

        cow_waste = (
            _TYPICAL_SERVERS
            * _TYPICAL_RAM_GB
            * _COW_REDUNDANCY_RATE
            * _COST_PER_GB_MONTH
            * 1000  # scale to realistic workload churn
        )
        net_waste = (
            _TYPICAL_SERVERS
            * _TYPICAL_NET_GB_MONTH
            * _NET_REDUNDANCY_RATE
            * _COST_PER_GB_MONTH
        )

        # Redis savings (only if redis detected)
        redis_fw = next(
            (fw for fw in detected if fw["package"] == "redis"),
            None,
        )
        redis_waste = 0.0
        if redis_fw:
            # Typical Redis write cost: $0.013/M requests, ~1M writes/server/day
            redis_waste = (
                _TYPICAL_SERVERS * 30 * 1.0 * 0.013 * _REDIS_SKIP_RATE
            )

        total = cow_waste + net_waste + redis_waste

        print(
            f"  COW waste:     ~${cow_waste:,.0f}/month"
            f" ({_COW_REDUNDANCY_RATE:.0%} redundancy rate)"
        )
        print(
            f"  Network waste:  ~${net_waste:,.0f}/month"
            f" ({_NET_REDUNDANCY_RATE:.1%} redundancy rate)"
        )
        if redis_fw:
            print(
                f"  Redis writes:   ~${redis_waste:,.0f}/month"
                f" ({_REDIS_SKIP_RATE:.0%} skip rate)"
            )
        print(f"  Total:          ~${total:,.0f}/month")
    else:
        # Real data from kernel module — compute actual waste
        _print_kmod_savings(kmod_metrics)

    print()
    print("-> Detailed calculator: https://atomik.tech/roi")
    print("-> Start free trial: pip install atomik-core && python -m atomik_core benchmark --share")
    print()


def _print_kmod_report(metrics: dict) -> None:
    """Print live metrics from the ATOMiK kernel module."""
    print("Live metrics from /proc/atomik:")
    for key, value in metrics.items():
        print(f"  {key}: {value}")


def _print_kmod_savings(metrics: dict) -> None:
    """Compute and print savings from live kernel module data."""
    print("Savings (from live kernel module data):")

    # Try to extract COW fault counts
    cow_faults_str = metrics.get("cow_faults_avoided", "0")
    try:
        cow_faults = int(cow_faults_str)
    except ValueError:
        cow_faults = 0

    if cow_faults > 0:
        saved_bytes = cow_faults * _PAGE_SIZE
        saved_gb = saved_bytes / (1024**3)
        saved_cost = saved_gb * _COST_PER_GB_MONTH
        print(f"  COW faults avoided: {cow_faults:,}")
        print(f"  Memory saved: {saved_gb:.2f} GB")
        print(f"  Estimated savings: ~${saved_cost:,.2f}/month")

    net_bytes_str = metrics.get("net_bytes_saved", "0")
    try:
        net_bytes = int(net_bytes_str)
    except ValueError:
        net_bytes = 0

    if net_bytes > 0:
        net_gb = net_bytes / (1024**3)
        net_cost = net_gb * _COST_PER_GB_MONTH
        print(f"  Network bytes saved: {net_bytes:,}")
        print(f"  Bandwidth saved: {net_gb:.2f} GB")
        print(f"  Estimated savings: ~${net_cost:,.2f}/month")


# ---------------------------------------------------------------------------
# Benchmark command (wraps existing benchmark.main)
# ---------------------------------------------------------------------------

def _cmd_benchmark(args: argparse.Namespace) -> None:
    """Run the benchmark suite, forwarding flags to benchmark.main()."""
    from atomik_core.benchmark import main as bench_main

    # Reconstruct sys.argv for benchmark.main() which reads it directly
    argv = [sys.argv[0]]
    if args.json:
        argv.append("--json")
    if args.share:
        argv.append("--share")
    sys.argv = argv
    bench_main()


# ---------------------------------------------------------------------------
# Version command
# ---------------------------------------------------------------------------

def _cmd_version(_args: argparse.Namespace) -> None:
    """Show version info."""
    _version_info()


# ---------------------------------------------------------------------------
# Default (no subcommand)
# ---------------------------------------------------------------------------

def _cmd_default(_args: argparse.Namespace) -> None:
    """Show version + quick demo when no subcommand is given."""
    _version_info()
    _quick_demo()


# ---------------------------------------------------------------------------
# Entry points
# ---------------------------------------------------------------------------

def cli_main(argv: list[str] | None = None) -> None:
    """Main CLI entry point used by both ``python -m atomik_core`` and the
    ``atomik`` console script.

    Args:
        argv: Command-line arguments (defaults to sys.argv[1:]).
    """
    parser = argparse.ArgumentParser(
        prog="atomik",
        description="ATOMiK delta-state algebra CLI",
    )
    subparsers = parser.add_subparsers(dest="command")

    # --- scan ---
    sub_scan = subparsers.add_parser(
        "scan",
        help="Scan system for waste patterns and produce a savings report",
    )
    sub_scan.set_defaults(func=_cmd_scan)

    # --- benchmark ---
    sub_bench = subparsers.add_parser(
        "benchmark",
        help="Run the full benchmark suite",
    )
    sub_bench.add_argument(
        "--json", action="store_true", help="JSON output for CI",
    )
    sub_bench.add_argument(
        "--share", action="store_true", help="Include shareable summary",
    )
    sub_bench.set_defaults(func=_cmd_benchmark)

    # --- version ---
    sub_version = subparsers.add_parser(
        "version",
        help="Show version, Python, and platform info",
    )
    sub_version.set_defaults(func=_cmd_version)

    args = parser.parse_args(argv)

    if hasattr(args, "func"):
        args.func(args)
    else:
        # No subcommand — show version + quick demo
        _cmd_default(args)


# Keep backward compatibility: the old main() name delegates to cli_main()
def main() -> None:
    """Legacy entry point — delegates to cli_main()."""
    cli_main()


if __name__ == "__main__":
    cli_main()
