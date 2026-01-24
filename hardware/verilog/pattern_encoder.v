// ATOMiK Pattern Encoder Module
// 4-bit pattern generation from 4 temporal frames
// Target: Tang Nano 9K (Gowin GW1NR-9)

`timescale 1ns / 1ps

module pattern_encoder (
    input  wire        clk,           // 27 MHz system clock
    input  wire        rst_n,         // Active-low reset
    input  wire [3:0]  tile_bits,     // 4 binary values (one per frame)
    input  wire        input_valid,   // Input bits are valid
    output reg  [3:0]  pattern_id,    // 4-bit pattern identifier
    output reg         pattern_valid  // Pattern output is valid
);

    // Pattern encoding: direct mapping from 4 temporal bits
    // The 4-bit pattern captures the temporal signature of the tile
    // across 4 consecutive frames.
    //
    // Pattern meanings (example interpretations):
    // 0000 (0x0): Static dark
    // 1111 (0xF): Static bright
    // 0001 (0x1): Appearing edge
    // 1000 (0x8): Disappearing edge
    // 0011 (0x3): Rising transition
    // 1100 (0xC): Falling transition
    // 0101 (0x5): Flicker pattern A
    // 1010 (0xA): Flicker pattern B
    
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pattern_id <= 4'b0;
            pattern_valid <= 1'b0;
        end else begin
            if (input_valid) begin
                // Direct mapping: tile_bits become pattern_id
                pattern_id <= tile_bits;
                pattern_valid <= 1'b1;
            end else begin
                pattern_valid <= 1'b0;
            end
        end
    end

endmodule


// Pattern Accumulator - collects tile bits across frames
module pattern_accumulator (
    input  wire        clk,
    input  wire        rst_n,
    input  wire        binary_in,      // Single binary tile value
    input  wire        input_valid,    // Binary input is valid
    input  wire        frame_sync,     // New frame signal
    output reg  [3:0]  pattern_bits,   // Accumulated 4 bits
    output reg         pattern_ready   // 4 frames accumulated
);

    reg [1:0] frame_count;  // Which frame (0-3)
    reg [3:0] accumulator;
    
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            frame_count <= 2'b0;
            accumulator <= 4'b0;
            pattern_bits <= 4'b0;
            pattern_ready <= 1'b0;
        end else begin
            if (frame_sync) begin
                // Advance to next frame slot
                if (frame_count == 2'd3) begin
                    // Output complete pattern
                    pattern_bits <= accumulator;
                    pattern_ready <= 1'b1;
                    frame_count <= 2'b0;
                    accumulator <= 4'b0;
                end else begin
                    frame_count <= frame_count + 1;
                    pattern_ready <= 1'b0;
                end
            end else if (input_valid) begin
                // Store binary value in appropriate slot
                accumulator[frame_count] <= binary_in;
                pattern_ready <= 1'b0;
            end else begin
                pattern_ready <= 1'b0;
            end
        end
    end

endmodule
