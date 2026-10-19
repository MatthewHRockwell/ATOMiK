create_project -in_memory -part xc7z020clg484-2
read_verilog all_low_full.v
read_xdc all_low_full.xdc
synth_design -top all_low_full
opt_design
place_design
route_design
write_bitstream -force all_low_full.bit
puts "ALL-LOW FULL bitstream built (273 pins)"
