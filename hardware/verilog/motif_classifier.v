// ATOMiK Motif Classifier Module
// Pattern matching LUT for delta classification
// Target: Tang Nano 9K (Gowin GW1NR-9)

`timescale 1ns / 1ps

module motif_classifier (
    input  wire        clk,           // 27 MHz system clock
    input  wire        rst_n,         // Active-low reset
    input  wire [63:0] delta,         // 64-bit delta word input
    input  wire        delta_valid,   // Delta input is valid
    output reg  [3:0]  motif_id,      // 4-bit motif identifier
    output reg         motif_valid    // Motif output is valid
);

    // Motif definitions (4-bit identifiers)
    localparam MOTIF_STATIC      = 4'h0;
    localparam MOTIF_HORIZONTAL  = 4'h1;
    localparam MOTIF_VERTICAL    = 4'h2;
    localparam MOTIF_DIAGONAL_NE = 4'h3;
    localparam MOTIF_DIAGONAL_NW = 4'h4;
    localparam MOTIF_DIAGONAL_SE = 4'h5;
    localparam MOTIF_DIAGONAL_SW = 4'h6;
    localparam MOTIF_EXPANSION   = 4'h7;
    localparam MOTIF_CONTRACTION = 4'h8;
    localparam MOTIF_ROTATION_CW = 4'h9;
    localparam MOTIF_ROTATION_CC = 4'hA;
    localparam MOTIF_FLICKER     = 4'hB;
    localparam MOTIF_EDGE_APPEAR = 4'hC;
    localparam MOTIF_EDGE_DISAPP = 4'hD;
    localparam MOTIF_NOISE       = 4'hE;
    localparam MOTIF_RESERVED    = 4'hF;
    
    // Bit population count
    wire [6:0] bit_count;
    
    // Split delta into regions for analysis
    wire [31:0] upper_half = delta[63:32];
    wire [31:0] lower_half = delta[31:0];
    wire [6:0] upper_count;
    wire [6:0] lower_count;
    
    // Population count function
    function [6:0] popcount32;
        input [31:0] data;
        integer i;
        reg [6:0] count;
        begin
            count = 0;
            for (i = 0; i < 32; i = i + 1) begin
                count = count + data[i];
            end
            popcount32 = count;
        end
    endfunction
    
    assign upper_count = popcount32(upper_half);
    assign lower_count = popcount32(lower_half);
    assign bit_count = upper_count + lower_count;
    
    // Classification logic
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            motif_id <= MOTIF_STATIC;
            motif_valid <= 1'b0;
        end else begin
            if (delta_valid) begin
                motif_valid <= 1'b1;
                
                // Classification decision tree
                if (delta == 64'b0) begin
                    // No change - static
                    motif_id <= MOTIF_STATIC;
                end
                else if (bit_count < 7'd4) begin
                    // Very few bits changed - noise
                    motif_id <= MOTIF_NOISE;
                end
                else if (bit_count > 7'd48) begin
                    // Many bits changed - expansion/contraction
                    if (upper_count > lower_count) begin
                        motif_id <= MOTIF_EXPANSION;
                    end else begin
                        motif_id <= MOTIF_CONTRACTION;
                    end
                end
                else begin
                    // Check for directional patterns
                    if ((upper_count > lower_count + 7'd8)) begin
                        motif_id <= MOTIF_VERTICAL;
                    end
                    else if ((lower_count > upper_count + 7'd8)) begin
                        motif_id <= MOTIF_VERTICAL;
                    end
                    else begin
                        // Check horizontal patterns by byte variance
                        if (check_horizontal_pattern(delta)) begin
                            motif_id <= MOTIF_HORIZONTAL;
                        end
                        else if (bit_count > 7'd24) begin
                            motif_id <= MOTIF_FLICKER;
                        end
                        else begin
                            motif_id <= MOTIF_DIAGONAL_NE;  // Default diagonal
                        end
                    end
                end
            end else begin
                motif_valid <= 1'b0;
            end
        end
    end
    
    // Function to check for horizontal motion patterns
    function check_horizontal_pattern;
        input [63:0] data;
        reg [3:0] byte_counts [0:7];
        reg [6:0] variance;
        integer i;
        begin
            // Count bits in each byte
            for (i = 0; i < 8; i = i + 1) begin
                byte_counts[i] = popcount32({24'b0, data[i*8 +: 8]});
            end
            
            // Check if there's high variance between bytes
            // (indicating horizontal motion pattern)
            variance = 0;
            for (i = 0; i < 7; i = i + 1) begin
                if (byte_counts[i] > byte_counts[i+1]) begin
                    variance = variance + (byte_counts[i] - byte_counts[i+1]);
                end else begin
                    variance = variance + (byte_counts[i+1] - byte_counts[i]);
                end
            end
            
            check_horizontal_pattern = (variance > 7'd16);
        end
    endfunction

endmodule
