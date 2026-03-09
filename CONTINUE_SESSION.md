# Continue Session — ATOMiK Zynq Port

**Last updated**: March 8, 2026
**Board ETA**: ~March 22, 2026 (ALINX AX7020, XC7Z020-2CLG400I)

---

## Current Status: Waiting for Hardware

Everything that can be done without the physical board is done. Vivado is installed, PL synthesis passes, all software is written and tested in mock mode.

---

## What's Complete

### Tooling (Z0.1, Z0.2 partial)
- **Vivado 2025.2 ML Standard** installed at `/opt/Xilinx/2025.2/`
  - Source: `source /opt/Xilinx/2025.2/Vivado/settings64.sh`
  - Zynq-7000 parts recognized, PL-only build passes cleanly
- **Gowin EDA** still at `/opt/gowin/IDE/` (Tang Nano 9K / v3 SoC)
- **11 reference documents** written in `docs/reference/xilinx/`

### RTL — `hardware/zynq/rtl/` (3 files)
| File | Purpose |
|------|---------|
| `atomik_axi4lite_wrapper.v` | AXI4-Lite ↔ 64-bit ATOMiK bridge. Register map: LOAD, ACCUM, STATE, STATUS, SWAP, CONFIG. LO/HI split protocol. |
| `atomik_zynq_top.v` | PL structural top (clock module + AXI wrapper) |
| `atomik_zynq_clk.v` | Clock generation (Phase 1: passthrough. Phase 2: MMCM placeholder) |

Shared core: `hardware/v3/rtl/atomik_v3_atomik.v` (single-bank ATOMiK, same as Tang Nano 9K)

### Build Scripts — `hardware/zynq/vivado/` (3 files)
| File | Purpose |
|------|---------|
| `build.tcl` | PL-only non-project synthesis + implementation. **Validated — runs clean.** Bitstream gated (needs `--bitstream` flag). |
| `block_design.tcl` | Full PS+PL block design: Zynq PS7 + ATOMiK @ 0x43C00000, DDR3, UART, ETH, USB, SD. **Primary flow for hardware.** |
| `program.tcl` | JTAG volatile programmer |

### Build Automation — `hardware/zynq/Makefile`
- `make sim` — iverilog testbench (52 AXI protocol tests)
- `make blockdesign` — Vivado block design (PS+PL, generates bitstream + .xsa)
- `make bitstream` — PL-only synthesis
- `make program` — JTAG loader
- `make clean` / `make help`

### Simulation — `hardware/zynq/sim/`
- `tb_axi4lite_wrapper.v` — 52 AXI4-Lite protocol tests (iverilog/vvp)

### Constraints — `hardware/zynq/constraints/`
- `ax7020.xdc` — FCLK_CLK0 @ 100 MHz. No PL I/O pins needed (AXI connects internally).

### Device Tree — `hardware/zynq/dts/`
- `atomik_uio.dtsi` — ATOMiK at 0x43C00000, `compatible = "generic-uio"`, maps to `/dev/uio0`

### C Library — `software/libatomik/`
| File | Purpose |
|------|---------|
| `libatomik.h` | Public API: register map, handle struct, all operations |
| `libatomik.c` | UIO mmap implementation (open, close, load, accum, read, swap, etc.) |
| `test_libatomik.c` | 33 tests — **33/33 PASS** in mock mode |
| `Makefile` | `test-mock` (local), `lib` (cross-compile .so/.a), `test-hw` (ARM target) |

### Python Bindings — `software/libatomik/`
| File | Purpose |
|------|---------|
| `atomik_zynq.py` | Pure Python UIO mmap wrapper + mock backend |
| `test_atomik_zynq.py` | 35 tests — **35/35 PASS** in mock mode |

### PL-Only Synthesis Results (xc7z020, March 8 2026)
| Resource | Used | Available | Util% |
|----------|------|-----------|-------|
| LUT Logic | 287 | 53,200 | 0.54% |
| LUT RAM | 344 | 17,400 | 1.98% |
| Flip Flops | 471 | 106,400 | 0.44% |
| BRAM | 0 | 140 | 0% |
| DSP | 0 | 220 | 0% |

Timing unconstrained in PL-only mode (no PS clock source). Will be constrained in block design.

---

## Specifications & Task Tracking

