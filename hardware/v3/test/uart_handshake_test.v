// Test with CORRECT bus handshake protocol
// Hold reg_dat_we HIGH until peripheral accepts, rather than single-cycle pulse
module uart_handshake_test (
    input clk,          // 27 MHz direct
    output uart_tx,
    output [4:0] gpio,        // GPIO pins 25-29 for state debug
    output gpio_heartbeat,    // Pin 31: Toggles on FSM progress
    output gpio_uart_tap      // Pin 32: Mirrors internal uart_tx before pad
);

reg [7:0] reset_cnt = 0;
wire resetn = &reset_cnt;
always @(posedge clk) begin
    if (!resetn)
        reset_cnt <= reset_cnt + 1;
end

// State machine with proper handshake
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
    .clk(clk),
    .resetn(resetn),
    .ser_tx(uart_tx_internal),  // Connect to internal signal
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

// Debug outputs
wire uart_tx_internal;  // Internal uart_tx signal before pad

assign gpio = {1'b0, state[3:0]};  // Show FSM state
assign uart_tx = uart_tx_internal;  // Route to pad
assign gpio_uart_tap = uart_tx_internal;  // Tap internal signal to GPIO

// Heartbeat: toggles every time we advance state or write to UART
reg heartbeat_reg = 0;
always @(posedge clk) begin
    if (!resetn)
        heartbeat_reg <= 0;
    else if (state != 0)  // Toggle whenever FSM is active
        heartbeat_reg <= ~heartbeat_reg;
end
assign gpio_heartbeat = heartbeat_reg;

always @(posedge clk) begin
    if (!resetn) begin
        state <= 0;
        counter <= 0;
        reg_div_we_r <= 0;
        reg_dat_we_r <= 0;
    end else begin
        counter <= counter + 1;

        case (state)
            // State 0: Wait after reset (eliminate first-cycle hazards)
            0: begin
                reg_div_we_r <= 0;
                reg_dat_we_r <= 0;
                if (counter > 1000) begin
                    state <= 1;
                    counter <= 0;
                end
            end

            // State 1: Assert CLKDIV write (sticky until accepted)
            1: begin
                reg_div_we_r <= 4'b1111;
                reg_div_di_r <= 232;  // 27 MHz / 115200 - 2 = 232
                state <= 2;
            end

            // State 2: Hold CLKDIV write, wait for acceptance
            // For CLKDIV, reg_dat_wait doesn't apply - just hold for 1 cycle
            2: begin
                reg_div_we_r <= 0;  // Deassert after 1 cycle
                counter <= 0;
                state <= 3;
            end

            // State 3: Wait after CLKDIV write (let send_dummy complete)
            3: begin
                if (counter > 4000) begin
                    state <= 4;
                    counter <= 0;
                end
            end

            // State 4: Assert DATA write (sticky)
            4: begin
                reg_dat_we_r <= 1;
                reg_dat_di_r <= 8'h55;
                state <= 5;
            end

            // State 5: Wait for UART to be ready (reg_dat_wait LOW)
            // Then data will be latched on THIS cycle
            5: begin
                if (!reg_dat_wait) begin
                    // UART is ready, data latches this cycle
                    // Stay in this state with reg_dat_we HIGH for one more cycle
                    reg_dat_we_r <= 1;
                    state <= 6;
                end else begin
                    // UART busy, keep holding
                    reg_dat_we_r <= 1;
                end
            end

            // State 6: Data was latched, now wait for ack (reg_dat_wait HIGH)
            6: begin
                if (reg_dat_wait) begin
                    // Acknowledged! Now we can deassert
                    reg_dat_we_r <= 0;
                    state <= 7;
                    counter <= 0;
                end else begin
                    // Still waiting for ack
                    reg_dat_we_r <= 1;
                end
            end

            // State 7: Wait between transmissions
            7: begin
                if (counter > 5000) begin
                    state <= 4;  // Send another byte
                    counter <= 0;
                end
            end

            default: state <= 0;
        endcase
    end
end

endmodule
