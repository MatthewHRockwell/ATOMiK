# Start bare-metal UART sender and keep running for 120s
set SCRIPT_DIR [file dirname [info script]]

connect
after 500

targets -set -filter {name =~ "APU*"}
rst -system
after 1000

source $SCRIPT_DIR/ps7_init_rk7020f.tcl
ps7_init
ps7_post_config

targets -set -filter {name =~ "ARM*#0"}
stop
after 100

dow /home/mattrock/Projects/ATOMiK/hardware/zynq/test/uart_sender.elf
puts "ELF loaded"

# Verify load
set w0 [mrd -force -value 0x00000000]
puts [format "OCM\[0\]=0x%08X (expect 0xE3A0D803)" $w0]

con
puts "ARM core started"

# Give it time to configure UART and start transmitting
after 3000

set sr [mrd -force -value 0xE000002C]
puts [format "SR=0x%08X TACTIVE=%d" $sr [expr {($sr >> 11) & 1}]]

# Keep alive for 300 seconds
after 300000
stop
puts "Done"
