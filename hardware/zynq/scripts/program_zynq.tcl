# =============================================================================
# Zynq Programming Script for XSDB
# Initializes PS7 (PLLs, DDR, clocks, MIO) then programs PL bitstream
#
# Usage: xsdb program_zynq.tcl <bitfile>
# =============================================================================

set SCRIPT_DIR [file dirname [info script]]
set bitfile [lindex $argv 0]

if {$bitfile eq ""} {
    puts "Usage: xsdb program_zynq.tcl <bitfile>"
    exit 1
}

puts "============================================"
puts " Zynq Programming: PS7 Init + PL Bitstream"
puts " Bitfile: $bitfile"
puts "============================================"

# Connect to hardware
connect

# List targets
puts "\nAvailable targets:"
targets

# Target the APU (ARM Cortex-A9)
targets -set -filter {name =~ "APU*"}
puts "\nTargeted: [targets -filter {name =~ "APU*"}]"

# Stop the processor
stop

# Step 1: Initialize PS7
puts "\n[1] Initializing PS7..."
source $SCRIPT_DIR/ps7_init_rk7020f.tcl
ps7_init
ps7_post_config
puts "    PS7 initialized (PLLs, DDR, MIO, clocks)."

# Step 2: Program PL bitstream
puts "\n[2] Programming PL bitstream..."
targets -set -filter {name =~ "PL*" || name =~ "xc7z*"}
fpga $bitfile
puts "    PL programmed: $bitfile"

# Step 3: Verify
puts "\n============================================"
puts " Done. PS7 initialized + PL configured."
puts " LEDs and HDMI should now be active."
puts "============================================"

disconnect
exit
