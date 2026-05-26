# ATOMiK - Hardware-Accelerated State Change Detection

**Status:** Historical demo summary. Use this only with the current evidence
labels, claims registry, and proof packet. Do not quote this file as a universal
performance claim.

## Problem

Systems that track mutable state often need to detect what changed. In the
workload below, the software baseline rescans memory, so cost scales with both
the number of tracked regions and region size. As tracked state grows, detection
can become the bottleneck.

## Solution

ATOMiK evaluates a different path: keep a reference state, accumulate meaningful
changes, and check change state without repeatedly rescanning full regions. In
this benchmark, detection scales with region count rather than region size.

## Live Hardware Result

**Evidence label:** `LIVE_MEASURED` for the listed workload measurements when
quoted with artifact, platform, and caveat.

Measured on a Zynq FPGA running Linux 6.9:

```text
                          Software        ATOMiK       Speedup
8 regions x 4KB:         6,955,438 cy     1,223 cy     5,687x
64 regions x 4KB:       55,125,636 cy    11,837 cy     4,657x
64 regions x 1KB:        1,392,901 cy     5,881 cy       237x
```

These numbers are specific to the measured change-detection workload. They do
not establish universal speedup, battery, cooling, water, footprint, or product
readiness claims.

## How It Works

```text
current_state = initial_state XOR accumulator
```

- **LOAD**: set initial state for a tracked region.
- **ACCUM**: XOR each write delta into the accumulator.
- **DETECT**: read whether the accumulator is zero.

The algebraic model uses XOR properties including commutativity,
associativity, identity, and self-inverse cancellation. Formal proof work is
present in `math/proofs/`; public proof counts should only be quoted from an
audited proof packet or current claims registry.

## What We Have Built

- **Hardware path:** ATOMiK core on FPGA, with Zynq used for the Linux-integrated
  validation path.
- **Runtime:** libatomik C library interfaces for load, accumulate, and read
  operations.
- **Validation:** Linux userspace-to-FPGA proof artifacts, adapter validation,
  and simulation artifacts are tracked separately by evidence label.
- **Formal proof work:** Public proof files exist, but theorem counts are not a
  public headline unless reconciled across the site, README, deck, and registry.

## Why Now

Hardware acceleration is moving toward the edge, and state-heavy workloads can
pay a real cost for repeated scans, syncs, replay, and reconstruction. ATOMiK's
commercial path starts by measuring one workload, one baseline, and one painful
constraint before making broader claims.

## Ask

We are looking for design partners in edge computing, embedded systems, agent
infrastructure, and monitoring workloads that need faster state tracking on
constrained hardware.

**ATOMiK Project** - [atomik.tech](https://atomik.tech)
