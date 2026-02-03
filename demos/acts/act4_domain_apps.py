"""Act 4: Domain Applications — Finance tick stream, sensor fusion, alerts.

Shows how the same delta algebra applies to real-world verticals.
"""

from __future__ import annotations

import random

from demos.acts.base import ActBase, ActResult
from demos.node import NodeController

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
        details.append(f"  Initial state S0 = 0x{portfolio_state:016X}")
        details.append("")

        # Simulate 5 price tick deltas, showing state after each
        ticks = [_RNG.getrandbits(64) for _ in range(5)]
        state_after_4 = None
        for i, tick in enumerate(ticks):
            finance.accumulate(tick)
            current_state = finance.read()
            details.append(f"  Tick {i + 1}: delta 0x{tick:016X} -> state 0x{current_state:016X}")
            if i == 3:  # Save state after tick 4 for undo verification
                state_after_4 = current_state

        details.append("")
        state_after_5 = finance.read()
        details.append(f"  State after 5 ticks: 0x{state_after_5:016X}")
        details.append(f"  State after 4 ticks: 0x{state_after_4:016X}  <-- undo target")
        details.append("")

        # Undo last tick (self-inverse): apply tick 5 again
        details.append(f"  Undo: re-apply tick 5 (0x{ticks[-1]:016X})")
        finance.accumulate(ticks[-1])
        after_undo = finance.read()

        # Verify: should equal state after first 4 ticks
        ok = after_undo == state_after_4
        details.append(f"  State after undo:    0x{after_undo:016X}")
        details.append(f"  Expected (after 4):  0x{state_after_4:016X}")
        details.append(f"  Match: {after_undo:016X} == {state_after_4:016X} ? [{'PASS' if ok else 'FAIL'}]")
        all_passed &= ok

        # ── Sensor: multi-stream fusion + alerts (Node 2) ─────────────
        details.append("")
        details.append("--- Sensor: Multi-Stream Fusion + Alerts ---")
        sensor = nodes[1]
        baseline = 0x0000_0000_0000_0000
        sensor.load(baseline)
        details.append(f"  Initial state = 0x{baseline:016X}")
        details.append("")

        # 3 sensor streams contributing deltas - show each fusion step
        streams = [
            ("IMU-accel", [_RNG.getrandbits(64) for _ in range(3)]),
            ("IMU-gyro", [_RNG.getrandbits(64) for _ in range(3)]),
            ("GPS", [_RNG.getrandbits(64) for _ in range(2)]),
        ]

        expected = baseline
        for stream_name, stream_deltas in streams:
            details.append(f"  Stream: {stream_name}")
            for j, d in enumerate(stream_deltas):
                sensor.accumulate(d)
                expected ^= d
                current = sensor.read()
                details.append(f"    [{j + 1}] delta 0x{d:016X} -> fused 0x{current:016X}")
            details.append("")

        fused = sensor.read()
        ok = fused == expected
        details.append(f"  Final fused state:   0x{fused:016X}")
        details.append(f"  Expected (XOR all):  0x{expected:016X}")
        details.append(f"  Match: [{'PASS' if ok else 'FAIL'}]")
        all_passed &= ok
        details.append("")

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
