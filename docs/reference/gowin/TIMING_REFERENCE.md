# Gowin Timing Constraints Reference

Reference documentation for SDC timing constraints on GW1NR-9 FPGA.

**Source**: SUG940-1.3E (Gowin Design Timing Constraints User Guide), SUG113-1.1E (Gowin FPGA Design Guide)

---

## SDC File Overview

SDC (Synopsys Design Constraints) files define timing requirements for synthesis and place-and-route. Gowin EDA supports standard SDC syntax with some extensions.

### File Format

- Extension: `.sdc`
- Comments: `#` or `//` for single-line, `/* */` for multi-line
- Wildcards: `*` (multiple chars), `?` (single char)

---

## Clock Constraints

### create_clock (Primary Clock)

Defines a primary clock on a port or pin.

```tcl
create_clock -name clock_name -period period_ns [get_ports port_name]
```

**Parameters**:
- `-name`: Clock name for reference in other constraints
- `-period`: Clock period in nanoseconds
- `get_ports`: Target port (external pin)
- `get_pins`: Target pin (internal, e.g., PLL output)

**Examples**:

```tcl
# 27 MHz input clock (period = 1000/27 = 37.037 ns)
create_clock -name clk_27m -period 37.037 [get_ports sys_clk]

# 100 MHz clock
create_clock -name clk_100m -period 10.0 [get_ports fast_clk]

# Clock on internal PLL output
create_clock -name clk_pll -period 10.582 [get_pins u_pll/CLKOUT]
```

### create_generated_clock (Derived Clock)

Defines a clock derived from another clock (e.g., PLL output, divider).

```tcl
create_generated_clock -name clock_name \
    -source source_clock \
    -multiply_by M -divide_by D \
    [get_pins pin_name]
```

**Examples**:

```tcl
# PLL output: 27 MHz × 7 / 2 = 94.5 MHz
create_generated_clock -name clk_94m5 \
    -source [get_ports sys_clk] \
    -multiply_by 7 -divide_by 2 \
    [get_pins u_pll/CLKOUT]

# Divide-by-2 clock
create_generated_clock -name clk_div2 \
    -source [get_pins clk_reg/Q] \
    -divide_by 2 \
    [get_pins div2_reg/Q]
```

### set_clock_uncertainty

Specifies clock uncertainty (jitter + skew).

```tcl
set_clock_uncertainty -setup uncertainty_ns [get_clocks clock_name]
set_clock_uncertainty -hold uncertainty_ns [get_clocks clock_name]
```

**Example**:

```tcl
# 0.5 ns setup uncertainty, 0.2 ns hold uncertainty
set_clock_uncertainty -setup 0.5 [get_clocks clk_pll]
set_clock_uncertainty -hold 0.2 [get_clocks clk_pll]
```

### set_clock_groups

Defines clock domain relationships.

```tcl
# Asynchronous clocks (no timing paths between them)
set_clock_groups -asynchronous \
    -group [get_clocks clk_a] \
    -group [get_clocks clk_b]

# Exclusive clocks (never active simultaneously)
set_clock_groups -exclusive \
    -group [get_clocks clk_mode1] \
    -group [get_clocks clk_mode2]
```

---

## I/O Delay Constraints

### set_input_delay

Specifies input arrival time relative to clock edge.

```tcl
set_input_delay -clock clock_name -max max_delay [get_ports port_name]
set_input_delay -clock clock_name -min min_delay [get_ports port_name]
```

**Parameters**:
- `-clock`: Reference clock
- `-max`: Maximum delay (for setup analysis)
- `-min`: Minimum delay (for hold analysis)
- `-clock_fall`: Reference falling edge (optional)

**Examples**:

```tcl
# Input data with 2ns max delay, 0.5ns min delay
set_input_delay -clock clk_pll -max 2.0 [get_ports data_in[*]]
set_input_delay -clock clk_pll -min 0.5 [get_ports data_in[*]]

# UART input with relaxed timing
set_input_delay -clock clk_pll -max 5.0 [get_ports uart_rx]
```

### set_output_delay

Specifies output required time relative to clock edge.

```tcl
set_output_delay -clock clock_name -max max_delay [get_ports port_name]
set_output_delay -clock clock_name -min min_delay [get_ports port_name]
```

**Examples**:

