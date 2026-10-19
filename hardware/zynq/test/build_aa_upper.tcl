create_project -in_memory -part xc7z020clg484-2
read_verilog aa_upper.v
read_xdc aa_upper.xdc
synth_design -top aa_upper
opt_design
place_design
route_design
write_bitstream -force aa_upper.bit
