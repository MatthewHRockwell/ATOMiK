# atomik-core

**O(1) state reconstruction. 99% less bandwidth. Formally proven.**

ATOMiK is a delta-state algebra that replaces snapshots, event replay, and full-state replication with four operations. Works on any processor — no FPGA required.

## Install

```bash
pip install atomik-core
```

**Zero dependencies.** Python 3.9+.

## Quick Start

```python
from atomik_core import AtomikContext

# Create a context and set initial state
ctx = AtomikContext()
ctx.load(0xDEADBEEF)

# Accumulate deltas (XOR — order doesn't matter)
ctx.accum(0x000000FF)
print(f"State: 0x{ctx.read():08x}")  # 0xdeadbe10

# Undo any delta by re-applying it (self-inverse)
ctx.rollback(0x000000FF)
assert ctx.read() == 0xDEADBEEF
```

## The 4 Operations

| Operation | What it does | Complexity |
|-----------|-------------|------------|
| `load(value)` | Set reference state, clear accumulator | O(1) |
| `accum(delta)` | XOR delta into accumulator | O(1) |
| `read()` | Reconstruct state: reference ^ accumulator | O(1) |
| `swap()` | Atomic snapshot + new epoch | O(1) |

Everything else — rollback, merge, fingerprinting, streaming — is built on these four.

## Why ATOMiK?

### 99% Less Network Traffic

Send 8-byte deltas instead of full state copies.

```python
from atomik_core import DeltaStream

# Sender and receiver start with same reference
sender = DeltaStream()
receiver = DeltaStream()
sender.load(0, initial_state=0xCAFE)
receiver.load(0, initial_state=0xCAFE)

# Sender produces a delta (8 bytes, not full state)
msg = sender.accum(0, delta=0x00FF)

# Receiver applies it — converges to same state
receiver.apply(msg)
assert sender.read(0) == receiver.read(0)
```

| State Size | Full Replication (10K updates) | ATOMiK Deltas | Reduction |
|-----------|-------------------------------|---------------|-----------|
| 1 KB | 10.2 MB | 80 KB | **99.2%** |
| 64 KB | 655 MB | 80 KB | **99.99%** |

### 333,333x Less Memory for Rollback

ATOMiK uses 24 bytes regardless of history length. No snapshot stack.

```python
ctx = AtomikContext()
ctx.load(0xAAAA)

# Apply 1 million deltas...
for i in range(1_000_000):
    ctx.accum(i)

# Undo all of them (XOR self-inverse)
for i in range(1_000_000):
    ctx.rollback(i)

assert ctx.read() == 0xAAAA  # Back to original
# Memory used: 24 bytes. Snapshot approach: 8 MB.
```

### 1,291x Faster Change Detection

Track changes incrementally in O(1), not O(n).

```python
from atomik_core import Fingerprint

fp = Fingerprint(width=64)
fp.load(reference_data)     # Set reference fingerprint

# When a field changes, update in O(1):
fp.accumulate_delta(old_value, new_value)

if fp.changed:
    print("Data modified!")
```

### No Consensus Protocol

XOR is commutative — deltas can arrive in any order. All nodes converge.

```python
# Three nodes, each produces deltas independently
nodes = [DeltaStream() for _ in range(3)]
for n in nodes:
    n.load(0, 0xAAAA)

m0 = nodes[0].accum(0, 0x0001)
m1 = nodes[1].accum(0, 0x0010)
m2 = nodes[2].accum(0, 0x0100)

# Broadcast all-to-all (any order)
for i, node in enumerate(nodes):
    for j, msg in enumerate([m0, m1, m2]):
        if i != j:
            node.apply(msg)

# All converge — no Raft, no Paxos, no leader election
assert nodes[0].read(0) == nodes[1].read(0) == nodes[2].read(0)
```

## Multi-Context Table

Track state across 256 independent addresses:

```python
from atomik_core import AtomikTable

table = AtomikTable(num_contexts=256)
table.load(addr=0, initial_state=0x1000)
table.load(addr=1, initial_state=0x2000)

table.accum(addr=0, delta=0x0001)
assert table.read(0) == 0x1001
assert table.read(1) == 0x2000  # Independent

# Batch update multiple addresses at once
table.batch_accum({0: 0x0010, 1: 0x0020, 2: 0x0030})

# Snapshot all non-zero contexts
active = table.snapshot()
```

## Formally Proven

Every algebraic property is proven in Lean4 — not tested, proven:

- **Commutativity**: `accum(a); accum(b) == accum(b); accum(a)`
- **Associativity**: `(a ^ b) ^ c == a ^ (b ^ c)`
- **Self-inverse**: `accum(d); accum(d) == identity`
- **Identity**: `accum(0) == identity`

92 theorems total. [See proofs →](https://github.com/MatthewHRockwell/ATOMiK/tree/main/math/proofs)

## C Library

Single-header C99 library with the same API:

```c
#define ATOMIK_IMPLEMENTATION
#include "atomik_core.h"

atomik_ctx_t ctx;
atomik_init(&ctx);
atomik_load(&ctx, 0xDEADBEEFCAFEBABEULL);
atomik_accum(&ctx, 0x00000000000000FFULL);
uint64_t state = atomik_read(&ctx);
```

## Hardware Upgrade Path

When software performance isn't enough:

| Implementation | Throughput | Latency |
|---------------|-----------|---------|
| Python (`atomik-core`) | 5M ops/sec | ~200 ns |
| C (`atomik_core.h`) | 500M ops/sec | ~2 ns |
| FPGA (ATOMiK hardware) | 69.7 Gops/sec | 10.6 ns/op |

Same API. Same algebra. Same proofs. Just faster.

## Examples

- [Distributed Cache Sync](examples/distributed_cache.py) — 3-node convergence without consensus
- [IoT Sensor Fusion](examples/iot_sensor_fusion.py) — 99.2% bandwidth reduction
- [Real-Time Analytics](examples/realtime_analytics.py) — Trading P&L tracking at 2.5M ops/sec

## License

Apache 2.0 for evaluation and non-commercial use. [Commercial license](https://github.com/MatthewHRockwell/ATOMiK) required for production deployment. Patent pending.
