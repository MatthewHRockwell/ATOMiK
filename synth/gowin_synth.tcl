# =============================================================================
# ATOMiK Gowin EDA Synthesis Script (TCL)
#
# Target: GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)
# Tool:   Gowin EDA V1.9.11.03 Education
#
# Usage:
#   Option 1: Open in Gowin EDA GUI and run
#   Option 2: gw_sh gowin_synth.tcl
#
# Version: 2.0 (Updated for ATOMiK Core v2 delta architecture)
# Date:    January 25, 2026
# =============================================================================

# -----------------------------------------------------------------------------
# Project Configuration
# -----------------------------------------------------------------------------
set PROJECT_NAME "ATOMiK"
set DEVICE "GW1NR-LV9QN88PC6/I5"
set FAMILY "GW1NR-9C"

# Get script directory for relative paths
set SCRIPT_DIR [file dirname [info script]]
set PROJECT_ROOT [file normalize "$SCRIPT_DIR/.."]

# -----------------------------------------------------------------------------
# Create Project (or open existing)
# -----------------------------------------------------------------------------
puts "=============================================="
puts " ATOMiK Synthesis - Gowin EDA"
puts "=============================================="
puts ""
puts "Project Root: $PROJECT_ROOT"
puts "Device:       $DEVICE"
puts ""

# Create new project
create_project -name $PROJECT_NAME -dir "$PROJECT_ROOT/impl" -device $DEVICE

# -----------------------------------------------------------------------------
# Add RTL Source Files
# -----------------------------------------------------------------------------
puts "Adding RTL source files..."

# PLL modules
add_file -type verilog "$PROJECT_ROOT/rtl/pll/atomik_pll_94p5m.v"
# add_file -type verilog "$PROJECT_ROOT/rtl/pll/atomik_pll_81m.v"   # Alternative

# Core v2 Delta Architecture (NEW)
add_file -type verilog "$PROJECT_ROOT/rtl/atomik_delta_acc.v"
add_file -type verilog "$PROJECT_ROOT/rtl/atomik_state_rec.v"
add_file -type verilog "$PROJECT_ROOT/rtl/atomik_core_v2.v"

# Legacy modules (for compatibility)
add_file -type verilog "$PROJECT_ROOT/rtl/atomik_core.v"
add_file -type verilog "$PROJECT_ROOT/rtl/atomik_bios.v"
add_file -type verilog "$PROJECT_ROOT/rtl/atomik_uart_rx.v"
add_file -type verilog "$PROJECT_ROOT/rtl/atomik_uart_tx.v"

# Top-level
add_file -type verilog "$PROJECT_ROOT/rtl/atomik_top.v"

# -----------------------------------------------------------------------------
# Add Constraint Files
# -----------------------------------------------------------------------------
puts "Adding constraint files..."

# Physical constraints (pin assignments)
add_file -type cst "$PROJECT_ROOT/constraints/atomik_constraints.cst"

# Timing constraints
add_file -type sdc "$PROJECT_ROOT/constraints/timing_constraints.sdc"

# -----------------------------------------------------------------------------
# Synthesis Options
# -----------------------------------------------------------------------------
puts "Configuring synthesis options..."

# General options
set_option -top_module atomik_top
set_option -verilog_std v2001

# Optimization settings
set_option -use_sspi_as_gpio 1
set_option -use_mspi_as_gpio 1
set_option -use_done_as_gpio 0
set_option -use_ready_as_gpio 0
set_option -use_reconfign_as_gpio 0
set_option -use_i2c_as_gpio 0

# Resource usage
set_option -use_dsp 0              # No DSP blocks needed (XOR is LUT-based)
set_option -use_bsram 0            # No block RAM needed for core v2

# Timing options
set_option -timing_driven 1        # Enable timing-driven synthesis
set_option -frequency 95           # Target 95 MHz (slightly above 94.5 MHz PLL)

# FSM encoding
set_option -fsm_extract 1
set_option -fsm_encoding auto

# Optimization level
set_option -optimization_level 2   # High optimization

# -----------------------------------------------------------------------------
# Place & Route Options
# -----------------------------------------------------------------------------
puts "Configuring place & route options..."

set_option -place_option 1         # Timing-driven placement
set_option -route_option 1         # Timing-driven routing
set_option -ireg_in_iob 0          # Don't pack input registers into IOB
set_option -oreg_in_iob 0          # Don't pack output registers into IOB

# -----------------------------------------------------------------------------
# Run Synthesis
# -----------------------------------------------------------------------------
puts ""
puts "Running synthesis..."
puts "----------------------------------------------"

run syn

# Check synthesis result
if {[catch {get_property -name "syn_result"} result]} {
    puts "ERROR: Synthesis failed!"
    exit 1
}

puts "Synthesis complete."

# -----------------------------------------------------------------------------
# Run Place & Route
# -----------------------------------------------------------------------------
puts ""
puts "Running place & route..."
puts "----------------------------------------------"

run pnr

puts "Place & route complete."

# -----------------------------------------------------------------------------
# Generate Reports
# -----------------------------------------------------------------------------
puts ""
puts "Generating reports..."

# Create reports directory
file mkdir "$PROJECT_ROOT/reports"

# Timing report
# report_timing -file "$PROJECT_ROOT/reports/timing_report.txt"

# Resource utilization
# report_area -file "$PROJECT_ROOT/reports/resource_report.txt"

# -----------------------------------------------------------------------------
# Generate Bitstream
# -----------------------------------------------------------------------------
puts ""
puts "Generating bitstream..."

run all

puts ""
puts "=============================================="
puts " Synthesis Complete"
puts "=============================================="
puts ""
puts "Output files:"
puts "  Bitstream: $PROJECT_ROOT/impl/pnr/$PROJECT_NAME.fs"
puts "  Netlist:   $PROJECT_ROOT/impl/gwsynthesis/$PROJECT_NAME.vg"
puts ""
puts "To program the FPGA:"
puts "  1. Connect Tang Nano 9K via USB"
puts "  2. Open Gowin Programmer"
puts "  3. Load $PROJECT_NAME.fs"
puts "  4. Click 'Program/Configure'"
puts ""
