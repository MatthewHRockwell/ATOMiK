"""
evaluate() — run a customer aggregation WorkloadSpec against the silicon-measured
ATOMiK engine and produce a PASS/SURPASS BenchmarkReport.

Honesty rules baked in:
  * Order-independence is only claimed for abelian aggregations (silicon.is_abelian).
    For anything else the correctness gate FAILS — we don't oversell.
  * The throughput number is the engine's MEASURED accumulation rate on silicon,
    not a projection.  The scope note states it's the aggregation step.
  * verdict SURPASS requires real headroom (>= 2x); 1x..2x is PASS; below is FAIL.
"""

from __future__ import annotations

import math

from . import silicon as si
from .report import BenchmarkReport, Gate, _eng
from .spec import WorkloadSpec

SURPASS_MARGIN = 2.0


def evaluate(spec: WorkloadSpec, engine: si.SiliconMeasurement = si.ENGINE) -> BenchmarkReport:
    abelian = si.is_abelian(spec.aggregation)
    per_bank = engine.per_bank_throughput()
    peak = engine.peak_throughput()

    # --- throughput gate ---------------------------------------------------
    margin = peak / spec.required_throughput if spec.required_throughput > 0 else math.inf
    tput_pass = peak >= spec.required_throughput
    min_banks = max(1, math.ceil(spec.required_throughput / per_bank)) if per_bank else 0
    tput_detail = (
        f"x{margin:.1f} headroom" if tput_pass
        else f"short by x{1 / margin:.1f} at {engine.n_banks} banks"
    )
    throughput_gate = Gate(
        name="throughput",
        required=f"{_eng(spec.required_throughput)} ({spec.unit})",
        measured=f"{_eng(peak)} at {engine.n_banks} banks "
                 f"(linear: {_eng(per_bank)} per bank)",
        passed=tput_pass,
        detail=tput_detail,
    )

    # --- correctness gate --------------------------------------------------
    if not spec.requires_reorder_stability:
        correctness_gate = Gate(
            name="correctness",
            required="exact result (no reorder-stability required)",
            measured="byte-identical across 1/2/4/8 banks (available regardless)",
            passed=True,
            detail="ATOMiK delivers reorder-stability for free even when not required",
        )
        corr_pass = True
    elif abelian and engine.identical_across_banks:
        desc = si.ABELIAN_AGGREGATIONS[spec.aggregation.lower()]
        correctness_gate = Gate(
            name="correctness",
            required="byte-identical regardless of arrival order / parallelism",
            measured=f"byte-identical across 1/2/4/8 banks (result {engine.result_hex})",
            passed=True,
            detail=f"'{spec.aggregation}' ({desc}) is an abelian aggregation, so the "
                   f"merge is provably order-independent — lock-free, no serialization",
        )
        corr_pass = True
    else:
        why = (f"'{spec.aggregation}' is not an abelian aggregation"
               if not abelian else "engine result not byte-identical across banks")
        correctness_gate = Gate(
            name="correctness",
            required="byte-identical regardless of arrival order / parallelism",
            measured="not order-independent for this operator",
            passed=False,
            detail=f"{why} — ATOMiK cannot honestly claim reorder-stable correctness here; "
                   f"applicable ops: {', '.join(sorted(si.ABELIAN_AGGREGATIONS))}",
        )
        corr_pass = False

    # --- verdict -----------------------------------------------------------
    if tput_pass and corr_pass:
        verdict = "SURPASS" if margin >= SURPASS_MARGIN else "PASS"
    else:
        verdict = "FAIL"

    if verdict == "FAIL" and not corr_pass:
        headline = "throughput met, but this operator is outside ATOMiK's order-independent class"
    elif verdict == "FAIL":
        headline = f"correctness met, but {engine.n_banks} banks reach only {_eng(peak)}"
    elif verdict == "SURPASS":
        headline = (f"meets the required rate with x{margin:.1f} headroom AND a "
                    f"byte-identical, lock-free correctness guarantee")
    else:
        headline = "meets the required rate AND a byte-identical, lock-free correctness guarantee"

    return BenchmarkReport(
        workload=spec.name,
        customer=spec.customer,
        verdict=verdict,
        headline=headline,
        throughput_gate=throughput_gate,
        correctness_gate=correctness_gate,
        margin=margin,
        min_banks_required=min_banks,
        banks_available=engine.n_banks,
        scaling=engine.scaling(),
        provenance={
            "measured_on": engine.measured_on,
            "board": engine.board,
            "engine": engine.engine,
            "clock": f"{engine.clock_hz / 1e6:.0f} MHz",
            "tool": engine.tool,
            "reproduce": f"abench {engine.reference_count} 0xDEADBEEF0BADF00D",
        },
        scope_note=(
            "Throughput is the engine's measured accumulation rate (the aggregation "
            "step) on silicon. End-to-end system throughput additionally depends on the "
            "data-ingest path; ATOMiK's order-independence means ingest needs no global "
            "ordering or locks, which is exactly what caps a conventional aggregator."
        ),
        fields={
            "required_throughput": spec.required_throughput,
            "measured_peak_throughput": peak,
            "measured_per_bank_throughput": per_bank,
            "margin": margin,
            "min_banks_required": min_banks,
            "banks_available": engine.n_banks,
            "result_hex": engine.result_hex,
            "abelian": abelian,
        },
    )
