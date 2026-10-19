"""
Cost-of-Lock value translator — make the bar economically legible, honestly.

  * ATOMiK's capability is MEASURED (silicon.py engine, byte-identical) — never a
    fabricated speedup multiplier.
  * The "savings" are arithmetic on the BUYER'S OWN stated baseline (their
    instances, their $/mo, the overhead THEY attribute to coordination).
  * What ATOMiK removes is the coordination tax for correctness-under-concurrency
    — an architectural fact for abelian merges (no lock is needed), not a
    "we're faster" claim.  Every line is tagged MEASURED or YOUR BASELINE.

Source-of-truth mirror of website/src/lib/cost-of-lock.ts.
"""

from __future__ import annotations

import json
from dataclasses import asdict, dataclass, field

from . import silicon as si
from .harness import evaluate
from .report import _eng
from .spec import WorkloadSpec

APPROACH_LABEL = {
    "lock": "lock contention / serialized hot path",
    "shard": "shard fan-out maintained to avoid contention",
    "crdt": "CRDT metadata + reconciliation overhead",
    "consensus": "consensus / coordinator round-trip overhead",
}


@dataclass
class ValueInputs:
    aggregation: str
    required_throughput: float
    approach: str = "lock"
    reorder: bool = True
    instances: int | None = None
    instance_monthly: float | None = None
    coordination_overhead_pct: float | None = None
    p99_ms: float | None = None
    p99_lock_induced: bool = False


@dataclass
class ValueMemo:
    applicable: bool
    reason: str
    headline: str
    approach_label: str
    min_banks: int
    banks_available: int
    margin: float
    peak_throughput: float
    monthly_baseline: float | None
    coordination_tax_monthly: float | None
    reclaimed_pct: float | None
    lines: list[dict]  # {label, value, source}
    caveats: list[str] = field(default_factory=list)

    def to_dict(self) -> dict:
        return asdict(self)

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)


def _usd(n: float) -> str:
    return "$" + format(round(n), ",")


_CAVEATS = [
    "ATOMiK figures are MEASURED on silicon (the aggregation/merge step); savings are "
    "arithmetic on YOUR stated baseline.",
    "What is removed is the coordination tax for correctness under concurrency — an "
    "architectural property of abelian merges, not a throughput multiplier.",
    "End-to-end impact additionally depends on your ingest path; this is not a universal "
    "saving.",
]


