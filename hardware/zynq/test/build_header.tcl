create_project -in_memory -part xc7z020clg484-2
read_verilog header_test.v
read_xdc header_test.xdc
set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
synth_design -top header_test
opt_design
place_design
route_design
write_bitstream -force header_test.bit
