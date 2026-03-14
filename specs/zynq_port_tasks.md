# ATOMiK Zynq Port — Phased Implementation Task List

**Reference**: `specs/zynq_port.md`
**Target**: ALINX AX7020 (XC7Z020-2CLG400I)
**Baseline**: v3.1.0 SoC — RV64I + ATOMiK @ 21.6 MHz, 6,287 LUT (73%), all tests passing on Tang Nano 9K

---

## Zynq Phase 0: Documentation & Tooling

**Goal**: Complete reference documentation and set up the Vivado/PetaLinux development environment. Everything downstream depends on this.

### Z0.1 Reference Documentation
- [x] Create `docs/reference/xilinx/` directory and `README.md`
- [x] Write `AX7020_BOARD_REFERENCE.md`
- [x] Write `ZYNQ_PS_CONFIGURATION.md`
- [x] Write `AXI_INTEGRATION_GUIDE.md`
- [x] Write `CLOCK_REFERENCE.md`
- [x] Write `RESOURCE_BUDGET_GUIDE.md`
- [x] Write `VIVADO_BUILD_GUIDE.md`
- [x] Write `LINUX_SETUP_GUIDE.md`
- [x] Write `specs/zynq_port.md` (architecture spec)
- [x] Update `ROADMAP.md` with Zynq port section
- [x] Create `hardware/zynq/` directory and `README.md`

### Z0.2 Vivado Environment Setup
- [x] Install Vivado 2025.2 ML Standard (at `/opt/Xilinx/2025.2/`)
- [ ] Create AX7020 board files / import ALINX BSP
- [ ] Build trivial PL blink design, program via JTAG
- [x] Verify Vivado TCL flow (non-project mode) — `build.tcl` synth+impl PASS on xc7z020

### Z0.3 PetaLinux / Linux Setup
- [ ] Install PetaLinux (matching Vivado version)
- [ ] Create PetaLinux project with AX7020 BSP (ALINX provides one)
- [ ] Build and boot minimal Linux (verify UART, ETH)
- [ ] Document boot sequence and serial console access
- [ ] Set up SSH access via GigE

### Z0.4 Cross-Compilation Toolchain
- [ ] Verify `arm-linux-gnueabihf-gcc` is installed
- [ ] Build trivial C program, run on Zynq via SSH
- [ ] Set up build system (Makefile) for userspace apps
- [ ] Verify Python3 available on target (for scripting)

**Exit criteria**: All 11 reference documents written. Vivado synthesizes for AX7020 (DONE — PL-only build PASS). Blink design on hardware (requires board). PetaLinux boots to a Linux shell with UART and Ethernet. Cross-compiled C binary runs on target via SSH.

---

## Zynq Phase 1: ATOMiK PL Bringup (N=1)

**Goal**: Get a single ATOMiK bank working as an AXI4-Lite peripheral, accessible from Linux userspace via UIO.

**Dependencies**: Zynq Phase 0 (Vivado + Linux environment required)

### Z1.1 AXI4-Lite Wrapper RTL
- [x] Write `atomik_axi4lite_wrapper.v` (adapts `atomik_v3_atomik.v` to AXI4-Lite)
- [x] Implement register map (LOAD, ACCUM, STATE, STATUS, SWAP, CONFIG)
- [x] Handle 32-bit AXI <-> 64-bit datapath (LO/HI register pair, HI triggers op)
- [x] Add `DONT_TOUCH` attributes (replacing Gowin `syn_keep`) on XOR paths
- [x] Simulation testbench: AXI4-Lite protocol verification (iverilog — `hardware/zynq/sim/tb_axi4lite_wrapper.v`)

### Z1.2 Vivado Block Design
- [ ] Create PS block design (Zynq PS, DDR3 config, UART, ETH)
- [ ] Add ATOMiK as custom AXI4-Lite peripheral in PL
- [ ] Connect via M_AXI_GP0 (general-purpose master port)
- [ ] Assign address map (0x43C00000, 4KB range)
- [ ] Generate output products, create HDL wrapper

### Z1.3 MMCM Clock Configuration
- [x] Configure MMCM for ATOMiK clock domain (target: 100–500 MHz from 50 MHz FCLK)
  - MMCM instantiated in `atomik_axi4lite_wrapper.v`, parameterized via `CORE_FREQ_MHZ`
  - Fmax sweep script (`scripts/fmax_sweep.py`) tests 100–500 MHz in 10 MHz steps
- [x] Implement CDC bridge between AXI clock and ATOMiK clock
  - `rtl/atomik_cdc_bridge.v` — toggle-handshake protocol, 2FF synchronizers both directions
  - Core-side FSM: 11 states (IDLE→EXEC→WAIT1-7→SETTLE→ACK) for 4-stage SWAP pipeline + BRAM output register
  - AXI-side FSM: 3 states (IDLE→WAIT→DONE)
