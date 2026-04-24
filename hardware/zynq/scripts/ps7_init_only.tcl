# Initialize PS7 only (run AFTER PL is already programmed via Vivado)
# Usage: xsdb ps7_init_only.tcl

set SCRIPT_DIR [file dirname [info script]]

connect
targets

# Target ARM core
targets -set -filter {name =~ "APU*"}
stop

# Initialize PS7
source $SCRIPT_DIR/ps7_init_rk7020f.tcl
ps7_init
ps7_post_config

puts "PS7 initialized."
disconnect
exit
