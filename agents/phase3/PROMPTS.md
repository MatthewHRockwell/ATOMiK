# ATOMiK Phase 3: Hardware Synthesis
# Claude Code CLI Prompt Reference
# Date: January 25, 2026

## Overview

This document contains the prompts for executing Phase 3 using Claude Code CLI.
Each task has a designated model and specific deliverables.

## Pre-Execution Checklist

Before running any prompts:
1. [ ] Ensure Gowin EDA is installed and licensed
2. [ ] Verify existing RTL compiles: `cd rtl && iverilog -o test atomik_core.v`
3. [ ] Generate Gowin_rPLL reference module in `rtl/pll/` (if needed)
4. [ ] Confirm API key is set: `echo $ANTHROPIC_API_KEY`

## Context Files to Load

For all Phase 3 tasks, Claude Code should first read:
```
/read docs/ATOMiK_Development_Roadmap.md
/read reports/comparison.md
/read math/proofs/ATOMiK/Properties.lean
/read rtl/atomik_core.v
/read rtl/pll/atomik_pll_81m.v
/read .github/atomik-status.yml
```

---

## Task Prompts

### T3.1: RTL Architecture Specification
**Model**: Claude Sonnet 4.5
**Command**: `claude --model claude-sonnet-4-5`

```
You are implementing Phase 3 of the ATOMiK project: Hardware Synthesis.

CONTEXT:
- ATOMiK is a delta-state compute architecture where computation uses XOR-based 
  delta composition instead of traditional read-modify-write
- Phase 1 proved mathematical properties in Lean4 (commutativity, associativity, 
  self-inverse) that GUARANTEE hardware optimization opportunities
- Phase 2 benchmarks showed 95-100% memory traffic reduction and 85% parallel 
  efficiency potential, but a 32% read penalty in software due to O(N) reconstruction
- The hardware goal is to ELIMINATE the read penalty via parallel XOR trees

TASK T3.1: Create RTL Architecture Specification

Create a comprehensive RTL architecture document at specs/rtl_architecture.md that includes:

1. **Architecture Overview**
   - Block diagram (ASCII art) of ATOMiK core v2
   - Data flow for accumulate (write) and reconstruct (read) operations
   - Interface with existing atomik_top.v

2. **Module Definitions**
   - atomik_delta_acc: Delta accumulator (single-cycle XOR accumulation)
   - atomik_state_rec: State reconstructor (single-cycle current state output)
   - atomik_core_v2: Top-level integration

3. **Interface Specifications**
   - Port list with widths, directions, descriptions
   - Timing diagrams for accumulate and read operations
   - Reset behavior and initialization

4. **Microarchitecture Details**
   - Register descriptions (accumulator, initial_state)
   - Combinational logic (XOR paths)
   - Control FSM (if needed, or explain why not)

5. **Hardware Optimization Strategy**
   - How commutativity enables parallel reduction (cite Phase 1 theorem)
   - Why XOR is single-cycle (no carry propagation)
   - Future extension points for parallel delta input

6. **Target Constraints**
   - Device: GW1NR-LV9QN88PC6/I5 (Gowin GW1NR-9)
   - Clock: 27 MHz input, 81 MHz via existing PLL
   - Target frequency: 50 MHz minimum (conservative for Phase 3)
   - Delta/State width: 64 bits

7. **Verification Plan**
   - Test vectors needed
   - Equivalence check against Python reference (experiments/benchmarks/atomik/delta_engine.py)

Read the existing rtl/atomik_core.v and rtl/atomik_top.v for interface reference.
Read math/proofs/ATOMiK/Properties.lean for the proven properties to cite.

Output: specs/rtl_architecture.md

After completion, update .github/atomik-status.yml to mark T3.1_rtl_architecture: complete
```

---

### T3.2: Delta Accumulator Module
**Model**: Claude Sonnet 4.5
**Command**: `claude --model claude-sonnet-4-5`

