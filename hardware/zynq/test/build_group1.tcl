create_project -in_memory -part xc7z020clg484-2
read_verilog group1.v
read_xdc group1.xdc
synth_design -top group1
opt_design
place_design
route_design
write_bitstream -force group1.bit
