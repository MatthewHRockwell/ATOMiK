# ATOMiK v3 Hardware

Custom RV32I CPU core with integrated ATOMiK delta-state datapath, targeting the Tang Nano 9K (GW1NR-LV9QN88PC6/I5).

## Current Status: v3.0.0-alpha

**Boot Chain:** BROM → ISP timeout → Flash XIP — fully validated
- CPU running at 21.6 MHz (108 MHz PLL / 5), zero timing violations
- Fmax 21.766 MHz, +0.77% margin, zero TNS
- ISP flash programmer with NACK semantics and readback verify
- Persistent flash boot validated with extended stability tests

**Resolved Issues:** V3-018 (GCC UB), V3-019 (ISP erase), V3-020 (timing violations)

See `docs/KNOWN_ISSUES.md` for details.

## ISP Flash Programming

The ISP (In-System Programming) protocol allows firmware to be written to SPI flash over UART without physical reset buttons.

### Prerequisites

- Bitstream: `synth/atomik_v3_soc.fs`
- Firmware: A `.v` hex file (e.g., `soc/firmware/fw-flash/test_flash_minimal.v`)
- UART: `/dev/ttyUSB1` at 115200 baud

### Programming Procedure

**The UART port must be opened before loading the bitstream.** The CH552T USB-UART bridge on the Tang Nano 9K drops data if the port isn't open when the FPGA starts. The ISP handshake window opens immediately after BROM boot, so any delay from opening the port afterward can miss it.

```bash
# Automated (recommended):
cd hardware/v3/synth
python3 isp_flash_programmer.py ../soc/firmware/fw-flash/test_flash_minimal.v

# The programmer handles the sequence automatically:
# 1. Opens UART (before bitstream load)
# 2. Loads bitstream via openFPGALoader (triggers CPU reset → ISP starts)
# 3. Flushes stale UART data, then performs ISP handshake
# 4. Programs firmware in 256-byte pages with checksums
# 5. Reads back and verifies every byte
```

### ISP Protocol Commands

| Command | Byte | Description |
|---------|------|-------------|
| Handshake | `0x55` | Host sends; BROM replies `0x56` |
| WBUF | `0x10 len data...` | Write up to 256 bytes to page buffer |
| ESEC | `0x30 addr2 addr1 addr0` | Erase 4KB sector |
| WPAG | `0x40 addr2 addr1 addr0` | Program page from buffer |
| RDBK | `0x50 addr2 addr1 addr0 len` | Read back flash data + checksum |
| RST | `0xF0` | Reset to BROM |

NACK responses: `0x4F` (WPAG with empty buffer), `0xFF` (unknown command).

### Boot Sequence After Programming

After programming, the boot chain is:
1. BROM starts, sends ISP handshake probe on UART
2. If no host responds within ~16s (at 21.6 MHz), BROM prints `JUMP!`
3. CPU jumps to flash XIP at address `0x00000000`
4. Flash firmware executes (e.g., prints `F!F!`)

To observe the full boot chain after power-cycle:
```bash
# Load bitstream to SRAM (does not persist across power cycles)
openFPGALoader -b tangnano9k synth/atomik_v3_soc.fs
# Wait ~16s for ISP timeout, then firmware runs
# Monitor: picocom /dev/ttyUSB1 -b 115200
```

## Directory Structure

```
hardware/v3/
├── rtl/              Verilog source modules (CPU, decode, ALU, CSR, regfile, LSU)
├── sim/
│   ├── iverilog/     Module-level testbenches (iverilog/VVP)
│   ├── verilator/    Verilator C++ test harnesses
│   └── compliance/   RISC-V ISA compliance test runner
├── soc/              SoC integration
│   ├── atomik_v3_soc.v       Top-level SoC (CPU + BSRAM + UART + SPI flash)
│   ├── firmware/fw-brom/     ISP boot ROM firmware (C)
│   ├── firmware/fw-flash/    Flash test firmware (assembly)
│   └── gowin_ip/             Gowin IP cores (PLL, BSRAM)
├── synth/            Gowin EDA synthesis (SDC, programmer, bitstream)
├── deploy/           Session logs and debug notes
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
