"""
WorkloadSpec — a customer aggregation workload + the bar they gate commitment on.

A spec is deliberately small: it captures what a customer can state in a
procurement conversation ("we ingest ~N events/s, they arrive out of order from
M producers, the rollup must be exactly correct, and we won't sign below R
events/s") and nothing ATOMiK can't honestly speak to.
"""

from __future__ import annotations

import json
from dataclasses import asdict, dataclass
from pathlib import Path


@dataclass
class WorkloadSpec:
    """A customer aggregation/reduction workload and its throughput-at-correctness gate."""

    name: str
    # The aggregation operator.  Must be one of silicon.ABELIAN_AGGREGATIONS for
    # ATOMiK to claim order-independence (the harness checks this).
    aggregation: str
    # The throughput the customer requires to commit, in events (deltas) / second.
    required_throughput: float
    # The correctness bar.  ATOMiK's claim is "byte-identical regardless of arrival
    # order or parallelism" — set this True when the customer needs exact,
    # reorder-stable results (the usual reason they're locking today).
    requires_reorder_stability: bool = True

    # Descriptive workload shape (does not change the verdict, but frames the report).
    events_per_window: int | None = None     # e.g. events per second / per batch
    producers: int = 1                       # concurrent sources feeding deltas
    out_of_order: bool = True                # do events arrive out of order?
    unit: str = "events/s"                   # label for throughput numbers
    customer: str = ""                       # prospect name (for the report header)
    notes: str = ""

    @classmethod
    def from_json(cls, path: str | Path) -> WorkloadSpec:
        data = json.loads(Path(path).read_text())
        known = {f for f in cls.__dataclass_fields__}  # noqa: F841 (clarity)
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})

    def to_dict(self) -> dict:
        return asdict(self)
