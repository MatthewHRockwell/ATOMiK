# ATOMiK Zynq Port — ALINX AX7020

ATOMiK multi-bank accelerator on Xilinx Zynq-7020 PL fabric, accessed from Linux via AXI4-Lite + UIO.

**Board:** ALINX AX7020 (XC7Z020-2CLG400I, industrial grade, -2 speed)
**Architecture:** ATOMiK as AXI4-Lite peripheral — ARM Cortex-A9 runs Linux on PS
**Status:** Pre-board — directory structure and documentation ready

---

## Quick Start

```bash
# 1. Build bitstream (requires Vivado WebPACK)
source /tools/Xilinx/Vivado/2023.2/settings64.sh
make bitstream

# 2. Program FPGA via JTAG
make program

# 3. Boot Linux from SD card
# (PetaLinux image must be prepared separately — see docs/reference/xilinx/LINUX_SETUP_GUIDE.md)

# 4. On the Zynq (via SSH or serial):
cd /home/root/atomik
./atomik_test          # Run ATOMiK verification tests
./atomik_bench         # Run performance benchmarks
```

---

## Directory Structure

```
hardware/zynq/
├── README.md                   # This file
├── Makefile                    # Top-level build (bitstream + firmware)
│
├── rtl/                        # PL-specific RTL (wrappers, clocking)
│   ├── atomik_axi4lite_wrapper.v   # AXI4-Lite slave → ATOMiK core
│   ├── atomik_zynq_top.v           # PL top-level
│   └── atomik_zynq_clk.v           # MMCM instantiation
│
├── constraints/                # Vivado constraints
│   └── ax7020.xdc                  # Pin assignments + timing
│
├── vivado/                     # Vivado project scripts
│   ├── create_project.tcl          # Create Vivado project
│   ├── build.tcl                   # Non-project-mode full build
│   └── block_design.tcl            # PS + PL block design (IPI)
│
├── sim/                        # PL simulation
│   ├── tb_axi4lite_wrapper.v       # AXI4-Lite protocol testbench
│   └── tb_zynq_integration.v       # Full PL integration test
│
├── firmware/                   # Software for ARM PS
│   ├── linux/                      # Linux userspace
│   │   ├── libatomik.h             # ATOMiK C library header
│   │   ├── libatomik.c             # UIO mmap wrapper
│   │   ├── atomik_test.c           # Verification test program
│   │   ├── atomik_bench.c          # Performance benchmark suite
│   │   ├── Makefile                # Cross-compilation
│   │   └── devicetree/
│   │       └── atomik-overlay.dts  # Device tree overlay
│   └── baremetal/                  # Standalone (no-OS) testing
│       ├── atomik_standalone.c     # Bare-metal test application
│       └── lscript.ld              # Linker script
│
├── petalinux/                  # PetaLinux project (gitignored, regenerable)
│   └── README.md                   # Setup instructions
│
├── experiments/                # Benchmark results
│   └── data/                       # JSONL pool (append-only)
│
└── deploy/                     # Session logs and debug notes
    └── README.md
```

---

## Shared RTL

The core ATOMiK modules live in `hardware/v3/rtl/` and are **shared** — not duplicated:

| Module | Path | Purpose |
|--------|------|---------|
| `atomik_v3_atomik.v` | `hardware/v3/rtl/` | Single-bank ATOMiK core (XOR acc + BSRAM state table) |
| `atomik_v3_parallel.v` | `hardware/v3/rtl/` | N-bank parallel with XOR merge tree |

The Vivado project references these via relative paths in the filelist or TCL `add_files` commands. The `rtl/` directory in this tree contains only Zynq-specific wrapper logic (AXI4-Lite interface, MMCM clocking, PL top-level).

### Synthesis Attribute Translation

| Gowin (GW1NR-9) | Xilinx (7-series) | Purpose |
|------------------|-------------------|---------|
| `(* syn_keep = 1 *)` | `(* DONT_TOUCH = "TRUE" *)` | Prevent XOR path optimization |
| `(* syn_preserve = 1 *)` | `(* DONT_TOUCH = "TRUE" *)` | Prevent register removal |
| `(* syn_ramstyle = "block_ram" *)` | `(* ram_style = "block" *)` | Force BRAM inference |
| `(* syn_dspstyle = "dsp" *)` | `(* use_dsp = "yes" *)` | Force DSP inference |

