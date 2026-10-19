create_project -in_memory -part xc7z020clg484-2
read_verilog all_low.v
read_xdc all_low.xdc
synth_design -top all_low
opt_design
place_design
route_design
write_bitstream -force all_low.bit
puts "ALL-LOW bitstream built for CLG484"
