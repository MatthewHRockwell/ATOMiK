
# ATOMiK v3 — Phase 1 Initiation Prompt
**Phase 1: Custom RV64I CPU Core**

---

## Purpose

Build a **correct, minimal, multi-cycle RV64I CPU core** that passes the `rv64ui-p-*` compliance suite under Verilator and synthesizes on GW1NR-9 within budget. This phase is **CPU-only**: no ATOMiK integration yet. The goal is to establish a **trustworthy control plane** before wiring in the delta-state engine.

**Success Criteria (Exit Gates):**
- All `rv64ui-p-*` tests pass in Verilator (47/47).
- CPU-only synthesis meets: **≤ 2,500 LUT4**, **Fmax ≥ 25 MHz**.
- Directed tests validate ALU, branches, and LSU (especially LD/SD via 64→32 adapter).
- No regressions introduced to v2 CI jobs.

> **Process Reminder:** Keep **`ATOMiK/docs/KNOWN_ISSUES.md`** up to date. Log every significant hardware/software issue encountered, its root cause, and the resolution. Treat it as a first-class troubleshooting reference for Phase 1 and beyond.

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
- Prefer **process improvements** that reduce human error, shorten feedback loops, or increase verification confidence.
- Treat **tooling improvements** (coverage scripts, compliance runners, synthesis report parsers, log summarizers, waveform helpers) as **first-class deliverables**:
  - Check them into the repo when useful,
  - Reference them in the **Phase Context file**,
  - Carry them forward to subsequent phases.

---

## Cross-Phase Continuity & Review Discipline (Mandatory)

### 1. Living Context File (Phase Context)
- Maintain a **single context file** (e.g., `ATOMiK/docs/PHASE_CONTEXT.md` or `docs/v3_context.md`) that:
  - Is **rewritten/curated at the end of each phase**.
  - Contains only **current, relevant facts**: architecture decisions, constraints, verified assumptions, open risks, and accepted tradeoffs.
  - Explicitly removes outdated assumptions and superseded decisions.
- This file is the **handoff artifact between phases** and must be updated before starting the next phase.

### 2. Blind Validation via Agent (Regular Audit)
- At regular milestones (e.g., after major features land, before declaring Phase 1 “done”):
  - **Spin up a separate validation agent** that has **no prior context** of the project.
  - Provide it only:
    - The Phase 1 acceptance criteria,
    - The current design/RTL summaries,
    - The test results and synthesis reports.
  - Instruct it to perform a **blind validation**:
    - Check for **violations of acceptance criteria**,
    - Identify **likely bugs, blind spots, or false assumptions**,
    - Flag **scope creep, architectural drift, or unjustified claims**.
- Treat this as a **hostile external design review**. The goal is to catch mistakes and weak assumptions early.
- Summarize findings and either:
  - Fix the issues, or
  - Explicitly accept and record the risk in the **Phase Context file**.

### 3. Next-Phase Initiation Prompt
- When Phase 1 exits:
  - Generate a **Phase 2 Initiation Prompt** in the same style as this document:
    - Restate goals and acceptance criteria,
    - Carry forward only validated constraints from the **Phase Context file**,
    - Re-list risks, watch items, and process rules (tools, agents, validation).
- This ensures **each phase starts with a clean, explicit contract** rather than accumulated tribal knowledge.

> **Forward Guardrail (Phase 3):** The Phase 3 initiation prompt must explicitly require **minimal firmware first** ("hello world" over UART, LED blink, basic boot checks). Port the full test suite only after boot, UART, and memory map are proven stable.

---

## Architectural Guardrails

### 1. Microarchitecture
- Multi-cycle FSM: `FETCH → DECODE → EXECUTE → MEMORY → WRITEBACK`.
- Single, explicit PC update point (one mux, one enable).
- Writes commit at **end of WRITEBACK**; next instruction begins after WB completes.
- Make FSM state visible in waveforms (enum/one-hot) for fast debug.

### 2. Register File (BSRAM)
- 32 × 64-bit in **one BSRAM block**, true dual-port.
- RS1/RS2 read in parallel during **DECODE**; accept **1-cycle read latency**.
- Ensure **write-after-read hazards** are avoided by committing writes at end of WB.
- x0 must always read as zero (via address decode or output mux).

