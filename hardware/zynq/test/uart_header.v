module uart_header(
    inout  [14:0] DDR_addr,
    inout  [2:0]  DDR_ba,
    inout         DDR_cas_n,
    inout         DDR_ck_n,
    inout         DDR_ck_p,
    inout         DDR_cke,
    inout         DDR_cs_n,
    inout  [3:0]  DDR_dm,
    inout  [31:0] DDR_dq,
    inout  [3:0]  DDR_dqs_n,
    inout  [3:0]  DDR_dqs_p,
    inout         DDR_odt,
    inout         DDR_ras_n,
    inout         DDR_reset_n,
    inout         DDR_we_n,
    inout         FIXED_IO_ddr_vrn,
    inout         FIXED_IO_ddr_vrp,
    inout  [53:0] FIXED_IO_mio,
    inout         FIXED_IO_ps_clk,
    inout         FIXED_IO_ps_porb,
    inout         FIXED_IO_ps_srstb,
    output        uart_tx,
    output [3:0]  leds
);

wire fclk;

// PS7 block for FCLK
blink_sys_wrapper u_sys (
    .DDR_addr(DDR_addr), .DDR_ba(DDR_ba), .DDR_cas_n(DDR_cas_n),
    .DDR_ck_n(DDR_ck_n), .DDR_ck_p(DDR_ck_p), .DDR_cke(DDR_cke),
    .DDR_cs_n(DDR_cs_n), .DDR_dm(DDR_dm), .DDR_dq(DDR_dq),
    .DDR_dqs_n(DDR_dqs_n), .DDR_dqs_p(DDR_dqs_p), .DDR_odt(DDR_odt),
    .DDR_ras_n(DDR_ras_n), .DDR_reset_n(DDR_reset_n), .DDR_we_n(DDR_we_n),
    .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn), .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
    .FIXED_IO_mio(FIXED_IO_mio), .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
    .FIXED_IO_ps_porb(FIXED_IO_ps_porb), .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
    .fclk_clk0(fclk)
);

// Heartbeat counter on FCLK (50 MHz)
reg [31:0] cnt = 0;
always @(posedge fclk) cnt <= cnt + 1;

// Visual heartbeat on 4 header pins
assign leds[0] = cnt[23];
assign leds[1] = cnt[24];
assign leds[2] = cnt[25];
assign leds[3] = cnt[22];

// Simple UART TX — sends "ATOMiK\r\n" every ~1 second
// 115200 baud at 50 MHz: divider = 434
localparam BAUD_DIV = 434;
localparam MSG_LEN = 10;

reg [8:0] baud_cnt = 0;
reg [3:0] bit_cnt = 0;
reg [3:0] msg_idx = 0;
reg [9:0] shift_reg = 10'h3FF; // idle HIGH
reg [23:0] pause_cnt = 0;
reg sending = 0;

wire [7:0] msg_char;
assign msg_char = (msg_idx == 0) ? "A" :
                  (msg_idx == 1) ? "T" :
                  (msg_idx == 2) ? "O" :
                  (msg_idx == 3) ? "M" :
                  (msg_idx == 4) ? "i" :
                  (msg_idx == 5) ? "K" :
                  (msg_idx == 6) ? " " :
                  (msg_idx == 7) ? "O" :
                  (msg_idx == 8) ? "K" :
                  (msg_idx == 9) ? 8'h0A : 8'h00;

assign uart_tx = shift_reg[0];

always @(posedge fclk) begin
    if (!sending) begin
        pause_cnt <= pause_cnt + 1;
        if (pause_cnt == 0) begin // ~0.3 sec at 50 MHz
            sending <= 1;
            msg_idx <= 0;
            bit_cnt <= 0;
            baud_cnt <= 0;
            shift_reg <= {1'b1, msg_char, 1'b0}; // stop + data + start
        end
    end else begin
        if (baud_cnt == BAUD_DIV - 1) begin
            baud_cnt <= 0;
            shift_reg <= {1'b1, shift_reg[9:1]}; // shift right
            if (bit_cnt == 9) begin
                bit_cnt <= 0;
                if (msg_idx == MSG_LEN - 1) begin
                    sending <= 0;
                    shift_reg <= 10'h3FF;
                end else begin
                    msg_idx <= msg_idx + 1;
                    shift_reg <= {1'b1, msg_char, 1'b0};
                end
            end else begin
                bit_cnt <= bit_cnt + 1;
            end
        end else begin
            baud_cnt <= baud_cnt + 1;
        end
    end
end

endmodule
