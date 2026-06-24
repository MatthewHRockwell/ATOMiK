"""
BenchmarkReport — the PASS/SURPASS artifact handed to a prospect.

It separates the two gates a contract actually turns on:
  * THROUGHPUT  — measured engine rate vs the customer's required rate.
  * CORRECTNESS — byte-identical-under-reordering (the property they'd otherwise
                  need a lock for, which kills throughput).
Both must pass.  Every figure is tagged with its source so the report survives
scrutiny; it renders to dict/JSON (for atomik.tech) and Markdown (to hand over).
"""

from __future__ import annotations

import json
import math
from dataclasses import asdict, dataclass, field


def _eng(n: float) -> str:
    """Human throughput, e.g. 800_000_000 -> '800 M/s'."""
    for div, suf in ((1e9, "G"), (1e6, "M"), (1e3, "K")):
        if n >= div:
            v = n / div
            return f"{v:.0f} {suf}/s" if v >= 100 or v == int(v) else f"{v:.2f} {suf}/s"
    return f"{n:.0f}/s"


@dataclass
class Gate:
    name: str
    required: str
    measured: str
    passed: bool
    detail: str = ""


@dataclass
class BenchmarkReport:
    workload: str
    customer: str
    verdict: str                    # "PASS" | "SURPASS" | "FAIL"
    headline: str                   # one-line summary
    throughput_gate: Gate
    correctness_gate: Gate
    margin: float                   # measured_throughput / required_throughput
    min_banks_required: int         # banks needed to hit the bar
    banks_available: int
    scaling: dict[int, float]       # measured speedup per bank count
    provenance: dict[str, str]
    scope_note: str
    fields: dict = field(default_factory=dict)  # raw numbers for the website

    # -- renderers ----------------------------------------------------------
    def to_dict(self) -> dict:
        d = asdict(self)
        d["scaling"] = {str(k): v for k, v in self.scaling.items()}
        return d

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)

    def to_markdown(self) -> str:
        ok = lambda b: "PASS" if b else "FAIL"  # noqa: E731
        who = f"{self.customer} / {self.workload}" if self.customer else self.workload
        # truncate (don't round) so per-run setup overhead shows honestly and
        # matches the on-device ladder / abench (1.99/3.99/7.99, not 2/4/8).
        scale = "  ".join(f"{b}->{math.floor(s * 100) / 100:.2f}x"
                          for b, s in sorted(self.scaling.items()))
        lines = [
            f"# ATOMiK Benchmark Proof — {who}",
            "",
            f"**VERDICT: {self.verdict}** — {self.headline}",
            "",
            "## Throughput gate",
            f"- Required: **{self.throughput_gate.required}**",
            f"- ATOMiK (measured on silicon): **{self.throughput_gate.measured}**",
            f"- {ok(self.throughput_gate.passed)} — {self.throughput_gate.detail}",
            "",
            "## Correctness gate (the part a lock would cost you)",
            f"- Required: {self.correctness_gate.required}",
            f"- ATOMiK: {self.correctness_gate.measured}",
            f"- {ok(self.correctness_gate.passed)} — {self.correctness_gate.detail}",
            "",
            "## Measured linear scaling (silicon)",
            f"- Speedup vs 1 bank: {scale}",
            f"- To meet the bar you need **{self.min_banks_required} bank(s)**; "
            f"the chip provides **{self.banks_available}**.",
            "",
            "## Provenance",
        ]
        lines += [f"- {k}: {v}" for k, v in self.provenance.items()]
        lines += ["", f"> Scope: {self.scope_note}"]
        return "\n".join(lines)
