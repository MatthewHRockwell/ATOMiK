# ATOMiK v3 --- Phase 2 Initiation Prompt
**Phase 2: ATOMiK v3 Datapath Integration**

---

## Purpose

Integrate the ATOMiK delta-state engine with the v3 RV64I CPU core. This phase has **two critical objectives**:

1. **Resolve the LUT budget crisis** inherited from Phase 1 (8,013 LUT CPU-only, 97% utilization). The BSRAM register file optimization is **BLOCKING** --- no feature work until CPU-only LUT is below 3,000.
2. **Wire the ATOMiK datapath** --- BSRAM state table, XOR accumulator, state reconstructor, and 4 custom instructions (ATOMIK.LOAD/ACCUM/READ/SWAP) executing via direct wire (no bus, no CDC).

**Success Criteria (Exit Gates):**
- CPU-only synthesis: **<= 2,500 LUT4** (down from 8,013 via BSRAM regfile + optimizations)
- Combined CPU + ATOMiK synthesis: **<= 2,700 LUT4**, **Fmax >= 25 MHz**
- All 4 custom instructions pass directed testbench verification
- CLS mapping: **<= 1.2 CLS/bit** for ATOMiK datapath (target: 1.0)
- rv64ui-p-* compliance: **no regressions** (53/54 still pass)
- BSRAM usage: register file + state table (verify count)
- Phase 1 blind validation deferred items resolved (coverage, LSU testbench, bus assertion)

> **Process Reminder:** Keep **`ATOMiK/docs/KNOWN_ISSUES.md`** up to date. Log every significant issue encountered, its root cause, and the resolution.

---

## Coordination & Tooling Mandate (Always In Effect)

You are to act as the **coordinator, project manager, and technical adviser**:

- Continuously **identify, design, or refine tools** (scripts, harnesses, generators, checkers, CI jobs, debug utilities) that **optimize the workflow** for the current task set.
- **Employ model-appropriate agents** for lower-level or specialized tasks (e.g., focused code review, spec consistency checks, test generation, synthesis log analysis).
- Orchestrate and delegate when it makes sense, but remain the **filter and single source of truth** for:
  - Architectural decisions
  - Acceptance status
  - Phase readiness
  - Final conclusions and next actions
- Treat **tooling improvements** as **first-class deliverables**.

---

## Cross-Phase Continuity & Review Discipline (Mandatory)

### 1. Living Context File (Phase Context)
- Maintain `ATOMiK/docs/ATOMiK_v3_Phase_Context.md` as the single source of truth.
- **Rewrite** at the end of Phase 2 to reflect only current, relevant facts.
- This file is the **handoff artifact** to Phase 3.

### 2. Blind Validation via Agent (Regular Audit)
- Before declaring Phase 2 "done":
  - **Spin up a separate validation agent** with **no prior context**.
  - Provide only: Phase 2 acceptance criteria, design summaries, test results, synthesis reports.
  - Instruct it to perform a **hostile external design review** per `docs/ATOMiK_v3_Blind_Validation_Agent_Template.md`.
- Summarize findings. Fix or explicitly accept and record risks.

### 3. Next-Phase Initiation Prompt
- When Phase 2 exits:
  - Generate a **Phase 3 Initiation Prompt** in the same style.
  - Carry forward only validated constraints from the Phase Context file.

> **Forward Guardrail (Phase 3):** The Phase 3 initiation prompt must explicitly require **minimal firmware first** ("hello world" over UART, LED blink, basic boot checks). Port the full test suite only after boot, UART, and memory map are proven stable.

---

## Inherited State from Phase 1

### What Exists and Works
- **9-module RV64I CPU** in `hardware/v3/rtl/`: cpu, fetch, decode, regfile, alu, branch, lsu, csr, control
- **53/54 rv64ui-p-* compliance pass** (only `ma_data` fails by design)
- **Directed tests**: ALU 24/24, Decoder 42/42, Regfile 38/38, Branch 15/15
- **Compliance infrastructure**: Verilator runner, ELF loader, memory model
- **Gowin synthesis script**: `synth_v3.tcl` (synthesis works, PnR fails on CPU-only I/O count --- expected)
- **Custom-0 decode**: `dec_is_custom0` signal already wired in `atomik_v3_cpu.v`

