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
# Version: 2.5 (Fixed file list to match existing RTL)
# Date:    January 25, 2026
# =============================================================================

# -----------------------------------------------------------------------------
# Project Configuration
# -----------------------------------------------------------------------------
set PROJECT_NAME "ATOMiK"

# Get script directory for relative paths
set SCRIPT_DIR [file dirname [info script]]
set PROJECT_ROOT [file normalize "$SCRIPT_DIR/.."]
set PROJECT_FILE "$PROJECT_ROOT/ATOMiK.gprj"

# -----------------------------------------------------------------------------
# Open Project
# -----------------------------------------------------------------------------
puts "=============================================="
puts " ATOMiK Synthesis - Gowin EDA"
puts "=============================================="
puts ""
puts "Project Root: $PROJECT_ROOT"
puts "Project File: $PROJECT_FILE"
puts ""

# Check if project file exists
if {![file exists $PROJECT_FILE]} {
    puts "ERROR: Project file not found: $PROJECT_FILE"
    exit 1
}

# Open existing project
puts "Opening project..."
open_project $PROJECT_FILE

# -----------------------------------------------------------------------------
# Set Options
# -----------------------------------------------------------------------------
puts ""
puts "Configuring synthesis options..."

set_option -top_module atomik_top
set_option -verilog_std v2001
set_option -use_sspi_as_gpio 1
set_option -use_mspi_as_gpio 1
set_option -timing_driven 1
set_option -print_all_synthesis_warning 1

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
puts " Synthesis Complete"
puts "=============================================="
puts ""
puts "Output files:"
puts "  Bitstream: $IMPL_DIR/pnr/$PROJECT_NAME.fs"
puts "  Reports:   $IMPL_DIR/pnr/${PROJECT_NAME}_tr.html (timing)"
puts "             $IMPL_DIR/pnr/${PROJECT_NAME}_pr.html (P&R)"
puts ""
puts "To program the FPGA:"
puts "  openFPGALoader -b tangnano9k $IMPL_DIR/pnr/$PROJECT_NAME.fs"
puts ""
