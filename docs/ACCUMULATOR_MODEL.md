# ATOMiK Accumulator Model

## Core Architecture

ATOMiK reconstructs state as:

```
current_state = initial_state ⊕ accumulator
```

The hardware has two components:
- **State table**: 256 entries x 64 bits — stores initial reference states, indexed by address
- **Accumulator**: one 64-bit register — stores the XOR of all accumulated deltas

The accumulator is **shared** across all state table addresses. It is not per-address.

## Operations

| Operation | Effect | Accumulator |
|-----------|--------|-------------|
| **LOAD(addr, value)** | `state_table[addr] = value`, `active_addr = addr` | Cleared to 0 |
| **ACCUM(delta)** | `acc = acc ⊕ delta` | Modified |
| **READ()** | Returns `state_table[active_addr] ⊕ acc` | Unchanged |
| **SWAP(addr)** | `state_table[active_addr] = current_state`, `active_addr = addr` | Cleared to 0 |

## Key Constraint: One Region at a Time

Because the accumulator is shared:
- You can track **one region** with incremental deltas (O(1) per write, O(1) to detect change)
- To track **multiple regions**, you must switch context between them using LOAD or SWAP, which **clears the accumulator**
- Multi-region detection requires N sequential checks, each clearing the accumulator

This means:
- **Single-region tracking**: ATOMiK accumulates deltas at write time. Detection is O(1) — just check `acc_zero`.
- **Multi-region sequential scan**: ATOMiK checks one region at a time. Detection cost is O(N) in regions but O(1) per region regardless of region size.
- **True parallel multi-region tracking** requires the **multi-bank** architecture (N > 1), where each bank has its own independent accumulator.

## Correct Usage Patterns

### Pattern 1: Single Region, Incremental Tracking
```c
atomik_load(a, 0, initial_fingerprint);
// ... writes happen, each accumulates its delta:
atomik_accum(a, write_delta);
// ... later, check if anything changed:
if (!atomik_acc_zero(a)) {
    // Region changed since last LOAD
}
```

### Pattern 2: Sequential Multi-Region Detection
```c
// For each region, compute fingerprint and compare
for (int i = 0; i < N; i++) {
    uint64_t new_fp;
    int changed = atomik_detect_changed(a, i, expected_fp[i], data[i], size, &new_fp);
    if (changed) {
        expected_fp[i] = new_fp;  // advance reference for next check
        handle_change(i);
    }
}
```
Note: `atomik_detect_changed()` uses LOAD internally, so each call clears the accumulator.
The `new_fp` out-parameter returns the current fingerprint so the caller can advance the reference.

### Pattern 3: Incremental with Periodic Snapshot (SWAP)
```c
atomik_load(a, 0, fingerprint);
// Accumulate deltas over time...
atomik_accum(a, delta1);
atomik_accum(a, delta2);
// At checkpoint: promote current state to new reference
atomik_swap(a, 0);  // state_table[0] = current_state, acc = 0
// Now tracking changes from the NEW state
```

## Multi-Bank Architecture (Future)

With N > 1 banks, each bank has its own accumulator. This enables:
- **Parallel tracking**: N regions tracked simultaneously without switching
- **Lock-free accumulation**: XOR commutativity means multiple producers can feed the same bank in any order

The multi-bank architecture has been validated in synthesis on Zynq:
- N=1: 302 LUT, 444 MHz
- N=16: 941 LUT, 267 MHz
- N=512: 23,542 LUT, 136 MHz (69.7 Gops/s)

Hardware validation of multi-bank parallel operation: 80/80 PASS on Tang Nano 9K.

## What acc_zero Actually Checks

`acc_zero` returns 1 if the accumulator is exactly zero. This means:
- After LOAD: acc_zero = 1 (accumulator was cleared)
- After ACCUM(delta): acc_zero = 1 only if all accumulated deltas cancel (XOR to zero)
- After SWAP: acc_zero = 1 (accumulator was cleared)

It does **not** compare current state with initial state. It checks the accumulator register itself.