```tcl
# Output data with 2ns max delay requirement
set_output_delay -clock clk_pll -max 2.0 [get_ports data_out[*]]
set_output_delay -clock clk_pll -min 0.5 [get_ports data_out[*]]

# LED outputs (relaxed timing)
set_output_delay -clock clk_pll -max 10.0 [get_ports led[*]]
```

---

## Path Constraints

### set_false_path

Removes timing requirement from specific paths.

```tcl
# From specific port
set_false_path -from [get_ports port_name]

# Between clock domains
set_false_path -from [get_clocks clk_a] -to [get_clocks clk_b]

# To specific registers
set_false_path -to [get_pins reg_name/D]
```

**Common Uses**:

```tcl
# Asynchronous reset - no timing requirement
set_false_path -from [get_ports sys_rst_n]

# Static configuration signals
set_false_path -from [get_ports config_*]

# Cross-domain paths (handled by synchronizers)
set_false_path -from [get_clocks clk_slow] -to [get_clocks clk_fast]
```

### set_max_delay / set_min_delay

Specifies explicit path delay constraints.

```tcl
set_max_delay delay_ns -from source -to destination
set_min_delay delay_ns -from source -to destination
```

**Examples**:

```tcl
# Maximum 5ns for combinational path
set_max_delay 5.0 -from [get_pins combo_in/*] -to [get_pins combo_out/*]

# Minimum 1ns for hold fix
set_min_delay 1.0 -from [get_pins fast_reg/Q] -to [get_pins slow_reg/D]
```

### set_multicycle_path

Allows paths to take multiple clock cycles.

```tcl
set_multicycle_path num_cycles -setup -from source -to destination
set_multicycle_path num_cycles -hold -from source -to destination
```

**Important**: Hold multiplier is typically (setup_cycles - 1).

**Examples**:

```tcl
# 2-cycle path (data valid every other cycle)
set_multicycle_path 2 -setup -from [get_pins slow_data/*] -to [get_pins dest_reg/*]
set_multicycle_path 1 -hold -from [get_pins slow_data/*] -to [get_pins dest_reg/*]

# 3-cycle path
set_multicycle_path 3 -setup -from [get_clocks clk_div4] -to [get_clocks clk_main]
set_multicycle_path 2 -hold -from [get_clocks clk_div4] -to [get_clocks clk_main]
```

---

## Complete SDC Example for ATOMiK

```tcl
#=============================================================================
# ATOMiK Timing Constraints
# Target: Tang Nano 9K (GW1NR-LV9QN88PC6/I5)
# PLL Output: 94.5 MHz
#=============================================================================

#-----------------------------------------------------------------------------
# Clock Definitions
#-----------------------------------------------------------------------------

# Primary input clock (27 MHz crystal)
create_clock -name clk_27m -period 37.037 [get_ports sys_clk]

# PLL output clock (94.5 MHz)
# Period = 1000 / 94.5 = 10.582 ns
create_generated_clock -name clk_pll \
    -source [get_ports sys_clk] \
    -multiply_by 7 -divide_by 2 \
    [get_pins gen_pll.u_pll/CLKOUT]

#-----------------------------------------------------------------------------
# Clock Uncertainty
#-----------------------------------------------------------------------------

# PLL jitter + routing skew
set_clock_uncertainty -setup 0.3 [get_clocks clk_pll]
set_clock_uncertainty -hold 0.1 [get_clocks clk_pll]

#-----------------------------------------------------------------------------
# Input Delays
#-----------------------------------------------------------------------------

# UART RX (asynchronous, but constrain for metastability)
set_input_delay -clock clk_pll -max 5.0 [get_ports uart_rx]
set_input_delay -clock clk_pll -min 0.0 [get_ports uart_rx]

#-----------------------------------------------------------------------------
# Output Delays
#-----------------------------------------------------------------------------

# UART TX
set_output_delay -clock clk_pll -max 2.0 [get_ports uart_tx]
set_output_delay -clock clk_pll -min 0.0 [get_ports uart_tx]

# LEDs (relaxed - visual output only)
set_output_delay -clock clk_pll -max 10.0 [get_ports led[*]]

#-----------------------------------------------------------------------------
# False Paths
#-----------------------------------------------------------------------------

# Asynchronous reset
set_false_path -from [get_ports sys_rst_n]

# PLL lock signal (quasi-static)
set_false_path -from [get_pins gen_pll.u_pll/LOCK]

#-----------------------------------------------------------------------------
# Design-Specific Constraints
#-----------------------------------------------------------------------------

# ATOMiK core: All paths should close in single cycle
# No multicycle paths needed for basic delta accumulator

#=============================================================================
# End of SDC
#=============================================================================
```

