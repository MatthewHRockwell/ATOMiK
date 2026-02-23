// Minimal hardware test for manual_uart_tx module
// Sends 0x55 continuously at 115200 baud (13.5 MHz clock)
module test_manual_uart_hardware (
    input clk,          // 27 MHz crystal
    output uart_tx,
    output debug_led    // LED to show we're running
);

// Clock divider (÷2 → 13.5 MHz)
wire clk_p;
Gowin_CLKDIV u_div_2 (
    .clkout(clk_p),
    .hclkin(clk),
    .resetn(1'b1)
);

// Reset
reg [7:0] reset_cnt = 0;
wire resetn = &reset_cnt;
always @(posedge clk_p) begin
    if (!resetn)
        reset_cnt <= reset_cnt + 1;
end

// State machine
reg [3:0] state = 0;
reg [31:0] counter = 0;

// UART interface signals
wire [31:0] reg_dat_do;
wire [31:0] reg_div_do;
wire reg_dat_wait;

reg [3:0] reg_div_we_r = 0;
reg [31:0] reg_div_di_r = 0;
reg reg_dat_we_r = 0;
reg [31:0] reg_dat_di_r = 0;

// Instantiate manual_uart_tx
manual_uart_tx u_uart (
    .clk(clk_p),
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

// Debug LED: ON when state >= 3 (transmitting)
assign debug_led = (state >= 3);

always @(posedge clk_p) begin
    if (!resetn) begin
        state <= 0;
        counter <= 0;
        reg_div_we_r <= 0;
        reg_dat_we_r <= 0;
    end else begin
        counter <= counter + 1;

        // Default: clear write enables
        reg_div_we_r <= 0;
        reg_dat_we_r <= 0;

        case (state)
            0: begin
                // Wait after reset
                if (counter > 1000) begin
                    state <= 1;
                    counter <= 0;
                end
            end

            1: begin
                // Write CLKDIV (13.5 MHz / 115200 - 2 = 115)
                reg_div_we_r <= 4'b1111;
                reg_div_di_r <= 115;
                state <= 2;
            end

            2: begin
                // Wait one cycle after CLKDIV write
                state <= 3;
                counter <= 0;
            end

            3: begin
                // Send 0x55 repeatedly
                if (!reg_dat_wait) begin
                    reg_dat_we_r <= 1;
                    reg_dat_di_r <= 8'h55;
                    counter <= 0;
                    state <= 4;
                end
            end

            4: begin
                // Wait for transmission to complete
                if (!reg_dat_wait) begin
                    // Ready for next byte - wait a bit then send another
                    if (counter > 1000) begin
                        state <= 3;
                        counter <= 0;
                    end
                end
            end

            default: state <= 0;
        endcase
    end
end

endmodule
