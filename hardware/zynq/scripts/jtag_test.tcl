# JTAG connectivity test for ALINX AX7020
open_hw_manager
connect_hw_server
open_hw_target

set devices [get_hw_devices]
puts "Found [llength $devices] device(s):"
foreach dev $devices {
    puts "  Device: $dev"
    set idcode [get_property IDCODE $dev]
    puts "  IDCODE: $idcode"
}

close_hw_target
close_hw_server
close_hw_manager
puts "JTAG test complete."
