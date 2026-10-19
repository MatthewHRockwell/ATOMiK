create_project -in_memory -part xc7z020clg484-2
add_files {uart_header.v .gen/sources_1/bd/blink_sys/hdl/blink_sys_wrapper.v}
add_files [glob .gen/sources_1/bd/blink_sys/synth/*.v]
read_xdc uart_header.xdc
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]
set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
synth_design -top uart_header
opt_design
place_design
route_design
write_bitstream -force uart_header.bit
