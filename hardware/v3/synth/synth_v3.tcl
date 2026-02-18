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
set IMPL_DIR "$PROJECT_ROOT/hardware/v3/synth/impl"
set TOP_MODULE "atomik_v3_cpu"

# Device: Tang Nano 9K
set DEVICE "GW1NR-LV9QN88PC6/I5"

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
# Collect RTL Source Files (exclude stub)
# -----------------------------------------------------------------------------
set rtl_files [glob -nocomplain "$V3_RTL_DIR/atomik_v3_*.v"]
set cpu_files {}
foreach f $rtl_files {
    if {[string match "*stub*" $f]} continue
    lappend cpu_files $f
}

if {[llength $cpu_files] == 0} {
    puts "ERROR: No CPU RTL files found in $V3_RTL_DIR"
    exit 1
}

puts "Source files:"
foreach f $cpu_files {
    puts "  [file tail $f]"
}
puts ""

# -----------------------------------------------------------------------------
# Create Project
# -----------------------------------------------------------------------------
file mkdir $IMPL_DIR
set gprj_file "$IMPL_DIR/ATOMiK_v3.gprj"

# Write a minimal .gprj file
set gprj_fd [open $gprj_file w]
puts $gprj_fd {<?xml version="1" encoding="UTF-8"?>}
puts $gprj_fd {<!DOCTYPE gowin-fpga-project>}
puts $gprj_fd {<Project>}
puts $gprj_fd {    <Template>FPGA</Template>}
puts $gprj_fd {    <Version>5</Version>}
puts $gprj_fd {    <Device name="GW1NR-9C" pn="GW1NR-LV9QN88PC6/I5">gw1nr9c-004</Device>}
puts $gprj_fd {    <FileList>}
close $gprj_fd

# Append source files to the project file
set gprj_fd [open $gprj_file a]
foreach f $cpu_files {
    puts $gprj_fd "        <File path=\"$f\" type=\"file.verilog\" enable=\"1\"/>"
}
puts $gprj_fd {    </FileList>}
puts $gprj_fd {</Project>}
close $gprj_fd

open_project $gprj_file

# Files are already listed in the .gprj — do NOT call add_file (causes duplicates)

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
puts "Results in: $IMPL_DIR"
puts ""
