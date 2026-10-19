create_project -in_memory -part xc7z020clg484-2
read_verilog group2.v
read_xdc group2.xdc
synth_design -top group2
opt_design
place_design
route_design
write_bitstream -force group2.bit