- **Architecture spec**: `specs/zynq_port.md` (591 lines — register map, scaling plan, risk register)
- **Task list**: `specs/zynq_port_tasks.md` (4 phases, checkbox-tracked)
- **Board reference docs**: `docs/reference/xilinx/` (7 docs)
- **Zynq README**: `hardware/zynq/README.md` (quick start, architecture diagram, tool requirements)

---

## When the Board Arrives — Do This

### Step 1: Board files and blink test (Z0.2)
```bash
# Source Vivado
source /opt/Xilinx/2025.2/Vivado/settings64.sh

# Import ALINX board files (check what ships with the board)
# Then: trivial PL blink design → JTAG program → verify LEDs
```

### Step 2: PS+PL block design (Z1.2)
```bash
cd hardware/zynq
make blockdesign    # Runs block_design.tcl → bitstream + .xsa
make program        # JTAG load to FPGA
```
This creates the Zynq PS (ARM cores, DDR3, peripherals) + ATOMiK PL in one design.

### Step 3: Linux setup (Z0.3)
- Install PetaLinux (matching Vivado 2025.2)
- Create project with AX7020 BSP (ALINX provides one)
- Build, boot via SD card, verify UART + SSH
- Add UIO device tree overlay from `hardware/zynq/dts/atomik_uio.dtsi`

### Step 4: First ATOMiK operation from Linux (Z1.5, Z1.6)
```bash
# On target (Zynq ARM):
# Quick smoke test with devmem2
devmem2 0x43C0001C    # Read STATUS register

# Run libatomik tests
./test_libatomik       # 33 hardware tests

# Python
python3 test_atomik_zynq.py  # 35 tests
```

### Step 5: Measure and document (Z1.4)
- Record definitive utilization + Fmax from block design build
- Update `specs/zynq_port_tasks.md` checkboxes
- Compare against estimates in `docs/reference/xilinx/RESOURCE_BUDGET_GUIDE.md`

---

## Key Files Quick Reference

```
hardware/zynq/
├── rtl/                          # Zynq-specific Verilog
│   ├── atomik_axi4lite_wrapper.v # AXI ↔ ATOMiK bridge
│   ├── atomik_zynq_top.v        # PL top-level
│   └── atomik_zynq_clk.v        # Clock (Phase 1: passthrough)
├── vivado/
│   ├── build.tcl                 # PL-only synthesis (validated)
│   ├── block_design.tcl          # PS+PL primary flow
│   └── program.tcl               # JTAG loader
├── sim/
│   └── tb_axi4lite_wrapper.v     # 52 AXI protocol tests
├── constraints/
│   └── ax7020.xdc                # Timing constraints
├── dts/
│   └── atomik_uio.dtsi           # UIO device tree overlay
├── Makefile                      # sim, blockdesign, bitstream, program
└── README.md                     # Overview + architecture diagram

software/libatomik/
├── libatomik.h                   # C API header
├── libatomik.c                   # C UIO implementation
├── test_libatomik.c              # C tests (33/33 mock PASS)
├── atomik_zynq.py                # Python bindings
├── test_atomik_zynq.py           # Python tests (35/35 mock PASS)
└── Makefile                      # test-mock, lib, test-hw

specs/
├── zynq_port.md                  # Architecture specification
└── zynq_port_tasks.md            # Phased task list with checkboxes

docs/reference/xilinx/            # 7 reference documents
hardware/v3/rtl/atomik_v3_atomik.v  # Shared ATOMiK core (used by both platforms)
```

---

## Design Decisions to Remember

- **ATOMiK is an AXI peripheral**, not a custom CPU. ARM Cortex-A9 runs Linux; ATOMiK lives in PL fabric.
- **32-bit AXI ↔ 64-bit datapath**: LO write latches, HI write triggers operation. This is protocol-critical.
- **Atomic read snapshot**: STATE_LO read triggers latch, STATE_HI returns captured upper half.
- **Phase 1 = single clock domain** (FCLK_CLK0 @ 100 MHz for both AXI and ATOMiK). Phase 2 adds MMCM for separate ATOMiK clock (~200 MHz) with CDC bridge.
- **Same ATOMiK core** as Tang Nano 9K v3 SoC — `atomik_v3_atomik.v` is shared, not duplicated.
- **Do NOT source Vivado TCL from /dev/stdin** — it silently fails. Always write a .tcl file.
