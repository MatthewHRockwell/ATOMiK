# ATOMiK — Delta-State Computing in Silicon

**1 Billion Operations/Second on a $13.50 Chip**

ATOMiK is a hardware-accelerated delta-state computing architecture that replaces traditional full-state updates with XOR-based delta accumulation. Every operation completes in a single clock cycle (10.6 ns), scales linearly with parallel banks, and is backed by 108 machine-verified mathematical proofs.

---

## Key Metrics

| Metric | Value |
|--------|-------|
| **Deployment Status** | ✅ **PRODUCTION** — Two SoC generations deployed on Tang Nano 9K |
| **v3 SoC (latest)** | Custom RV64I CPU + ATOMiK direct-wire, 1280×720 HDMI, 8-screen demo |
| **v2 SoC** | PicoRV32 + ATOMiK accelerator, 81 MHz, dual-clock CDC |
| Throughput (validated) | **1,056 Mops/s** (16 parallel banks) |
| v3 Memcpy speedup | **6.4× faster** than software (v2 was 12% slower) |
| Operation latency | **10.6 ns** (single cycle @ 94.5 MHz standalone) |
| Memory reduction | **95-100%** (sparse deltas vs. dense state) |
| Formal proofs | **108** (Lean4 verified, 0 sorry statements) |
| Hardware tests | **80/80** passing (sweep) + v3: 9/9 ATOMiK + 10/10 Phase 2 + 6/6 Display |
| Timing closure | **0 TNS** across all clock domains (v2 and v3) |
| LUT utilization | **69%** (v3 SoC) / **44%** (v2 SoC) / 20% (standalone 16-bank) |
| SDK languages | **5** (Python, Rust, C, JavaScript, Verilog) |
| SDK tests | **353** passing |
| Device cost | **$13.50** (Tang Nano 9K FPGA) |
| Total dev cost | **~$225** (AI-augmented development) |

## How It Works

Traditional systems copy full state on every update. ATOMiK stores only the changes (deltas) and reconstructs state on demand using XOR — a mathematically perfect operation with zero carry chains and natural parallelism.

```
State_current = Initial XOR delta_1 XOR delta_2 XOR ... XOR delta_n
```

**Properties** (formally proven):
- **Commutative**: Order doesn't matter — enables lock-free parallelism
- **Self-inverse**: Every change is its own undo — no checkpoints needed
- **Single-cycle**: No carry propagation — pure LUT-based computation

## Architecture

N parallel XOR accumulator banks with a binary merge tree achieve linear throughput scaling. 16 banks on a $13.50 FPGA break the 1 Gops/s barrier. The architecture extends to 32x, 64x, and beyond on larger FPGAs.

## Market Applications

- **High-Frequency Trading**: Single-cycle tick processing with instant trade reversal
- **IoT/Sensor Fusion**: Lock-free multi-stream merge at edge-device power budgets
- **Video Processing**: 95% memory reduction for frame delta pipelines
- **Database Replication**: O(1) state reconstruction vs. O(N) event replay
- **Digital Twins**: Commutative merge enables distributed state synchronization
- **Gaming**: Order-independent multiplayer state sync with instant rollback

---

## Competitive Moat

- **Patent Pending**: Architecture and execution model under IP protection
- **Formal Verification**: 108 Lean4 proofs — machine-verified, not hand-tested
- **Hardware Validated**: Real FPGA silicon, not just simulation
- **Full Stack**: Math proofs + RTL + SDK + agentic pipeline — 6 phases complete
- **Linear Scaling**: Proven to 16x, extends to 64x+ with larger devices

## Business Model

1. **IP Licensing**: RTL cores for chip designers and FPGA integrators
2. **Hardware Accelerator IP**: Pre-built vertical modules (HFT, IoT, video)
3. **SDK Platform**: Schema-driven code generation subscription
4. **Professional Services**: Custom enterprise integration

---

## Development Status

| Phase | Status |
|-------|--------|
| Mathematical Formalization (108 proofs) | ✅ Complete |
| Hardware Synthesis (Tang Nano 9K) | ✅ Complete |
| SDK Code Generation (5 languages) | ✅ Complete |
| Agentic Pipeline (25 modules, 353 tests) | ✅ Complete |
| Parallel Scaling (16x, 1 Gops/s) | ✅ Complete |
| **v2 Production SoC** (PicoRV32) | ✅ **DEPLOYED** (Feb 2026) |
| **v3 Production SoC** (Custom RV64I + HDMI) | ✅ **DEPLOYED** (Mar 2026) |
| 3-Node VC Demo | ✅ Complete |
| Zynq Port (ALINX AX7020) | 🔄 In Progress (52/52 sim tests, board on order) |

**v3 SoC Milestone** (Mar 2026): Custom RV64I CPU with direct-wire ATOMiK integration deployed on Tang Nano 9K. Features 1280×720@60Hz HDMI output with delta-driven display pipeline (`pixel_out = pixel_ref ⊕ LUT[index]`), 8-screen auto-cycling investor demo, and 6.4× memcpy speedup over software. All test suites passing (9/9 ATOMiK, 10/10 Phase 2, 6/6 Display). Zero TNS across all clock domains.

**v2 SoC Milestone** (Feb 2026): PicoRV32 SoC accelerator with dual-clock CDC, 81 MHz ATOMiK core, clean timing closure, persistent flash deployment.

## Contact

**ATOMiK — Delta-State Computing in Silicon**
*Patent Pending*
Repository: github.com/MatthewHRockwell/ATOMiK
License: Apache 2.0 (evaluation) — Commercial license available
