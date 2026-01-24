// ATOMiK Delta Core Module
// 64-bit XOR-based stateless delta computation engine
// Target: Tang Nano 9K (Gowin GW1NR-9)

`timescale 1ns / 1ps

module delta_core (
    input  wire        clk,           // 27 MHz system clock
    input  wire        rst_n,         // Active-low reset
    input  wire        enable,        // Processing enable
    input  wire [63:0] prev_word,     // Previous 64-bit voxel word
    input  wire [63:0] curr_word,     // Current 64-bit voxel word
    input  wire        words_valid,   // Input words are valid
    output reg  [63:0] delta_word,    // XOR delta output
    output reg         event_valid,   // Delta represents a change event
    output reg         output_ready   // Output is ready to be consumed
);

    // Internal state machine
    localparam IDLE    = 2'b00;
    localparam COMPUTE = 2'b01;
    localparam OUTPUT  = 2'b10;
    
    reg [1:0] state;
    reg [63:0] delta_reg;
    
    // Bit population counter for event detection
    wire [6:0] popcount;
    
    // XOR computation (purely combinational)
    wire [63:0] xor_result = prev_word ^ curr_word;
    
    // Population count using tree reduction
    // Count bits in delta to determine event significance
    function [6:0] count_ones;
        input [63:0] data;
        integer i;
        reg [6:0] count;
        begin
            count = 0;
            for (i = 0; i < 64; i = i + 1) begin
                count = count + data[i];
            end
            count_ones = count;
        end
    endfunction
    
    assign popcount = count_ones(delta_reg);
    
    // Main state machine
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            delta_word <= 64'b0;
            delta_reg <= 64'b0;
            event_valid <= 1'b0;
            output_ready <= 1'b0;
        end else begin
            case (state)
                IDLE: begin
                    output_ready <= 1'b0;
                    event_valid <= 1'b0;
                    
                    if (enable && words_valid) begin
                        // Compute XOR immediately
                        delta_reg <= xor_result;
                        state <= COMPUTE;
                    end
                end
                
                COMPUTE: begin
                    // Determine if this delta represents a significant event
                    // Threshold: more than 4 bits changed
                    if (popcount > 7'd4) begin
                        event_valid <= 1'b1;
                    end else begin
                        event_valid <= 1'b0;
                    end
                    
                    delta_word <= delta_reg;
                    output_ready <= 1'b1;
                    state <= OUTPUT;
                end
                
                OUTPUT: begin
                    // Hold output for one cycle, then return to idle
                    output_ready <= 1'b0;
                    event_valid <= 1'b0;
                    state <= IDLE;
                end
                
                default: begin
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule
