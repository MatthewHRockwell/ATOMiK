# ATOMiK v3 --- Phase Context (Living Document)

> **Purpose:** This file is the single source of truth carried between
> phases. It is **rewritten/curated at each phase boundary** to retain
> only current, relevant facts.

------------------------------------------------------------------------

## 1. Phase Summary

-   **Current Phase:** Phase 3 --- SoC Integration (Tang Nano 9K)
-   **Date Updated:** 2026-03-03
-   **Status:** Boot Chain Complete (BROM → ISP → Flash XIP validated). Timing violations open (V3-020).
-   **Exit Decision:** Conditional Go (requires timing fix and HDMI verification)

------------------------------------------------------------------------

## 2. Scope & Goals (Current Phase)

Phase 3 integrates the v3 CPU + ATOMiK into a complete SoC with peripherals (UART, GPIO, HDMI),
boots from SPI flash on Tang Nano 9K, and validates the full system on hardware.

**Achieved:**
- Full SoC synthesis (5,594 LUT, 16 BSRAM)
- SRAM deployment working (bitstream loaded to volatile memory)
- UART communication functional (115200 baud)
- Three critical bugs discovered and fixed (power-on reset, bus arbiter, UART timing)
- ISP flasher ported to RV64I and running on hardware (ISP_STAGE3)
- Flash boot chain validated: BROM → ISP timeout (~14s) → JUMP! → XIP → F!F! repeating
- ISP programming flow working: handshake, sector erase, page program, checksum verify
- Critical bugs fixed: V3-018 (GCC null-pointer UB), V3-019 (ESEC gating)
- Golden tag locked: `v3-boot-chain-golden`

**Remaining:**
- Fix timing violations: 40 setup violations at 25.2 MHz, Fmax 24.745 MHz (V3-020)
- ISP protocol robustness: NACK/error codes, readback verify command
- Flash deployment to persistent storage (bitstream)
- HDMI output verification
- Full hardware test suite execution (ATOMiK custom instructions on hardware)

**Exclusions:** Display pipeline (Phase 4), multi-node streaming (Phase 5), parallel banks (Phase 6).

------------------------------------------------------------------------

## 3. Acceptance Criteria (Contract)

| Criterion | Target | Actual | Status |
|-----------|--------|--------|--------|
| Full SoC synthesis | Completes with zero TNS | 5,594 LUT, 16 BSRAM, 0 TNS | PASS |
| Synthesis LUT budget | ≤3,100 LUT4 | 5,594 LUT | FAIL (64-bit inherently larger) |
| Synthesis Fmax | ≥25 MHz | 25.324 MHz (13 logic levels) | PASS |
| CLS utilization | <90% | 87% (3,738/4,320) | PASS |
| BSRAM utilization | <80% | 62% (16/26) | PASS |
| UART communication | 115200 baud functional | Working (ISP flasher verified) | PASS |
| ATOMiK tests (sim) | 9/9 Verilator | 9/9 PASS | PASS |
| ATOMiK tests (hardware) | 9/9 on real FPGA | Pending (requires flash-resident test firmware) | PENDING |
| SRAM deployment | Bitstream loads, UART works | Working after 3 critical bug fixes | PASS |
| Flash boot chain | BROM → ISP → XIP execution | Validated: 946 clean F!F! prints, golden tag locked | PASS |
| ISP flash programming | Program firmware via UART ISP | Working: handshake, erase, program, checksum | PASS |
| Timing violations | Zero setup violations | 40 violations at 25.2 MHz, Fmax 24.745 MHz (V3-020) | FAIL |
| Flash deployment (persistent) | Persistent boot from flash | Not yet tested (bitstream persistence) | PENDING |
| HDMI output | Test pattern displays | Not yet verified | PENDING |
| KNOWN_ISSUES.md | Updated | V3-009 through V3-020 added | PASS |
| Phase Context | Updated | This document | PASS |

------------------------------------------------------------------------

## 4. Architecture Snapshot (As-Built)

### CPU Core (`atomik_v3_cpu`)

Multi-cycle RV64I CPU with 5-state FSM:
```
FETCH → DECODE → EXECUTE → MEMORY → WRITEBACK
```

- **FETCH**: Present PC to 32-bit bus, latch instruction on `mem_ready`
- **DECODE**: Decode opcode, registered BSRAM regfile reads (1-cycle latency)
- **EXECUTE**: ALU/branch/jump computation, ATOMiK custom-0 execution
- **MEMORY**: Load/store bus transactions (skipped for ALU/branch/custom)
- **WRITEBACK**: Write result to regfile, advance PC