- [x] Write XDC timing constraints for both domains
  - `vivado/timing.xdc` — dual-clock constraints, false paths on CDC toggle signals
- [x] Run timing analysis, verify zero TNS
  - **444.4 MHz ceiling** (WNS=+0.009ns) with aggressive Vivado directives
  - Full ceiling characterization: N=1 (444 MHz) through N=512 (136 MHz) — see Phase 2

### Z1.4 Synthesis and Resource Check
- [x] Run Vivado synthesis + implementation (PL-only, `build.tcl`)
  - Initial: 287 LUT logic (0.54%), 344 LUT RAM (1.98%), 471 FF (0.44%), 0 BRAM, 0 DSP
  - After XPM BRAM optimization: 289 LUT (0.54%), 1 BRAM36, 0 LUTRAM — state table in dedicated BRAM
- [x] Fmax sweep with MMCM-driven core clock (PL-only, `scripts/fmax_sweep.py`)
  - 400 MHz timing-met (WNS=+0.001ns) — 4.2x improvement over Gowin GW1NR-9 (95.9 MHz)
  - Key optimizations: XPM BRAM (READ_LATENCY_B=2), 4-stage SWAP pipeline, aggressive Vivado directives
- [x] Compare against estimates in `RESOURCE_BUDGET_GUIDE.md`
- [x] Update docs with measured values (replace ~ estimates)

### Z1.5 Linux UIO Driver
- [x] Write device tree overlay for ATOMiK peripheral (`compatible = "generic-uio"`)
  - `hardware/zynq/dts/atomik_uio.dtsi` — ATOMiK at 0x43C00000, generic-uio compatible
- [ ] Rebuild PetaLinux with UIO kernel config (`CONFIG_UIO`, `CONFIG_UIO_PDRV_GENIRQ`)
- [x] Write userspace test program:
  - Open `/dev/uio0`, mmap register space
  - LOAD initial state, ACCUM delta, READ current_state
  - Verify XOR cancellation: `load(x), accum(x), read() == 0`
  - Verify state reconstruction: `load(a), accum(d), read() == a^d`
  - `software/libatomik/test_libatomik.c` — 33 tests, mock mode passes locally
- [ ] Verify from command line: `devmem2` reads at ATOMiK base address

### Z1.6 libatomik C Library
- [x] Write `libatomik.h` / `libatomik.c` (UIO mmap wrapper)
  - `software/libatomik/libatomik.h` — public API with register map, handle struct, all operations
  - `software/libatomik/libatomik.c` — UIO mmap implementation
- [x] Functions: `atomik_open()`, `atomik_close()`, `atomik_load()`, `atomik_accum()`, `atomik_read()`, `atomik_swap()`
  - Plus: `atomik_acc_zero()`, `atomik_bank_count()`, `atomik_version()`, `atomik_set_enable()`, `atomik_is_enabled()`
- [x] Handles LO/HI register splitting internally
- [x] Build as shared library (`libatomik.so`)
  - `software/libatomik/Makefile` — targets: test-mock, lib, test-hw, clean
  - Compiles with `-Wall -Wextra -Werror` zero warnings
- [x] Mock test suite: 33/33 PASS (runs locally without hardware)
- [ ] Unit tests on target (requires hardware)

**Exit criteria**: ATOMiK N=1 operational as AXI4-Lite peripheral. UIO driver working. `libatomik.so` passes all unit tests on target. XOR cancellation and state reconstruction verified from Linux userspace. Synthesis report shows LUT6/FF/BRAM usage and Fmax.

---

## Zynq Phase 2: Multi-Bank Scaling ✅ (Ceiling Characterization Complete)

**Goal**: Scale to N=16 and beyond, benchmark against Gowin results. Explore Zynq-exclusive configurations impossible on the GW1NR-9.

**Status**: Ceiling characterization complete for N=1,4,16,64,256,512. N=1024 sweep in progress.

### Z2.1 N-Bank Parallel Core
- [x] Write `atomik_core_zynq_parallel.v` — parameterized 1-512+ banks
- [x] Balanced binary XOR merge tree (no DONT_TOUCH — Vivado LUT6 packing optimal)
- [x] Shared XPM BRAM (256x64 state table), 4-stage SWAP pipeline
- [x] Simulation: 10/10 tests for N=4 (`sim/tb_parallel_banks.v`)
- [x] CI integration: parallel bank testbench in GitHub Actions