def compute_value(inp: ValueInputs, engine: si.SiliconMeasurement = si.ENGINE) -> ValueMemo:
    spec = WorkloadSpec(name="value", aggregation=inp.aggregation,
                        required_throughput=inp.required_throughput,
                        requires_reorder_stability=inp.reorder)
    r = evaluate(spec, engine)
    label = APPROACH_LABEL.get(inp.approach, inp.approach)
    peak = engine.peak_throughput()
    margin = r.margin
    min_banks = r.min_banks_required
    banks = r.banks_available

    def base(applicable, reason, headline, lines=None, **extra):
        return ValueMemo(
            applicable=applicable, reason=reason, headline=headline, approach_label=label,
            min_banks=min_banks, banks_available=banks, margin=margin, peak_throughput=peak,
            monthly_baseline=extra.get("monthly_baseline"),
            coordination_tax_monthly=extra.get("coordination_tax_monthly"),
            reclaimed_pct=extra.get("reclaimed_pct"),
            lines=lines or [], caveats=list(_CAVEATS),
        )

    if not r.correctness_gate.passed:
        return base(False,
                    f"'{inp.aggregation}' is order-dependent — ATOMiK won't claim it removes "
                    "your coordination here.",
                    "No order-independent value claim for this operator.")
    if not r.throughput_gate.passed:
        return base(False,
                    f"Your rate exceeds the measured {_eng(peak)} engine ceiling — no headroom "
                    "to translate.",
                    "Required rate is above the measured engine ceiling.")

    lines = [{
        "label": "ATOMiK sustains your rate",
        "value": f"{_eng(inp.required_throughput)} on {min_banks} of {banks} banks "
                 f"({margin:.1f}x headroom), lock-free, byte-identical",
        "source": "MEASURED",
    }]

    monthly_baseline = coordination_tax = reclaimed_pct = None
    if inp.instances and inp.instance_monthly:
        monthly_baseline = inp.instances * inp.instance_monthly
        lines.append({
            "label": "Provisioned baseline",
            "value": f"{_usd(monthly_baseline)}/mo ({inp.instances} x {_usd(inp.instance_monthly)})",
            "source": "YOUR BASELINE",
        })

    if inp.coordination_overhead_pct and inp.coordination_overhead_pct > 0:
        reclaimed_pct = inp.coordination_overhead_pct
        if monthly_baseline is not None:
            coordination_tax = monthly_baseline * reclaimed_pct / 100
            lines.append({
                "label": "Coordination tax (removed)",
                "value": f"~{_usd(coordination_tax)}/mo — {reclaimed_pct:g}% of baseline you "
                         f"attribute to {label}",
                "source": "YOUR BASELINE",
            })
        else:
            lines.append({
                "label": "Coordination overhead (removed)",
                "value": f"{reclaimed_pct:g}% of capacity you attribute to {label}",
                "source": "YOUR BASELINE",
            })

    if inp.p99_ms and inp.p99_ms > 0 and inp.p99_lock_induced:
        lines.append({
            "label": "Tail latency at peak",
            "value": f"The {label} component of your {inp.p99_ms:g} ms p99 at peak is removed — "
                     "ATOMiK's latency is deterministic (no lock or cache-coherency stall).",
            "source": "YOUR BASELINE",
        })

    if coordination_tax is not None:
        headline = f"~{_usd(coordination_tax)}/mo of {label} is what order-independence removes"
    elif reclaimed_pct is not None:
        headline = (f"{reclaimed_pct:g}% of capacity spent on {label} is what order-independence "
                    "removes")
    else:
        headline = f"the {label} you maintain for correctness is what order-independence removes"

    return base(True,
                f"'{inp.aggregation}' is abelian and {_eng(inp.required_throughput)} fits the "
                "measured engine — the coordination tax is removable.",
                headline, lines=lines, monthly_baseline=monthly_baseline,
                coordination_tax_monthly=coordination_tax, reclaimed_pct=reclaimed_pct)


def render_value_memo(inp: ValueInputs, memo: ValueMemo, *, company: str = "",
                      workload: str = "Aggregation workload",
                      engine: si.SiliconMeasurement = si.ENGINE) -> str:
    who = company.strip() if company.strip() else "________________________"
    tag = {"MEASURED": "[MEASURED]", "YOUR BASELINE": "[YOUR BASELINE]"}
    reproduce = f"abench {engine.reference_count} 0xDEADBEEF0BADF00D"
    out = [
        "# ATOMiK — Cost-of-Lock Value Memo (DRAFT)",
        "",
        f"- **Prepared for:** {who}",
        f"- **Workload:** {workload}",
        f"- **Operator / rate:** {inp.aggregation} at {_eng(inp.required_throughput)}",
        f"- **Today:** {memo.approach_label}",
        "",
        f"**{'VALUE' if memo.applicable else 'NO CLAIM'}** — {memo.headline}",
        "",
    ]
    if not memo.applicable:
        out += [f"> {memo.reason}", ""]
    else:
        out.append("## What the numbers say")
        out += [f"- {tag[ln['source']]} **{ln['label']}:** {ln['value']}" for ln in memo.lines]
        out += [
            "",
            "## The argument",
            f"For an abelian aggregation like `{inp.aggregation}`, the merged result is "
            f"byte-identical regardless of arrival order or parallelism (measured on the "
            f"{engine.board}, result {engine.result_hex} across 1/2/4/8 banks). That is exactly "
            f"the property a {memo.approach_label} exists to guarantee — so it is no longer "
            "required. The tax above, from your stated baseline, is what that removes.",
            "",
        ]
    out.append("## Scope & honesty")
    out += [f"- {c}" for c in memo.caveats]
    out += ["", f"> Reproduce the silicon facts: `{reproduce}`."]
    return "\n".join(out)
