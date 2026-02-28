// Manual UART test - sends 0x55 (01010101) continuously at 115200 baud
module uart_test (
    input clk,          // 27 MHz
    output reg uart_tx
);

// 27 MHz / 115200 baud = 234.375 cycles per bit
// Use 234 cycles per bit
reg [7:0] bit_counter;
reg [3:0] bit_index;
reg [7:0] data_byte;

initial begin
    uart_tx = 1'b1;  // UART idle is high
    bit_counter = 0;
    bit_index = 0;
    data_byte = 8'h55;  // Alternating pattern - easy to see on scope/serial
end

always @(posedge clk) begin
    if (bit_counter >= 234) begin
        bit_counter <= 0;

        // Send start bit (0), 8 data bits, stop bit (1)
        case (bit_index)
            0: uart_tx <= 1'b0;  // Start bit
            1: uart_tx <= data_byte[0];
            2: uart_tx <= data_byte[1];
            3: uart_tx <= data_byte[2];
            4: uart_tx <= data_byte[3];
            5: uart_tx <= data_byte[4];
            6: uart_tx <= data_byte[5];
            7: uart_tx <= data_byte[6];
            8: uart_tx <= data_byte[7];
            9: uart_tx <= 1'b1;  // Stop bit
            default: uart_tx <= 1'b1;  // Idle
        endcase

        if (bit_index >= 10) begin
            bit_index <= 0;  // Restart after stop bit + idle
        end else begin
            bit_index <= bit_index + 1;
        end
    end else begin
        bit_counter <= bit_counter + 1;
    end
end

endmodule