### Z2.2 Ceiling Frequency Characterization
- [x] Enhanced `fmax_sweep.py` with 4 strategy presets + ceiling search algorithm
- [x] Characterized 6 bank configurations with all-strategy boundary comparison:

| N | Ceiling | Fmax | LUT (%) | Throughput | Best Strategy |
|--:|--------:|-----:|--------:|-----------:|:-------------|
| 1 | 444.4 MHz | 446.2 | 302 (0.6%) | 446 Mops/s | aggressive |
| 4 | 347.8 MHz | 350.3 | 543 (1.0%) | 1.4 Gops/s | maximum |
| 16 | 266.7 MHz | 274.0 | 941 (1.8%) | 4.4 Gops/s | maximum |
| 64 | 205.1 MHz | 209.9 | 3,498 (6.6%) | 13.4 Gops/s | aggressive |
| 256 | 148.1 MHz | 149.0 | 15,197 (28.6%) | 38.1 Gops/s | baseline |
| 512 | 135.6 MHz | 136.1 | 23,542 (44.3%) | **69.7 Gops/s** | aggressive |

- [x] Key findings:
  - Sub-linear LUT scaling: ~34 marginal LUT/bank at N=256→512
  - Strategy depends on design size: small N → aggressive, medium N → maximum, large N → aggressive/baseline
  - All configs share 1 BRAM (256x64 state table)
  - Peak throughput: 69.7 Gops/s (N=512 @ 135.6 MHz)
- [x] N=1024 attempted: 47,263 LUT (88.8%) fits in LUT slots, but 66,820 FF + LUT packing requires 14,331 slices vs 13,300 available — **cannot place on XC7Z020**
- [x] **N=512 confirmed as maximum configuration** for XC7Z020 (for N=1024+ need XC7Z045 or UltraScale+)

### Z2.3 Userspace Benchmark Suite (Pending Hardware)
- [ ] Port `perf_bench` measurement framework to Linux userspace (UIO-based)
- [ ] Capture 530+ measurements (matching v3 suite)
- [ ] Cross-platform comparison table: Zynq vs. Gowin (same test vectors)
- [ ] Store results in `hardware/zynq/experiments/data/` (JSONL pool format)

**Exit criteria**: Ceiling characterization COMPLETE for N=1 through N=512. Hardware benchmarks pending board arrival (~Mar 22).

---

## Zynq Phase B: RV64I Co-Processor ✅ (RTL + Sim Complete)

**Goal**: Port v3 CPU to Zynq PL with direct-wire ATOMiK custom instructions. ARM dispatches work via shared DDR3 + mailbox.

**Status**: All RTL modules complete, 60/60 simulation tests passing, firmware infrastructure ready.

### ZB.1 RTL Modules (Complete)
- [x] `atomik_v3_regfile_zynq.v` — Distributed RAM register file (32x64-bit, 2 copies)
- [x] `atomik_v3_atomik_zynq.v` — Distributed RAM ATOMiK state table (256x64-bit)
- [x] `atomik_v3_cpu_zynq.v` — RV64I CPU wrapper (instantiates Zynq-ported submodules)
- [x] `atomik_mailbox.v` — Dual-port ARM↔RV64I mailbox (AXI4-Lite + v3 bus)
- [x] `atomik_v3_bus_to_axi.v` — v3 32-bit bus → AXI4 master bridge (DDR3 access)
- [x] `atomik_coprocessor_top.v` — Top-level: CPU + bus mux + BRAM + mailbox + AXI bridge

### ZB.2 Block Design (Complete)
- [x] Updated `vivado/block_design.tcl` — Phase A + B integration
  - AXI Interconnect: M00→ATOMiK N=16, M01→Mailbox
  - S_AXI_HP0 for co-processor DDR3 access
  - IRQ_F2P for mailbox doorbell interrupt
  - Address map: ATOMiK @ 0x43C00000, Mailbox @ 0x43C10000, HP0 DDR3 256MB

### ZB.3 Simulation (Complete — 60/60)
- [x] CDC testbench: 37/37 tests (Phase A accelerator path)
- [x] Parallel bank testbench: 10/10 tests (N=4 configuration)
- [x] Co-processor testbench: 13/13 tests
  - Mailbox register R/W (5 tests)
  - CPU boot + BRAM round-trip (2 tests)
  - ATOMIK.LOAD + READ (1 test)
  - ATOMIK.ACCUM + READ (1 test)
  - XOR self-inverse (1 test)
  - ATOMIK.SWAP + READ after (2 tests)
  - ACCUM after SWAP (1 test)

