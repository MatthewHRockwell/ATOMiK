# =============================================================================
# ATOMiK Phase 6: Parallel Banks Synthesis Script (TCL)
#
# Target: GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)
# Tool:   Gowin EDA V1.9.11.03 Education
#
# Usage:
#   gw_sh gowin_synth_parallel.tcl
#
# This synthesizes atomik_top_parallel with N_BANKS=4 parallel XOR
# accumulator banks and XOR merge tree.
#
# Version: 6.0
# Date:    January 27, 2026
# =============================================================================

# -----------------------------------------------------------------------------
# Project Configuration
# -----------------------------------------------------------------------------
set PROJECT_NAME "ATOMiK"

set SCRIPT_DIR [file dirname [info script]]
set PROJECT_ROOT [file normalize "$SCRIPT_DIR/../.."]
set PROJECT_FILE "$PROJECT_ROOT/hardware/ATOMiK_parallel.gprj"

# -----------------------------------------------------------------------------
# Open Project
# -----------------------------------------------------------------------------
puts "=============================================="
puts " ATOMiK Phase 6: Parallel Banks Synthesis"
puts "=============================================="
puts ""
puts "Project Root: $PROJECT_ROOT"
puts "Project File: $PROJECT_FILE"
puts ""

if {![file exists $PROJECT_FILE]} {
    puts "ERROR: Project file not found: $PROJECT_FILE"
    exit 1
}

puts "Opening project..."
open_project $PROJECT_FILE

# -----------------------------------------------------------------------------
# Set Options
# -----------------------------------------------------------------------------
puts ""
puts "Configuring synthesis options..."

set_option -top_module atomik_top_parallel
set_option -verilog_std v2001
set_option -use_sspi_as_gpio 1
set_option -use_mspi_as_gpio 1
set_option -timing_driven 1
set_option -print_all_synthesis_warning 1

# ALU extraction control:
# Gowin maps wide XOR to carry-chain ALU mode even when carry propagation
# is unnecessary.  RTL syn_keep attributes on merge tree and state
# reconstruction wires prevent this for the XOR paths.
# Uncomment below to globally disable ALU inference (affects counters too):
# set_option -use_alu 0

# -----------------------------------------------------------------------------
# Run Synthesis
# -----------------------------------------------------------------------------
puts ""
puts "Running synthesis..."
puts "----------------------------------------------"

run syn

puts ""
puts "Synthesis complete."

# -----------------------------------------------------------------------------
# Run Place & Route
# -----------------------------------------------------------------------------
puts ""
puts "Running place & route..."
puts "----------------------------------------------"

run pnr

puts ""
puts "Place & route complete."

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
set IMPL_DIR "$PROJECT_ROOT/impl"

puts ""
puts "=============================================="
puts " Phase 6 Parallel Banks - Synthesis Complete"
puts "=============================================="
puts ""
puts "Output files:"
puts "  Bitstream: $IMPL_DIR/pnr/$PROJECT_NAME.fs"
puts "  Reports:   $IMPL_DIR/pnr/${PROJECT_NAME}.rpt.txt (resource)"
puts "             $IMPL_DIR/pnr/${PROJECT_NAME}.tr.html  (timing)"
puts ""
puts "To program the FPGA:"
puts "  openFPGALoader -b tangnano9k $IMPL_DIR/pnr/$PROJECT_NAME.fs"
puts ""