### ATOMiK Datapath (`atomik_v3_atomik`)

Direct-wired to CPU (no bus, no CDC):
- **Accumulator**: 64-bit XOR with synchronous clear on LOAD
- **State Table**: 256×64-bit via 2 SDPB blocks (BSRAM)
- **Reconstructor**: `current_state = state_table[active_addr] ^ accumulator`
- **Active Address**: 8-bit register, set by LOAD or SWAP

Custom-0 encoding (opcode 0x0B):
| funct3 | Instruction | Operation |
|--------|-------------|-----------|
| 000 | ATOMIK.LOAD | state_table[rs1] = rs2, clear acc, set addr |
| 001 | ATOMIK.ACCUM | acc ^= rs1 |
| 010 | ATOMIK.READ | rd = state_table[addr] ^ acc |
| 011 | ATOMIK.SWAP | addr = rs1 (acc unchanged) |

### Module Hierarchy

```
atomik_v3_cpu (top)
├── atomik_v3_fetch      — PC, instruction latch, bus interface
├── atomik_v3_decode     — RV64I + custom-0 decode, immediate gen
├── atomik_v3_regfile    — 32×64-bit BSRAM (4 blocks, dual SDP)
├── atomik_v3_alu        — 64-bit ALU, shared barrel shifter
├── atomik_v3_branch     — Branch comparator (6 conditions)
├── atomik_v3_lsu        — Load/store with 64→32 adapter
├── atomik_v3_csr        — Minimal M-mode CSRs
├── atomik_v3_atomik     — ATOMiK datapath (2 BSRAM)
└── atomik_v3_control    — 5-state FSM controller
```

------------------------------------------------------------------------

## 5. Verified Assumptions

| Assumption | Validation |
|------------|------------|
| BSRAM regfile fits in 4 blocks | Gowin synthesis: 4 SDPB inferred |
| Registered reads work with multi-cycle FSM | 53/54 compliance pass |
| Custom-0 instructions encode/decode correctly | `.insn r 0x0B` assembles, CPU decodes funct3 |
| ATOMiK XOR algebra works in hardware | 21 iverilog + 11 Verilator tests |
| Accumulator is global (shared across contexts) | SWAP tests verify: acc persists across addr change |
| LOAD clears accumulator | Directed tests confirm acc=0 after LOAD |
| State table survives SWAP | Multi-context tests verify init_data persists |
| Shared barrel shifter works for SLL/SRL/SRA | All 12 shift compliance tests pass |
| CSR narrowing doesn't break compliance | 53/54 pass with minimal CSRs |

------------------------------------------------------------------------

## 6. Open Risks & Watch Items

### RISK-1: Combined LUT exceeds 2,700 target (3,181 actual)

- **Impact:** Less room for SoC peripherals in Phase 3. Total SoC may exceed 50% utilization.
- **Mitigation options:**
  - Iterative shifter (saves ~300 LUT, adds up to 63 cycles per shift)
  - W-variant instruction removal if firmware doesn't use them (~100 LUT)
  - Re-evaluate target: 2,700 was aspirational. v2 SoC used 3,838 LUT total with PicoRV32.
- **Decision point:** Phase 3 SoC synthesis will determine if this is actually blocking.

### RISK-2: Fmax not measured (PnR fails on CPU-only)

- **Impact:** Fmax target (25 MHz) unverified until SoC wrapper is built in Phase 3.
- **Mitigation:** Logic levels = 13 from synthesis report. v2 achieved 30.6 MHz CPU Fmax with comparable logic depth. Should be fine.

### RISK-3: Regfile testbench broken

- **Impact:** No standalone regfile verification via iverilog.
- **Mitigation:** Full CPU works (compliance passes). Fix testbench in Phase 3.

------------------------------------------------------------------------

## 7. Key Tradeoffs & Rationale

### Shared barrel shifter vs. separate SLL/SRL/SRA

- **Chosen:** Single right-shift barrel with bit-reversal for SLL
- **Alternative:** Separate left/right barrel shifters, or iterative shifter
- **Rationale:** Bit reversal is free in hardware (routing). Saves ~280 LUT vs dual barrel. Keeps single-cycle shift execution.

### Counter removal (mcycle/minstret)

- **Chosen:** Read-only zero
- **Alternative:** Keep 64-bit counters
- **Rationale:** Saves ~300 LUT. Compliance tests don't read these. Can be re-added if needed.

### ALU reuse for branch/JAL targets

- **Chosen:** Route branch/JAL target through ALU (alu_operand_a = PC)
- **Alternative:** Separate pc+imm adder
- **Rationale:** Saves 63 ALU carry chains. Multi-cycle FSM means ALU is free during branch/JAL.

