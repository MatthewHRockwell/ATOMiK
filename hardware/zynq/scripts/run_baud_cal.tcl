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
dow /home/mattrock/Projects/ATOMiK/hardware/zynq/test/uart_baud_cal.elf
con
after 2000
set sr [mrd -force -value 0xE000002C]
puts [format "TACTIVE=%d" [expr {($sr >> 11) & 1}]]
after 60000
stop
