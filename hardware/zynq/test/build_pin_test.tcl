create_project -in_memory -part xc7z020clg484-2
read_verilog pin_test.v
read_xdc pin_test.xdc
synth_design -top pin_test
opt_design
place_design
route_design
write_bitstream -force pin_test.bit
