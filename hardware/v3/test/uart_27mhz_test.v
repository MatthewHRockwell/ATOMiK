// Test with 27 MHz direct clock (no CLKDIV) to isolate clock issue
module uart_27mhz_test (
    input clk,          // 27 MHz direct
    output uart_tx
);

reg [7:0] reset_cnt = 0;
wire resetn = &reset_cnt;
always @(posedge clk) begin
    if (!resetn)
        reset_cnt <= reset_cnt + 1;
end

// State machine (same as uart_nowrapper_test but with 27 MHz timing)
reg [3:0] state = 0;
reg [31:0] counter = 0;

wire [31:0] reg_dat_do;
wire [31:0] reg_div_do;
wire reg_dat_wait;

reg [3:0] reg_div_we_r = 0;
reg [31:0] reg_div_di_r = 0;
reg reg_dat_we_r = 0;
reg [31:0] reg_dat_di_r = 0;

simpleuart u_uart (
    .clk(clk),          // 27 MHz direct
    .resetn(resetn),
    .ser_tx(uart_tx),
    .ser_rx(1'b1),

    .reg_div_we(reg_div_we_r),
    .reg_div_di(reg_div_di_r),
    .reg_div_do(reg_div_do),

    .reg_dat_we(reg_dat_we_r),
    .reg_dat_re(1'b0),
    .reg_dat_di(reg_dat_di_r),
    .reg_dat_do(reg_dat_do),
    .reg_dat_wait(reg_dat_wait)
);

always @(posedge clk) begin
    if (!resetn) begin
        state <= 0;
        counter <= 0;
        reg_div_we_r <= 0;
        reg_dat_we_r <= 0;
    end else begin
        counter <= counter + 1;

        // Default: no writes
        reg_div_we_r <= 0;
        reg_dat_we_r <= 0;

        case (state)
            0: begin
                if (counter > 1000) begin
                    state <= 1;
                    counter <= 0;
                end
            end

            1: begin
                // Write CLKDIV for 27 MHz / 115200 = 234
                reg_div_we_r <= 4'b1111;
                reg_div_di_r <= 232;  // 234 - 2
                state <= 2;
            end

            2: begin
                // Wait for 15 dummy bits
                if (counter > 4000) begin
                    state <= 3;
                    counter <= 0;
                end
            end

            3: begin
                // Send 'U' (0x55)
                if (!reg_dat_wait) begin
                    reg_dat_we_r <= 1;
                    reg_dat_di_r <= 8'h55;
                    counter <= 0;
                end else if (counter > 5000) begin
                    state <= 4;  // Error
                end
            end

            4: begin
                // Error state
            end
        endcase
    end
end

endmodule
