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
- [ ] Install Vivado WebPACK (free for XC7Z020)
- [ ] Create AX7020 board files / import ALINX BSP
- [ ] Build trivial PL blink design, program via JTAG
- [ ] Verify Vivado TCL flow (non-project mode)

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

**Exit criteria**: All 11 reference documents written. Vivado synthesizes a blink design for AX7020. PetaLinux boots to a Linux shell with UART and Ethernet. Cross-compiled C binary runs on target via SSH.

---

## Zynq Phase 1: ATOMiK PL Bringup (N=1)

**Goal**: Get a single ATOMiK bank working as an AXI4-Lite peripheral, accessible from Linux userspace via UIO.

**Dependencies**: Zynq Phase 0 (Vivado + Linux environment required)

### Z1.1 AXI4-Lite Wrapper RTL
- [ ] Write `atomik_axi4lite_wrapper.v` (adapts `atomik_v3_atomik.v` to AXI4-Lite)
- [ ] Implement register map (LOAD, ACCUM, STATE, STATUS, SWAP, CONFIG)
- [ ] Handle 32-bit AXI <-> 64-bit datapath (LO/HI register pair, HI triggers op)
- [ ] Add `DONT_TOUCH` attributes (replacing Gowin `syn_keep`) on XOR paths
- [ ] Simulation testbench: AXI4-Lite protocol verification (iverilog or Vivado sim)

### Z1.2 Vivado Block Design
- [ ] Create PS block design (Zynq PS, DDR3 config, UART, ETH)
- [ ] Add ATOMiK as custom AXI4-Lite peripheral in PL
- [ ] Connect via M_AXI_GP0 (general-purpose master port)
- [ ] Assign address map (0x43C00000, 4KB range)
- [ ] Generate output products, create HDL wrapper

### Z1.3 MMCM Clock Configuration
- [ ] Configure MMCM for ATOMiK clock domain (target: 100 MHz from 50 MHz FCLK)
- [ ] Implement CDC bridge between AXI clock and ATOMiK clock
- [ ] Write XDC timing constraints for both domains
- [ ] Run timing analysis, verify zero TNS

### Z1.4 Synthesis and Resource Check
- [ ] Run Vivado synthesis + implementation
- [ ] Record: LUT6, FF, BRAM, DSP, Fmax from utilization/timing reports
- [ ] Compare against estimates in `RESOURCE_BUDGET_GUIDE.md`
- [ ] Update docs with measured values (replace ~ estimates)

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

## Zynq Phase 2: Multi-Bank Scaling

**Goal**: Scale to N=16 and N=64 banks, benchmark against Gowin results. Explore Zynq-exclusive configurations impossible on the GW1NR-9.

**Dependencies**: Zynq Phase 1 (AXI wrapper + UIO driver required)

### Z2.1 N=16 Instantiation
- [ ] Update block design with `atomik_v3_parallel.v` (N_BANKS=16)
- [ ] AXI wrapper: add bank_select register, parallel input support
- [ ] Synthesis, implementation, timing closure at target frequency
- [ ] Record resource usage and Fmax
- [ ] Benchmark: throughput vs. N=1, compare against Gowin N=16 (864 Mops/s)

### Z2.2 N=64 Exploration
- [ ] Instantiate N_BANKS=64 (Zynq-exclusive — impossible on Gowin)
- [ ] Synthesis, implementation, timing closure
- [ ] Record resource usage, Fmax, throughput
- [ ] Document scaling curve (N vs. LUT, N vs. Fmax, N vs. throughput)

### Z2.3 N=256 Stress Test (Optional)
- [ ] Instantiate N_BANKS=256 (pushing Zynq limits)
- [ ] Record resource usage (expected ~18,000 LUT6, ~34% utilization)
- [ ] Determine maximum feasible Fmax
- [ ] Assess diminishing returns vs. AXI bandwidth bottleneck

### Z2.4 Userspace Benchmark Suite
- [ ] Port `perf_bench` measurement framework to Linux userspace (UIO-based)
- [ ] Capture 530+ measurements (matching v3 suite):
  - Single-op latency: load, accum, read, swap
  - Burst accumulate: 1, 4, 16, 64, 256 operations
  - Change detection: ATOMiK vs. software memcmp (256B, 1KB, 4KB)
  - Memory overhead: ATOMiK tracking cost vs. plain memcpy
- [ ] Cross-platform comparison table: Zynq vs. Gowin (same test vectors)
- [ ] Store results in `hardware/zynq/experiments/data/` (JSONL pool format)

**Exit criteria**: N=16 timing met, throughput measured and compared against Gowin N=16 (1,080 Mops/s). N=64 synthesized and characterized (Zynq-exclusive data point). Benchmark suite produces 530+ measurements with cross-platform comparison. Scaling curve documented.

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
Zynq Phase 2: Multi-Bank Scaling            │
    │                                       │
    v                                       │
Zynq Phase 3: Display & Integration ───────┘
```

Phase 0 must complete before Phase 1 can begin (Vivado + Linux environment required).
Phase 1 must complete before Phase 2 (AXI wrapper + UIO driver required).
Phase 2 and Phase 3 are partially independent — HDMI and sensor work can start during Phase 2.
