create_project -in_memory -part xc7z020clg484-2
read_verilog group0.v
read_xdc group0.xdc
synth_design -top group0
opt_design
place_design
route_design
write_bitstream -force group0.bit
