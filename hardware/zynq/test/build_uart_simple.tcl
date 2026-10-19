create_project -in_memory -part xc7z020clg484-2
read_verilog uart_simple.v
read_xdc uart_simple.xdc
synth_design -top uart_simple
opt_design
place_design
route_design
write_bitstream -force uart_simple.bit
