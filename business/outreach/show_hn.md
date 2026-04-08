# Show HN Post — ATOMiK

## Title

Show HN: ATOMiK – 1 Gops/s delta-state computation on a $13.50 FPGA, 108 formal proofs

## Body

I've been building ATOMiK for the past 6 months. It's a hardware-accelerated delta-state computing architecture — and it's running on real silicon.

**The idea:** Instead of copying full state on every update (read-modify-write), accumulate XOR deltas. State reconstruction is always O(1): `State = Initial XOR d1 XOR d2 XOR ... XOR dn`. One operation, one clock cycle, 10.6 nanoseconds.

**Why XOR is interesting for this:**
- Commutative → order doesn't matter → lock-free parallelism for free
- Self-inverse → every operation is its own undo → no checkpoints needed
- Zero carry chains → single-cycle at any width → pure LUT computation
- Associative → parallel bank architecture → linear throughput scaling

**What I built:**
- 108 formally verified proofs in Lean4 (not tested — machine-verified)
- Production SoC on Tang Nano 9K FPGA ($13.50) with custom RV64I CPU + HDMI
- 1 Gops/s throughput with 16 parallel banks (validated)
- 80/80 hardware sweep tests + 5/5 integration tests
- SDK in Python, Rust, C, JavaScript, and Verilog (353 tests)
- Patent pending

**Total cost: $225.** Consumer laptop, two Tang Nano 9K boards, that's it.

**How it compares:**
- vs. Event Sourcing (Kafka): O(1) reconstruction vs. O(N) replay
- vs. CRDTs: Hardware speed (10.6ns) vs. software speed (microseconds)
- vs. Traditional FPGA accelerators: One reusable primitive vs. months of custom RTL per application

The architecture scales linearly — 16 banks on this tiny FPGA already break 1 Gops/s. On larger devices (Zynq, Kintex), 32x and 64x configurations are straightforward.

I'm building this into an IP licensing company (ARM model). The formal verification moat is real — you can't shortcut 92 machine-checked theorems in Lean4.

Code is on GitHub (Apache 2.0 for evaluation): https://github.com/MatthewHRockwell/ATOMiK

Happy to answer questions about the math, the hardware, or the business model. I built every piece of this — proofs, RTL, SoC, SDK — so I can go deep on any of it.
