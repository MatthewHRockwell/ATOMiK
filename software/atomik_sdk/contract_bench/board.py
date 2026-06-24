"""
Live measurement — run a customer's aggregation on REAL silicon, at their scale.

`measure_on_board(count)` drives `abench <count> <seed>` on a connected AX7020
over the serial console, parses the per-bank cycle table + the byte-identical
verification, and returns a fresh SiliconMeasurement.  Feed that straight into
evaluate() and the report says "measured live on silicon at your volume" instead
of quoting the recorded reference.

The throughput conclusion is count-independent (the engine sustains ~1
accumulation/bank/cycle regardless of count), so a live run doesn't change the
verdict — it proves it, at the prospect's own scale, during the call.

Serial I/O is isolated here; `parse_abench()` is pure and unit-tested so the
parsing survives without a board attached.
"""

from __future__ import annotations

import re
import time
from collections import Counter

from .silicon import SiliconMeasurement

_HEX16 = re.compile(r"^0x[0-9a-f]{16}$")

DEFAULT_SEED = "0xDEADBEEF0BADF00D"
_ROW = re.compile(r"\s*(\d+)\s+(\d+)\s+[\d.]+\s+(0x[0-9a-fA-F]+)\s+OK\b")


def parse_abench(out: str, count: int, clock_hz: int = 100_000_000) -> SiliconMeasurement:
    """Parse abench console output into a SiliconMeasurement. Raises on no data."""
    cycles: dict[int, int] = {}
    hashes: list[str] = []
    for line in out.splitlines():
        m = _ROW.search(line)
        if m:
            banks, cyc, hx = int(m.group(1)), int(m.group(2)), m.group(3).lower()
            if banks in (1, 2, 4, 8) and cyc > 0:
                cycles[banks] = cyc
                hashes.append(hx)
    if not cycles:
        raise RuntimeError("no abench result rows parsed from board output")
    # abench prints the same result on every bank row; over a lossy UART a single
    # row's 16 hex digits can get dropped/mangled (-> 0x0).  Take the most common
    # VALID, non-zero 16-digit value so one corrupted row can't poison the report.
    valid = [h for h in hashes if _HEX16.match(h) and int(h, 16) != 0]
    result_hex = Counter(valid).most_common(1)[0][0] if valid else "0x0"
    return SiliconMeasurement(
        board="ALINX/HamGeek AX7020 (XC7Z020-2CLG484)",
        engine="parallel-bank delta-state accumulator @0xF0021000",
        clock_hz=clock_hz,
        n_banks=max(cycles),
        reference_count=count,
        cycles=cycles,
        result_hex=result_hex,
        identical_across_banks=("ALL MATCH" in out),
        measured_on="live",
        tool="abench (live, on-call)",
    )


def measure_on_board(
    count: int,
    *,
    seed: str = DEFAULT_SEED,
    port: str = "/dev/ttyUSB2",
    clock_hz: int = 100_000_000,
    pause_aos: bool = True,
    timeout: float = 45.0,
) -> SiliconMeasurement:
    """Run `abench <count> <seed>` on the connected board and return its measurement."""
    import serial  # optional: only needed for live runs

    s = serial.Serial(port, 115200, timeout=2)

    def send(cmd: str) -> None:
        s.write(b"\n")
        time.sleep(0.3)
        s.reset_input_buffer()
        s.write(cmd.encode() + b"\n")

    try:
        if pause_aos:  # free the engine + console from the compositor
            send("pkill -STOP atomik_os 2>/dev/null; echo PAUSED")
            time.sleep(1.5)
            s.read(2000)
        send(f"/abench {count} {seed}")
        buf, t0 = "", time.time()
        while time.time() - t0 < timeout:
            buf += s.read(4000).decode(errors="replace")
            if "ALL MATCH" in buf or "FAILED" in buf:
                break
            time.sleep(0.3)
    finally:
        if pause_aos:
            try:
                send("pkill -CONT atomik_os 2>/dev/null")
                time.sleep(0.5)
            except Exception:
                pass
        s.close()
    return parse_abench(buf, count, clock_hz)
