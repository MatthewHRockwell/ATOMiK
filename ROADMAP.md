# ATOMiK — Roadmap & Execution Plan

**Document Version:** 2.3
**Date:** March 6, 2026
**Author:** Matt Rockwell + Claude (Planning Partner)
**Status:** ACTIVE — v2 Production SoC deployed, v3.1.0 production deployed (HD 1280x720 HDMI, benchmarked + parallel banks validated)

---

## Table of Contents

1. [Vision Summary](#1-vision-summary)
2. [PicoRV32 + ATOMiK Integration](#2-picorv32--atomik-integration) ✅
3. [Runtime & Delta-State Memory](#3-runtime--delta-state-memory) ✅
4. [Core Generator & Synthesis Pipeline](#4-core-generator--synthesis-pipeline) ✅
5. [OS Shell & User Interface](#5-os-shell--user-interface)
6. [Library Replacement Pipeline](#6-library-replacement-pipeline)
7. [Multi-Node Scaling Demo](#7-multi-node-scaling-demo) ✅
8. [IDE & Comparative Visualization](#8-ide--comparative-visualization)
9. [Executive Automation Pipelines](#9-executive-automation-pipelines)
10. [Post-Demo Expansion](#10-post-demo-expansion)
11. [Repository Structure](#11-repository-structure)
12. [Business Strategy Notes](#12-business-strategy-notes)
13. [Reference Links & Resources](#13-reference-links--resources)

---

## 1. Vision Summary

ATOMiK v2 represents a strategic pivot from SDK-focused tooling to a full operating environment built on delta-state computation. The core thesis: a $13.50 FPGA board running ATOMiK should boot, execute programs, and interact with users (keyboard, mouse, monitor) in a way that is competitive with hardware costing orders of magnitude more.

### Key Strategic Decisions

- **Linux-native development environment** — Replace Windows with Ubuntu on the development laptop for native Gowin toolchain, bash CLI, and FPGA programming automation
- **PicoRV32 as the RISC-V front-end** — Gowin-native soft-core (RV32IMC) optimized for GW1NR-9, providing familiar ISA with ATOMiK delta-state execution backend
- **Familiar but unique UI** — Not an Ubuntu clone; an intuitive, potentially novel interface that leverages ATOMiK's delta-streaming capabilities
- **Thin C runtime (not a new language)** — Standard C syntax with library replacements that swap Von Neumann state management for delta-state operations, invisible to the programmer
- **No equity dilution** — Revenue via IP licensing and OS platform licensing; retain full ownership
- **Automated executive pipelines** — Agentic automation for business development, outreach, social media, and demo generation

### The Demo Story (3 Layers)

1. **Layer 1 — Familiar**: It boots. It runs C programs. Keyboard works. Display works. Nothing weird.
2. **Layer 2 — Fast**: Watch checkpoint/rollback. Watch deterministic replay. Zero overhead. Try that on x86.
3. **Layer 3 — Secure**: Try to exploit it. Run Metasploit. Run AFL++. The attack surface doesn't exist.

### The Encore

Three Tang Nano 9K boards scaling linearly, proving the architecture extends beyond a single node.

### Hardware

| Item | Details |
|------|---------|
| **Dev Laptop** | Lenovo IdeaPad 3 14ALC6 — Ryzen 7 5700U, 8 GB RAM, 512 GB NVMe, AMD Radeon integrated |
| **FPGA Boards** | 1x Tang Nano 9K in hand, 2 more arriving in 5-7 days (all same revision) |
| **Display** | HDMI monitor available and preferred |
| **Input** | USB HID via MAX3421E SPI breakout → GPIO-wired female USB-A connectors |
| **Enclosure** | Custom build planned: 3D printer + machining + smelting |

---

## 2. PicoRV32 + ATOMiK Integration

**Goal:** Get PicoRV32 running on the Tang Nano 9K alongside the ATOMiK delta accumulator core. Prove they can coexist on the same FPGA with acceptable resource utilization. Achieve "hello world" over UART.

**Estimated Duration:** 2-3 weeks

### 2.1 Bring Up PicoRV32 (Standalone)

#### 2.1.1 Clone the Reference Implementation

```bash
cd ~/Projects
git clone https://github.com/sipeed/TangNano-9K-example.git
cd TangNano-9K-example/picotiny
```

#### 2.1.2 Tasks

- [x] **Task 1.1.1:** Open the picotiny project in Gowin EDA
  - File: `TangNano-9K-example/picotiny/project/picotiny.gprj`
  - Enable "Use MSPI as regular IO" in Project → Configuration → Place&Route → Dual-Purpose Pin
  - Run Clean & Rerun All
  - **Requirement:** Synthesis and P&R complete without errors
  - **Reminder:** The Gowin Programmer may need to use "embFlash Erase, Program" (NOT "Verify") for Tang Nano 9K
  - **Result:** Synthesized via `gw_sh` (TCL scripting) — zero errors, P&R completed in 7s

- [x] **Task 1.1.2:** Flash the PicoRV32 bitstream to the FPGA
  ```bash
  openFPGALoader -b tangnano9k -f picotiny.fs
  ```
  - **Result:** Flashed to persistent storage, CRC check passed

- [x] **Task 1.1.3:** Program the firmware via UART
  ```bash
  cd TangNano-9K-example/picotiny
  python sw/pico-programmer.py example-fw-flash.v /dev/ttyUSB1
  ```
  - Press S1 button when prompted "Waiting for reset"
  - Find correct serial port with `ls /dev/ttyUSB*`
  - **Result:** 11,761 bytes, 3 sectors, 46 pages — flashing completed

- [x] **Task 1.1.4:** Verify UART output
  ```bash
  picocom /dev/ttyUSB1 -b 115200
  ```
  - **Result:** PicoSoC boot menu confirmed — LED toggle, SPI flash, benchmark commands functional

- [x] **Task 1.1.5:** Document resource utilization
  - Record: LUT usage, FF usage, BRAM usage, Fmax
  - Compare against ATOMiK standalone utilization (7% LUT single-core)
  - Document in `docs/PHASE1_PICORV32_BRINGUP.md`
  - **Result:** 4,357 LUT (51%), 1,930 FF (29%), 12 BSRAM (47%), Fmax 33.7 MHz. Combined with ATOMiK 4-bank leaves 41% LUT headroom.

#### 2.1.3 PicoRV32 Resource Budget (Reference)

| Configuration | Approx LUTs | Notes |
|---|---|---|
| PicoRV32 minimal (no peripherals) | ~1,500-2,000 | RV32I only |
| PicoRV32 + UART + SPI | ~2,500-3,500 | RV32IMC |
| PicoRV32 full (all peripherals) | ~4,000-5,000 | RV32IMC + everything |

Tang Nano 9K total: 8,640 LUTs
ATOMiK single-core: ~477 LUTs (7%)
ATOMiK 4-bank: ~745 LUTs
**Available for PicoRV32 (with 4-bank ATOMiK):** ~7,895 LUTs — should be plenty

### 2.2 Integrate ATOMiK Core with PicoRV32

- [x] **Task 1.2.1:** Design the memory map
  - ATOMiK at `0xC000_0000` (S3 Wishbone slot of picotiny's 1:4 mux)
  - 7 registers: LOAD, ACCUM, STATE, STATUS, CONFIG, INIT, DELTA
  - Documented in `hardware/picorv32/memory_map.md`

- [x] **Task 1.2.2:** Create ATOMiK bus interface wrapper
  - `hardware/picorv32/atomik_bus_wrapper.v` — wraps `atomik_core_v2` (DATA_WIDTH=32)
  - PicoRV32 valid/ready protocol, 1-cycle ready latency
  - Soft reset via CONFIG register bit 0

- [x] **Task 1.2.3:** Integrate into picotiny SoC
  - Replaced `assign wbp_ready = 1'b1;` stub with `atomik_bus_wrapper` instance
  - Synthesis: 4,578 logic (53%), 2,062 FF (31%), 12 BSRAM (47%)
  - ATOMiK overhead: +221 LUT, +132 FF (2.6% of total)
  - Timing: Fmax 31.032 MHz vs 25.2 MHz constraint — **PASS** (1.23x margin)
  - Bitstream flashed, CRC check passed

- [x] **Task 1.2.4:** Write bare-metal C test program
  - 11-test suite: reset, load, accumulate, XOR verify, cancellation, multi-delta, perf
  - Added `[X] ATOMiK delta test` menu option to PicoSoC firmware
  - Firmware built: 16,492 bytes (RV32I, -O3)

- [x] **Task 1.2.5:** Performance baseline — **11/11 tests PASS on hardware**
  - All ATOMiK operations verified on real Tang Nano 9K hardware
  - XOR cancellation (idempotency) confirmed: `a ⊕ d ⊕ d = a`
  - Multi-delta composition confirmed: `0 ⊕ 0x11.. ⊕ 0x22.. ⊕ 0x44.. = 0x77..`
  - Performance: 224 cycles (8.9 µs @ 25.2 MHz) for load+accumulate+read round-trip
  - Full results in `docs/PHASE1_INTEGRATION_RESULTS.md`

### 2.3 Phase 1 Validation Gate

- [x] PicoRV32 boots and runs bare-metal C code on Tang Nano 9K
- [x] ATOMiK delta accumulator is accessible as a memory-mapped peripheral (0xC0000000)
- [x] UART communication works bidirectionally (115200 baud, picocom verified)
- [x] Combined resource utilization is documented and leaves headroom (53% LUT, 56% remaining)
- [x] Timing closure is met (Fmax 31.0 MHz > 25.2 MHz constraint)
- [x] At least one C test program demonstrates delta accumulation via PicoRV32 (11/11 PASS)

---

## 3. Runtime & Delta-State Memory

**Goal:** Build the bridge between PicoRV32's standard C execution and ATOMiK's delta-state operations. Implement a minimal runtime that replaces key memory operations with delta-state equivalents.

**Estimated Duration:** 3-4 weeks

### 3.1 ATOMiK Runtime (Firmware-Level)

**Approach:** Rather than a separate libc directory, Phase 2 was implemented as firmware modules in `hardware/picorv32/firmware/`. This keeps all bare-metal code together and avoids premature abstraction.

- [x] **Task 2.1.1:** Create hardware abstraction layer (`atomik.h`)
  - Bank-aware inline API: `atomik_init()`, `atomik_load()`, `atomik_accumulate()`, `atomik_state()`, `atomik_get_delta()`, `atomik_unchanged()`, `atomik_undo()`, `atomik_fingerprint()`, `atomik_verify()`
  - Bank stride parameter for future multi-bank support

- [x] **Task 2.1.2:** Implement startup code and build system
  - `crt_flash.S` (from picotiny), `linker_flash.ld` with 2KB heap, `Makefile`
  - Firmware: 16,308 bytes, 3,680 bytes SRAM (44.9%)

- [x] **Task 2.1.3:** Implement `mini_printf` (`printf.h` / `printf.c`)
  - Supports: `%d`, `%u`, `%x` (with width/0-pad), `%s`, `%c`, `%%`
  - Powers-of-10 table with repeated subtraction (no div on RV32I)

- [x] **Task 2.1.4:** Implement tracked memory operations (`atomik_mem.h` / `atomik_mem.c`)
  - `atomik_memcpy_tracked()` — copy + XOR fingerprint (12.2% overhead)
  - `atomik_memset_verified()` — fill + XOR fingerprint (15.7% overhead)
  - `atomik_region_changed()` — 5.1x faster than sw_memcmp
  - `atomik_region_delta()` — cumulative XOR between two buffers

- [x] **Task 2.1.5:** Implement checkpoint/rollback demo
  - 4-field SensorState struct, 5 mutations with per-step fingerprinting
  - Rollback verification: 416 cycles to confirm state matches checkpoint
  - **Signature ATOMiK capability demonstration**

- [x] **Task 2.1.6:** Implement bump allocator with integrity tracking (`atomik_alloc.h` / `atomik_alloc.c`)
  - Allocation headers XOR-fingerprinted: `tag = address ^ size`
  - `atomik_heap_verify()` — 335 cycles to verify integrity
  - Honest bump allocator (free is no-op)

### 3.2 Phase 2 Validation Gate

- [x] ATOMiK runtime compiles for RV32I target (16,308 bytes, zero warnings)
- [x] At least 3 standard library functions have delta-state implementations (memcpy, memset, change-detect)
- [x] Performance comparison data: cycles for ATOMiK vs standard for each function
- [x] A C program using ATOMiK runtime can: initialize, allocate memory, compute, print results over UART
- [x] All results documented in `docs/PHASE2_RUNTIME_RESULTS.md`
- [x] 10/10 integration tests PASS on hardware, 11/11 Phase 1 tests still PASS

### 3.3 Automated Performance Benchmarking

- [x] **Task 2.3.1:** Create firmware performance benchmark module (`perf_bench.c`)
  - 550 machine-parseable `##PERF` measurements per run
  - 4 test categories: ATOMiK core ops, memory operations, burst/scaling, CPU baselines
  - 'R' command added to firmware menu

- [x] **Task 2.3.2:** Create Python capture and runner pipeline
  - `hardware/scripts/perf_capture.py` — UART capture and `##PERF` line parser
  - `hardware/scripts/perf_runner.py` — orchestrator with metadata, statistics, JSONL pooling
  - Append-only data pool at `hardware/experiments/data/hardware_perf/perf_pool.jsonl`
  - Automatic regression detection between consecutive runs

- [x] **Task 2.3.3:** Validate on hardware
  - 550 measurements captured across two consecutive runs
  - 11/11 ATOMiK HW tests, 10/10 integration tests passing
  - Key results: ATOMiK load=64cy, accum=70cy, read=99cy, roundtrip=285cy
  - Change detection 76-80% faster than sw_memcmp

---

## 4. Core Generator & Synthesis Pipeline

**Goal:** Build the automated pipeline: design parameters → Verilog generation → synthesis → place & route → program FPGA, all from the command line.

**Estimated Duration:** 2-3 weeks (overlapping with Phase 2)

### 4.1 Programming Execution Pipeline

- [ ] **Task 3.1.1:** Create `tools/atomik-build` script
  ```bash
  # Usage: atomik-build [--banks N] [--freq F] [--target sram|flash]
  ```
  Single command from design change to running hardware.

- [ ] **Task 3.1.2:** Support both toolchains
  - Gowin EDA (official) — via `gw_sh` TCL scripting
  - OSS CAD Suite (Yosys + nextpnr + Apicula) — fully scriptable
  - Select via `--toolchain gowin|oss` flag

- [ ] **Task 3.1.3:** Build log parsing and reporting
  - Extract: LUT, FF, BRAM, Fmax, timing violations → JSON report

### 4.2 Core Generator Pipeline

- [ ] **Task 3.2.1:** Parameterized Verilog generator
  - Input: `N_BANKS`, `DATA_WIDTH`, `INCLUDE_UART`, `INCLUDE_SPI`, `INCLUDE_PICORV32`, `PLL_CONFIG`
  - Output: Complete Verilog design ready for synthesis

- [ ] **Task 3.2.2:** Configuration profiles
  ```yaml
  # configs/minimal.yaml
  n_banks: 1
  data_width: 64
  include_picorv32: false
  include_uart: true

  # configs/full_os.yaml
  n_banks: 4
  data_width: 64
  include_picorv32: true
  include_uart: true
  include_spi: true
  pll_frequency: 81000000
  ```

- [ ] **Task 3.2.3:** Sweep automation
  - Generate and synthesize multiple configurations automatically
  - Produce comparison table

### 4.3 Phase 3 Validation Gate

- [ ] `atomik-build` works end-to-end: parameters → FPGA programmed
- [ ] Both Gowin and OSS toolchains supported
- [ ] Configuration profiles produce correct Verilog
- [ ] Sweep automation produces comparative resource/performance tables
- [ ] Build pipeline documented in `docs/BUILD_PIPELINE.md`

---

## 5. OS Shell & User Interface

**Goal:** Build a minimal interactive environment — keyboard input, display output, command interpretation. The user can type commands, see output, and interact with the system.

**Estimated Duration:** 3-4 weeks

### 5.1 I/O Infrastructure

- [ ] **Task 4.1.1:** HDMI display output
  - Tang Nano 9K has an HDMI connector
  - Reference: `TangNano-9K-example` has HDMI display examples
  - Implement text framebuffer (80x25 or similar character mode)

- [ ] **Task 4.1.2:** Keyboard/mouse input via USB HID
  - **RECOMMENDED: MAX3421E USB Host IC via SPI**
    - ~$5 breakout board handles entire USB protocol, exposes SPI interface
    - PicoRV32 already has SPI → **zero additional FPGA LUTs** for USB
    - Wiring: female USB-A connector → MAX3421E breakout → SPI pins on Tang Nano 9K GPIO
    - Fits custom enclosure: MAX3421E mounts inside, USB-A on case, SPI wires to FPGA
    - **Part to order:** Search "MAX3421E USB host module" on Amazon/AliExpress ($3-8)
  - **Phased approach:**
    - Phase 4 initial: UART serial terminal for input (immediate, no extra hardware)
    - Phase 4 mid: Integrate MAX3421E for USB HID
  - **Resource budget with MAX3421E approach:**
    - PicoRV32 full: ~4,000-5,000 LUTs
    - ATOMiK 4-bank: ~745 LUTs
    - HDMI text output: ~500-1,000 LUTs
    - MAX3421E via SPI: ~0 additional FPGA LUTs
    - Total: ~5,245-6,745 vs 8,640 available — comfortable headroom

- [ ] **Task 4.1.3:** Basic framebuffer driver
  - Character cell rendering, cursor management, scrolling, basic color

### 5.2 Command Shell

- [ ] **Task 4.2.1:** Minimal shell implementation
  - Command line input (readline-like, with backspace), command parsing
  - Built-in commands: `help`, `status`, `atomik`, `echo`, `clear`

- [ ] **Task 4.2.2:** ATOMiK-specific commands
  - `atomik status` — bank states, accumulator values, throughput
  - `atomik bench` — performance benchmark
  - `atomik demo` — delta-state demonstration (accumulate, undo, verify)
  - `atomik checkpoint` — snapshot current state
  - `atomik rollback` — restore to checkpoint (instant undo)

- [ ] **Task 4.2.3:** System information
  - `sysinfo` — clock frequency, LUT usage, bank count, uptime, memory usage

### 5.3 UI Exploration (Post-Basic Shell)

- [ ] **Task 4.3.1:** Investigate delta-streaming UI concepts
  - Real-time state diffs, animated transitions, "time travel" through computation history
  - Document in `docs/UI_CONCEPTS.md`

### 5.4 Phase 4 Validation Gate

- [ ] Text output displays on HDMI monitor (or UART terminal at minimum)
- [ ] Interactive command shell accepts input and produces output
- [ ] ATOMiK hardware status is visible to the user
- [ ] At least one interactive demo shows delta-state behavior
- [ ] **THIS IS THE "IT BOOTS" DEMO MOMENT**

---

## 6. Library Replacement Pipeline

**Goal:** Systematically replace standard C library functions with delta-state implementations using automated testing and comparison.

**Estimated Duration:** 4-6 weeks (ongoing, parallelizable)

### 6.1 Automated Replacement Pipeline

- [ ] **Task 5.1.1:** Pipeline architecture
  ```
  For each libc function:
    1. Analyze: What state management pattern does it use?
    2. Generate: Delta-state equivalent implementation
    3. Compile: Both standard and ATOMiK versions for RV32IMC
    4. Test: Run identical test vectors against both
    5. Measure: Cycles, memory usage, correctness
    6. Log: Results to database/JSON
    7. Flag: Functions needing human review
  ```

- [ ] **Task 5.1.2:** Function categorization
  - **Category A — Direct delta-state mapping** (big wins): `memcpy`, `memmove`, `memset`, `memcmp`, `strcpy`, `strncpy`, `strcmp`, `strncmp`
  - **Category B — Indirect benefit** (moderate wins): `malloc`, `free`, `realloc`, `calloc`, `qsort`, `bsearch`
  - **Category C — Minimal impact** (implement standard): `printf` family, math functions
  - **Category D — Architecture-specific advantage**: `setjmp`/`longjmp` (checkpoint/restore → delta chains), `fork` equivalent (delta chain branching)

- [ ] **Task 5.1.3:** Parallel testing infrastructure
  - Multiple agents test different function families simultaneously
  - Results feed into relationship graph

### 6.2 Performance Relationship Graph

- [ ] **Task 5.2.1:** Data collection schema
  ```json
  {
    "function": "memcpy",
    "category": "A",
    "standard_cycles": 156,
    "atomik_cycles": 12,
    "speedup": "13x",
    "correctness": "PASS",
    "dependencies": ["memset"],
    "notes": "64-bit aligned copy, single XOR"
  }
  ```

- [ ] **Task 5.2.2:** Visualization
  - Dependency graph, performance heatmap by category, cumulative speedup curve

### 6.3 Phase 5 Validation Gate

- [ ] At least 20 libc functions have ATOMiK implementations
- [ ] Automated test pipeline runs without manual intervention
- [ ] Performance comparison data for all implemented functions
- [ ] Relationship graph generated and documented
- [ ] Aggregate performance improvement quantified

---

## 7. Multi-Node Scaling Demo

**Goal:** Connect three Tang Nano 9K boards and demonstrate linear scaling across hardware nodes.

**Estimated Duration:** 2-3 weeks

**Hardware:** 1 board in hand now, 2 more arriving in 5-7 days (all same revision)

### 7.1 Multi-Board Communication

- [ ] **Task 6.1.1:** Inter-board communication protocol
  - Options: UART, SPI, or GPIO-based custom protocol
  - Design delta-state synchronization protocol
  - **Key property:** Commutativity means sync order doesn't matter

- [ ] **Task 6.1.2:** Physical setup
  - 3x Tang Nano 9K, inter-board wiring, single USB hub for programming, single HDMI output

- [ ] **Task 6.1.3:** Distributed delta accumulation
  - Deltas distributed across boards, state reconstruction merges results
  - Demonstrate: N boards → N× throughput (linear scaling)

### 7.2 Scaling Demo

- [ ] **Task 6.2.1:** Benchmark: 1 board vs 2 boards vs 3 boards
  - Same workload, measure throughput, demonstrate linear scaling

- [ ] **Task 6.2.2:** Visual demo
  - Real-time throughput display on HDMI
  - Add/remove boards live, show throughput change
  - **THIS IS THE "ENCORE" DEMO MOMENT**

### 7.3 Phase 6 Validation Gate

- [ ] 3-board system operates correctly
- [ ] Linear scaling demonstrated with benchmarks
- [ ] Live add/remove node capability
- [ ] Demo is reproducible and polished

---

## 8. IDE & Comparative Visualization

**Goal:** Build an ATOMiK-specific development environment showing parameter changes and their effects on performance, with side-by-side comparison against Von Neumann architectures.

**Estimated Duration:** 3-4 weeks

### 8.1 ATOMiK IDE Features

- [ ] **Task 7.1.1:** Parameter dashboard — sliders for N_BANKS, frequency, data width, PLL config; real-time estimated resource utilization
- [ ] **Task 7.1.2:** One-click build & flash — integrates Phase 3 pipeline with visual progress
- [ ] **Task 7.1.3:** Performance comparison view — ATOMiK vs conventional; throughput, latency, power, cost, resource usage; historical tracking
- [ ] **Task 7.1.4:** Technology choice — Web-based (React/Vue) or native (Python/Qt); decide based on speed to build and demo-friendliness

### 8.2 Phase 7 Validation Gate

- [ ] IDE displays parameter dashboard with real-time estimates
- [ ] Build pipeline integration works end-to-end from IDE
- [ ] Comparative visualization shows ATOMiK vs Von Neumann data
- [ ] Non-technical user can understand the performance difference

---

## 9. Executive Automation Pipelines

**Goal:** Automate business development, outreach, social media, and demo generation — freeing Matt to focus on technology.

**Estimated Duration:** Ongoing, built incrementally starting after Phase 4

**IMPORTANT:** Only begin AFTER the "it boots" demo (Phase 4) produces shareable artifacts.

### 9.1 Social Media Pipeline

- [ ] Milestone detection (Git commits, build results, benchmarks)
- [ ] Content generation (Twitter/X, LinkedIn, blogs) — all posts require human approval
- [ ] Visual asset generation (charts, diagrams, screenshots, videos)

### 9.2 Outreach Pipeline

- [ ] Target identification (licensees, industry contacts, conference organizers)
- [ ] Personalized outreach drafts — all outreach requires human approval

### 9.3 Demo Pipeline

- [ ] Automated demo recording (boot, benchmark, capture output → video/GIF)
- [ ] Live demo server (stretch goal — remote access to running ATOMiK board via web terminal)

### 9.4 Phase 8 Validation Gate

- [ ] At least one pipeline produces usable output
- [ ] Human approval workflow in place for all external communications
- [ ] Pipeline runs in background without blocking development

---

## 10. Post-Demo Expansion

**Goal:** Advanced features and optimizations building on the working demo.

### 10.1 Security Validation

- [ ] Set up Metasploit framework, run standard exploit modules against ATOMiK OS
- [ ] Set up AFL++ fuzzing
- [ ] CVE reproduction testing (Heartbleed, Dirty COW, etc.)
- [ ] Document results as third-party validation evidence

### 10.2 Video Processing (Sparse Vector-Sum)

- [ ] Implement sparse, vector-sum non-overlapping pooling
- [ ] Implement delta-based decoding for video frames
- [ ] Benchmark against conventional video processing
- [ ] Demo: real-time video processing on Tang Nano 9K

### 10.3 Advanced UI Concepts

- [ ] Delta-streaming interactive elements
- [ ] State time-travel visualization
- [ ] Apple-level UX polish

### 10.4 Formal Verification Extensions

- [ ] Extend Lean4 proofs to cover RISC-V instruction semantics
- [ ] Prove delta-state execution preserves RV32IMC architectural behavior
- [ ] Massive differentiator for licensing discussions

---

## 11. Repository Structure

```
ATOMiK/
├── hardware/                    # FPGA/ASIC hardware design
│   ├── rtl/                     # Verilog RTL source (existing)
│   │   ├── atomik_core_v2.v
│   │   ├── atomik_parallel_acc.v
│   │   ├── atomik_bus_wrapper.v # NEW: AHB/Wishbone interface
│   │   └── ...
│   ├── picorv32/                # NEW: PicoRV32 integration
│   │   ├── picorv32.v
│   │   ├── atomik_soc.v         # Combined SoC top-level
│   │   └── memory_map.md
│   ├── sim/                     # Testbenches
│   ├── sweep/                   # Synthesis sweep configs
│   ├── constraints/             # Pin constraints
│   └── scripts/                 # Hardware build/validation scripts
├── runtime/                     # NEW: ATOMiK C runtime
│   ├── atomik-libc/             # Delta-state C library replacement
│   ├── firmware/                # Bare-metal firmware for PicoRV32
│   └── tests/
├── tools/                       # NEW: Build and automation tools
│   ├── atomik-build             # Synthesis pipeline script
│   ├── core-generator/
│   ├── config/                  # Build configuration profiles
│   └── library-replacer/        # Automated libc replacement pipeline
├── os/                          # NEW: OS/UI layer
│   ├── shell/
│   ├── drivers/                 # Display, keyboard, storage
│   ├── ui/
│   └── fs/
├── ide/                         # NEW: ATOMiK IDE
├── automation/                  # NEW: Executive automation pipelines
├── math/proofs/                 # Lean4 formal proofs (existing, 92 theorems)
├── software/                    # Python SDK (existing — may be deprioritized)
├── demos/                       # Demo system
├── business/                    # Investor materials
├── papers/                      # Research publications
├── docs/                        # Documentation
├── specs/                       # Formal specs
├── sdk/                         # SDK schemas/generated (may be deprioritized)
└── archive/                     # Historical phase reports
```

---

## 12. Business Strategy Notes

### Revenue Model: Licensing, Not Equity

- **IP Licensing:** Patent-pending architecture, execution model, and methods. License the RTL cores, runtime, and methodology.
- **OS Platform Licensing:** When ATOMiK OS is demonstrably competitive with conventional hardware at a fraction of the cost.
- **Developer Tools:** ATOMiK IDE, build pipeline, library replacement infrastructure.
- **No VC/investor equity dilution:** A working $15 board competing with $1,500+ hardware is a self-evident value proposition that drives licensing revenue without outside investment.

### IP Protection Reminders

- Patent filing is pending — continue documenting novel methods with dates
- Lean4 formal proofs provide mathematical evidence of architecture properties — defensible IP
- Library replacement performance data is valuable trade secret

### Key Milestones for External Communication

1. **"It boots"** — Phase 4 → First public demo artifact
2. **"It's fast"** — Phase 5 → Performance comparison evidence
3. **"It's secure"** — Phase 9.1 → Security validation evidence
4. **"It scales"** — Phase 6 → Multi-node demo
5. **"It's the platform"** — IDE + automation → Developer and business readiness

---

## 13. Reference Links & Resources

### Tang Nano 9K
- Sipeed Wiki: https://wiki.sipeed.com/hardware/en/tang/Tang-Nano-9K/Nano-9K.html
- Example Repository: https://github.com/sipeed/TangNano-9K-example
- PicoRV32 Tutorial: https://wiki.sipeed.com/hardware/en/tang/Tang-Nano-9K/examples/picorv.html

### PicoRV32
- Original Repository: https://github.com/YosysHQ/picorv32

### Gowin EDA
- Download: https://www.gowinsemi.com/en/support/home/
- License Application: https://www.gowinsemi.com/en/support/license/
- Linux Notes: https://gist.github.com/retrofun/57b6f0bbca01f0650a8b7137f69dd674
- Easy Linux Setup: https://github.com/abhra0897/gowin-easy-linux

### openFPGALoader
- Repository: https://github.com/trabucayre/openFPGALoader
- Sipeed Linux Guide: https://wiki.sipeed.com/hardware/en/tang/Tang-Nano-Doc/flash-in-linux.html

### OSS CAD Suite
- Repository: https://github.com/YosysHQ/oss-cad-suite-build
- Lushay Labs Guide: https://learn.lushaylabs.com/getting-setup-with-the-tang-nano-9k/

### RISC-V Toolchain
- GNU Toolchain: https://github.com/riscv-collab/riscv-gnu-toolchain

### Claude Code
- Setup: https://code.claude.com/docs/en/setup

### Security Testing Tools (Phase 9)
- Metasploit: https://github.com/rapid7/metasploit-framework
- AFL++: https://github.com/AFLplusplus/AFLplusplus
- syzkaller: https://github.com/google/syzkaller
- Lynis: https://github.com/CISOfy/lynis

---

## 14. ATOMiK v3 Architecture Status

The v3 architecture is a ground-up redesign: custom RV64I CPU with ATOMiK custom instructions wired directly into the execute stage (no bus, no CDC). Full task list at [`specs/atomik_v3_tasks.md`](specs/atomik_v3_tasks.md).

### Completed
- **Phase 0**: Tooling & infrastructure (Verilator, iverilog, compliance runner, Gowin synthesis)
- **Phase 1**: Custom RV64I CPU core — 53/54 rv64ui-p-* compliance (only `ma_data` misaligned access fails, expected)
- **Phase 2**: ATOMiK v3 datapath — 4 custom instructions (LOAD, ACCUM, READ, SWAP), 1.016 CLS/bit mapping, 21+11 tests passing
- **Phase 3**: SoC integration — 5,594 LUT, 16 BSRAM, full SoC on Tang Nano 9K
  - SRAM + persistent flash deployment working, UART functional
  - ISP flasher ported to RV64I, flash programming validated
  - Boot chain validated: BROM → ISP timeout → JUMP! → XIP → full firmware (golden tag: `v3-boot-chain-golden`)
  - All v2 tests ported: ATOMiK 9/9, Phase 2 10/10, Checkpoint, Memory, Heap — all PASS
  - V3-020 resolved: downclocked to 21.6 MHz, zero TNS
- **Phase 4**: Delta-driven display pipeline + HD HDMI
  - `pixel_out = pixel_ref ⊕ LUT[index]` — zero-cost unchanged pixels
  - BSRAM-based: delta color LUT (256×24-bit) + scanline delta buffer (1280×9-bit)
  - 6/6 display tests PASS, **HDMI 1280×720 @ 60 Hz** on Dell monitor
  - Dual-PLL architecture: CPU @ 21.6 MHz (PLL1), HDMI pixel @ 74.25 MHz (PLL2: 371.25 MHz ÷5)
  - Decode pipelining: DECODE_REG state added, CPU Fmax 23.2 MHz (+7.4% margin)
  - Pixel pipeline optimized: 3-stage TMDS, 3-stage svo_tcard, parallel prefix gray2bin, enc→tmds register buffer
  - +321 LUT, +1 BSRAM from Phase 3 baseline (v3.0.0); +693 LUT total at v3.1.0
- **Phase 6**: Parallel banks (synthesis-validated)
  - `atomik_v3_parallel.v`: N=1,2,4,8,16 banks with XOR merge tree
  - Shared BSRAM state table (constant 2 blocks regardless of N)
  - 20/20 simulation tests PASS, 20-config synthesis sweep
  - Best timing-met: N=16 @ 67.5 MHz = **1,080 Mops/s** (+2.3% vs v2)
  - Per-bit LUT efficiency: 1.36 LUT/bit/bank (v3) vs 2.71 (v2) — 50% better
- **Phase 7**: Benchmarking & production hardening
  - 530 hardware measurements, all perfectly deterministic (zero variance)
  - ATOMiK roundtrip: 285 cy (v2) → 160 cy (v3), **-44%**
  - ATOMiK memcpy 256B: +12% overhead (v2) → **-84.5% faster** (v3)
  - Change detection 256B: -80% (v2) → **-89.4%** (v3)
  - Full comparison: [`hardware/v3/experiments/V2_VS_V3_COMPARISON.md`](hardware/v3/experiments/V2_VS_V3_COMPARISON.md)

### Releases
- **v3.0.0** — Production SoC: RV64I + ATOMiK + 640×480 HDMI + all tests passing
- **v3.1.0** — HD HDMI upgrade: 1280×720@60Hz + decode pipelining + pixel pipeline optimization
  - LUT: 6,287 (73%), CLS: 3,783 (88%), BSRAM: 20 (77%)
  - Pixel Fmax: 74.384 MHz (+0.18% margin), CPU Fmax: 23.192 MHz (+7.4% margin)
  - All tests passing: 53/54 compliance, 9/9 ATOMiK, 10/10 integration, 6/6 display

### Pending
- **Phase 5**: I/O & multi-node streaming (IDES16/OSER16 LVDS)

### Assessed & Closed (No Further Action)
- **CPU clock upgrade** (21.6→27 MHz): Blocked — CPU Fmax only 23.2 MHz, need ≥30 MHz for 27 MHz target
- **QSPI enable**: Low priority — 2-5% speedup, instruction fetch rarely the bottleneck on single-issue in-order cores

---

*This document is a living roadmap. Update as decisions are made and phases are completed.*
*Last updated: March 6, 2026*