### What Does NOT Work
- **LUT budget**: 8,013 LUT (97%) --- must be reduced to <= 2,500 CPU-only
- **No ATOMiK integration** --- custom-0 instructions decode but execute as NOP
- **No Verilator coverage** --- required by Phase 1 initiation prompt, deferred
- **No standalone LSU testbench** --- blind validation finding
- **No bus mutual exclusion assertion** --- blind validation finding

### Synthesis Baseline (Phase 1 CPU-only)

| Resource | Usage | Target |
|----------|-------|--------|
| LUT | 8,013 (97%) | <= 2,500 |
| Register | 2,893 (44%) | -- |
| BSRAM | 0/26 | -- |
| Fmax | 28.8 MHz | >= 25 MHz |

---

## Phase 2 Task Ordering

### Priority 0: Phase 1 Debt (BLOCKING)

These must be completed BEFORE any Phase 2 feature work.

#### P0.1: Verilator Coverage Analysis
- Enable `--coverage-line` on the compliance runner build
- Generate coverage report after running all rv64ui-p-* tests
- Examine `atomik_v3_decode.v` and `atomik_v3_control.v` for uncovered paths
- Treat uncovered lines in decode and control logic as **action items**

#### P0.2: Standalone LSU Testbench
- Write `sim/iverilog/tb_v3_lsu.v`
- Test: LB/LH/LW/LD at byte offsets 0-3, SB/SH/SW/SD at byte offsets 0-3
- Test: LD/SD two-transaction path (lower word first, upper word to addr+4)
- Test: Multi-cycle bus delay (deassert `bus_ready` for 1-5 cycles)
- Test: Back-to-back transactions

#### P0.3: Bus Mutual Exclusion Assertion
- Add check in compliance runner: abort if `fetch_bus_valid && lsu_bus_valid` ever both high
- Verify no assertion fires during full compliance run

### Priority 1: BSRAM Register File (BLOCKING for LUT target)

#### P1.1: BSRAM Register File Implementation
- Replace behavioral `reg [63:0] regs [1:31]` with BSRAM-backed storage
- **GW1NR-9 BSRAM constraint**: True dual-port mode has max 16-bit width per port
- **Options** (evaluate before implementing):
  - **Option A**: 4 BSRAM blocks (duplicated SDP, 64-bit width, simultaneous RS1/RS2 read) --- zero DECODE cycle impact, uses 4/26 BSRAM
  - **Option B**: 2 BSRAM blocks (SDP, 64-bit width, sequential RS1/RS2 reads in 2 sub-cycles) --- adds 1 cycle to DECODE, uses 2/26 BSRAM
  - **Option C**: Gowin pROM or other special primitive --- investigate
- Maintain x0 hardwire-to-zero (output mux, not stored)
- BSRAM write in WRITEBACK stage (synchronous, gated by `regfile_wen && rd != 0`)
- **Key**: registered reads change timing. Compliance MUST still pass after this change.

#### P1.2: Synthesis Verification After Regfile
- Synthesize CPU-only after BSRAM regfile
- Target: **<= 3,000 LUT** (ideally <= 2,500)
- If still over 3,000: implement barrel shifter optimization (two-stage 32+fine)
- If still over 2,500: optimize CSR read mux (group by address range)

#### P1.3: Compliance Regression After Regfile
- Run full rv64ui-p-* suite
- Must still be 53/54 pass (no regressions)

### Priority 2: ATOMiK Datapath Integration

#### P2.1: ATOMiK Accumulator Module
- Write `hardware/v3/rtl/atomik_v3_acc.v`
- Parameterized DW (default 64)
- XOR accumulation: `acc <= acc ^ delta_in` on `accum_en`
- Clear on `load_en`: `acc <= 0`
- Zero detection: `acc_zero = ~(|acc)`
- Apply `(* syn_preserve = 1 *)` on accumulator register

