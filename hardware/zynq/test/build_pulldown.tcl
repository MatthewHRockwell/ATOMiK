create_project -in_memory -part xc7z020clg484-2
read_verilog led_static.v
read_xdc pulldown_test.xdc
synth_design -top led_static
set_property BITSTREAM.CONFIG.UNUSEDPIN PULLDOWN [current_design]
opt_design
place_design
route_design
write_bitstream -force pulldown_test.bit
puts "Pull-down test bitstream ready"