```
You are implementing Phase 3, Task T3.2 of the ATOMiK project.

CONTEXT:
- Read specs/rtl_architecture.md for the design specification (from T3.1)
- The delta accumulator is the WRITE PATH of ATOMiK
- It must perform: acc <= acc ^ delta_in in a SINGLE CLOCK CYCLE
- XOR has no carry propagation, so 64-bit XOR is trivially single-cycle

TASK T3.2: Implement Delta Accumulator Module

Create rtl/atomik_delta_acc.v with:

1. **Module Interface**
   ```verilog
   module atomik_delta_acc #(
       parameter WIDTH = 64
   ) (
       input  wire             clk,
       input  wire             rst,
       input  wire             enable,      // Accumulate when high
       input  wire [WIDTH-1:0] delta_in,    // Delta to accumulate
       output reg  [WIDTH-1:0] accumulator, // Current accumulated value
       output wire             valid        // Accumulator has valid data
   );
   ```

2. **Behavior**
   - On reset: accumulator <= 0
   - On enable: accumulator <= accumulator ^ delta_in
   - valid goes high after first accumulation (or always high after reset)

3. **Design Requirements**
   - Fully synchronous (all state changes on posedge clk)
   - Parameterized width (default 64)
   - No combinational loops
   - Clean reset behavior

4. **Testbench**: Create sim/tb_delta_acc.v with test cases:
   - Reset behavior (accumulator should be 0)
   - Single accumulation
   - Multiple accumulations (verify XOR chain)
   - Self-inverse test: acc ^ delta ^ delta should equal original acc
   - Back-to-back accumulations
   - Enable gating (no change when enable=0)

5. **Verification Script**: Create sim/run_delta_acc.sh
   ```bash
   #!/bin/bash
   iverilog -o sim_delta_acc rtl/atomik_delta_acc.v sim/tb_delta_acc.v
   vvp sim_delta_acc
   ```

After implementation, run the testbench and verify all tests pass.

Output files:
- rtl/atomik_delta_acc.v
- sim/tb_delta_acc.v
- sim/run_delta_acc.sh

Update .github/atomik-status.yml to mark T3.2_delta_accumulator: complete
```

---

### T3.3: State Reconstructor Module
**Model**: Claude Sonnet 4.5
**Command**: `claude --model claude-sonnet-4-5`

```
You are implementing Phase 3, Task T3.3 of the ATOMiK project.

CONTEXT:
- Read specs/rtl_architecture.md for the design specification
- The state reconstructor is the READ PATH of ATOMiK
- Key insight: Current state = initial_state ^ accumulator (SINGLE XOR!)
- This is O(1), not O(N) like the software implementation
- This eliminates the 32% read penalty observed in Phase 2 benchmarks

TASK T3.3: Implement State Reconstructor Module

Create rtl/atomik_state_rec.v with:

1. **Module Interface**
   ```verilog
   module atomik_state_rec #(
       parameter WIDTH = 64
   ) (
       input  wire             clk,
       input  wire             rst,
       input  wire [WIDTH-1:0] initial_state,  // S₀
       input  wire [WIDTH-1:0] accumulator,    // Σδᵢ from delta_acc
       input  wire             read_enable,    // Trigger reconstruction
       output reg  [WIDTH-1:0] current_state,  // S₀ ⊕ Σδᵢ
       output reg              state_valid     // Output is valid
   );
   ```

2. **Behavior**
   - Combinational: current_state = initial_state ^ accumulator
   - Optional registered output for timing closure
   - state_valid indicates when output is stable

3. **Design Options** (implement the simpler one, note the other):
   - **Option A (Combinational)**: Pure combo logic, 0-cycle latency
   - **Option B (Registered)**: 1-cycle latency, better timing
   - Recommend Option B for timing closure margin

4. **Testbench**: Create sim/tb_state_rec.v with test cases:
   - Initial state with zero accumulator (should output initial_state)
   - Initial state with non-zero accumulator
   - Verify: initial ^ acc ^ acc = initial (round-trip)
   - Multiple different initial states
   - Timing verification (output stable within clock period)

5. **Verification Script**: Create sim/run_state_rec.sh

Output files:
- rtl/atomik_state_rec.v
- sim/tb_state_rec.v
- sim/run_state_rec.sh

Update .github/atomik-status.yml to mark T3.3_state_reconstructor: complete
```

