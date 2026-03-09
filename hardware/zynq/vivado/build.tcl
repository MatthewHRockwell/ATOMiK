# ==============================================================================
# ATOMiK Zynq Build Script (Non-Project Mode)
#
# Target:  ALINX AX7020 (XC7Z020-2CLG400I)
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

# ATOMiK core RTL (shared with v3 — Phase 1: single bank only)
read_verilog $V3_RTL_DIR/atomik_v3_atomik.v

# Zynq-specific wrapper, clock module, and top-level
read_verilog $ZYNQ_DIR/rtl/atomik_axi4lite_wrapper.v
read_verilog $ZYNQ_DIR/rtl/atomik_zynq_clk.v
read_verilog $ZYNQ_DIR/rtl/atomik_zynq_top.v

# Constraints
read_xdc $ZYNQ_DIR/constraints/ax7020.xdc

# ------------------------------------------------------------------------------
# Synthesis
# ------------------------------------------------------------------------------

puts ""
puts "Running synthesis..."
puts "----------------------------------------------"

synth_design -top atomik_zynq_top -part xc7z020clg400-2

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
