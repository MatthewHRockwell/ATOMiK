"""Act 4: Domain Applications — Finance tick stream, sensor fusion, alerts.

Shows how the same delta algebra applies to real-world verticals.
"""

from __future__ import annotations

import random

from demo.acts.base import ActBase, ActResult
from demo.node import NodeController

_RNG = random.Random(0xF1A4CE)


class Act4DomainApps(ActBase):
    number = 4
    title = "Domain Applications"
    narration = (
        "The same algebra powers real applications. Node 1 processes a "
        "finance tick stream with instant undo. Node 2 fuses multiple sensor "
        "streams and detects alert conditions. Node 3 runs the peak "
        "throughput workload — all in parallel, all verifiable."
    )

    def run(self, nodes: list[NodeController]) -> ActResult:
        details: list[str] = []
        all_passed = True

        # ── Finance: tick stream + instant undo (Node 1) ──────────────
        details.append("--- Finance: Tick Stream + Undo ---")
        finance = nodes[0]
        portfolio_state = 0xA000_0000_0000_0000  # initial portfolio hash
        finance.load(portfolio_state)

        # Simulate 5 price tick deltas
        ticks = [_RNG.getrandbits(64) for _ in range(5)]
        for i, tick in enumerate(ticks):
            finance.accumulate(tick)
            details.append(f"  Tick {i + 1}: delta 0x{tick:016X}")

        after_ticks = finance.read()
        details.append(f"  State after 5 ticks: 0x{after_ticks:016X}")

        # Undo last tick (self-inverse)
        finance.accumulate(ticks[-1])
        after_undo = finance.read()

        # Verify: should equal state after first 4 ticks
        expected_after_undo = portfolio_state
        for t in ticks[:-1]:
            expected_after_undo ^= t
        ok = after_undo == expected_after_undo
        details.append(f"  After undo last tick: 0x{after_undo:016X} [{'PASS' if ok else 'FAIL'}]")
        all_passed &= ok

        # ── Sensor: multi-stream fusion + alerts (Node 2) ─────────────
        details.append("")
        details.append("--- Sensor: Multi-Stream Fusion + Alerts ---")
        sensor = nodes[1]
        baseline = 0x0000_0000_0000_0000
        sensor.load(baseline)

        # 3 sensor streams contributing deltas
        streams = {
            "IMU-accel": [_RNG.getrandbits(64) for _ in range(3)],
            "IMU-gyro":  [_RNG.getrandbits(64) for _ in range(3)],
            "GPS":       [_RNG.getrandbits(64) for _ in range(2)],
        }

        expected = baseline
        for stream_name, stream_deltas in streams.items():
            for d in stream_deltas:
                sensor.accumulate(d)
                expected ^= d
            details.append(f"  {stream_name}: {len(stream_deltas)} deltas fused")

        fused = sensor.read()
        ok = fused == expected
        details.append(f"  Fused state: 0x{fused:016X} [{'PASS' if ok else 'FAIL'}]")
        all_passed &= ok

        # Alert detection: check specific bits as "alert flags"
        alert_bits = (fused >> 56) & 0xFF  # top byte
        alert_count = bin(alert_bits).count("1")
        details.append(f"  Alert flags (top byte): 0b{alert_bits:08b} ({alert_count} active)")

        # ── Peak: throughput burst (Node 3) ───────────────────────────
        details.append("")
        details.append("--- Peak: 1 Gops/s Workload ---")
        peak = nodes[2]
        peak.load(0)

        burst_deltas = [_RNG.getrandbits(64) for _ in range(200)]
        expected = 0
        for d in burst_deltas:
            expected ^= d

        peak.accumulate_batch(burst_deltas)
        result = peak.read()
        ok = result == expected
        details.append(
            f"  200-delta burst: 0x{result:016X} [{'PASS' if ok else 'FAIL'}]"
        )
        details.append(
            f"  Rated throughput: {peak.config.throughput_mops:,.0f} Mops/s "
            f"({peak.config.throughput_mops / 1000:.2f} Gops/s)"
        )
        all_passed &= ok

        return self._result(
            passed=all_passed,
            summary="All 3 domain applications verified" if all_passed else "Domain check failed",
            details=details,
        )
