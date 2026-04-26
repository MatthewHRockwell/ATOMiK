# ATOMiK

**A hardware primitive for state-aware computing**

---

## The problem

Modern systems spend enormous compute and energy rediscovering what changed.

Agents, replicas, caches, sessions, logs, and edge endpoints repeatedly rescan, compare, and rebuild state — even when only a small part actually changed.

## The ATOMiK approach

ATOMiK turns change into a hardware primitive.

Instead of forcing software to repeatedly detect deltas, ATOMiK tracks state transitions directly in silicon so systems can act on meaningful change without paying the full rediscovery cost each time.

## What we demonstrated live

- ATOMiK running on real hardware (Zynq-7020 FPGA, NaxRiscv RV64GC, Ubuntu 24.04)
- Interactive state changes on the board
- Multi-surface update across HDMI, LCD, and browser replica
- Live benchmark results across 256B, 4KB, and 64KB workloads

**69% lower compute / energy / cost**
**~$34K annual savings model at 1,000 servers**

## Adoption path

ATOMiK is demonstrated with a standard C / GCC workflow:

```c
#include "atomik.h"

atomik_load(slot, initial_state);
atomik_accum(delta);
uint64_t current = atomik_read(slot);
```

Same compiler. No new language. No new religion.

## Initial wedge

State-heavy edge and distributed systems where most software effort is spent figuring out what changed rather than acting on it.

## What investors should remember

1. ATOMiK makes change detection and propagation dramatically cheaper.
2. It is already running live in hardware.
3. It has a believable adoption path.

---

**Matthew Rockwell**
ATOMiK
