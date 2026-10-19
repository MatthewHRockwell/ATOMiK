# Blink test using PS7 FCLK_CLK0 — tests if external oscillator is the problem
set SCRIPT_DIR [file dirname [info script]]

create_project -in_memory -part xc7z020clg400-2

create_bd_design "blink_sys"
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7

set_property -dict [list \
    CONFIG.PCW_FPGA_FCLK0_ENABLE {1} \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {50} \
    CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125} \
    CONFIG.PCW_USE_M_AXI_GP0 {0} \
] [get_bd_cells ps7]

apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]

create_bd_port -dir O fclk_clk0
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_ports fclk_clk0]

validate_bd_design
generate_target all [get_files blink_sys.bd]
make_wrapper -files [get_files blink_sys.bd] -top

add_files [glob .gen/sources_1/bd/blink_sys/hdl/*_wrapper.v]

# Write the top-level verilog
set fp [open "$SCRIPT_DIR/blink_fclk_top.v" w]
puts $fp "module blink_fclk_top("
puts $fp "    inout \[14:0\] DDR_addr, inout \[2:0\] DDR_ba, inout DDR_cas_n,"
puts $fp "    inout DDR_ck_n, inout DDR_ck_p, inout DDR_cke, inout DDR_cs_n,"
puts $fp "    inout \[3:0\] DDR_dm, inout \[31:0\] DDR_dq, inout \[3:0\] DDR_dqs_n,"
puts $fp "    inout \[3:0\] DDR_dqs_p, inout DDR_odt, inout DDR_ras_n,"
puts $fp "    inout DDR_reset_n, inout DDR_we_n,"
puts $fp "    inout FIXED_IO_ddr_vrn, inout FIXED_IO_ddr_vrp,"
puts $fp "    inout \[53:0\] FIXED_IO_mio, inout FIXED_IO_ps_clk,"
puts $fp "    inout FIXED_IO_ps_porb, inout FIXED_IO_ps_srstb,"
puts $fp "    output \[3:0\] led"
puts $fp ");"
puts $fp "wire fclk;"
puts $fp "blink_sys_wrapper u_sys ("
puts $fp "    .DDR_addr(DDR_addr), .DDR_ba(DDR_ba), .DDR_cas_n(DDR_cas_n),"
puts $fp "    .DDR_ck_n(DDR_ck_n), .DDR_ck_p(DDR_ck_p), .DDR_cke(DDR_cke),"
puts $fp "    .DDR_cs_n(DDR_cs_n), .DDR_dm(DDR_dm), .DDR_dq(DDR_dq),"
puts $fp "    .DDR_dqs_n(DDR_dqs_n), .DDR_dqs_p(DDR_dqs_p), .DDR_odt(DDR_odt),"
puts $fp "    .DDR_ras_n(DDR_ras_n), .DDR_reset_n(DDR_reset_n), .DDR_we_n(DDR_we_n),"
puts $fp "    .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn), .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),"
puts $fp "    .FIXED_IO_mio(FIXED_IO_mio), .FIXED_IO_ps_clk(FIXED_IO_ps_clk),"
puts $fp "    .FIXED_IO_ps_porb(FIXED_IO_ps_porb), .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),"
puts $fp "    .fclk_clk0(fclk)"
puts $fp ");"
puts $fp "reg \[25:0\] counter = 0;"
puts $fp "always @(posedge fclk) counter <= counter + 1;"
puts $fp "assign led\[0\] = ~counter\[23\];"
puts $fp "assign led\[1\] = ~counter\[24\];"
puts $fp "assign led\[2\] = ~counter\[25\];"
puts $fp "assign led\[3\] = ~counter\[22\];"
puts $fp "endmodule"
close $fp

add_files $SCRIPT_DIR/blink_fclk_top.v
set_property top blink_fclk_top [current_fileset]

# Constraints — LEDs only (PS pins handled by block design)
set cfp [open "$SCRIPT_DIR/blink_fclk.xdc" w]
puts $cfp "set_property PACKAGE_PIN M14 \[get_ports {led\[0\]}\]"
puts $cfp "set_property PACKAGE_PIN M15 \[get_ports {led\[1\]}\]"
puts $cfp "set_property PACKAGE_PIN K16 \[get_ports {led\[2\]}\]"
puts $cfp "set_property PACKAGE_PIN J16 \[get_ports {led\[3\]}\]"
puts $cfp "set_property IOSTANDARD LVCMOS33 \[get_ports {led\[*\]}\]"
close $cfp
add_files -fileset constrs_1 $SCRIPT_DIR/blink_fclk.xdc

synth_design -top blink_fclk_top
opt_design
place_design
route_design
write_bitstream -force $SCRIPT_DIR/blink_fclk.bit
puts "FCLK blink test bitstream generated."
