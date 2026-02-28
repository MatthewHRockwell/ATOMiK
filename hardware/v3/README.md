# ATOMiK v3 Hardware

Custom RV64I CPU core with integrated ATOMiK delta-state datapath, targeting the Tang Nano 9K (GW1NR-LV9QN88PC6/I5).

## Current Status (Phase 3D Complete) ✅

**Hardware Validation:** 62/62 tests PASS
- ✅ BRINGUP_MODE: CPU baseline working
- ✅ BISECT_STEP8: 10 UART reads pass
- ✅ BISECT_STEP7: 10,000 UART reads pass (60/60 consecutive)
- ✅ Extended stress: 50 consecutive test cycles (~3.5 min)
- ✅ Thermal stability: Validated through 60s warmup
- ✅ ISP handshake: 0x55→0x56 ACK + echo working

**Timing:** Clean closure at 25.2 MHz (TNS = 0.000 ns, Fmax = 25.201 MHz)

**Known Issues:**
- ⚠️ Thin timing margin (+0.004%) - consider 24 MHz for production
- ⚠️ AUIPC instruction broken (workaround: use `li` instead of `la`)
- ⏸️ Flash boot chain not yet validated
- ⏸️ ATOMiK hardware tests pending

See `deploy/HARDWARE_VALIDATION_COMPLETE.md` for detailed test results.

## Directory Structure

```
hardware/v3/
├── rtl/              Verilog source modules
├── sim/
│   ├── iverilog/     Module-level testbenches (iverilog/VVP)
│   ├── verilator/    Verilator C++ test harnesses
│   └── compliance/   RISC-V ISA compliance test runner (scaffold)
├── synth/            Gowin EDA synthesis scripts
├── Makefile          Unified build system
└── README.md         This file
```

## Quick Start

```bash
# Lint all RTL
make lint

# Run iverilog smoke tests
make sim-iverilog

# Run Verilator smoke tests
make sim-verilator

# Run both
make sim-smoke

# Default target (lint + sim-smoke)
make
```

## Tool Requirements

| Tool | Version | Location |
|------|---------|----------|
| Verilator | 5.021+ | `~/Tools/oss-cad-suite/bin/verilator` |
| iverilog | 13.0+ | `~/Tools/oss-cad-suite/bin/iverilog` |
| riscv64-unknown-elf-gcc | 13.2.0+ | `/usr/bin/` |
| Gowin EDA | V1.9.12.01 | `/opt/gowin/IDE/` (local synthesis only) |

The Makefile auto-discovers tools from `~/Tools/oss-cad-suite/bin/` first, falling back to system PATH.

## Build Targets

| Target | Description |
|--------|-------------|
| `make lint` | Verilator `--lint-only -Wall` on all `rtl/*.v` |
| `make sim-iverilog` | Compile and run all `sim/iverilog/tb_*.v` |
| `make sim-verilator` | Verilate, compile, run all `sim/verilator/tb_*.cpp` |
| `make sim-smoke` | Both iverilog and Verilator smoke tests |
| `make compliance` | RISC-V compliance tests (placeholder) |
| `make synth` | Gowin synthesis reminder (local only) |
| `make clean` | Remove build artifacts |
| `make all` | `lint` + `sim-smoke` (default) |

## Relationship to v2

The v2 hardware (`hardware/rtl/`, `hardware/sim/`, `hardware/synth/`) remains unchanged. v3 is a clean-room implementation with a custom CPU core (replacing PicoRV32) and redesigned ATOMiK datapath.

## Spec and Roadmap

- Architecture spec: `specs/atomik_v3.md`
- Task breakdown: `specs/atomik_v3_tasks.md`