---

### T3.4: Full Verilog Implementation
**Model**: Claude Sonnet 4.5
**Command**: `claude --model claude-sonnet-4-5`

```
You are implementing Phase 3, Task T3.4 of the ATOMiK project.

CONTEXT:
- T3.2 created atomik_delta_acc.v (write path)
- T3.3 created atomik_state_rec.v (read path)
- Now integrate them into atomik_core_v2.v

TASK T3.4: Create Integrated ATOMiK Core v2

Create rtl/atomik_core_v2.v that:

1. **Instantiates Sub-modules**
   - atomik_delta_acc (accumulator)
   - atomik_state_rec (reconstructor)

2. **Top-Level Interface**
   ```verilog
   module atomik_core_v2 #(
       parameter WIDTH = 64
   ) (
       input  wire             clk,
       input  wire             rst,
       
       // Configuration
       input  wire [WIDTH-1:0] initial_state,
       input  wire             load_initial,   // Load new initial state
       
       // Delta input (write path)
       input  wire [WIDTH-1:0] delta_in,
       input  wire             delta_valid,
       output wire             delta_ready,
       
       // State output (read path)  
       input  wire             state_read,
       output wire [WIDTH-1:0] state_out,
       output wire             state_valid,
       
       // Status
       output wire [31:0]      delta_count,    // Number of deltas accumulated
       output wire             busy
   );
   ```

3. **Operating Modes**
   - IDLE: Waiting for commands
   - ACCUMULATE: Processing delta input
   - READ: Outputting current state
   - RESET: Clearing accumulator

4. **Control Logic**
   - Simple FSM or combinational control
   - Handle simultaneous read/write (what's the policy?)
   - Delta counter for debugging/verification

5. **Integration with Existing RTL**
   - Review atomik_top.v for how to integrate
   - Maintain backward compatibility if possible

6. **Lint Check**: Run `verilator --lint-only rtl/atomik_core_v2.v`

Output files:
- rtl/atomik_core_v2.v

Update .github/atomik-status.yml to mark T3.4_verilog_implementation: complete
```

---

### T3.5: Simulation & Verification
**Model**: Claude Sonnet 4.5
**Command**: `claude --model claude-sonnet-4-5`

```
You are implementing Phase 3, Task T3.5 of the ATOMiK project.

CONTEXT:
- All RTL modules are implemented (T3.2-T3.4)
- Now we need comprehensive verification
- Tests must verify equivalence with Python reference model

TASK T3.5: Create Comprehensive Simulation Suite

1. **Top-Level Testbench**: Create sim/tb_atomik_core.v
   
   Test cases:
   - Basic accumulation sequence
   - State read after accumulations
   - Reset and re-accumulate
   - Interleaved read/write operations
   - Boundary values (all 0s, all 1s, alternating bits)
   - Self-inverse verification (delta ^ delta = 0)
   - Commutativity test (order independence)
   - Long sequence (100+ deltas)

2. **Python Reference Model**: Create sim/reference_model.py
   ```python
   # Generate test vectors that match delta_engine.py behavior
   # Output as Verilog $readmemh compatible files
   ```

3. **Test Vector Files**: Create sim/vectors/
   - test_basic.hex
   - test_boundary.hex
   - test_sequence.hex

4. **Verification Script**: Create sim/run_all_tests.sh
   ```bash
   #!/bin/bash
   set -e
   
   echo "Running delta accumulator tests..."
   ./run_delta_acc.sh
   
   echo "Running state reconstructor tests..."
   ./run_state_rec.sh
   
   echo "Running integrated core tests..."
   iverilog -o sim_core rtl/atomik_core_v2.v rtl/atomik_delta_acc.v \
            rtl/atomik_state_rec.v sim/tb_atomik_core.v
   vvp sim_core
   
   echo "All tests passed!"
   ```

5. **Waveform Generation**: Add $dumpfile/$dumpvars for GTKWave

6. **Results Documentation**: Create sim/SIMULATION_RESULTS.md
   - Test case descriptions
   - Pass/fail status
   - Waveform screenshots (placeholder paths)

Output files:
- sim/tb_atomik_core.v
- sim/reference_model.py
- sim/vectors/*.hex
- sim/run_all_tests.sh
- sim/SIMULATION_RESULTS.md

Update .github/atomik-status.yml to mark T3.5_simulation_verification: complete
```