### Accumulator is global (not per-context)

- **Chosen:** Single 64-bit accumulator shared across all SWAP contexts
- **Alternative:** Per-context accumulator stored in BSRAM alongside state table
- **Rationale:** Matches v2 architecture. SWAP is a "view change" — you see a different initial state XOR'd with the same accumulator. This is the correct delta-state algebra semantics.

------------------------------------------------------------------------

## 8. Tooling & Infrastructure State

| Tool | Status | Notes |
|------|--------|-------|
| Verilator (compliance) | Working | 53/54 pass, ELF loader, tohost monitoring |
| Verilator (coverage) | Working | 98% line coverage with merged ATOMiK tests |
| iverilog (unit tests) | Working | ALU, ATOMiK, branch, decode, LSU, smoke all pass |
| Gowin EDA synthesis | Working | synth_v3.tcl, PnR fails on I/O (expected) |
| riscv64-unknown-elf-gcc | Working | `.insn r` for custom-0 encoding |
| Makefile targets | Working | lint, sim-iverilog, sim-verilator, compliance, coverage |
| Debug runner | Available | debug_runner.cpp for single-test trace analysis |

------------------------------------------------------------------------

## 9. Resource & Performance Snapshot

### Combined CPU + ATOMiK (Gowin GW1NR-9)

| Resource | Count | Available | Utilization |
|----------|-------|-----------|-------------|
| Logic (total) | 3,437 | 8,640 | 40% |
| LUT | 3,181 | — | — |
| ALU | 256 | — | — |
| Register (FF) | 601 | 6,693 | 9% |
| BSRAM | 6 | 26 | 23% |
| Logic Levels | 13 | — | — |

### Per-Module Breakdown

| Module | LUT | ALU | DFF | BSRAM |
|--------|-----|-----|-----|-------|
| cpu_top (wiring/mux) | 1,590 | 0 | 0 | 0 |
| ALU (barrel shifter) | 601 | 128 | 0 | 0 |
| CSR | 487 | 0 | 260 | 0 |
| regfile | 135 | 0 | 0 | 4 |
| ATOMiK | 130 | 0 | 72 | 2 |
| fetch | 115 | 0 | 98 | 0 |
| LSU | 111 | 0 | 168 | 0 |
| control | 6 | 0 | 3 | 0 |
| decode | 5 | 0 | 0 | 0 |
| branch | 0 | 128 | 0 | 0 |

### ATOMiK CLS Mapping

- **1.016 CLS/bit** (target ≤1.2, v2 baseline ~1.7)
- 130 LUT + 72 FF + 2 BSRAM for full 64-bit datapath
- XOR reconstructor: 64 LUT2 (1 per bit) — optimal

------------------------------------------------------------------------

## 10. Memory Configuration Record

### BSRAM 1-4: Register File (`atomik_v3_regfile`)

- **Purpose:** 32×64-bit RV64I general-purpose registers
- **Configuration:** 2 banks (A/B) × 2 SDPB (low32/high32) = 4 BSRAM
- **Port config:** Simple dual-port (one write, one read per bank)
- **Read-during-write:** Read returns old data (BSRAM default)
- **Output registering:** Yes (1-cycle read latency)
- **Write enable:** Synchronous, gated by `rd_wen && (rd_addr != 0)`
- **Width/depth:** Each SDPB: 32×32-bit (only 32 entries used of 256 possible)
- **x0 hardwire:** Write-enable blocked for address 0; read returns 0 via post-read mux

### BSRAM 5-6: ATOMiK State Table (`atomik_v3_atomik`)

- **Purpose:** 256×64-bit initial state storage for delta-state reconstruction
- **Configuration:** 2 SDPB (low32/high32)
- **Port config:** Simple dual-port (write on LOAD, continuous read of active_addr)
- **Read-during-write:** Read returns old data
- **Output registering:** Yes (1-cycle read latency)
- **Write enable:** `load_en` only (ATOMIK.LOAD instruction)
- **Width/depth:** Each SDPB: 256×32-bit

------------------------------------------------------------------------

## 11. Known Issues Cross-Reference

