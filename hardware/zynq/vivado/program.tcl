# ==============================================================================
# ATOMiK Zynq JTAG Programmer (Volatile)
#
# Target:  ALINX AX7020 (XC7Z020-2CLG400I)
# Purpose: Load bitstream to PL via JTAG. Configuration lost on power cycle.
#
# Usage:
#   vivado -mode batch -source vivado/program.tcl
#
# Run from hardware/zynq/ directory.
#
# Date: March 2026
# ==============================================================================

set SCRIPT_DIR [file dirname [info script]]
set ZYNQ_DIR   [file normalize "$SCRIPT_DIR/.."]
set BITSTREAM  "$ZYNQ_DIR/output/atomik_zynq.bit"

if {![file exists $BITSTREAM]} {
    puts "ERROR: Bitstream not found: $BITSTREAM"
    puts "Run 'make bitstream' or 'make blockdesign' first."
    exit 1
}

# Connect to hardware
open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target

# Program the FPGA
set_property PROGRAM.FILE $BITSTREAM [current_hw_device]
program_hw_devices [current_hw_device]

puts "FPGA programmed (volatile). Power cycle will clear."
close_hw_manager