---

### T3.6: Timing Closure Optimization
**Model**: Claude Sonnet 4.5
**Command**: `claude --model claude-sonnet-4-5`

```
You are implementing Phase 3, Task T3.6 of the ATOMiK project.

CONTEXT:
- RTL is verified in simulation
- Now prepare for synthesis with timing constraints
- Target: Gowin GW1NR-9, 50 MHz minimum

TASK T3.6: Create Timing Constraints and Optimize

1. **SDC Constraints File**: Create constraints/atomik_timing.sdc
   ```tcl
   # Clock definition
   create_clock -name clk_27m -period 37.037 [get_ports clkin]
   create_clock -name clk_81m -period 12.346 [get_pins rpll_inst/CLKOUT]
   
   # Input/output delays
   set_input_delay -clock clk_81m -max 2.0 [get_ports delta_in*]
   set_output_delay -clock clk_81m -max 2.0 [get_ports state_out*]
   
   # False paths (if any)
   # set_false_path -from [get_ports rst]
   ```

2. **Critical Path Analysis**
   - Identify potential critical paths (64-bit XOR should be fine)
   - Document expected timing margin
   - Note any paths that might need pipelining

3. **Gowin-Specific Constraints**: Create constraints/atomik_gowin.cst
   - Pin assignments (reference existing constraints/)
   - I/O standards
   - Clock routing preferences

4. **Timing Report Template**: Create reports/timing_analysis.md
   - Expected vs actual timing
   - Slack summary
   - Critical path description

Output files:
- constraints/atomik_timing.sdc
- constraints/atomik_gowin.cst (or update existing)
- reports/timing_analysis.md (template)

Update .github/atomik-status.yml to mark T3.6_timing_optimization: complete
```

---

### T3.7: FPGA Synthesis Scripts
**Model**: Claude Sonnet 4.5
**Command**: `claude --model claude-sonnet-4-5`