| Issue | Status | Relevance |
|-------|--------|-----------|
| V3-001: LSU rdata gating | Fixed | Lesson: don't gate data on registered done |
| V3-002: MemModel base address | Fixed | — |
| V3-003: Regfile testbench timing | Fixed (Phase 1) | — |
| V3-004: Gowin double-add files | Fixed | — |
| V3-005: LUT budget | Resolved | 5,594 LUT in full SoC (65% utilization) |
| V3-006: ma_data test | Accepted | By design, no misaligned traps |
| V3-007: PnR I/O count | Fixed | SoC wrapper provides proper I/O |
| V3-008: Regfile TB for BSRAM | Open | iverilog only, not RTL |
| V3-009: Power-on reset failure | **Fixed** | **CRITICAL: sys_resetn constant, FSMs uninitialized** |
| V3-010: Bus arbiter crosstalk | **Fixed** | **CRITICAL: mem_ready gating needed** |
| V3-011: UART timing violation | **Fixed** | **HIGH: CLKDIV write requires delay before DATA** |

------------------------------------------------------------------------

## 12. Phase 3 Critical Bugs Discovered

During SRAM deployment and hardware validation, three critical bugs were discovered that prevented the SoC from functioning on real hardware (all passed in Verilator simulation):

### V3-009: Power-on Reset Failure
**Impact:** CPU FSMs powered on in unknown state, completely non-functional
**Root Cause:** `sys_resetn = 1'b1` constant assignment doesn't initialize flip-flops on FPGA power-up
**Fix:** Added 256-cycle reset counter with initial block
**Lesson:** Always use explicit reset generators on FPGAs; never rely on constant-high reset

### V3-010: Bus Arbiter Ready Signal Crosstalk
**Impact:** Fetch unit saw LSU bus responses, corrupting instruction fetch
**Root Cause:** Both fetch and LSU connected to `mem_ready` directly without gating
**Fix:** Gated `mem_ready` based on bus ownership (`lsu_has_bus`)
**Lesson:** When multiple masters share a bus, gate ready signals by ownership

### V3-011: UART Timing Violation
**Impact:** No UART output after boot (transmitter idle despite firmware writes)
**Root Cause:** simpleuart sends 15 idle bits after CLKDIV write (~1740 cycles); firmware wrote DATA immediately
**Fix:** Added 2000-cycle delay after UART initialization in firmware
**Lesson:** Peripheral initialization may have timing requirements not exposed in register interface

All three bugs were Verilator-silent (simulations passed) because Verilator initializes registers to zero and timing is idealized. Hardware validation is essential.

---

## 13. Phase Exit Summary

### Done and stable:
- Full SoC synthesis: 5,594 LUT, 16 BSRAM, zero TNS, 25.324 MHz Fmax
- UART communication: 115200 baud working on hardware
- ATOMiK tests: 9/9 Verilator simulation pass
- SRAM deployment: bitstream loads, firmware boots, ISP flasher functional
- Three critical hardware bugs discovered and fixed
- Firmware ported to RV64I (boot ROM 1,456 bytes, flash firmware 6,344 bytes)
- Single clock domain: CPU + ATOMiK @ 25.2 MHz (freed 1 PLL vs v2)

### Remaining for Phase 3 completion:
- Flash deployment to persistent storage (currently SRAM-only)
- HDMI output verification (test pattern display)
- Full hardware test suite execution (9 ATOMiK tests over UART)

### Constraints for next phase (Phase 4):
- SoC uses 16/26 BSRAM (62%), 10 remaining for display pipeline
- CLS at 87% (3,738/4,320), tightest resource — display must be CLS-efficient
- LUT at 65% (5,594/8,640), 3,046 remaining for display + control logic
- Single 25.2 MHz clock domain for CPU/ATOMiK, 126 MHz HDMI serializer
- ATOMiK state table in BSRAM (256×64-bit), accessible via custom instructions

------------------------------------------------------------------------

## 14. Next Phase Handoff

-   **Next Phase:** Phase 4 --- Display Pipeline (after Phase 3 flash deployment complete)
-   **Key constraints:**
    - 5,594 LUT used by SoC; 3,046 LUT available for display pipeline
    - 16 BSRAM used; 10 available (need +2 for delta color LUT + scanline buffer)
    - CLS at 87% (tightest resource) — display pipeline must be CLS-efficient
    - 1 PLL available (PLL2) if needed for display-specific clocking
-   **Key risks:**
    - CLS budget nearly exhausted — delta color LUT and scanline buffer must be BSRAM-based
    - Display pipeline control logic must fit in 3,046 LUT (~200 LUT target)
    - CDC between ATOMiK (25.2 MHz) and pixel clock (25.2 MHz) — same domain, no CDC needed!
-   **Prerequisites (Phase 3 completion):**
    - Flash deployment working (persistent boot)
    - HDMI test pattern verified on screen
    - Full hardware test suite passing (9/9 ATOMiK tests over UART)

------------------------------------------------------------------------

> **Maintenance Rule:** At the end of each phase, **rewrite this
> document** to reflect only the current truth.
