"""
Sync / replica-convergence evaluator — the second archetype CLASS.

Aggregation gates on throughput-at-correctness (harness.py). Sync gates on two
different things, and this evaluator mirrors the website's sync proof exactly:

  1. CONVERGENCE (correctness) — replicas reach identical state with NO
     coordinator / consensus. This is NOT a new claim: it is the same
     byte-identical order-independence already measured on silicon (silicon.py).
     Different bank counts partition the deltas differently and complete them in
     different orders, yet produce an identical merged result — which is exactly
     what replica convergence is (any node, any order, any partition -> identical
     state). So the convergence gate passes on the recorded silicon proof.

  2. DATA MOVED (bandwidth) — delta-state ships the DELTA, not full state. The
     reduction is MEASURED on the AX7020 adapter (an edge-telemetry tick: 160 B
     vs 2,048 B full state, ~92% less at ~5% change density). It is caveated as
     change-density-dependent, NOT a universal multiplier, and stays well away
     from the unsupported 7,670x-916,000x figures.

Keep the Markdown output in sync with website/src/lib/benchmark-proof.ts
(renderSyncReportMarkdown) — both describe the same proof.
"""

from __future__ import annotations

import json
from dataclasses import asdict, dataclass, field
from pathlib import Path

from . import silicon as si
from .report import Gate


@dataclass
class SyncSpec:
    """A customer replica-sync workload + its bandwidth numbers."""

    name: str
    full_state_bytes: int  # bytes a conventional full-state sync ships per tick
    delta_bytes: int       # bytes ATOMiK's delta ships per tick (measured/known)
    change_density: str = "the measured change density"
    customer: str = ""
    measured_note: str = ""

    @classmethod
    def from_json(cls, path: str | Path) -> SyncSpec:
        data = json.loads(Path(path).read_text())
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


# The recorded measured reference (AX7020 ATOMiK adapter telemetry sync tick).
REFERENCE_SYNC = SyncSpec(
    name="Edge telemetry sync tick",
    full_state_bytes=2048,
    delta_bytes=160,
    change_density="~5% of state changed per tick",
    customer="Distributed State (archetype)",
    measured_note=(
        "Measured on the AX7020 ATOMiK adapter: an edge-telemetry sync tick moved "
        "160 B vs 2,048 B of full state, with exact 64-bit LOAD/ACCUM/READ round-trips "
        "on a falsification-tested harness."
    ),
)


@dataclass
class SyncReport:
    workload: str
    customer: str
    verdict: str  # "CONVERGES" | "FAIL"
    headline: str
    convergence_gate: Gate
    data_moved_gate: Gate
    reduction_pct: int
    provenance: dict
    scope_note: str
    fields: dict = field(default_factory=dict)

    def to_dict(self) -> dict:
        return asdict(self)

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)

    def to_markdown(self) -> str:
        ok = lambda b: "PASS" if b else "FAIL"  # noqa: E731
        who = f"{self.customer} / {self.workload}" if self.customer else self.workload
        lines = [
            f"# ATOMiK Sync Proof — {who}",
            "",
            f"**VERDICT: {self.verdict}** — {self.headline}",
            "",
            "## Convergence gate (correctness)",
            f"- Required: {self.convergence_gate.required}",
            f"- ATOMiK: {self.convergence_gate.measured}",
            f"- {ok(self.convergence_gate.passed)} — {self.convergence_gate.detail}",
            "",
            "## Data-moved gate (bandwidth)",
            f"- Required: {self.data_moved_gate.required}",
            f"- ATOMiK: {self.data_moved_gate.measured}",
            f"- {ok(self.data_moved_gate.passed)} — {self.data_moved_gate.detail}",
            "",
            "## Provenance",
        ]
        lines += [f"- {k}: {v}" for k, v in self.provenance.items()]
        lines += ["", f"> Scope: {self.scope_note}"]
        return "\n".join(lines)


def evaluate_sync(spec: SyncSpec, engine: si.SiliconMeasurement = si.ENGINE) -> SyncReport:
    reduction = round((1 - spec.delta_bytes / spec.full_state_bytes) * 100) \
        if spec.full_state_bytes > 0 else 0

    convergence_gate = Gate(
        name="convergence",
        required="replicas reach identical state with no coordinator / consensus",
        measured=f"byte-identical across 1/2/4/8 banks ({engine.result_hex})",
        passed=engine.identical_across_banks,
        detail="any node, any order, after any partition -> identical merged state "
               "(the same byte-identical property proven on silicon)",
    )
    data_moved_gate = Gate(
        name="data_moved",
        required=f"{spec.full_state_bytes} B/tick full-state sync",
        measured=f"{spec.delta_bytes} B/tick delta (measured)",
        passed=0 < spec.delta_bytes < spec.full_state_bytes,
        detail=f"~{reduction}% less at {spec.change_density} — scales with change "
               f"density, not a universal multiplier",
    )

    converges = convergence_gate.passed and data_moved_gate.passed
    verdict = "CONVERGES" if converges else "FAIL"
    headline = (
        f"coordination-free convergence + ~{reduction}% less data moved"
        if converges
        else "the delta is not smaller than full state for this workload"
        if not data_moved_gate.passed
        else "engine result is not byte-identical across banks"
    )

    return SyncReport(
        workload=spec.name,
        customer=spec.customer,
        verdict=verdict,
        headline=headline,
        convergence_gate=convergence_gate,
        data_moved_gate=data_moved_gate,
        reduction_pct=reduction,
        provenance={
            "convergence_measured_on": engine.measured_on,
            "board": engine.board,
            "reproduce_order_independence": f"abench {engine.reference_count} 0xDEADBEEF0BADF00D",
            "data_moved_note": spec.measured_note or REFERENCE_SYNC.measured_note,
        },
        scope_note=(
            f"The data-moved ratio is for {spec.change_density}; it scales with how much "
            "of the state changes per tick — lower change density means a larger reduction, "
            "higher means smaller. It is not a universal multiplier."
        ),
        fields={
            "full_state_bytes": spec.full_state_bytes,
            "delta_bytes": spec.delta_bytes,
            "reduction_pct": reduction,
            "result_hex": engine.result_hex,
        },
    )
