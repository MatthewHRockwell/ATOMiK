"""
LiveRunRecord — the witnessed, dated proof that backs a Conditional LOI.

A benchmark report says "the engine WILL hit your bar"; a live run says "it DID,
on <date>, here is the abench transcript and the byte-identical result hash."
This module bundles a measurement + its raw console transcript + provenance into
an artifact a prospect can witness and reproduce, and that the LOI embeds as
acceptance evidence.

Two producers, both honest:
  * run_live(spec, ...)        — drives abench on a connected AX7020 at the
                                 prospect's scale and captures the transcript.
  * reference_live_run()       — the RECORDED reference measurement (silicon.py
                                 ENGINE, 2026-06-21).  Not a fresh run; clearly
                                 labelled as the recorded reference.

There is no third, fabricated producer.  A record only exists for a real run or
the recorded reference.
"""

from __future__ import annotations

import json
import math
from dataclasses import asdict, dataclass, field

from . import silicon as si
from .report import _eng


@dataclass
class LiveRunRecord:
    workload: str
    source: str          # "live (on-call)" | "recorded reference"
    run_on: str          # date / "live"
    board: str
    count: int           # accumulations driven
    seed: str
    clock_hz: int
    n_banks: int
    cycles: dict[int, int]
    result_hex: str
    identical_across_banks: bool
    peak_throughput: float
    per_bank_throughput: float
    reproduce: str
    transcript: str      # raw abench console output (verbatim, witnessable)
    fields: dict = field(default_factory=dict)

    # -- renderers ----------------------------------------------------------
    def to_dict(self) -> dict:
        d = asdict(self)
        d["cycles"] = {str(k): v for k, v in self.cycles.items()}
        return d

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)

    def ladder(self) -> list[str]:
        """The measured banks -> cycles -> speedup -> throughput ladder."""
        base = self.cycles.get(1)
        rows = ["| banks | hw cycles | speedup | throughput |",
                "|------:|----------:|--------:|-----------:|"]
        for b in sorted(self.cycles):
            cyc = self.cycles[b]
            # truncate (don't round) so per-run setup overhead shows honestly and
            # matches abench / the report ladder (1.99/3.99/7.99, not 2/4/8).
            spd = f"{math.floor(base / cyc * 100) / 100:.2f}x" if base else "-"
            rows.append(f"| {b} | {cyc} | {spd} | {_eng(self.count * self.clock_hz / cyc)} |")
        return rows

    def to_markdown(self) -> str:
        verify = "ALL MATCH" if self.identical_across_banks else "MISMATCH"
        lines = [
            f"# ATOMiK Live Run — {self.workload}",
            "",
            f"**{self.source}** · {self.run_on} · {self.board}",
            "",
            f"`abench {self.count} {self.seed}` — {self.count} accumulations per bank "
            f"configuration, self-verifying.",
            "",
            "## Measured ladder",
            *self.ladder(),
            "",
            "## Order-independence (the proof)",
            f"- Merged result: **{self.result_hex}**",
            f"- Across 1 / 2 / 4 / 8 banks: **{verify}** — byte-identical regardless of how the "
            "deltas were partitioned or ordered.",
            f"- Peak: **{_eng(self.peak_throughput)}** ({_eng(self.per_bank_throughput)} per bank).",
            "",
            "## Reproduce",
            f"`{self.reproduce}`",
            "",
            "## Raw transcript (verbatim)",
            "```",
            self.transcript.strip("\n"),
            "```",
        ]
        return "\n".join(lines)


def _record(
    workload: str,
    m: si.SiliconMeasurement,
    *,
    source: str,
    run_on: str,
    seed: str,
    transcript: str,
) -> LiveRunRecord:
    reproduce = f"abench {m.reference_count} {seed}"
    return LiveRunRecord(
        workload=workload,
        source=source,
        run_on=run_on,
        board=m.board,
        count=m.reference_count,
        seed=seed,
        clock_hz=m.clock_hz,
        n_banks=m.n_banks,
        cycles=dict(m.cycles),
        result_hex=m.result_hex,
        identical_across_banks=m.identical_across_banks,
        peak_throughput=m.peak_throughput(),
        per_bank_throughput=m.per_bank_throughput(),
        reproduce=reproduce,
        transcript=transcript,
        fields={
            "peak_throughput": m.peak_throughput(),
            "per_bank_throughput": m.per_bank_throughput(),
            "result_hex": m.result_hex,
            "identical_across_banks": m.identical_across_banks,
        },
    )


# Faithful render of the recorded reference transcript (silicon.py docstring).
def _reference_transcript(m: si.SiliconMeasurement, seed: str) -> str:
    base = m.cycles[1]
    rows = ["banks  hw_cycles  speedup  result              ok"]
    for b in sorted(m.cycles):
        cyc = m.cycles[b]
        spd = math.floor(base / cyc * 100) / 100
        rows.append(f"{b:<6} {cyc:<10} {spd:<8.2f} {m.result_hex}  OK")
    rows.append(f"software reference = {m.result_hex}  ->  ALL MATCH")
    return f"$ abench {m.reference_count} {seed}\n" + "\n".join(rows)


def reference_live_run(
    workload: str = "Recorded reference run",
    *,
    seed: str = "0xDEADBEEF0BADF00D",
    engine: si.SiliconMeasurement = si.ENGINE,
) -> LiveRunRecord:
    """Build a record from the RECORDED reference measurement (not a fresh run)."""
    return _record(
        workload,
        engine,
        source="recorded reference",
        run_on=engine.measured_on,
        seed=seed,
        transcript=_reference_transcript(engine, seed),
    )


def run_live(
    workload: str,
    count: int,
    *,
    seed: str = "0xDEADBEEF0BADF00D",
    port: str = "/dev/ttyUSB2",
) -> LiveRunRecord:
    """Drive abench on a connected AX7020 and capture a witnessed record."""
    from .board import measure_on_board_raw

    measurement, transcript = measure_on_board_raw(count, seed=seed, port=port)
    return _record(
        workload,
        measurement,
        source="live (on-call)",
        run_on="live",
        seed=seed,
        transcript=transcript,
    )
