# Program bitstream to AX7020 PL (volatile — SRAM only)
set bitfile [lindex $argv 0]
if {$bitfile eq ""} {
    puts "Usage: vivado -mode batch -source program_bit.tcl -tclargs <bitfile>"
    exit 1
}
puts "Programming: $bitfile"

open_hw_manager
connect_hw_server
open_hw_target

set device [get_hw_devices xc7z020_1]
current_hw_device $device
set_property PROGRAM.FILE $bitfile $device
program_hw_devices $device

puts "Programming complete."
close_hw_target
disconnect_hw_server
close_hw_manager
