// ATOMiK UART Transmitter Module
// 115200 baud serial output for delta/motif data
// Target: Tang Nano 9K (Gowin GW1NR-9)

`timescale 1ns / 1ps

module uart_tx #(
    parameter CLK_FREQ = 27_000_000,  // 27 MHz system clock
    parameter BAUD_RATE = 115200      // Target baud rate
)(
    input  wire        clk,           // System clock
    input  wire        rst_n,         // Active-low reset
    input  wire [7:0]  data_in,       // 8-bit data to transmit
    input  wire        tx_start,      // Start transmission
    output reg         tx_line,       // Serial output line
    output reg         tx_busy,       // Transmitter is busy
    output reg         tx_done        // Transmission complete
);

    // Calculate baud rate divider
    localparam CLKS_PER_BIT = CLK_FREQ / BAUD_RATE;
    localparam CNT_WIDTH = $clog2(CLKS_PER_BIT);
    
    // State machine states
    localparam IDLE       = 3'b000;
    localparam START_BIT  = 3'b001;
    localparam DATA_BITS  = 3'b010;
    localparam STOP_BIT   = 3'b011;
    localparam CLEANUP    = 3'b100;
    
    reg [2:0] state;
    reg [CNT_WIDTH-1:0] clk_count;
    reg [2:0] bit_index;
    reg [7:0] tx_data;
    
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            tx_line <= 1'b1;  // Idle high
            tx_busy <= 1'b0;
            tx_done <= 1'b0;
            clk_count <= 0;
            bit_index <= 0;
            tx_data <= 8'b0;
        end else begin
            case (state)
                IDLE: begin
                    tx_line <= 1'b1;
                    tx_done <= 1'b0;
                    clk_count <= 0;
                    bit_index <= 0;
                    
                    if (tx_start) begin
                        tx_busy <= 1'b1;
                        tx_data <= data_in;
                        state <= START_BIT;
                    end else begin
                        tx_busy <= 1'b0;
                    end
                end
                
                START_BIT: begin
                    tx_line <= 1'b0;  // Start bit is low
                    
                    if (clk_count < CLKS_PER_BIT - 1) begin
                        clk_count <= clk_count + 1;
                    end else begin
                        clk_count <= 0;
                        state <= DATA_BITS;
                    end
                end
                
                DATA_BITS: begin
                    tx_line <= tx_data[bit_index];
                    
                    if (clk_count < CLKS_PER_BIT - 1) begin
                        clk_count <= clk_count + 1;
                    end else begin
                        clk_count <= 0;
                        
                        if (bit_index < 7) begin
                            bit_index <= bit_index + 1;
                        end else begin
                            bit_index <= 0;
                            state <= STOP_BIT;
                        end
                    end
                end
                
                STOP_BIT: begin
                    tx_line <= 1'b1;  // Stop bit is high
                    
                    if (clk_count < CLKS_PER_BIT - 1) begin
                        clk_count <= clk_count + 1;
                    end else begin
                        clk_count <= 0;
                        tx_done <= 1'b1;
                        state <= CLEANUP;
                    end
                end
                
                CLEANUP: begin
                    tx_busy <= 1'b0;
                    tx_done <= 1'b0;
                    state <= IDLE;
                end
                
                default: begin
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule


// Multi-byte UART transmitter for sending 64-bit delta words
module uart_tx_64 #(
    parameter CLK_FREQ = 27_000_000,
    parameter BAUD_RATE = 115200
)(
    input  wire        clk,
    input  wire        rst_n,
    input  wire [63:0] data_in,       // 64-bit data to transmit
    input  wire        tx_start,      // Start transmission
    output wire        tx_line,       // Serial output
    output reg         tx_busy,       // Transmitter is busy
    output reg         tx_done        // All 8 bytes transmitted
);

    // State machine
    localparam IDLE = 2'b00;
    localparam SEND = 2'b01;
    localparam WAIT = 2'b10;
    localparam DONE = 2'b11;
    
    reg [1:0] state;
    reg [2:0] byte_index;
    reg [63:0] data_reg;
    
    // Single-byte UART signals
    reg [7:0] byte_to_send;
    reg byte_start;
    wire byte_busy;
    wire byte_done;
    
    // Instantiate single-byte UART
    uart_tx #(
        .CLK_FREQ(CLK_FREQ),
        .BAUD_RATE(BAUD_RATE)
    ) uart_byte (
        .clk(clk),
        .rst_n(rst_n),
        .data_in(byte_to_send),
        .tx_start(byte_start),
        .tx_line(tx_line),
        .tx_busy(byte_busy),
        .tx_done(byte_done)
    );
    
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            byte_index <= 0;
            data_reg <= 64'b0;
            byte_to_send <= 8'b0;
            byte_start <= 1'b0;
            tx_busy <= 1'b0;
            tx_done <= 1'b0;
        end else begin
            case (state)
                IDLE: begin
                    tx_done <= 1'b0;
                    byte_start <= 1'b0;
                    
                    if (tx_start) begin
                        data_reg <= data_in;
                        byte_index <= 0;
                        tx_busy <= 1'b1;
                        state <= SEND;
                    end else begin
                        tx_busy <= 1'b0;
                    end
                end
                
                SEND: begin
                    // Send current byte (LSB first)
                    byte_to_send <= data_reg[7:0];
                    byte_start <= 1'b1;
                    state <= WAIT;
                end
                
                WAIT: begin
                    byte_start <= 1'b0;
                    
                    if (byte_done) begin
                        if (byte_index < 7) begin
                            // Shift to next byte
                            data_reg <= {8'b0, data_reg[63:8]};
                            byte_index <= byte_index + 1;
                            state <= SEND;
                        end else begin
                            state <= DONE;
                        end
                    end
                end
                
                DONE: begin
                    tx_done <= 1'b1;
                    tx_busy <= 1'b0;
                    state <= IDLE;
                end
                
                default: begin
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule
