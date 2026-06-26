"""
CLI: turn a customer workload spec into a PASS/SURPASS benchmark proof.

    python -m atomik_sdk.contract_bench.cli spec.json            # Markdown
    python -m atomik_sdk.contract_bench.cli spec.json --json     # JSON for the website
    python -m atomik_sdk.contract_bench.cli --required 50e6 --agg sum --name "Rollup"
"""

from __future__ import annotations

import argparse
import json
import sys

from . import silicon as si
from .harness import evaluate
from .loi import render_conditional_loi
from .spec import WorkloadSpec
from .sync import SyncSpec, evaluate_sync


def _is_sync_spec(path: str) -> bool:
    try:
        data = json.loads(open(path).read())
    except Exception:
        return False
    return data.get("kind") == "sync" or "delta_bytes" in data


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
    p.add_argument("--loi", action="store_true",
                   help="emit a Conditional LOI (draft) instead of the proof report")
    p.add_argument("--company", default="", help="counterparty name for the LOI")
    p.add_argument("--drafted-on", default="____-__-__", help="LOI draft date (YYYY-MM-DD)")
    args = p.parse_args(argv)

    # sync / replica-convergence specs (kind:"sync" or carrying delta_bytes) route
    # to the sync evaluator — a different archetype class, different gates.
    if args.spec and _is_sync_spec(args.spec):
        sync_spec = SyncSpec.from_json(args.spec)
        sync_report = evaluate_sync(sync_spec)
        if args.loi:
            print(render_conditional_loi(sync_spec, sync_report,
                                         company=args.company, drafted_on=args.drafted_on))
        else:
            print(sync_report.to_json() if args.json else sync_report.to_markdown())
        return 0 if sync_report.verdict == "CONVERGES" else 2

    spec = _build_spec(args)
    engine = si.ENGINE
    if args.board:
        from .board import measure_on_board
        count = args.count or spec.events_per_window or 65537
        engine = measure_on_board(count, port=args.port)

    report = evaluate(spec, engine)
    if args.loi:
        print(render_conditional_loi(spec, report, company=args.company,
                                     drafted_on=args.drafted_on, engine=engine))
    else:
        print(report.to_json() if args.json else report.to_markdown())
    # exit code: 0 = PASS/SURPASS, 2 = FAIL (so it's usable in CI / scripts)
    return 0 if report.verdict in ("PASS", "SURPASS") else 2


if __name__ == "__main__":
    sys.exit(main())
