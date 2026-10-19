"""
render_conditional_loi — the instrument a prospect counter-signs.

A benchmark/sync report proves the bar; the Conditional LOI turns that proof
into a commitment: if ATOMiK demonstrates the bar on a live run of the
counterparty's workload, the counterparty intends to proceed to a named next
step.  It embeds the spec + verdict, states the acceptance test, and leaves
blanks for the legal specifics.

It is the source-of-truth mirror of website/src/lib/benchmark-proof.ts
(renderConditionalLOI) — keep the two in sync.

Honesty: on a FAIL verdict the bar section states plainly that ATOMiK does NOT
commit (no LOI is manufactured for a workload that doesn't pass).
"""

from __future__ import annotations

from . import silicon as si
from .live_run import LiveRunRecord
from .report import BenchmarkReport, _eng
from .spec import WorkloadSpec
from .sync import SyncReport, SyncSpec

_BLANK = "________________________"


def render_conditional_loi(
    spec: WorkloadSpec | SyncSpec,
    report: BenchmarkReport | SyncReport,
    *,
    company: str = "",
    drafted_on: str = "____-__-__",
    live_run: LiveRunRecord | None = None,
    engine: si.SiliconMeasurement = si.ENGINE,
) -> str:
    counterparty = company.strip() if company.strip() else _BLANK
    reproduce = f"abench {engine.reference_count} 0xDEADBEEF0BADF00D"

    head = [
        "# ATOMiK — Conditional Letter of Intent (DRAFT)",
        "",
        "**This is a non-binding draft for discussion.** Binding terms require a "
        "mutually executed agreement, and any proprietary workload data requires an "
        "NDA and an agreed evaluation scope.",
        "",
        f"- **Drafted:** {drafted_on}",
        f'- **Between:** Rockwell Intelligence ("ATOMiK") and **{counterparty}** ("Counterparty")',
        f"- **Workload:** {spec.name}",
        "",
        "## 1. Purpose",
        "This letter records a conditional commitment: if ATOMiK demonstrates the bar "
        "in Section 3 on a live run of the Counterparty's workload on ATOMiK silicon "
        "(Section 4), the Counterparty intends to proceed to the next step in Section 5.",
        "",
    ]

    if isinstance(spec, WorkloadSpec) and isinstance(report, BenchmarkReport):
        body = _aggregation_body(spec, report, engine, reproduce)
    elif isinstance(spec, SyncSpec) and isinstance(report, SyncReport):
        body = _sync_body(spec, report, reproduce)
    else:
        raise TypeError("spec and report must both be aggregation or both sync")

    witnessed: list[str] = []
    if live_run is not None:
        witnessed = [
            "## 4a. Silicon backing — witnessed run",
            f"- Source: **{live_run.source}** ({live_run.run_on}) on {live_run.board}",
            f"- Engine sustained **{_eng(live_run.peak_throughput)}** "
            f"({_eng(live_run.per_bank_throughput)} per bank) at {live_run.n_banks} banks.",
            f"- Merged result **{live_run.result_hex}**, "
            f"{'byte-identical' if live_run.identical_across_banks else 'NOT identical'} "
            "across 1 / 2 / 4 / 8 banks — the order-independence proof.",
            f"- Reproduce: `{live_run.reproduce}`",
            "",
            "This run witnesses the engine ceiling and order-independence that back the bar. "
            "The acceptance test in Section 4 re-runs at the Counterparty's operator and scale.",
            "",
        ]

    tail = [
        *witnessed,
        "## 5. Counterparty's conditional commitment",
        "Upon a PASS, the Counterparty intends in good faith to proceed to (select one):",
        "- [ ] Paid pilot / evaluation",
        "- [ ] Design-partner LOI",
        "- [ ] Purchase order / commercial terms",
        "",
        "within **____ days** of the acceptance test.",
        "",
        "## 6. Scope & limitations (read before signing)",
        "- The throughput commitment covers the **engine's accumulation / merge step "
        "measured on silicon**, not end-to-end system throughput, which depends on the "
        "Counterparty's ingest path.",
        "- The correctness / order-independence guarantee applies **only to abelian "
        "operators**. For order-dependent operators ATOMiK makes no reorder-stable claim.",
        "- Any sync bandwidth figure **scales with change density** and is not a universal "
        "multiplier.",
        "- This draft is **non-binding**; binding terms require a mutually executed agreement.",
        "",
        "## 7. Provenance",
        f"- Engine: {engine.engine}",
        f"- Measured: {engine.measured_on} on {engine.board}",
        f"- Byte-identical result hash across banks: {engine.result_hex}",
        f"- Reproduce: {reproduce}",
        "",
        "## 8. Signatures",
        "",
        "ATOMiK / Rockwell Intelligence — Matthew Rockwell   "
        "____________________   Date ________",
        "",
        f"{counterparty} — ____________________   Date ________",
    ]

    return "\n".join(head + body + tail)