```
You are implementing Phase 3, Task T3.7 of the ATOMiK project.

CONTEXT:
- RTL verified, constraints ready
- Now create synthesis flow for Gowin FPGA
- Reference existing impl/ directory structure

TASK T3.7: Create FPGA Synthesis Flow

1. **Gowin Project File**: Update or create ATOMiK.gprj
   - Add new RTL files
   - Set device parameters
   - Configure synthesis options

2. **Synthesis Script**: Create synth/run_synthesis.sh
   ```bash
   #!/bin/bash
   # Run Gowin synthesis from command line
   # Requires Gowin EDA in PATH
   
   gw_sh synth/gowin_synth.tcl
   ```

3. **TCL Synthesis Script**: Create synth/gowin_synth.tcl
   ```tcl
   # Gowin synthesis TCL script
   set_device -device GW1NR-9C -package QN88 -speed 6
   
   # Add source files
   add_file rtl/atomik_core_v2.v
   add_file rtl/atomik_delta_acc.v
   add_file rtl/atomik_state_rec.v
   add_file rtl/pll/atomik_pll_81m.v
   
   # Add constraints
   add_file constraints/atomik_gowin.cst
   add_file constraints/atomik_timing.sdc
   
   # Run synthesis
   run_synthesis
   run_pnr
   
   # Generate reports
   report_timing -file reports/timing_report.txt
   report_area -file reports/area_report.txt
   ```

4. **Alternative: Yosys/nextpnr Flow**: Create synth/run_yosys.sh
   ```bash
   # Open-source synthesis alternative
   yosys -p "read_verilog rtl/*.v; synth_gowin -top atomik_top; write_json atomik.json"
   nextpnr-gowin --json atomik.json --write atomik.fs --device GW1NR-9C
   ```

5. **Build Documentation**: Create synth/README.md
   - Prerequisites
   - Build instructions
   - Troubleshooting

Output files:
- synth/run_synthesis.sh
- synth/gowin_synth.tcl
- synth/run_yosys.sh (alternative)
- synth/README.md

Update .github/atomik-status.yml to mark T3.7_fpga_synthesis: complete
```

---

### T3.8: Resource Utilization Analysis
**Model**: Claude Sonnet 4.5
**Command**: `claude --model claude-sonnet-4-5`

```
You are implementing Phase 3, Task T3.8 of the ATOMiK project.

CONTEXT:
- Synthesis complete (or use estimates if not yet run)
- Analyze resource usage and compare to budget

TASK T3.8: Resource Utilization Analysis

1. **Parse Synthesis Reports** (if available)
   - LUT usage
   - Register count
   - BRAM usage (should be 0 for this design)
   - DSP usage (should be 0)

2. **Create Analysis Report**: Create reports/resource_utilization.md

   Include:
   - **Summary Table**
     | Resource | Used | Available | Utilization |
     |----------|------|-----------|-------------|
     | LUT4     | ?    | 8640      | ?%          |
     | FF       | ?    | 6480      | ?%          |
     | BRAM     | 0    | 26        | 0%          |
   
   - **Module Breakdown**
     - atomik_delta_acc: estimated ~65 LUTs (64-bit XOR + register)
     - atomik_state_rec: estimated ~64 LUTs (64-bit XOR)
     - atomik_core_v2: estimated ~50 LUTs (control logic)
     - Total: ~180 LUTs (~2% of device)
   
   - **Comparison to Budget**
     - Target: ≤80% LUT utilization
     - Actual: ~2% (massive headroom)
   
   - **Scaling Analysis**
     - What if WIDTH=128? Estimate ~360 LUTs
     - What if WIDTH=256? Estimate ~720 LUTs
     - Parallel delta inputs? +N×WIDTH LUTs per input

3. **Power Estimation** (if available from tools)
   - Static power
   - Dynamic power at target frequency
   - Comparison to traditional state-centric design

Output files:
- reports/resource_utilization.md

Update .github/atomik-status.yml to mark T3.8_resource_analysis: complete
```

---

### T3.9: Hardware Validation Report
**Model**: Claude Sonnet 4.5
**Command**: `claude --model claude-sonnet-4-5`

