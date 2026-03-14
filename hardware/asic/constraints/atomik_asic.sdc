# ==============================================================================
# ATOMiK ASIC Timing Constraints
#
# Foundry-agnostic SDC compatible with:
#   - Synopsys Design Compiler (DC)
#   - Cadence Genus / Innovus
#   - OpenSTA (OpenLane / OpenROAD)
#   - Xilinx Vivado (for verification)
#
# Target: ATOMiK IP core (single clock domain)
# Parameterize CLK_PERIOD for your target frequency:
#   200 MHz → 5.0 ns
#   400 MHz → 2.5 ns
#   500 MHz → 2.0 ns
#
# ATOMiK Project — March 2026
# ==============================================================================

# ==============================================================================
# Clock Definition
# ==============================================================================

# Core clock — parameterize for your target frequency
# Default: 200 MHz (5.0 ns period) — conservative for initial tapeout
set CLK_PERIOD 5.0
set CLK_NAME   "clk_core"

create_clock -name $CLK_NAME -period $CLK_PERIOD [get_ports clk]

# Clock uncertainty — accounts for jitter, skew, and OCV
# Adjust based on foundry PVT characterization
set_clock_uncertainty -setup 0.20 [get_clocks $CLK_NAME]
set_clock_uncertainty -hold  0.05 [get_clocks $CLK_NAME]

# Clock transition — maximum rise/fall time on clock network
set_clock_transition 0.10 [get_clocks $CLK_NAME]

# ==============================================================================
# JTAG Clock (separate domain, lower frequency)
# ==============================================================================

create_clock -name "tck" -period 100.0 [get_ports tck]

# JTAG is asynchronous to core clock
set_clock_groups -asynchronous \
    -group [get_clocks $CLK_NAME] \
    -group [get_clocks tck]

# ==============================================================================
# Reset
# ==============================================================================

# Reset is asynchronous — constrain as false path for timing
# (reset synchronizer inside the design handles metastability)
set_false_path -from [get_ports rst_n]
set_false_path -from [get_ports trst_n]

# ==============================================================================
# Input Delays
# ==============================================================================

# Operation interface — assume signals arrive within 40% of clock period
set INPUT_DELAY [expr {$CLK_PERIOD * 0.40}]

set_input_delay -clock $CLK_NAME -max $INPUT_DELAY [get_ports op_valid]
set_input_delay -clock $CLK_NAME -max $INPUT_DELAY [get_ports {op_code[*]}]
set_input_delay -clock $CLK_NAME -max $INPUT_DELAY [get_ports {op_addr[*]}]
set_input_delay -clock $CLK_NAME -max $INPUT_DELAY [get_ports {op_data[*]}]

# BIST start
set_input_delay -clock $CLK_NAME -max $INPUT_DELAY [get_ports bist_start]

# Scan interface
set_input_delay -clock $CLK_NAME -max $INPUT_DELAY [get_ports scan_enable]
set_input_delay -clock $CLK_NAME -max $INPUT_DELAY [get_ports scan_in]

# Hold input delay (min)
set INPUT_DELAY_MIN [expr {$CLK_PERIOD * 0.05}]

set_input_delay -clock $CLK_NAME -min $INPUT_DELAY_MIN [get_ports op_valid]
set_input_delay -clock $CLK_NAME -min $INPUT_DELAY_MIN [get_ports {op_code[*]}]
set_input_delay -clock $CLK_NAME -min $INPUT_DELAY_MIN [get_ports {op_addr[*]}]
set_input_delay -clock $CLK_NAME -min $INPUT_DELAY_MIN [get_ports {op_data[*]}]
set_input_delay -clock $CLK_NAME -min $INPUT_DELAY_MIN [get_ports bist_start]
set_input_delay -clock $CLK_NAME -min $INPUT_DELAY_MIN [get_ports scan_enable]
set_input_delay -clock $CLK_NAME -min $INPUT_DELAY_MIN [get_ports scan_in]

# ==============================================================================
# Output Delays
# ==============================================================================

# Results — assume capturing flip-flop needs 40% of clock period
set OUTPUT_DELAY [expr {$CLK_PERIOD * 0.40}]

set_output_delay -clock $CLK_NAME -max $OUTPUT_DELAY [get_ports result_valid]
set_output_delay -clock $CLK_NAME -max $OUTPUT_DELAY [get_ports {result_data[*]}]
set_output_delay -clock $CLK_NAME -max $OUTPUT_DELAY [get_ports op_ready]

# BIST outputs
set_output_delay -clock $CLK_NAME -max $OUTPUT_DELAY [get_ports bist_done]
set_output_delay -clock $CLK_NAME -max $OUTPUT_DELAY [get_ports bist_pass]
set_output_delay -clock $CLK_NAME -max $OUTPUT_DELAY [get_ports {bist_count[*]}]

# Scan output
set_output_delay -clock $CLK_NAME -max $OUTPUT_DELAY [get_ports scan_out]

# Hold output delay (min)
set OUTPUT_DELAY_MIN [expr {$CLK_PERIOD * 0.05}]

set_output_delay -clock $CLK_NAME -min $OUTPUT_DELAY_MIN [get_ports result_valid]
set_output_delay -clock $CLK_NAME -min $OUTPUT_DELAY_MIN [get_ports {result_data[*]}]
set_output_delay -clock $CLK_NAME -min $OUTPUT_DELAY_MIN [get_ports op_ready]
set_output_delay -clock $CLK_NAME -min $OUTPUT_DELAY_MIN [get_ports bist_done]
set_output_delay -clock $CLK_NAME -min $OUTPUT_DELAY_MIN [get_ports bist_pass]
set_output_delay -clock $CLK_NAME -min $OUTPUT_DELAY_MIN [get_ports {bist_count[*]}]
set_output_delay -clock $CLK_NAME -min $OUTPUT_DELAY_MIN [get_ports scan_out]

# ==============================================================================
# Design Rules
# ==============================================================================

# Maximum transition time — prevents excessive slew on long nets
set_max_transition 0.30 [current_design]

# Maximum fanout — prevents excessive loading
set_max_fanout 20 [current_design]

# Maximum capacitance — foundry-specific, adjust per library
# set_max_capacitance 0.5 [current_design]

# ==============================================================================
# DFT Mode Constraints
# ==============================================================================

# In scan mode, the shift clock may run slower
# Uncomment and adjust for your DFT methodology:
# set_case_analysis 1 [get_ports scan_enable]
# create_clock -name "scan_clk" -period 10.0 [get_ports clk]

# ==============================================================================
# SRAM Timing
# ==============================================================================

# If using foundry SRAM macros, add their timing constraints here:
# set_dont_touch [get_cells u_core/u_state_table]
# set_timing_derate -early 0.95 [get_cells u_core/u_state_table]
# set_timing_derate -late  1.05 [get_cells u_core/u_state_table]

# ==============================================================================
# Multi-Cycle Paths (if applicable)
# ==============================================================================

# ATOMiK READ/SWAP with READ_LATENCY=2 takes 3 cycles.
# The pipeline handles this internally — no external multi-cycle needed.
# If integrating with external logic that samples result_data:
# set_multicycle_path 3 -setup -from [get_pins u_core/op_valid_reg/Q] \
#                                -to [get_pins u_core/result_data_reg*/D]

# ==============================================================================
# Area Optimization Hints
# ==============================================================================

# Allow register merging for low-fanout duplicates
# set_register_merging [current_design]

# Compile ultra settings (Synopsys DC):
# compile_ultra -retime -area_high_effort_script
