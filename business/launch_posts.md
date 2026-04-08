# ATOMiK Launch Posts

## Hacker News — Show HN

**Title:** Show HN: ATOMiK – O(1) state reconstruction via XOR algebra, formally proven in Lean4

**Text:**

Hey HN,

I built ATOMiK, a delta-state algebra that replaces snapshots, event replay, and full-state replication with four XOR-based operations: LOAD, ACCUM, READ, SWAP.

The core insight: if you represent state changes as XOR deltas, you get commutativity (order doesn't matter), self-inverse (undo = re-apply), and O(1) reconstruction (current_state = reference ^ accumulator). No event log replay. No snapshot stacks. 24 bytes of memory regardless of history length.

**What it does:**
- 99% less network bandwidth: send 8-byte deltas instead of full state copies
- 333,333x less memory for rollback: 24 bytes vs 8MB for 1M checkpoints
- 1,291x faster change detection: O(1) incremental fingerprint vs O(n) rehash
- No consensus protocol: XOR commutativity means all nodes converge regardless of message ordering

**How it's validated:**
- 108 formal proofs in Lean4 (not tests — proofs)
- 417 tests across Python, C, and hardware
- Hardware-validated on FPGA: 69.7 Gops/s peak throughput
- Custom RISC-V CPU with ATOMiK as native ISA extensions

**Install:**
```
pip install atomik-core
```
Zero dependencies. Python 3.9+. Also available as a single-header C99 library.

The math is real — every algebraic property (commutativity, associativity, identity, self-inverse) is machine-checked in Lean4. The Python library is the pure-software implementation; the same algebra runs on FPGA hardware at 69.7 billion operations/second when you need the throughput.

GitHub: https://github.com/MatthewHRockwell/ATOMiK
PyPI: https://pypi.org/project/atomik-core/
Landing page: https://matthewhrockwell.github.io/ATOMiK/

Happy to answer questions about the algebra, the formal proofs, or the hardware implementation.

---

## Reddit r/programming

**Title:** I formally proved a 4-operation state algebra in Lean4, then built it in Python, C, and on custom FPGA hardware — ATOMiK is now on PyPI

**Text:**

After a year of work, ATOMiK is live on PyPI (`pip install atomik-core`).

**The idea:** Every state management system I've worked with (event sourcing, CRDT, snapshot replication) trades bandwidth, memory, or ordering guarantees. ATOMiK sidesteps all three by using XOR as the combining operation.

Since XOR forms an abelian group:
- **Commutative**: deltas can arrive in any order → no consensus protocol needed
- **Self-inverse**: applying a delta twice cancels it → undo is free, no snapshot stack
- **O(1) reconstruction**: `current = reference ^ accumulator` → no replay

This sounds too simple to work, which is why I proved it in Lean4 — 108 theorems covering the complete algebraic structure.

**Measured results (Python, zero dependencies):**
- Network: 80KB of deltas vs 655MB of full copies (64KB state, 10K updates)
- Memory: 24 bytes for rollback regardless of history (vs 8MB for 1M snapshots)
- Detection: Incremental fingerprint is 1,291x faster than SHA-256 on 1MB buffers
- Throughput: 5M+ ops/sec in pure Python, 500M in C, 69.7B on FPGA

The same algebra runs identically across Python → C → FPGA → custom RISC-V ISA. We have a full SoC on a $13.50 FPGA with ATOMiK as native CPU instructions.

- PyPI: https://pypi.org/project/atomik-core/
- GitHub: https://github.com/MatthewHRockwell/ATOMiK
- Site: https://matthewhrockwell.github.io/ATOMiK/

---

## Reddit r/Python

**Title:** atomik-core: Zero-dependency state algebra — O(1) rollback, 99% bandwidth reduction, formally proven (pip install atomik-core)

**Text:**

Just published `atomik-core` to PyPI. It's a delta-state algebra library built on XOR — four operations that replace snapshots, event replay, and full-state sync.

```python
from atomik_core import AtomikContext

ctx = AtomikContext()
ctx.load(0xDEADBEEF)
ctx.accum(0x000000FF)        # XOR delta in
print(f"0x{ctx.read():08x}") # 0xdeadbe10

ctx.rollback(0x000000FF)     # Undo = re-apply
assert ctx.read() == 0xDEADBEEF
```

**Why you'd use it:**
- **Distributed sync without ordering:** DeltaStream sends 8-byte messages. Nodes converge regardless of arrival order (XOR is commutative). No Raft/Paxos needed.
- **Rollback without snapshots:** 24 bytes of memory whether you've applied 10 or 10 million deltas. Self-inverse property means undo = re-apply.
- **Change detection in O(1):** Fingerprint class tracks changes incrementally — 1,291x faster than rehashing with SHA-256 on 1MB buffers.

Zero dependencies, Python 3.9+, fully typed (py.typed marker), 50 tests. Also ships as a single-header C99 library.

The math behind it is formally verified — 108 Lean4 theorems, not just tests.

```
pip install atomik-core
```

- GitHub: https://github.com/MatthewHRockwell/ATOMiK
- Docs: https://matthewhrockwell.github.io/ATOMiK/

---

## Reddit r/ExperiencedDevs (shorter, more technical)

**Title:** Formally verified XOR algebra for state management — shipped to PyPI after a year of Lean4 proofs + FPGA validation

**Text:**

I've been working on ATOMiK — a delta-state algebra where state is reconstructed (reference ^ accumulator) rather than stored. The algebra forms an abelian group under XOR, which gives you commutativity (no ordering), self-inverse (free undo), and O(1) reconstruction for free.

Published `atomik-core` to PyPI today. Zero deps, Python 3.9+, fully typed. 108 Lean4 formal proofs for the algebraic properties. Also available as a header-only C99 library.

The practical upshot: distributed nodes converge without consensus protocols, rollback needs 24 bytes regardless of history depth, and incremental change detection is O(1) per update instead of O(n) rehash.

We also have this running as native ISA extensions on a custom RISC-V core on FPGA (69.7 Gops/s peak). Same algebra, same API, just faster.

Would be interested in feedback from anyone working on distributed state, event sourcing, or CRDT systems.

https://pypi.org/project/atomik-core/
https://github.com/MatthewHRockwell/ATOMiK

---

## Twitter/X Thread

**Tweet 1:**
Just shipped atomik-core to PyPI.

4 operations. Zero dependencies. 108 formal proofs in Lean4.

State management, reconstructed.

pip install atomik-core

**Tweet 2:**
The insight: represent state changes as XOR deltas.

current_state = reference ^ accumulator

You get commutativity (order doesn't matter), self-inverse (undo = re-apply), and O(1) reconstruction — for free.

**Tweet 3:**
Measured results:
- 99.9% less network bandwidth
- 333,333x less memory for rollback
- 1,291x faster change detection
- No consensus protocol needed

Same algebra runs on Python → C → FPGA → custom RISC-V ISA.

**Tweet 4:**
The math isn't tested — it's proven.

108 Lean4 theorems covering commutativity, associativity, identity, and self-inverse.

If the proofs compile, the algebra is correct. Period.

**Tweet 5:**
Hardware-validated at 69.7 billion operations/second on FPGA.

Built a complete custom RISC-V CPU with ATOMiK as native instructions. On a $13.50 board.

GitHub: https://github.com/MatthewHRockwell/ATOMiK
Site: https://matthewhrockwell.github.io/ATOMiK/
