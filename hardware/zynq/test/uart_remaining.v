
module uart_remaining(
    inout [14:0] DDR_addr, inout [2:0] DDR_ba, inout DDR_cas_n,
    inout DDR_ck_n, inout DDR_ck_p, inout DDR_cke, inout DDR_cs_n,
    inout [3:0] DDR_dm, inout [31:0] DDR_dq, inout [3:0] DDR_dqs_n,
    inout [3:0] DDR_dqs_p, inout DDR_odt, inout DDR_ras_n,
    inout DDR_reset_n, inout DDR_we_n,
    inout FIXED_IO_ddr_vrn, inout FIXED_IO_ddr_vrp,
    inout [53:0] FIXED_IO_mio, inout FIXED_IO_ps_clk,
    inout FIXED_IO_ps_porb, inout FIXED_IO_ps_srstb,
    output [48:0] txpins
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

assign txpins = {49{tx_bit}};
endmodule