```
You are implementing Phase 3, Task T3.9 of the ATOMiK project.

CONTEXT:
- All implementation and verification complete
- Create final validation report for Phase 3

TASK T3.9: Create Hardware Validation Report

Create reports/PHASE_3_COMPLETION_REPORT.md with:

1. **Executive Summary**
   - Phase 3 objectives achieved
   - Key metrics vs targets

2. **Implementation Summary**
   | Module | File | Lines | Function |
   |--------|------|-------|----------|
   | Delta Accumulator | atomik_delta_acc.v | ? | XOR accumulation |
   | State Reconstructor | atomik_state_rec.v | ? | State output |
   | Core v2 | atomik_core_v2.v | ? | Integration |

3. **Verification Results**
   - Simulation test results
   - Coverage metrics (if available)
   - Comparison to Python reference model

4. **Synthesis Results**
   - Timing closure: Pass/Fail
   - Resource utilization summary
   - Power estimates

5. **Performance Validation**
   - Accumulate latency: 1 cycle (target: 1) ✓
   - Reconstruct latency: 1 cycle (target: 1) ✓
   - Maximum frequency achieved vs target

6. **Key Achievement: Read Penalty Eliminated**
   - Phase 2 software: 32% slower for read-heavy
   - Phase 3 hardware: 0% penalty (single-cycle XOR)
   - This validates the hardware optimization thesis

7. **Validation Gates**
   | Gate | Threshold | Actual | Status |
   |------|-----------|--------|--------|
   | RTL simulation | 100% pass | ? | ? |
   | Timing closure | Slack ≥ 0 | ? | ? |
   | LUT utilization | ≤ 80% | ? | ? |

8. **Lessons Learned**
   - What worked well
   - Challenges encountered
   - Recommendations for Phase 4

9. **Next Steps**
   - Phase 4: SDK Development
   - Future hardware optimizations (parallel input, wider bus)

Also update:
- README.md: Phase 3 status to Complete
- .github/atomik-status.yml: Full Phase 3 completion

Output files:
- reports/PHASE_3_COMPLETION_REPORT.md
- Update README.md
- Update .github/atomik-status.yml

Mark T3.9_hardware_validation: complete
Mark phase_3.status: complete
```

---

## Validation Prompts (Haiku)

Use between tasks for quick validation:

### Lint Check
**Model**: Claude Haiku 4.5
**Command**: `claude --model claude-haiku-4-5`

```
Run Verilog lint on the RTL files:

verilator --lint-only -Wall rtl/atomik_delta_acc.v
verilator --lint-only -Wall rtl/atomik_state_rec.v  
verilator --lint-only -Wall rtl/atomik_core_v2.v

Report any warnings or errors. Suggest fixes if needed.
```

### Simulation Check
**Model**: Claude Haiku 4.5
**Command**: `claude --model claude-haiku-4-5`

```
Run the simulation test suite:

cd sim && ./run_all_tests.sh

Report results. If any tests fail, identify the failing test and 
the expected vs actual values.
```

---

## Escalation Prompt (Opus)

Use only if complex architectural issues arise:

**Model**: Claude Opus 4.5
**Command**: `claude --model claude-opus-4-5`

```
[Describe the specific architectural challenge]

Consider the proven mathematical properties from Phase 1:
- Commutativity: δ₁⊕δ₂ = δ₂⊕δ₁
- Associativity: (δ₁⊕δ₂)⊕δ₃ = δ₁⊕(δ₂⊕δ₃)
- Self-inverse: δ⊕δ = 0

And the Phase 2 benchmark results:
- 85% parallel efficiency potential
- 32% read penalty in software (must be eliminated in hardware)

Propose an architectural solution that:
1. Maintains single-cycle operation
2. Is synthesizable on Gowin GW1NR-9
3. Does not violate the proven properties
```

---

## Quick Reference

| Task | Model | Est. Time | Dependencies |
|------|-------|-----------|--------------|
| T3.1 | Sonnet | 30 min | None |
| T3.2 | Sonnet | 20 min | T3.1 |
| T3.3 | Sonnet | 20 min | T3.1 |
| T3.4 | Sonnet | 30 min | T3.2, T3.3 |
| T3.5 | Sonnet | 45 min | T3.4 |
| T3.6 | Sonnet | 20 min | T3.4 |
| T3.7 | Sonnet | 20 min | T3.6 |
| T3.8 | Sonnet | 15 min | T3.7 |
| T3.9 | Sonnet | 30 min | All above |

**Total estimated time**: ~4 hours of agent execution
**Total estimated cost**: ~$100 (under $150 budget)