#### P2.2: BSRAM State Table
- Instantiate 1 BSRAM block (288x64-bit or 576x32-bit configuration)
- Single read port for `initial_state` lookup
- Write port for context initialization (ATOMIK.LOAD)
- Address register updated by ATOMIK.SWAP and ATOMIK.LOAD
- Read latency: 1 cycle

#### P2.3: State Reconstructor
- Pure combinational: `current_state = initial_state ^ accumulator`
- Apply `(* syn_keep = 1 *)` on output wire

#### P2.4: CPU Integration (Custom-0 Execute)
- Wire ATOMiK into the Execute stage using existing `dec_is_custom0` signal:
  - `ATOMIK.LOAD` (funct3=0x0): `bsram_addr <= rs1[8:0]`, `load_en` (clears acc), initiate BSRAM read
  - `ATOMIK.ACCUM` (funct3=0x1): `delta_in <= rs1_data`, `accum_en`
  - `ATOMIK.READ` (funct3=0x2): `rd <= initial_state ^ accumulator`
  - `ATOMIK.SWAP` (funct3=0x3): `bsram_addr <= rs1[8:0]` (acc unchanged)
- Custom instructions skip MEMORY stage (4 cycles: FETCH->DECODE->EXECUTE->WRITEBACK)
- No bus, no CDC, no protocol --- direct wires within the CPU module

### Priority 3: Verification

#### P3.1: Directed ATOMiK Testbenches
- Write testbenches (Verilator + iverilog) for each custom instruction:
  - ATOMIK.LOAD: verify accumulator clears, initial_state reads correctly
  - ATOMIK.ACCUM: verify `acc = acc ^ delta`, multi-delta composition
  - ATOMIK.READ: verify `rd = initial_state ^ accumulator`
  - ATOMIK.SWAP: verify bsram_addr changes, accumulator persists
  - Context switch patterns: instant switch, full switch (SWAP+LOAD), fork (READ->store->SWAP)
- Verify XOR cancellation: `ACCUM(d); ACCUM(d)` returns to previous value
- Verify commutativity: `d1 ^ d2 ^ d3 == d3 ^ d1 ^ d2`

#### P3.2: Compliance Regression
- Run full rv64ui-p-* after ATOMiK integration
- Must still be 53/54 pass

### Priority 4: Combined Synthesis

#### P4.1: CPU + ATOMiK Synthesis
- Synthesize combined design through Gowin EDA
- Measure: LUT4, FF, BSRAM, Fmax, logic levels, CLS utilization
- Target: **<= 2,700 LUT4**, **Fmax >= 25 MHz**
- Verify BSRAM usage count

#### P4.2: CLS Mapping Validation
- Check synthesis report for ATOMiK datapath CLS mapping
- Target: **<= 1.2 CLS/bit** (ideally 1.0)
- Verify zero ALU inference on XOR paths
- Compare against v2 baseline (~1.7 CLS/bit)

---

## Architectural Guardrails

### 1. Bus Interface: Do NOT Change
- 32-bit valid/ready protocol (PicoRV32-compatible)
- Fetch and LSU share bus via FSM mutual exclusion
- ATOMiK does NOT use the bus (direct wire)

### 2. FSM: Preserve State Encoding
- S_FETCH(0), S_DECODE(1), S_EXECUTE(2), S_MEMORY(3), S_WRITEBACK(4)
- Other modules depend on these values

### 3. BSRAM Discipline
- For **each** BSRAM instance, document in Phase Context:
  - Port configuration (true dual-port, simple dual-port)
  - Read-during-write behavior
  - Output registering
  - Write enable behavior
  - Width/depth configuration

### 4. Custom Instruction Encoding
```
[31:25]  [24:20]  [19:15]  [14:12]  [11:7]   [6:0]
funct7   rs2      rs1      funct3   rd       opcode
0000000  00000    src      000-011  dest     0001011 (CUSTOM_0)
```
- ATOMIK.LOAD:  funct3=000, rs1=address, rd=unused
- ATOMIK.ACCUM: funct3=001, rs1=delta, rd=unused
- ATOMIK.READ:  funct3=010, rs1=unused, rd=destination
- ATOMIK.SWAP:  funct3=011, rs1=address, rd=unused

