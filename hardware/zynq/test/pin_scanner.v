
module pin_scanner(
    inout [14:0] DDR_addr, inout [2:0] DDR_ba, inout DDR_cas_n,
    inout DDR_ck_n, inout DDR_ck_p, inout DDR_cke, inout DDR_cs_n,
    inout [3:0] DDR_dm, inout [31:0] DDR_dq, inout [3:0] DDR_dqs_n,
    inout [3:0] DDR_dqs_p, inout DDR_odt, inout DDR_ras_n,
    inout DDR_reset_n, inout DDR_we_n,
    inout FIXED_IO_ddr_vrn, inout FIXED_IO_ddr_vrp,
    inout [53:0] FIXED_IO_mio, inout FIXED_IO_ps_clk,
    inout FIXED_IO_ps_porb, inout FIXED_IO_ps_srstb,
    input [199:0] scan_in
);

wire fclk;
wire [31:0] M_AXI_GP0_araddr, M_AXI_GP0_awaddr, M_AXI_GP0_wdata, M_AXI_GP0_rdata;
wire [2:0]  M_AXI_GP0_arprot, M_AXI_GP0_awprot;
wire [3:0]  M_AXI_GP0_wstrb;
wire M_AXI_GP0_arvalid, M_AXI_GP0_arready;
wire M_AXI_GP0_awvalid, M_AXI_GP0_awready;
wire M_AXI_GP0_wvalid, M_AXI_GP0_wready;
wire M_AXI_GP0_rvalid, M_AXI_GP0_rready;
wire M_AXI_GP0_bvalid, M_AXI_GP0_bready;
wire [1:0] M_AXI_GP0_rresp, M_AXI_GP0_bresp;

scanner_sys_wrapper u_sys (
    .DDR_addr(DDR_addr), .DDR_ba(DDR_ba), .DDR_cas_n(DDR_cas_n),
    .DDR_ck_n(DDR_ck_n), .DDR_ck_p(DDR_ck_p), .DDR_cke(DDR_cke),
    .DDR_cs_n(DDR_cs_n), .DDR_dm(DDR_dm), .DDR_dq(DDR_dq),
    .DDR_dqs_n(DDR_dqs_n), .DDR_dqs_p(DDR_dqs_p), .DDR_odt(DDR_odt),
    .DDR_ras_n(DDR_ras_n), .DDR_reset_n(DDR_reset_n), .DDR_we_n(DDR_we_n),
    .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn), .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
    .FIXED_IO_mio(FIXED_IO_mio), .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
    .FIXED_IO_ps_porb(FIXED_IO_ps_porb), .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
    .fclk(fclk),
    .M_AXI_GP0_araddr(M_AXI_GP0_araddr),
    .M_AXI_GP0_arprot(M_AXI_GP0_arprot),
    .M_AXI_GP0_arvalid(M_AXI_GP0_arvalid),
    .M_AXI_GP0_arready(M_AXI_GP0_arready),
    .M_AXI_GP0_rdata(M_AXI_GP0_rdata),
    .M_AXI_GP0_rresp(M_AXI_GP0_rresp),
    .M_AXI_GP0_rvalid(M_AXI_GP0_rvalid),
    .M_AXI_GP0_rready(M_AXI_GP0_rready),
    .M_AXI_GP0_awaddr(M_AXI_GP0_awaddr),
    .M_AXI_GP0_awprot(M_AXI_GP0_awprot),
    .M_AXI_GP0_awvalid(M_AXI_GP0_awvalid),
    .M_AXI_GP0_awready(M_AXI_GP0_awready),
    .M_AXI_GP0_wdata(M_AXI_GP0_wdata),
    .M_AXI_GP0_wstrb(M_AXI_GP0_wstrb),
    .M_AXI_GP0_wvalid(M_AXI_GP0_wvalid),
    .M_AXI_GP0_wready(M_AXI_GP0_wready),
    .M_AXI_GP0_bresp(M_AXI_GP0_bresp),
    .M_AXI_GP0_bvalid(M_AXI_GP0_bvalid),
    .M_AXI_GP0_bready(M_AXI_GP0_bready)
);

// Simple AXI-lite slave: read-only registers holding pin states
// Base address: 0x43C00000 (GP0 default)
// Reg 0 (offset 0x00): scan_in[31:0]
// Reg 1 (offset 0x04): scan_in[63:32]
// ...
// Reg 6 (offset 0x18): scan_in[199:192] (8 bits valid)

// Sample all pins on clock edge
reg [199:0] pin_sample;
always @(posedge fclk) pin_sample <= scan_in;

// AXI-lite read slave (minimal)
reg arready_r, rvalid_r;
reg [31:0] rdata_r;

assign M_AXI_GP0_arready = arready_r;
assign M_AXI_GP0_rdata = rdata_r;
assign M_AXI_GP0_rresp = 2'b00;
assign M_AXI_GP0_rvalid = rvalid_r;

// Write channel: just acknowledge everything
assign M_AXI_GP0_awready = 1'b1;
assign M_AXI_GP0_wready = 1'b1;
assign M_AXI_GP0_bvalid = 1'b1;
assign M_AXI_GP0_bresp = 2'b00;

always @(posedge fclk) begin
    if (M_AXI_GP0_arvalid && !rvalid_r) begin
        arready_r <= 1'b1;
        rvalid_r <= 1'b1;
        case (M_AXI_GP0_araddr[4:2])
            3'd0: rdata_r <= pin_sample[31:0];
            3'd1: rdata_r <= pin_sample[63:32];
            3'd2: rdata_r <= pin_sample[95:64];
            3'd3: rdata_r <= pin_sample[127:96];
            3'd4: rdata_r <= pin_sample[159:128];
            3'd5: rdata_r <= pin_sample[191:160];
            3'd6: rdata_r <= {24'b0, pin_sample[199:192]};
            default: rdata_r <= 32'hDEADDEAD;
        endcase
    end else begin
        arready_r <= 1'b0;
        if (M_AXI_GP0_rready && rvalid_r)
            rvalid_r <= 1'b0;
    end
end

endmodule
