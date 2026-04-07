# ==============================================================================
# ATOMiK Zynq Build Script (Non-Project Mode)
#
# Target:  HamGeek RK-ZYNQ7020-F (XC7Z020-2CLG484I)
# NOTE:    PS_CLK = 32.214 MHz (not 33.333 MHz) — affects all PLL frequencies
#          IO PLL = 32.214 × 30 = 966 MHz, FCLK = 966/5/2 ≈ 96.6 MHz
# Purpose: PL-only synthesis for ATOMiK AXI4-Lite wrapper.
#          Use block_design.tcl for full PS+PL builds.
#
# Usage:
#   vivado -mode batch -source vivado/build.tcl
#
# Run from hardware/zynq/ directory.
#
# Date: March 2026
# ==============================================================================

# ------------------------------------------------------------------------------
# Path Setup
# ------------------------------------------------------------------------------
set SCRIPT_DIR [file dirname [info script]]
set ZYNQ_DIR   [file normalize "$SCRIPT_DIR/.."]
set V3_RTL_DIR [file normalize "$ZYNQ_DIR/../v3/rtl"]

puts "=============================================="
puts " ATOMiK Zynq Build — Non-Project Mode"
puts "=============================================="
puts ""
puts "Zynq Dir:     $ZYNQ_DIR"
puts "V3 RTL Dir:   $V3_RTL_DIR"
puts ""

# Use multiple threads if available
set_param general.maxThreads 4

# Create output directories
file mkdir $ZYNQ_DIR/reports
file mkdir $ZYNQ_DIR/output

# ------------------------------------------------------------------------------
# Read Sources
# ------------------------------------------------------------------------------

# ATOMiK core RTL — Zynq-optimized (BRAM + pipelined SWAP)
read_verilog $ZYNQ_DIR/rtl/atomik_core_zynq.v

# Zynq-specific wrapper, CDC bridge, clock module, and top-level
read_verilog $ZYNQ_DIR/rtl/atomik_axi4lite_wrapper.v
read_verilog $ZYNQ_DIR/rtl/atomik_cdc_bridge.v
read_verilog $ZYNQ_DIR/rtl/atomik_zynq_clk.v
read_verilog $ZYNQ_DIR/rtl/atomik_zynq_top.v

# Constraints
# Use board-specific constraints (rk7020f for HamGeek, ax7020 for ALINX reference)
if {[file exists $ZYNQ_DIR/constraints/rk7020f.xdc]} {
    read_xdc $ZYNQ_DIR/constraints/rk7020f.xdc
} else {
    read_xdc $ZYNQ_DIR/constraints/ax7020.xdc
}

# ------------------------------------------------------------------------------
# Synthesis
# ------------------------------------------------------------------------------

puts ""
puts "Running synthesis..."
puts "----------------------------------------------"

auto_detect_xpm
synth_design -top atomik_zynq_top -part xc7z020clg484-2

# Post-synthesis reports
report_utilization -file $ZYNQ_DIR/reports/post_synth_util.rpt
report_timing_summary -file $ZYNQ_DIR/reports/post_synth_timing.rpt

# Save synthesis checkpoint
write_checkpoint -force $ZYNQ_DIR/output/post_synth.dcp

# ------------------------------------------------------------------------------
# Implementation
# ------------------------------------------------------------------------------

puts ""
puts "Running implementation..."
puts "----------------------------------------------"

opt_design
place_design
route_design

# Post-implementation reports
report_timing_summary -file $ZYNQ_DIR/reports/timing_summary.rpt
report_utilization -file $ZYNQ_DIR/reports/post_impl_util.rpt
report_power -file $ZYNQ_DIR/reports/power.rpt
report_clock_utilization -file $ZYNQ_DIR/reports/clock_util.rpt
report_methodology -file $ZYNQ_DIR/reports/methodology.rpt

# Save implementation checkpoint
write_checkpoint -force $ZYNQ_DIR/output/post_impl.dcp

# ------------------------------------------------------------------------------
# Bitstream — skipped for PL-only builds
# ------------------------------------------------------------------------------
# Bitstream generation requires a full PS+PL block design with the Zynq PS IP
# instantiated (provides FCLK, AXI ports, and MIO pin assignments).
# Use block_design.tcl for bitstream-capable builds once hardware is available.
#
# To override (e.g., for testing with dummy I/O constraints):
#   vivado -mode batch -source vivado/build.tcl -tclargs --bitstream

set gen_bitstream 0
if {[info exists argc] && $argc > 0} {
    foreach arg $argv {
        if {$arg eq "--bitstream"} { set gen_bitstream 1 }
    }
}

if {$gen_bitstream} {
    puts ""
    puts "Generating bitstream (--bitstream flag set)..."
    write_bitstream -force $ZYNQ_DIR/output/atomik_zynq.bit
}

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------

puts ""
puts "=============================================="
puts " Build complete — PL synthesis + implementation"
puts "  Checkpoint:    output/post_impl.dcp"
puts "  Timing report: reports/timing_summary.rpt"
puts "  Utilization:   reports/post_impl_util.rpt"
puts "=============================================="
puts ""

# Print utilization summary to console
report_utilization -hierarchical -hierarchical_depth 1
