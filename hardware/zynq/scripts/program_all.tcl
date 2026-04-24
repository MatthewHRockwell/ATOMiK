# All-in-one: connect, reset, init, program, post-config
set SCRIPT_DIR [file dirname [info script]]
set bitfile [lindex $argv 0]

connect

# Check targets
set tgts [targets -target-properties]
puts "Targets found: [llength $tgts]"
targets

# Reset
targets -set -filter {name =~ "APU*"}
rst -system
after 2000

# PS init
source $SCRIPT_DIR/ps7_init_rk7020f.tcl
ps7_init
puts "PS7 init done"

# Program PL
targets -set -filter {name =~ "xc7z*"}
fpga -file $bitfile
puts "PL programmed"
after 500

# Post-config
targets -set -filter {name =~ "APU*"}
ps7_post_config
puts "Post-config done"

# Verify
set lvl [mrd -force -value 0xF8000900]
set rst [mrd -force -value 0xF8000240]
puts "LVL_SHFTR=0x[format %08X $lvl] FPGA_RST=0x[format %08X $rst]"

disconnect
