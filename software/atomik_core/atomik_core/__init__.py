"""ATOMiK Core — Delta-state algebra for any processor.

Pure-software implementation of the ATOMiK instruction set:
  LOAD  — Set initial reference state
  ACCUM — XOR delta into accumulator
  READ  — Reconstruct current state (reference XOR accumulator)
  SWAP  — Atomic read-and-reset (snapshot + new epoch)

Zero dependencies. Works on any Python 3.9+ interpreter.

Backed by 92 Lean4 theorems proving:
  - Commutativity:  accum(a); accum(b) == accum(b); accum(a)
  - Associativity:  (a ^ b) ^ c == a ^ (b ^ c)
  - Self-inverse:   accum(d); accum(d) == identity
  - Identity:       accum(0) == identity

Quick start:
    from atomik_core import AtomikContext

    ctx = AtomikContext()
    ctx.load(0xDEADBEEF)
    ctx.accum(0x000000FF)
    assert ctx.read() == 0xDEADBE10

Multi-context table:
    from atomik_core import AtomikTable

    table = AtomikTable(num_contexts=256)
    table.load(addr=0, initial_state=0xCAFEBABE)
    table.accum(addr=0, delta=0x00000001)
    assert table.read(addr=0) == 0xCAFEBABF

SPDX-License-Identifier: Apache-2.0
"""

__version__ = "0.3.0"

from atomik_core.context import AtomikContext
from atomik_core.table import AtomikTable
from atomik_core.stream import DeltaStream, DeltaMessage
from atomik_core.fingerprint import Fingerprint
from atomik_core.sync import SyncTable
from atomik_core.transport import MemoryTransport, CallbackTransport
from atomik_core.benchmark import (
    bench_rollback,
    bench_change_detection,
    bench_convergence,
    bench_bandwidth,
    bench_throughput,
)

try:
    from atomik_core.device import DeviceContext, DeviceTable
except ImportError:
    pass  # Not available on non-Linux or without /dev/atomik

__all__ = [
    "AtomikContext",
    "AtomikTable",
    "DeltaStream",
    "DeltaMessage",
    "Fingerprint",
    "SyncTable",
    "MemoryTransport",
    "CallbackTransport",
    "bench_rollback",
    "bench_change_detection",
    "bench_convergence",
    "bench_bandwidth",
    "bench_throughput",
    "DeviceContext",
    "DeviceTable",
]