### 3. ALU
- Implement RV64I integer ops + W-variants.
- Use Gowin carry chains for ADD/SUB; keep XOR on LUTs (no carry).
- Structure as sub-units (`adder/sub`, `logic`, `shift`, `compare`) with a small select mux to avoid wide mux trees.

### 4. Branch / Jump
- Support BEQ/BNE/BLT/BGE/BLTU/BGEU, JAL, JALR.
- Centralize PC control; no “distributed” PC writes.

### 5. Load/Store Unit + 64→32 Adapter
- LD/SD split into two 32-bit bus transactions (lower word first).
- Correct endianness, alignment handling, and sign/zero extension.
- Mirror v2 memory map.
- **Write directed tests** before full compliance: round-trip stores/loads, LB/LH/LW/LWU/LD edge cases, alignment behavior.

### 6. CSRs (Minimal)
- Implement **`rdcycle`** (read-only) initially.
- Add only the minimum additional CSR support required to pass compliance—no feature creep in Phase 1.

---

## Verification Strategy (Non-Negotiable)

### 1. Bring-Up Order
- ALU ops (ADD/SUB/XOR/OR/AND/shifts)
- LUI/AUIPC
- Branches
- Loads/Stores
- JAL/JALR

### 2. Instrumentation
- Log: PC, instruction, FSM state, regfile writeback.
- When a test fails, you must be able to pinpoint **which instruction retired incorrectly** and why.

### 3. Compliance
- Run full `rv64ui-p-*` under Verilator.
- Expect early wins followed by LSU/branch corner-case fixes—plan time accordingly.
- Keep failures and fixes documented in **`KNOWN_ISSUES.md`** and reflected in the Phase Context file.

### 4. Coverage-Driven Confidence
- **Enable Verilator coverage** (at least line coverage) on the RTL.
- Use coverage to confirm:
  - All instruction decode paths are exercised,
  - All major FSM states are reached,
  - Corner cases in branch, load/store, and writeback paths are hit.
- Treat uncovered lines/branches in decode and control logic as **action items**.
- Record coverage gaps and resolutions in **`KNOWN_ISSUES.md`** (if they expose bugs) and the **Phase Context file** (if they are intentional exclusions).

---

## Synthesis & Timing Checks

- Synthesize **CPU-only** early and often (stub ATOMiK if needed).
- Targets: **≤ 2,500 LUT4**, **Fmax ≥ 25 MHz**.
- Use synthesis to detect:
  - Unexpected resource growth,
  - Inferred structures (e.g., mux explosions, unintended RAM inference),
  - Timing-critical paths early.
- If over budget, likely culprits:
  - Decoder complexity
  - Shifter implementation
  - Over-wide muxes in ALU or writeback path
- Optimize structure before adding features.
- Summarize key synthesis findings and decisions in the **Phase Context file**.

---

## Memory Configuration Discipline

- Gowin BSRAM has multiple behavioral modes (read-first/write-first, output registers, byte enables, clocking options).
- For **each** BSRAM instance used in Phase 1 (especially the register file):
  - Document in the **Phase Context file**:
    - Port configuration (true dual-port vs. simple dual-port),
    - Read-during-write behavior,
    - Output registering,
    - Write enable behavior,
    - Any byte-enable or width adaptation settings.
- Rationale:
  - Ensures reproducibility,
  - Prevents silent behavioral changes during refactors,
  - Avoids regressions when moving into Phase 2 and Phase 3.

---

## Phase Discipline

- **Do not** add ATOMiK wiring, custom op execution, or peripherals in Phase 1.
- The only acceptable scope creep is what’s strictly required to pass RV64I compliance.
- Every non-obvious bug, tool quirk, or synthesis surprise goes into:
  - **`ATOMiK/docs/KNOWN_ISSUES.md`** (issues + fixes), and
  - The **Phase Context file** (current truths, constraints, and accepted risks).

---

## Definition of “Done”

Phase 1 is complete when:
- The CPU passes **all RV64I compliance tests** in simulation.
- CPU-only synthesis meets area and timing targets.
- Directed tests for ALU, branches, and LSU pass.
- **KNOWN_ISSUES.md** reflects real issues encountered and their resolutions.
- The **Phase Context file** is updated to reflect the final, validated state of Phase 1.
- A **blind-validation agent** has been run, and major findings have been fixed or consciously accepted and recorded.

Only then proceed to **Phase 2: ATOMiK v3 Datapath Integration** and generate the **Phase 2 Initiation Prompt**.
