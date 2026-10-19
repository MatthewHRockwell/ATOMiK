create_project -in_memory -part xc7z020clg484-2
read_verilog group3.v
read_xdc group3.xdc
synth_design -top group3
opt_design
place_design
route_design
write_bitstream -force group3.bit