---

## Timing Closure Strategies

From SUG113-1.1E (Gowin FPGA Design Guide):

### 1. Complete Clock Constraints

- **Always define primary clocks** - Default is 200 MHz if unspecified
- **Define all generated clocks** - PLL outputs, clock dividers
- **Specify clock uncertainty** - Account for jitter and skew

### 2. Reduce High-Fanout Signals

```verilog
// Use syn_maxfan attribute to limit fanout
(* syn_maxfan = 32 *) reg high_fanout_signal;
```

### 3. Use Global Resources

- **GSR (Global Set/Reset)**: For high-fanout reset signals
- **GCLK (Global Clock)**: For clock enable signals
- **HCLK (High-speed Clock)**: For timing-critical paths

### 4. Register I/O Paths

```verilog
// Input register (improves setup time)
always @(posedge clk) begin
    input_reg <= pad_input;
    // Use input_reg instead of pad_input
end

// Output register (improves clock-to-output)
always @(posedge clk) begin
    pad_output <= output_data;
end
```

### 5. Pipeline Long Paths

```verilog
// Before: Long combinational path
assign result = complex_function(a, b, c, d);

// After: Pipelined
always @(posedge clk) begin
    stage1 <= partial_function(a, b);
    stage2 <= combine_function(stage1, c);
    result <= final_function(stage2, d);
end
```

### 6. Synthesis Report Analysis

**Key metrics to check**:
- **Worst Negative Slack (WNS)**: Must be ≥ 0 for timing closure
- **Total Negative Slack (TNS)**: Sum of all negative slacks
- **Critical Path**: Longest delay path

**Note**: Synthesis timing is optimistic. Actual results after P&R are typically 1/3 to 1/2 worse.

---

## Common Timing Issues

### Setup Violation

**Symptom**: WNS < 0 (path too slow)

**Solutions**:
1. Reduce clock frequency
2. Add pipeline registers
3. Optimize combinational logic
4. Use faster device speed grade

### Hold Violation

**Symptom**: Hold slack < 0 (path too fast)

**Solutions**:
1. Add delay cells (DLLDLY primitive)
2. Use min_delay constraints
3. Check for clock skew issues

### Clock Domain Crossing

**Symptom**: Metastability, data corruption

**Solutions**:
1. Use synchronizer registers (2-3 flip-flops)
2. Use `set_false_path` between domains
3. Use proper handshaking protocols

---

## Timing Report Interpretation

### Key Sections

1. **Clock Summary**: All defined clocks and periods
2. **Path Groups**: Timing paths grouped by clock
3. **Critical Path**: Detailed breakdown of worst path
4. **Slack Summary**: WNS/TNS for each path group

### Example Critical Path Analysis

```
Path 1: MET Setup Check with Pin core_inst/data_out_reg[0]/D
Endpoint:   core_inst/data_out_reg[0]/D (^) checked with leading edge of 'clk_pll'
Beginpoint: core_inst/acc_reg[63]/Q (^) triggered by leading edge of 'clk_pll'

Path Groups: {clk_pll}
Data Delay: 8.234ns
Clock Skew: 0.150ns
Slack:      +2.198ns (MET)

Point                          Incr    Path
-----------------------------------------------------------
clock clk_pll (rise edge)      0.000   0.000
core_inst/acc_reg[63]/Q        0.523   0.523 (clock-to-Q)
xor_gate_1                     1.245   1.768
xor_gate_2                     1.312   3.080
... (more gates) ...
core_inst/data_out_reg[0]/D    0.234   8.234 (data arrival)

clock clk_pll (rise edge)      10.582  10.582
core_inst/data_out_reg[0]/CK   0.150   10.732 (clock arrival)
library setup time            -0.300   10.432
-----------------------------------------------------------
data required time                     10.432
data arrival time                      -8.234
-----------------------------------------------------------
slack (MET)                            +2.198
```

---

## Related Files

| File | Description |
|------|-------------|
| `hardware/constraints/atomik_timing.sdc` | Project timing constraints |
| `hardware/constraints/atomik_constraints.cst` | Physical constraints |
| [GPIO_REFERENCE.md](GPIO_REFERENCE.md) | I/O configuration |
| [CLOCK_REFERENCE.md](CLOCK_REFERENCE.md) | PLL configuration |

---

*Reference: SUG940-1.3E, SUG113-1.1E*
