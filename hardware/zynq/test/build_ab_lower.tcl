create_project -in_memory -part xc7z020clg484-2
read_verilog ab_lower.v
read_xdc ab_lower.xdc
synth_design -top ab_lower
opt_design
place_design
route_design
write_bitstream -force ab_lower.bit