def _aggregation_body(
    spec: WorkloadSpec,
    report: BenchmarkReport,
    engine: si.SiliconMeasurement,
    reproduce: str,
) -> list[str]:
    commits = report.verdict != "FAIL"
    peak = report.fields.get("measured_peak_throughput", engine.peak_throughput())
    per_bank = report.fields.get("measured_per_bank_throughput", engine.per_bank_throughput())
    abelian = report.fields.get("abelian", False)

    if commits:
        bar = (
            f"ATOMiK commits that the parallel-bank delta-state accumulator will sustain "
            f"**>= {_eng(spec.required_throughput)}** at **byte-identical correctness across "
            f"1 / 2 / 4 / 8 banks** for the **{spec.aggregation}** aggregation, on the "
            f"{engine.board} at {engine.clock_hz / 1e6:.0f} MHz."
        )
    else:
        bar = (
            f"On the current spec ATOMiK does **NOT** commit: '{spec.aggregation}' is outside "
            f"the order-independent (abelian) class, so ATOMiK will not claim reorder-stable "
            f"correctness for it. Re-scope the operator or drop the reorder requirement to proceed."
        )

    verdict_line = f"- Site-computed verdict vs the measured engine: **{report.verdict}**"
    if report.throughput_gate.passed:
        verdict_line += (
            f" — {report.margin:.1f}x headroom "
            f"({report.min_banks_required} of {report.banks_available} banks)."
        )
    else:
        verdict_line += "."

    return [
        "## 2. The workload",
        f"- Aggregation operator: **{spec.aggregation}**"
        + (" (abelian — order-independent)" if abelian else " (order-dependent)"),
        f"- Required throughput to commit: **{_eng(spec.required_throughput)}**",
        "- Correctness requirement: **"
        + (
            "byte-identical regardless of arrival order / parallelism"
            if spec.requires_reorder_stability
            else "exact result"
        )
        + "**",
        "",
        "## 3. ATOMiK's commitment (the bar)",
        bar,
        "",
        verdict_line,
        f"- Measured engine ceiling: **{_eng(peak)}** across {report.banks_available} banks "
        f"({_eng(per_bank)} per bank).",
        "",
        "## 4. Acceptance test",
        "A PASS is a live on-silicon run at the Counterparty's scale where BOTH hold:",
        "1. **Throughput:** sustained accumulation rate >= the required throughput in Section 3.",
        "2. **Correctness:** the merged result is byte-identical across 1 / 2 / 4 / 8 banks "
        "(the order-independence proof).",
        "",
        f"Witness / reproduce the reference run: `{reproduce}`. The Counterparty's run uses "
        "their operator and scale.",
        "",
    ]


def _sync_body(spec: SyncSpec, report: SyncReport, reproduce: str) -> list[str]:
    full = spec.full_state_bytes
    delta = spec.delta_bytes
    pct = report.reduction_pct
    commits = report.verdict == "CONVERGES"

    if commits:
        bar = (
            f"ATOMiK commits that replicas exchanging delta-state **converge to byte-identical "
            f"state with no coordinator or consensus** — any node, any order, after any partition "
            f"(the same order-independence proven on silicon) — and that a sync tick moves "
            f"**{delta} B vs {full} B** of full state (**~{pct}% less**) at {spec.change_density}."
        )
    else:
        bar = (
            f"On the current spec ATOMiK does **NOT** commit a bandwidth reduction: the stated "
            f"delta ({delta} B) is not smaller than full state ({full} B). Re-scope the change "
            f"density to proceed."
        )

    return [
        "## 2. The workload",
        "- Pattern: replica / state convergence by delta exchange",
        f"- Conventional full-state sync: **{full} B/tick**",
        f"- ATOMiK delta sync: **{delta} B/tick** at {spec.change_density}",
        "",
        "## 3. ATOMiK's commitment (the bar)",
        bar,
        "",
        "## 4. Acceptance test",
        "A PASS is a live on-silicon run where BOTH hold:",
        "1. **Convergence:** replicas merge to a byte-identical result regardless of delta "
        "order / partition.",
        "2. **Data moved:** the delta tick is materially smaller than full-state sync at the "
        "Counterparty's change density.",
        "",
        f"Witness / reproduce the order-independence: `{reproduce}`.",
        "",
    ]
