create_project -in_memory -part xc7z020clg484-2

create_bd_design "emio_sys"
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7
set_property -dict [list \
    CONFIG.PCW_FPGA_FCLK0_ENABLE {1} \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {50} \
    CONFIG.PCW_USE_M_AXI_GP0 {0} \
    CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125} \
    CONFIG.PCW_EN_UART0 {1} \
    CONFIG.PCW_UART0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_UART0_UART0_IO {EMIO} \
    CONFIG.PCW_EN_UART1 {1} \
    CONFIG.PCW_UART1_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_UART1_UART1_IO {EMIO} \
] [get_bd_cells ps7]

apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]

# Export EMIO UART ports
create_bd_port -dir O uart0_tx
create_bd_port -dir I uart0_rx
create_bd_port -dir O uart1_tx
create_bd_port -dir I uart1_rx
create_bd_port -dir O fclk

connect_bd_net [get_bd_pins ps7/UART0_TX] [get_bd_ports uart0_tx]
connect_bd_net [get_bd_ports uart0_rx] [get_bd_pins ps7/UART0_RX]
connect_bd_net [get_bd_pins ps7/UART1_TX] [get_bd_ports uart1_tx]
connect_bd_net [get_bd_ports uart1_rx] [get_bd_pins ps7/UART1_RX]
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_ports fclk]

validate_bd_design
generate_target all [get_files emio_sys.bd]
make_wrapper -files [get_files emio_sys.bd] -top
add_files [glob .gen/sources_1/bd/emio_sys/hdl/*_wrapper.v]

# Top module: fans EMIO UART TX to ALL header pins
set fp [open "emio_uart_top.v" w]
puts $fp {
module emio_uart_top(
    inout [14:0] DDR_addr, inout [2:0] DDR_ba, inout DDR_cas_n,
    inout DDR_ck_n, inout DDR_ck_p, inout DDR_cke, inout DDR_cs_n,
    inout [3:0] DDR_dm, inout [31:0] DDR_dq, inout [3:0] DDR_dqs_n,
    inout [3:0] DDR_dqs_p, inout DDR_odt, inout DDR_ras_n,
    inout DDR_reset_n, inout DDR_we_n,
    inout FIXED_IO_ddr_vrn, inout FIXED_IO_ddr_vrp,
    inout [53:0] FIXED_IO_mio, inout FIXED_IO_ps_clk,
    inout FIXED_IO_ps_porb, inout FIXED_IO_ps_srstb,
    output [31:0] txpins,
    input  [1:0]  rxpins
);
wire uart0_tx, uart1_tx, fclk;
emio_sys_wrapper u_sys (
    .DDR_addr(DDR_addr), .DDR_ba(DDR_ba), .DDR_cas_n(DDR_cas_n),
    .DDR_ck_n(DDR_ck_n), .DDR_ck_p(DDR_ck_p), .DDR_cke(DDR_cke),
    .DDR_cs_n(DDR_cs_n), .DDR_dm(DDR_dm), .DDR_dq(DDR_dq),
    .DDR_dqs_n(DDR_dqs_n), .DDR_dqs_p(DDR_dqs_p), .DDR_odt(DDR_odt),
    .DDR_ras_n(DDR_ras_n), .DDR_reset_n(DDR_reset_n), .DDR_we_n(DDR_we_n),
    .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn), .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
    .FIXED_IO_mio(FIXED_IO_mio), .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
    .FIXED_IO_ps_porb(FIXED_IO_ps_porb), .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
    .uart0_tx(uart0_tx), .uart0_rx(rxpins[0]),
    .uart1_tx(uart1_tx), .uart1_rx(rxpins[1]),
    .fclk(fclk)
);
// Fan both UART TX signals to all output pins (XOR them so both are active)
assign txpins = {32{uart0_tx & uart1_tx}};
endmodule
}
close $fp

add_files emio_uart_top.v
set_property top emio_uart_top [current_fileset]

# XDC: header pins for TX, two pins for RX
set fp2 [open "emio_uart.xdc" w]
set txpins {W12 V9 U9 AB12 Y10 U11 AB9 AB11 AB8 AA8 AB4 AA6 Y5 AA4 AB1 V12 V10 U10 AA12 Y11 U12 AB10 AA11 V8 W11 AB7 AA9 AA7 Y6 Y4 AB2 Y10}
for {set i 0} {$i < 32} {incr i} {
    puts $fp2 "set_property PACKAGE_PIN [lindex $txpins $i] \[get_ports {txpins[$i]}\]"
    puts $fp2 "set_property IOSTANDARD LVCMOS33 \[get_ports {txpins[$i]}\]"
}
puts $fp2 "set_property PACKAGE_PIN U9 \[get_ports {rxpins[0]}\]"
puts $fp2 "set_property PACKAGE_PIN U10 \[get_ports {rxpins[1]}\]"
puts $fp2 "set_property IOSTANDARD LVCMOS33 \[get_ports {rxpins[*]}\]"
close $fp2
add_files -fileset constrs_1 emio_uart.xdc

set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]
synth_design -top emio_uart_top
opt_design
place_design
route_design
write_bitstream -force emio_uart.bit
puts "EMIO UART BUILT"
