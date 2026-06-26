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
from .live_run import reference_live_run, run_live
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
    p.add_argument("--live-run", action="store_true",
                   help="drive abench on a connected AX7020 and emit a witnessed live-run record")
    p.add_argument("--reference-run", action="store_true",
                   help="emit a witnessed record from the RECORDED reference measurement")
    p.add_argument("--seed", default="0xDEADBEEF0BADF00D", help="abench seed for the live run")
    p.add_argument("--out", help="write the artifact to this path instead of stdout")
    args = p.parse_args(argv)

    # sync / replica-convergence specs (kind:"sync" or carrying delta_bytes) route
    # to the sync evaluator — a different archetype class, different gates.
    if args.spec and _is_sync_spec(args.spec):
        sync_spec = SyncSpec.from_json(args.spec)
        sync_report = evaluate_sync(sync_spec)
        record = _live_record(args, sync_spec.name)
        _emit(args, _render(args, sync_spec, sync_report, record))
        return 0 if sync_report.verdict == "CONVERGES" else 2

    spec = _build_spec(args)
    engine = si.ENGINE
    if args.board:
        from .board import measure_on_board
        count = args.count or spec.events_per_window or 65537
        engine = measure_on_board(count, port=args.port)

    report = evaluate(spec, engine)
    record = _live_record(args, spec.name)
    _emit(args, _render(args, spec, report, record, engine=engine))
    # exit code: 0 = PASS/SURPASS, 2 = FAIL (so it's usable in CI / scripts)
    return 0 if report.verdict in ("PASS", "SURPASS") else 2


def _live_record(args: argparse.Namespace, workload: str):
    """A witnessed run record if requested (live board or recorded reference)."""
    if args.live_run:
        count = args.count or 65537
        return run_live(workload, count, seed=args.seed, port=args.port)
    if args.reference_run:
        return reference_live_run(workload, seed=args.seed)
    return None


def _render(args, spec, report, record, *, engine=si.ENGINE) -> str:
    if args.loi:
        return render_conditional_loi(spec, report, company=args.company,
                                      drafted_on=args.drafted_on, live_run=record, engine=engine)
    if record is not None:
        return record.to_json() if args.json else record.to_markdown()
    return report.to_json() if args.json else report.to_markdown()


def _emit(args: argparse.Namespace, text: str) -> None:
    if args.out:
        with open(args.out, "w") as fh:
            fh.write(text + "\n")
    else:
        print(text)


if __name__ == "__main__":
    sys.exit(main())
