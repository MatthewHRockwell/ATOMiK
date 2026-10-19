create_project -in_memory -part xc7z020clg400-2
read_verilog all_low_400.v
read_xdc all_low_400.xdc
synth_design -top all_low_400
opt_design
place_design
route_design
write_bitstream -force all_low_400.bit
puts "ALL-LOW CLG400 bitstream built"