The AXI4-Lite wrapper will use Xilinx-native attributes. The shared core modules use Gowin attributes which Vivado ignores (treated as unknown, no error). If Vivado-specific attributes are needed on the shared modules, they can be added alongside Gowin attributes (both tools ignore unrecognized attributes from the other vendor).

---

## Architecture

```
                    ALINX AX7020
    ┌─────────────────────────────────────────────────┐
    │                                                 │
    │   PS (Processing System)                        │
    │   ┌───────────────────────┐                     │
    │   │  Dual Cortex-A9       │                     │
    │   │  667 MHz              │                     │
    │   │  1 GB DDR3            │                     │
    │   │  Linux (Ubuntu)       │                     │
    │   │                       │                     │
    │   │  ┌─────────────────┐  │    M_AXI_GP0        │
    │   │  │ UIO Driver      │──┼──────────────┐      │
    │   │  │ /dev/uio0       │  │              │      │
    │   │  │ mmap() regs     │  │              │      │
    │   │  └─────────────────┘  │              │      │
    │   │                       │              │      │
    │   │  UART0  ETH0  USB    │              │      │
    │   └───┼──────┼─────┼─────┘              │      │
    │       │      │     │                     │      │
    │   ────┴──────┴─────┴─── MIO ────         │      │
    │                                          │      │
    │   PL (Programmable Logic)                │      │
    │   ┌──────────────────────────────────────┤      │
    │   │                                      │      │
    │   │   AXI4-Lite Wrapper                  │      │
    │   │   ┌──────────────────────┐           │      │
    │   │   │  Register Map        │◀──────────┘      │
    │   │   │  (LOAD, ACCUM, READ, │                  │
    │   │   │   SWAP, STATUS, CFG) │                  │
    │   │   └──────────┬───────────┘                  │
    │   │              │ CDC                           │
    │   │   ┌──────────┴───────────┐                  │
    │   │   │  ATOMiK Multi-Bank   │                  │
    │   │   │  (N=1..256 banks)    │                  │
    │   │   │  XOR merge tree      │                  │
    │   │   │  BRAM state table    │                  │
    │   │   └──────────────────────┘                  │
    │   │              │                               │
    │   │   ┌──────────┴───────────┐                  │
    │   │   │  MMCM Clock Gen      │                  │
    │   │   │  50 MHz → 100+ MHz   │                  │
    │   │   └──────────────────────┘                  │
    │   │                                             │
    │   └─────────────────────────────────────────────│
    │                                                 │
    │   HDMI   GigE   JTAG   QSPI   SD   RTC        │
    └─────────────────────────────────────────────────┘
```

---

## Tool Requirements

| Tool | Version | Purpose | Install |
|------|---------|---------|---------|
| Vivado | 2023.2+ | Synthesis, implementation, JTAG programming | [AMD Downloads](https://www.xilinx.com/support/download.html) (WebPACK, free) |
| PetaLinux | 2023.2+ | Linux BSP, device tree, kernel | AMD installer |
| arm-linux-gnueabihf-gcc | 9+ | Cross-compile userspace apps | `apt install gcc-arm-linux-gnueabihf` |
| Icarus Verilog | Any | PL module simulation | `apt install iverilog` |
| Verilator | 5+ | PL linting | `apt install verilator` |

---

## Related Documentation

| Document | Path |
|----------|------|
| Architecture Spec | `specs/zynq_port.md` |
| Task List | `specs/zynq_port_tasks.md` |
| Xilinx Reference Docs | `docs/reference/xilinx/` |
| ATOMiK v3 Core RTL | `hardware/v3/rtl/atomik_v3_atomik.v` |
| Parallel Banks RTL | `hardware/v3/rtl/atomik_v3_parallel.v` |
| Gowin Reference Docs | `docs/reference/gowin/` |
| Project Roadmap | `ROADMAP.md` (Section 15) |

---

*Last Updated: March 7, 2026*
