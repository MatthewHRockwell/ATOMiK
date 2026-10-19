set SCRIPT_DIR [file dirname [info script]]

create_project -in_memory -part xc7z020clg484-2
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

set fp [open "$SCRIPT_DIR/led_blink_confirm.v" w]
puts $fp {
module led_blink_confirm(
    inout [14:0] DDR_addr, inout [2:0] DDR_ba, inout DDR_cas_n,
    inout DDR_ck_n, inout DDR_ck_p, inout DDR_cke, inout DDR_cs_n,
    inout [3:0] DDR_dm, inout [31:0] DDR_dq, inout [3:0] DDR_dqs_n,
    inout [3:0] DDR_dqs_p, inout DDR_odt, inout DDR_ras_n,
    inout DDR_reset_n, inout DDR_we_n,
    inout FIXED_IO_ddr_vrn, inout FIXED_IO_ddr_vrp,
    inout [53:0] FIXED_IO_mio, inout FIXED_IO_ps_clk,
    inout FIXED_IO_ps_porb, inout FIXED_IO_ps_srstb,
    output pl_led1, output pl_led2
);
wire fclk;
ps_sys_wrapper u_ps (
    .DDR_addr(DDR_addr), .DDR_ba(DDR_ba), .DDR_cas_n(DDR_cas_n),
    .DDR_ck_n(DDR_ck_n), .DDR_ck_p(DDR_ck_p), .DDR_cke(DDR_cke),
    .DDR_cs_n(DDR_cs_n), .DDR_dm(DDR_dm), .DDR_dq(DDR_dq),
    .DDR_dqs_n(DDR_dqs_n), .DDR_dqs_p(DDR_dqs_p), .DDR_odt(DDR_odt),
    .DDR_ras_n(DDR_ras_n), .DDR_reset_n(DDR_reset_n), .DDR_we_n(DDR_we_n),
    .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn), .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
    .FIXED_IO_mio(FIXED_IO_mio), .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
    .FIXED_IO_ps_porb(FIXED_IO_ps_porb), .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
    .fclk(fclk)
);
reg [25:0] cnt = 0;
always @(posedge fclk) cnt <= cnt + 1;
// LED1 blinks at ~1.5 Hz, LED2 blinks at ~0.75 Hz (visually distinct)
assign pl_led1 = cnt[24];
assign pl_led2 = cnt[25];
endmodule
}
close $fp

set xfp [open "$SCRIPT_DIR/led_blink_confirm.xdc" w]
puts $xfp {set_property PACKAGE_PIN V15 [get_ports pl_led1]}
puts $xfp {set_property IOSTANDARD LVCMOS33 [get_ports pl_led1]}
puts $xfp {set_property PACKAGE_PIN V13 [get_ports pl_led2]}
puts $xfp {set_property IOSTANDARD LVCMOS33 [get_ports pl_led2]}
puts $xfp {set_property SEVERITY {Warning} [get_drc_checks UCIO-1]}
puts $xfp {set_property SEVERITY {Warning} [get_drc_checks NSTD-1]}
close $xfp

add_files "$SCRIPT_DIR/led_blink_confirm.v"
add_files -fileset constrs_1 "$SCRIPT_DIR/led_blink_confirm.xdc"
set_property top led_blink_confirm [current_fileset]
synth_design -top led_blink_confirm
opt_design; place_design; route_design
write_bitstream -force "$SCRIPT_DIR/led_blink_confirm.bit"
puts "BUILT: led_blink_confirm.bit (LED1=V15 @ ~1.5Hz, LED2=V13 @ ~0.75Hz)"
