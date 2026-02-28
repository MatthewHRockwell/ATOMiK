// Direct simpleuart test - bypass all SoC logic
// This test directly instantiates simpleuart and drives it with
// a simple state machine to isolate the UART module behavior
module uart_direct_test (
    input clk,          // 27 MHz crystal
    output uart_tx
);

wire clk_p;
Gowin_CLKDIV u_div_2 (
    .clkout(clk_p),
    .hclkin(clk),
    .resetn(1'b1)
);

// Reset generator
reg [7:0] reset_cnt = 0;
wire resetn = &reset_cnt;
always @(posedge clk_p) begin
    if (!resetn)
        reset_cnt <= reset_cnt + 1;
end

// State machine to configure and send data
reg [3:0] state = 0;
reg [31:0] counter = 0;

wire [31:0] reg_div_do;
wire [31:0] reg_dat_do;
wire reg_dat_wait;

reg [3:0] reg_div_we_r = 0;
reg [31:0] reg_div_di_r = 0;
reg reg_dat_we_r = 0;
reg [31:0] reg_dat_di_r = 0;

simpleuart u_uart (
    .clk(clk_p),
    .resetn(resetn),
    .ser_tx(uart_tx),
    .ser_rx(1'b1),  // Tie high (idle)

    .reg_div_we(reg_div_we_r),
    .reg_div_di(reg_div_di_r),
    .reg_div_do(reg_div_do),

    .reg_dat_we(reg_dat_we_r),
    .reg_dat_re(1'b0),
    .reg_dat_di(reg_dat_di_r),
    .reg_dat_do(reg_dat_do),
    .reg_dat_wait(reg_dat_wait)
);

always @(posedge clk_p) begin
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
                // Wait for stable reset
                if (counter > 1000) begin
                    state <= 1;
                    counter <= 0;
                end
            end

            1: begin
                // Write CLKDIV
                // 13.5 MHz / 115200 - 2 = 115
                reg_div_we_r <= 4'b1111;
                reg_div_di_r <= 115;
                state <= 2;
            end

            2: begin
                // Wait for 15 dummy bits to complete
                // 15 bits * 116 cycles/bit = 1740 cycles
                if (counter > 2000) begin
                    state <= 3;
                    counter <= 0;
                end
            end

            3: begin
                // Send 'U' (0x55) continuously
                if (!reg_dat_wait) begin
                    reg_dat_we_r <= 1;
                    reg_dat_di_r <= 8'h55;
                    counter <= 0;
                end else if (counter > 2000) begin
                    // Timeout waiting for UART ready - something wrong
                    state <= 4;  // Error state
                end
            end

            4: begin
                // Error state - do nothing
            end
        endcase
    end
end

endmodule
