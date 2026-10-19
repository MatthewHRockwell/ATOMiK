create_project -in_memory -part xc7z020clg400-2
read_verilog led_static.v
read_xdc led_static.xdc
synth_design -top led_static
opt_design
place_design
route_design
write_bitstream -force led_static.bit
