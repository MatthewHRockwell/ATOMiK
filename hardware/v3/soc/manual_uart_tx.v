// =============================================================================
// Manual UART TX Peripheral
//
// Drop-in replacement for simpleuart (TX only, no RX).
// Based on proven uart_test.v manual bitbanging approach.
//
// Interface compatible with simpleuart register layout:
//   - Write to reg_div_we → sets baud rate divider
//   - Write to reg_dat_we → triggers byte transmission
//   - reg_dat_wait → HIGH while transmitting (backpressure)
//
// Proven working on Tang Nano 9K hardware (uart_test.v validated).
// =============================================================================

module manual_uart_tx (
    input wire        clk,
    input wire        resetn,

    output wire       ser_tx,
    input wire        ser_rx,     // Unused (RX not implemented)

    // Divider register interface (compatible with simpleuart)
    input wire [3:0]  reg_div_we,
    input wire [31:0] reg_div_di,
    output wire[31:0] reg_div_do,

    // Data register interface (compatible with simpleuart)
    input wire        reg_dat_we,
    input wire        reg_dat_re,  // Unused (RX not implemented)
    input wire [31:0] reg_dat_di,
    output wire[31:0] reg_dat_do,
    output wire       reg_dat_wait
);

    // =========================================================================
    // Baud Rate Divider Register
    // =========================================================================
    reg [31:0] cfg_divider;

    always @(posedge clk) begin
        if (!resetn) begin
            cfg_divider <= 32'd1;  // Default (same as simpleuart)
        end else begin
            if (reg_div_we[0]) cfg_divider[7:0]   <= reg_div_di[7:0];
            if (reg_div_we[1]) cfg_divider[15:8]  <= reg_div_di[15:8];
            if (reg_div_we[2]) cfg_divider[23:16] <= reg_div_di[23:16];
            if (reg_div_we[3]) cfg_divider[31:24] <= reg_div_di[31:24];
        end
    end

    assign reg_div_do = cfg_divider;

    // =========================================================================
    // TX State Machine
    // Based on proven uart_test.v manual bitbanging approach
    // =========================================================================
    reg [31:0] bit_counter;   // Counts clocks per bit (0 to cfg_divider)
    reg [3:0]  bit_index;     // Which bit we're sending (0-10)
    reg [7:0]  tx_data;       // Latched byte to transmit
    reg        tx_active;     // Transmission in progress
    reg        tx_shift_reg;  // Current bit being transmitted

    // UART TX output (idles HIGH)
    assign ser_tx = tx_shift_reg;

    // Busy flag (backpressure)
    assign reg_dat_wait = tx_active;

    // RX not implemented
    assign reg_dat_do = 32'hFFFFFFFF;

    always @(posedge clk) begin
        if (!resetn) begin
            bit_counter   <= 0;
            bit_index     <= 0;
            tx_data       <= 0;
            tx_active     <= 0;
            tx_shift_reg  <= 1'b1;  // UART idle is HIGH
        end else begin

            // ================================================================
            // State: IDLE - waiting for data write
            // ================================================================
            if (!tx_active) begin
                tx_shift_reg <= 1'b1;  // Keep TX line HIGH (idle)

                if (reg_dat_we) begin
                    // Latch data and start transmission
                    tx_data      <= reg_dat_di[7:0];
                    tx_active    <= 1'b1;
                    bit_index    <= 0;
                    bit_counter  <= 0;
                    tx_shift_reg <= 1'b0;  // Start bit (LOW)
                end
            end

            // ================================================================
            // State: TRANSMITTING - shift out bits at baud rate
            // ================================================================
            else begin
                if (bit_counter >= cfg_divider) begin
                    // Time to move to next bit
                    bit_counter <= 0;

                    if (bit_index < 10) begin
                        bit_index <= bit_index + 1;

                        // Shift to next bit
                        case (bit_index)
                            0: tx_shift_reg <= tx_data[0];  // Data bit 0
                            1: tx_shift_reg <= tx_data[1];  // Data bit 1
                            2: tx_shift_reg <= tx_data[2];  // Data bit 2
                            3: tx_shift_reg <= tx_data[3];  // Data bit 3
                            4: tx_shift_reg <= tx_data[4];  // Data bit 4
                            5: tx_shift_reg <= tx_data[5];  // Data bit 5
                            6: tx_shift_reg <= tx_data[6];  // Data bit 6
                            7: tx_shift_reg <= tx_data[7];  // Data bit 7
                            8: tx_shift_reg <= 1'b1;        // Stop bit (HIGH)
                            9: tx_shift_reg <= 1'b1;        // Idle (HIGH)
                            default: tx_shift_reg <= 1'b1;
                        endcase
                    end else begin
                        // Transmission complete
                        tx_active    <= 0;
                        bit_index    <= 0;
                        tx_shift_reg <= 1'b1;  // Return to idle
                    end

                end else begin
                    // Count clocks for current bit
                    bit_counter <= bit_counter + 1;
                end
            end
        end
    end

endmodule
