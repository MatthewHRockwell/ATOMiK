# ATOMiK v2 — Complete Roadmap & Execution Plan

**Document Version:** 1.1
**Date:** February 12, 2026
**Author:** Matt Rockwell + Claude (Planning Partner)
**Status:** ACTIVE — Phase 0 In Progress (see `PHASE0_LINUX_MIGRATION.md`)

---

## Table of Contents

1. [Vision Summary](#1-vision-summary)
2. [Phase 1 — PicoRV32 + ATOMiK Integration](#2-phase-1--picorv32--atomik-integration)
3. [Phase 2 — ATOMiK Runtime & Delta-State Memory](#3-phase-2--atomik-runtime--delta-state-memory)
4. [Phase 3 — Core Generator & Synthesis Pipeline](#4-phase-3--core-generator--synthesis-pipeline)
5. [Phase 4 — OS Shell & User Interface](#5-phase-4--os-shell--user-interface)
6. [Phase 5 — Library Replacement Pipeline (Automated)](#6-phase-5--library-replacement-pipeline-automated)
7. [Phase 6 — Multi-Node Scaling Demo](#7-phase-6--multi-node-scaling-demo)
8. [Phase 7 — ATOMiK IDE & Comparative Visualization](#8-phase-7--atomik-ide--comparative-visualization)
9. [Phase 8 — Executive Automation Pipelines](#9-phase-8--executive-automation-pipelines)
10. [Phase 9 — Post-Demo Expansion](#10-phase-9--post-demo-expansion)
11. [Repository Structure (v2)](#11-repository-structure-v2)
12. [Business Strategy Notes](#12-business-strategy-notes)
13. [Reference Links & Resources](#13-reference-links--resources)

---

## 1. Vision Summary

ATOMiK v2 represents a strategic pivot from SDK-focused tooling to a full operating environment built on delta-state computation. The core thesis: a $15 FPGA board running ATOMiK should boot, execute programs, and interact with users (keyboard, mouse, monitor) in a way that is competitive with hardware costing orders of magnitude more.

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

## 2. Phase 1 — PicoRV32 + ATOMiK Integration

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

- [ ] **Task 1.1.1:** Open the picotiny project in Gowin EDA
  - File: `TangNano-9K-example/picotiny/project/picotiny.gprj`
  - Enable "Use MSPI as regular IO" in Project → Configuration → Place&Route → Dual-Purpose Pin
  - Run Clean & Rerun All
  - **Requirement:** Synthesis and P&R complete without errors
  - **Reminder:** The Gowin Programmer may need to use "embFlash Erase, Program" (NOT "Verify") for Tang Nano 9K

- [ ] **Task 1.1.2:** Flash the PicoRV32 bitstream to the FPGA
  ```bash
  openFPGALoader -b tangnano9k -f picotiny.fs
  ```

- [ ] **Task 1.1.3:** Program the firmware via UART
  ```bash
  cd TangNano-9K-example/picotiny
  python sw/pico-programmer.py example-fw-flash.v /dev/ttyUSBx
  ```
  - Press S1 button when prompted "Waiting for reset"
  - Find correct serial port with `ls /dev/ttyUSB*`

- [ ] **Task 1.1.4:** Verify UART output
  ```bash
  picocom /dev/ttyUSB1 -b 115200
  ```

- [ ] **Task 1.1.5:** Document resource utilization
  - Record: LUT usage, FF usage, BRAM usage, Fmax
  - Compare against ATOMiK standalone utilization (7% LUT single-core)
  - Document in `docs/PHASE1_PICORV32_BRINGUP.md`

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

- [ ] **Task 1.2.1:** Design the memory map
  - ATOMiK delta accumulator as memory-mapped peripheral on PicoRV32's bus
  - Address ranges:
    - `0x0000_0000 - 0x0000_FFFF` — Instruction memory (BRAM)
    - `0x0001_0000 - 0x0001_FFFF` — Data memory (BRAM)
    - `0x0002_0000 - 0x0002_00FF` — UART
    - `0x0003_0000 - 0x0003_00FF` — SPI
    - `0x0004_0000 - 0x0004_00FF` — **ATOMiK Delta Accumulator** ← NEW

- [ ] **Task 1.2.2:** Create ATOMiK bus interface wrapper
  - Wrap `atomik_core_v2.v` with AHB-Lite or Wishbone slave interface
  - Registers:
    - `0x00` — LOAD initial state (write)
    - `0x04` — ACCUMULATE delta (write)
    - `0x08` — READ current state (read)
    - `0x0C` — STATUS (read: ready, busy, bank count)
    - `0x10` — CONFIG (write: select bank, reset)

- [ ] **Task 1.2.3:** Integrate into picotiny SoC
  - Add ATOMiK peripheral to the bus fabric
  - Synthesize the combined design
  - **Requirement:** Synthesis succeeds, timing closure met, resource utilization documented

- [ ] **Task 1.2.4:** Write bare-metal C test program
  ```c
  #define ATOMIK_BASE 0x00040000
  #define ATOMIK_LOAD    (*(volatile uint32_t*)(ATOMIK_BASE + 0x00))
  #define ATOMIK_ACCUM   (*(volatile uint32_t*)(ATOMIK_BASE + 0x04))
  #define ATOMIK_READ    (*(volatile uint32_t*)(ATOMIK_BASE + 0x08))

  void test_atomik() {
      ATOMIK_LOAD = 0xDEADBEEF;     // Set initial state
      ATOMIK_ACCUM = 0x000000FF;     // Apply delta
      uint32_t result = ATOMIK_READ;  // Read: should be 0xDEADBE10
      // Print over UART...
  }
  ```

- [ ] **Task 1.2.5:** Performance baseline
  - Measure: cycles for ATOMiK operations vs equivalent software operations
  - Measure: combined Fmax, total LUT/FF usage
  - Document results in `docs/PHASE1_INTEGRATION_RESULTS.md`

### 2.3 Phase 1 Validation Gate

- [ ] PicoRV32 boots and runs bare-metal C code on Tang Nano 9K
- [ ] ATOMiK delta accumulator is accessible as a memory-mapped peripheral
- [ ] UART communication works bidirectionally
- [ ] Combined resource utilization is documented and leaves headroom
- [ ] Timing closure is met
- [ ] At least one C test program demonstrates delta accumulation via PicoRV32

---

## 3. Phase 2 — ATOMiK Runtime & Delta-State Memory

**Goal:** Build the bridge between PicoRV32's standard C execution and ATOMiK's delta-state operations. Implement a minimal runtime that replaces key memory operations with delta-state equivalents.

**Estimated Duration:** 3-4 weeks

### 3.1 Minimal C Runtime (ATOMiK libc)

- [ ] **Task 2.1.1:** Create `atomik-libc/` directory structure
  ```
  atomik-libc/
  ├── include/
  │   ├── string.h      # ATOMiK-backed string operations
  │   ├── stdlib.h       # ATOMiK-backed memory management
  │   ├── stdio.h        # UART-backed printf (minimal)
  │   └── atomik.h       # Direct ATOMiK hardware access API
  ├── src/
  │   ├── string.c       # memcpy, memset, memcmp (delta-state backed)
  │   ├── stdlib.c       # malloc, free (delta-state allocator)
  │   ├── stdio.c        # printf, putchar (UART)
  │   └── atomik.c       # Hardware register access, init
  ├── crt0.S             # Startup code for PicoRV32
  ├── linker.ld          # Linker script
  └── Makefile
  ```

- [ ] **Task 2.1.2:** Implement startup code (`crt0.S`)
  - Initialize stack pointer, zero BSS section, initialize ATOMiK hardware, call `main()`

- [ ] **Task 2.1.3:** Implement `atomik.h` / `atomik.c` — direct hardware API
  - `atomik_init()`, `atomik_load(bank, value)`, `atomik_accumulate(bank, delta)`, `atomik_read(bank)`, `atomik_reset(bank)`, `atomik_undo(bank, delta)`

- [ ] **Task 2.1.4:** Implement delta-state `memcpy`
  - Compute XOR delta between source and destination, apply to ATOMiK accumulator
  - Measure cycle count vs standard implementation
  - **This is the first concrete "library replacement" data point**

- [ ] **Task 2.1.5:** Implement delta-state `memset`
  - Compute delta from current memory state to target fill value, apply via ATOMiK

- [ ] **Task 2.1.6:** Implement minimal `printf` over UART
  - Supports: `%d`, `%x`, `%s`, `%c` at minimum

### 3.2 Delta-State Memory Manager

- [ ] **Task 2.2.1:** Design delta-state aware allocator
  - Track allocations as delta chains rather than pointer tables
  - Support: `malloc()`, `free()`, `realloc()`

- [ ] **Task 2.2.2:** Implement and benchmark
  - Compare against standard bump allocator
  - Document memory overhead, cycle counts, fragmentation behavior

### 3.3 Phase 2 Validation Gate

- [ ] ATOMiK libc compiles for RV32IMC target
- [ ] At least 3 standard library functions have delta-state implementations
- [ ] Performance comparison data: cycles for ATOMiK vs standard for each function
- [ ] A C program using ATOMiK libc can: initialize, allocate memory, compute, print results over UART
- [ ] All results documented in `docs/PHASE2_RUNTIME_RESULTS.md`

---

## 4. Phase 3 — Core Generator & Synthesis Pipeline

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

## 5. Phase 4 — OS Shell & User Interface

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

## 6. Phase 5 — Library Replacement Pipeline (Automated)

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

## 7. Phase 6 — Multi-Node Scaling Demo

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

## 8. Phase 7 — ATOMiK IDE & Comparative Visualization

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

## 9. Phase 8 — Executive Automation Pipelines

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

## 10. Phase 9 — Post-Demo Expansion

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

## 11. Repository Structure (v2)

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

*This document is a living roadmap. Update as decisions are made and phases are completed.*
*Last updated: February 12, 2026*
