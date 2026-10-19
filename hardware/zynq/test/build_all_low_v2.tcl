create_project -in_memory -part xc7z020clg484-2
read_verilog all_low_v2.v
read_xdc all_low_v2.xdc
set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]
synth_design -top all_low_v2
opt_design
place_design
route_design
write_bitstream -force all_low_v2.bit
puts "ALL-LOW V2 built"
