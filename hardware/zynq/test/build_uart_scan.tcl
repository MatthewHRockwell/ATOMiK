# Build UART TX on ALL header pins simultaneously using PS7 FCLK
set SCRIPT_DIR [file dirname [info script]]

create_project -in_memory -part xc7z020clg484-2

# Create PS7 block design for FCLK
create_bd_design "ps_sys"
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7
set_property -dict [list \
    CONFIG.PCW_FPGA_FCLK0_ENABLE {1} \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {50} \
    CONFIG.PCW_USE_M_AXI_GP0 {0} \
    CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125} \
] [get_bd_cells ps7]
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]
create_bd_port -dir O fclk
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_ports fclk]
validate_bd_design
generate_target all [get_files ps_sys.bd]
make_wrapper -files [get_files ps_sys.bd] -top
add_files [glob .gen/sources_1/bd/ps_sys/hdl/*_wrapper.v]

# UART TX on all header pins — sends "OK\n" at 115200 baud using 50MHz FCLK
set fp [open "$SCRIPT_DIR/uart_scan.v" w]
puts $fp {
module uart_scan(
    inout [14:0] DDR_addr, inout [2:0] DDR_ba, inout DDR_cas_n,
    inout DDR_ck_n, inout DDR_ck_p, inout DDR_cke, inout DDR_cs_n,
    inout [3:0] DDR_dm, inout [31:0] DDR_dq, inout [3:0] DDR_dqs_n,
    inout [3:0] DDR_dqs_p, inout DDR_odt, inout DDR_ras_n,
    inout DDR_reset_n, inout DDR_we_n,
    inout FIXED_IO_ddr_vrn, inout FIXED_IO_ddr_vrp,
    inout [53:0] FIXED_IO_mio, inout FIXED_IO_ps_clk,
    inout FIXED_IO_ps_porb, inout FIXED_IO_ps_srstb,
    output [31:0] txpins
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

// UART TX — 115200 baud from 50MHz = divider 434
reg [8:0] baud_cnt = 0;
reg [3:0] bit_cnt = 0;
reg [9:0] shift = 10'h3FF;
reg [23:0] pause = 0;
reg active = 0;
reg [2:0] char_idx = 0;
wire tx_bit = shift[0];

// Message: "OK\r\n"
wire [7:0] msg = (char_idx==0) ? 8'h4F : // O
                 (char_idx==1) ? 8'h4B : // K
                 (char_idx==2) ? 8'h0D : // CR
                 8'h0A;                   // LF

always @(posedge fclk) begin
    if (!active) begin
        pause <= pause + 1;
        if (pause[23]) begin
            active <= 1; pause <= 0; char_idx <= 0;
            shift <= {1'b1, msg, 1'b0};
            bit_cnt <= 0; baud_cnt <= 0;
        end
    end else begin
        if (baud_cnt == 433) begin
            baud_cnt <= 0;
            shift <= {1'b1, shift[9:1]};
            if (bit_cnt == 9) begin
                bit_cnt <= 0;
                if (char_idx == 3) begin
                    active <= 0; shift <= 10'h3FF;
                end else begin
                    char_idx <= char_idx + 1;
                    shift <= {1'b1, msg, 1'b0};
                end
            end else bit_cnt <= bit_cnt + 1;
        end else baud_cnt <= baud_cnt + 1;
    end
end

// Drive ALL header pins with the same UART TX
assign txpins = {32{tx_bit}};
endmodule
}
close $fp

add_files $SCRIPT_DIR/uart_scan.v
set_property top uart_scan [current_fileset]

# XDC — all 32 valid header pins
set fp2 [open "$SCRIPT_DIR/uart_scan.xdc" w]
set pins {W12 V9 U9 AB12 Y10 U11 AB9 AB11 W8 AB8 AA8 AB4 AA6 Y5 AA4 AB1 V12 V10 U10 AA12 Y11 U12 AB10 AA11 V8 W11 AB7 AA9 AA7 Y6 Y4 AB2}
for {set i 0} {$i < [llength $pins]} {incr i} {
    puts $fp2 "set_property PACKAGE_PIN [lindex $pins $i] \[get_ports {txpins[$i]}\]"
    puts $fp2 "set_property IOSTANDARD LVCMOS33 \[get_ports {txpins[$i]}\]"
}
close $fp2
add_files -fileset constrs_1 $SCRIPT_DIR/uart_scan.xdc

set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
synth_design -top uart_scan
opt_design
place_design
route_design
write_bitstream -force $SCRIPT_DIR/uart_scan.bit
puts "UART SCAN bitstream built"
