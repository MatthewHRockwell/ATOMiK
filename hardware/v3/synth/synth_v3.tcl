# =============================================================================
# ATOMiK v3 Gowin EDA Synthesis Script (TCL)
#
# Target: GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)
# Tool:   Gowin EDA V1.9.12.01
#
# Usage:
#   gw_sh synth_v3.tcl
#
# NOTE: Requires Gowin EDA license. Synthesis is local-only (not run in CI).
#
# Date: February 2026
# =============================================================================

# -----------------------------------------------------------------------------
# Project Configuration
# -----------------------------------------------------------------------------
set SCRIPT_DIR [file dirname [info script]]
set PROJECT_ROOT [file normalize "$SCRIPT_DIR/../../.."]
set V3_RTL_DIR "$PROJECT_ROOT/hardware/v3/rtl"
set TOP_MODULE "atomik_v3_stub"

# Device: Tang Nano 9K
set DEVICE "GW1NR-LV9QN88PC6/I5"
set DEVICE_FAMILY "GW1NR-9"

puts "=============================================="
puts " ATOMiK v3 Synthesis - Gowin EDA"
puts "=============================================="
puts ""
puts "Project Root: $PROJECT_ROOT"
puts "RTL Dir:      $V3_RTL_DIR"
puts "Top Module:   $TOP_MODULE"
puts "Device:       $DEVICE"
puts ""

# -----------------------------------------------------------------------------
# Collect RTL Source Files
# -----------------------------------------------------------------------------
set rtl_files [glob -nocomplain "$V3_RTL_DIR/*.v"]
if {[llength $rtl_files] == 0} {
    puts "ERROR: No Verilog files found in $V3_RTL_DIR"
    exit 1
}

puts "Source files:"
foreach f $rtl_files {
    puts "  $f"
}
puts ""

# -----------------------------------------------------------------------------
# Create In-Memory Project
# -----------------------------------------------------------------------------
create_project -name "ATOMiK_v3" -dir "$PROJECT_ROOT/hardware/v3/synth/impl" \
    -device $DEVICE

# Add source files
foreach f $rtl_files {
    add_file $f
}

# -----------------------------------------------------------------------------
# Set Options
# -----------------------------------------------------------------------------
puts "Configuring synthesis options..."

set_option -top_module $TOP_MODULE
set_option -verilog_std v2001
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
puts ""
puts "=============================================="
puts " ATOMiK v3 Synthesis Complete"
puts "=============================================="
puts ""