### ZB.4 Firmware Infrastructure (Complete)
- [x] `firmware/crt_zynq.S` — Startup code (trap→mailbox, data copy, BSS zero)
- [x] `firmware/linker_ddr3.ld` — Linker script (DDR3 code, BRAM data/stack)
- [x] `firmware/atomik_v3.h` — Custom instruction wrappers (same encoding as v3)
- [x] `firmware/mailbox.h` — RV64I-side mailbox register access
- [x] `firmware/demo_atomik.c` — Demo firmware (self-test, workload dispatch, benchmark)
- [x] `firmware/Makefile` — Build system (1,080 bytes output)

### ZB.5 ARM-Side Software (Complete)
- [x] `software/libatomik/atomik_coprocessor.h` — Co-processor dispatch API
- [x] `software/libatomik/atomik_coprocessor.c` — Implementation (firmware load, mailbox, cmd/resp)
- [x] Updated `software/libatomik/Makefile` — `lib-coproc` target

### ZB.6 Hardware Validation (Pending Board)
- [ ] Vivado synthesis at 100 MHz target
- [ ] ARM boot → load demo firmware → verify boot magic (0xB007ED) via mailbox
- [ ] Run self-test command → verify 8/8 ATOMiK tests pass
- [ ] Run benchmark command → measure ATOMiK throughput from RV64I
- [ ] ARM dispatches ACCUM workload → verify result via shared DDR3

**Exit criteria**: All RTL simulation PASS (DONE). Firmware builds (DONE). Hardware validation pending board arrival (~Mar 22).

---

## Zynq Phase 3: Display & Integration

**Goal**: HDMI output, external sensor validation (power + thermal), documentation finalization.

**Dependencies**: Zynq Phase 1 (basic PL + UIO). Partially independent of Phase 2 — HDMI and sensor work can start during Phase 2.

### Z3.1 HDMI Output
- [ ] Evaluate Linux DRM/KMS framebuffer output (primary strategy)
- [ ] If PL-driven HDMI needed: port OSERDESE2 TMDS serialization
- [ ] Display ATOMiK benchmark results on HDMI
- [ ] Stretch goal: port delta display pipeline (`atomik_delta_display.v`)

### Z3.2 External Validation — Power
- [ ] Set up USB-C inline power sensor
- [ ] Measure board idle power (Linux running, no ATOMiK activity)
- [ ] Measure board power during ATOMiK burst operations (N=1, 16, 64)
- [ ] Calculate ATOMiK marginal power (total - idle)
- [ ] Derive nJ/op from measured power and cycle count
- [ ] Compare against Gowin 0.085 nJ/cycle (Tang Nano 9K)

### Z3.3 External Validation — Thermal
- [ ] Attach thermocouple to FPGA package
- [ ] Measure idle temperature
- [ ] Measure temperature during sustained ATOMiK workload
- [ ] Measure temperature delta (idle -> full load)
- [ ] Log time-series thermal data during benchmark runs

### Z3.4 Automated Measurement Scripts
- [ ] Python script to run benchmarks + capture power sensor data
- [ ] Synchronized power/performance logging
- [ ] Generate comparison charts (Gowin vs. Zynq)

### Z3.5 Documentation Finalization
- [ ] Fill in all POST sections in reference docs with measured data
- [ ] Write `GPIO_REFERENCE.md` (after board validation)
- [ ] Write `HDMI_DISPLAY_GUIDE.md` (after HDMI testing)
- [ ] Update `RESOURCE_BUDGET_GUIDE.md`: replace all ~estimates with actuals
- [ ] Update `KNOWN_ISSUES.md` with any Zynq-specific issues
- [ ] Update `ROADMAP.md` phase status to complete
- [ ] Tag release

**Exit criteria**: HDMI output working (DRM/KMS or PL-driven). Power and thermal measurements captured for N=1, 16, 64. Cross-platform comparison charts generated (Gowin vs. Zynq). All reference docs updated with measured values. Release tagged.

---

## Summary: Phase Dependencies

```
Zynq Phase 0: Docs & Tooling ─────────────┐
    │                                       │
    v                                       │
Zynq Phase 1: ATOMiK PL Bringup (N=1) ────┤
    │                                       │
    v                                       │
Zynq Phase 2: Multi-Bank Scaling ✅         │
    │                                       │
    v                                       │
Zynq Phase B: RV64I Co-Processor ✅ (sim)   │
    │                                       │
    v                                       │
Zynq Phase 3: Display & Integration ───────┘
```

Phase 0 must complete before Phase 1 can begin (Vivado + Linux environment required).
Phase 1 must complete before Phase 2 (AXI wrapper + UIO driver required).
Phase B RTL and sim are complete — hardware validation requires board (~Mar 22).
Phase 2 and Phase 3 are partially independent — HDMI and sensor work can start during Phase 2.