### 5. Synthesis Attributes
- `(* syn_preserve = 1 *)` on accumulator register (prevent merging with carry chains)
- `(* syn_keep = 1 *)` on state reconstructor output (prevent optimization of XOR)

---

## Verification Strategy

### 1. BSRAM Regfile Bring-Up Order
- Unit test the BSRAM regfile in isolation (iverilog) BEFORE integrating
- Run compliance IMMEDIATELY after integration (catch timing regressions early)
- Synthesize after compliance (catch LUT regressions)

### 2. ATOMiK Bring-Up Order
- Accumulator module unit test (standalone)
- State table unit test (standalone)
- Integration test (LOAD->ACCUM->READ sequence)
- Cancellation test (ACCUM(d)->ACCUM(d)->READ == initial_state)
- Commutativity test (different delta orders produce same result)
- Context switch test (SWAP between different state table entries)

### 3. Coverage
- Verilator line coverage on all RTL during compliance + ATOMiK tests
- Target: >90% on decode and control modules
- Treat uncovered paths as action items

### 4. Synthesis Checkpoints
- After BSRAM regfile: CPU-only synthesis (target <= 2,500 LUT)
- After ATOMiK integration: combined synthesis (target <= 2,700 LUT)
- After each: verify Fmax >= 25 MHz, check for unexpected resource growth

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| BSRAM regfile doesn't recover enough LUT | High | Plan B: barrel shifter optimization, CSR read mux. Plan C: iterative shifter (adds cycles). |
| BSRAM regfile changes break compliance | High | Run compliance immediately after every regfile change. |
| BSRAM dual-port width constraint (16-bit max) | Medium | 4-block duplicated SDP approach; verify block count fits budget. |
| DECODE timing changes with registered BSRAM reads | Medium | May need sub-cycle or pipeline bubble; monitor Fmax. |
| ATOMiK BSRAM state table conflicts with regfile BSRAM | Low | Separate BSRAM instances, independent address spaces. |
| CLS mapping suboptimal (>1.2 CLS/bit) | Medium | Adjust Verilog coding style, add synthesis attributes. |

---

## Files to Create

### RTL (1-3 files in `hardware/v3/rtl/`)
- `atomik_v3_acc.v` --- accumulator module
- Possibly: `atomik_v3_state_table.v` --- BSRAM state table (or integrated into cpu top)

### Testbenches
- `sim/iverilog/tb_v3_lsu.v` --- standalone LSU test (Phase 1 debt)
- `sim/iverilog/tb_v3_acc.v` --- accumulator unit test
- `sim/iverilog/tb_v3_atomik.v` --- integrated ATOMiK instruction test (or Verilator C++)

### Files to Modify
- `hardware/v3/rtl/atomik_v3_regfile.v` --- BSRAM replacement
- `hardware/v3/rtl/atomik_v3_cpu.v` --- ATOMiK integration, custom-0 execute logic
- `hardware/v3/rtl/atomik_v3_control.v` --- custom-0 FSM handling
- `hardware/v3/Makefile` --- coverage targets, new testbenches
- `hardware/v3/sim/compliance/compliance_runner.cpp` --- bus assertion, optional coverage
- `specs/atomik_v3_tasks.md` --- check off completed items
- `docs/ATOMiK_v3_Phase_Context.md` --- rewrite at phase exit

---

## Definition of "Done"

Phase 2 is complete when:
- BSRAM register file is implemented and CPU-only synthesis is **<= 2,500 LUT4**
- All 4 custom ATOMiK instructions execute correctly in simulation
- Combined CPU + ATOMiK synthesis: **<= 2,700 LUT4**, **Fmax >= 25 MHz**
- CLS mapping validated at **<= 1.2 CLS/bit**
- rv64ui-p-* compliance: **53/54 pass** (no regressions)
- Directed testbenches for all ATOMiK instructions pass
- Verilator coverage analysis shows >90% on decode and control modules
- **KNOWN_ISSUES.md** reflects all issues encountered
- **Phase Context file** is updated
- **Blind-validation agent** has been run and findings addressed
- **Phase 3 Initiation Prompt** is generated

Only then proceed to **Phase 3: SoC Integration**.
