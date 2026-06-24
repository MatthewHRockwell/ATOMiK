"""
CLI: turn a customer workload spec into a PASS/SURPASS benchmark proof.

    python -m atomik_sdk.contract_bench.cli spec.json            # Markdown
    python -m atomik_sdk.contract_bench.cli spec.json --json     # JSON for the website
    python -m atomik_sdk.contract_bench.cli --required 50e6 --agg sum --name "Rollup"
"""

from __future__ import annotations

import argparse
import sys

from . import silicon as si
from .harness import evaluate
from .spec import WorkloadSpec


def _build_spec(args: argparse.Namespace) -> WorkloadSpec:
    if args.spec:
        return WorkloadSpec.from_json(args.spec)
    if args.required is None:
        raise SystemExit("provide a spec file, or --required and --agg")
    return WorkloadSpec(
        name=args.name or "workload",
        aggregation=args.agg,
        required_throughput=float(args.required),
        customer=args.customer or "",
    )


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="ATOMiK aggregation benchmark proof")
    p.add_argument("spec", nargs="?", help="workload spec JSON")
    p.add_argument("--required", help="required throughput, events/s (e.g. 50e6)")
    p.add_argument("--agg", default="sum", help="aggregation op (sum/xor/count/...)")
    p.add_argument("--name", help="workload name")
    p.add_argument("--customer", help="customer / prospect name")
    p.add_argument("--json", action="store_true", help="emit JSON instead of Markdown")
    p.add_argument("--board", action="store_true",
                   help="measure LIVE on a connected AX7020 (runs abench at the workload scale)")
    p.add_argument("--count", type=int,
                   help="delta count for the live run (default: events_per_window or 65537)")
    p.add_argument("--port", default="/dev/ttyUSB2", help="board serial port")
    args = p.parse_args(argv)

    spec = _build_spec(args)
    engine = si.ENGINE
    if args.board:
        from .board import measure_on_board
        count = args.count or spec.events_per_window or 65537
        engine = measure_on_board(count, port=args.port)

    report = evaluate(spec, engine)
    print(report.to_json() if args.json else report.to_markdown())
    # exit code: 0 = PASS/SURPASS, 2 = FAIL (so it's usable in CI / scripts)
    return 0 if report.verdict in ("PASS", "SURPASS") else 2


if __name__ == "__main__":
    sys.exit(main())
