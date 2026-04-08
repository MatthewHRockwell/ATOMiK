# ATOMiK — Hardware-Accelerated State Change Detection

## Problem

Every system that tracks mutable state — agents, edge nodes, databases, security monitors — needs to detect what changed. Today that means rescanning memory: **O(N x size)**, scaling with both the number of tracked regions and their size. As state grows, detection becomes the bottleneck.

## Solution

ATOMiK is a hardware accelerator that makes change detection **O(N)** — scaling only with region count, not region size. Deltas accumulate in hardware at write time. Detection is a single register read per region: did anything change? Yes or no. Constant time.

## Live Hardware Result

Measured on a $50 Zynq FPGA running Linux 6.9:

```
                          Software        ATOMiK       Speedup
8 regions x 4KB:         6,955,438 cy     1,223 cy     5,687x
64 regions x 4KB:       55,125,636 cy    11,837 cy     4,657x
64 regions x 1KB:        1,392,901 cy     5,881 cy       237x
```

**Software gets 5,500x slower going from 256B to 4KB regions. ATOMiK stays flat.**

## How It Works

```
current_state = initial_state ⊕ accumulator
```

- **LOAD**: set initial state for a tracked region
- **ACCUM**: XOR each write delta into the accumulator (happens at write time)
- **DETECT**: read one flag — is the accumulator zero? (O(1))

The math is an Abelian group (XOR): commutative, associative, self-inverse. 92 Lean4 theorems prove the algebra. Order of writes doesn't matter. Deltas cancel automatically.

## What We've Built

- **Hardware**: ATOMiK core on FPGA ($13.50 Tang Nano 9K standalone, Zynq for Linux)
- **Runtime**: libatomik C library — `atomik_load()`, `atomik_accum()`, `atomik_read()`
- **Validation**: 16/16 Linux userspace PASS, 9/9 adapter PASS, 20/20 simulation PASS
- **Formal proof**: 92 Lean4 theorems on the delta-state algebra

## Why Now

Hardware acceleration is moving to the edge. ATOMiK is the first architecture that makes state-change detection a hardware primitive instead of a software scan. Every agent, every edge device, every security monitor that tracks state pays this cost today in software. We eliminate it.

## Ask

We're looking for design partners in edge computing, agent infrastructure, and security monitoring who need faster state tracking on constrained hardware.

**ATOMiK Project** — [atomik.tech](https://atomik.tech)
