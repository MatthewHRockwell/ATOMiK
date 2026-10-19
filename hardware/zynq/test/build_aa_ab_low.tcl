create_project -in_memory -part xc7z020clg484-2
read_verilog aa_ab_low.v
read_xdc aa_ab_low.xdc
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]
synth_design -top aa_ab_low
opt_design
place_design
route_design
write_bitstream -force aa_ab_low.bit
puts "AA/AB LOW bitstream built"
