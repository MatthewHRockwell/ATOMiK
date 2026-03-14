#!/usr/bin/env python3
"""Example: Real-Time Analytics with ATOMiK State Tables.

Problem: Track metrics across hundreds of dimensions in real-time.
Traditional approach: database writes, event logs, periodic aggregation.
ATOMiK approach: XOR-accumulate deltas per dimension, read any time.

Use cases:
  - Trading: track P&L deltas across 256 instruments
  - Monitoring: track error counts across 256 services
  - Gaming: track player state across 256 attributes
"""

import sys
import os
import time
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from atomik_core import AtomikTable


def main():
    print("=== ATOMiK Real-Time Analytics Example ===\n")

    # --- Scenario: Trading desk tracking P&L across instruments ---
    num_instruments = 64
    table = AtomikTable(num_contexts=num_instruments, width=64)

    print(f"1. Trading desk: {num_instruments} instruments\n")

    # Initialize all instruments with base P&L
    for i in range(num_instruments):
        table.load(i, 0)  # Start at zero P&L

    # Simulate trades (each trade is a delta to P&L)
    trades = [
        (0, 0x00000000000003E8),   # Instrument 0: +1000 (0x3E8)
        (5, 0x00000000000001F4),   # Instrument 5: +500
        (0, 0x0000000000000064),   # Instrument 0: +100 more
        (12, 0xFFFFFFFFFFFFFC18),  # Instrument 12: -1000 (two's complement)
        (5, 0x00000000000000C8),   # Instrument 5: +200 more
    ]

    print("2. Processing trades:\n")
    for instrument, delta in trades:
        table.accum(instrument, delta)
        current = table.read(instrument)
        print(f"   Instrument {instrument:2d}: delta=0x{delta:016x}, state=0x{current:016x}")

    # --- Snapshot: read all active instruments ---
    print(f"\n3. Portfolio snapshot (O(1) per read):\n")
    active = table.snapshot()
    for addr, state in sorted(active.items()):
        print(f"   Instrument {addr:2d}: 0x{state:016x}")

    print(f"\n   Active instruments: {len(active)} / {num_instruments}")
    print(f"   Snapshot size: {len(active) * 8} bytes (vs {num_instruments * 8} bytes full)")

    # --- Batch update ---
    print(f"\n4. Batch update (multiple instruments at once):\n")
    batch = {i: (i + 1) * 0x100 for i in range(8)}
    table.batch_accum(batch)
    print(f"   Updated {len(batch)} instruments in one call")

    # --- SWAP for epoch boundary ---
    print(f"\n5. End-of-day SWAP (atomic snapshot + reset):\n")
    eod_pnl = table.swap(0)
    print(f"   Instrument 0 EOD P&L: 0x{eod_pnl:016x}")
    print(f"   Instrument 0 now:     0x{table.read(0):016x} (new epoch, acc cleared)")

    # --- Throughput test ---
    print(f"\n6. Throughput test:\n")
    n = 500_000
    start = time.perf_counter()
    for i in range(n):
        table.accum(i % num_instruments, i)
    elapsed = time.perf_counter() - start
    ops_per_sec = n / elapsed
    print(f"   {n:,} ACCUM operations in {elapsed*1000:.1f} ms")
    print(f"   Throughput: {ops_per_sec:,.0f} ops/sec")
    print(f"   Latency:    {elapsed/n*1e6:.2f} us/op")


if __name__ == "__main__":
    main()
