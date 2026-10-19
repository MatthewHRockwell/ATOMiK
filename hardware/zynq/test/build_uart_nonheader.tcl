create_project -in_memory -part xc7z020clg484-2
set SCRIPT_DIR [file dirname [info script]]

# Recreate PS7 block design
create_bd_design "ps_sys"
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7
set_property -dict [list CONFIG.PCW_FPGA_FCLK0_ENABLE {1} CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {50} CONFIG.PCW_USE_M_AXI_GP0 {0} CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125}] [get_bd_cells ps7]
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]
create_bd_port -dir O fclk
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_ports fclk]
validate_bd_design
generate_target all [get_files ps_sys.bd]
make_wrapper -files [get_files ps_sys.bd] -top
add_files [glob .gen/sources_1/bd/ps_sys/hdl/*_wrapper.v]

add_files $SCRIPT_DIR/uart_nonheader.v
add_files -fileset constrs_1 $SCRIPT_DIR/uart_nonheader.xdc
set_property top uart_nonheader [current_fileset]
set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]

synth_design -top uart_nonheader
opt_design  
place_design
route_design
write_bitstream -force $SCRIPT_DIR/uart_nonheader.bit
puts "BUILT"
