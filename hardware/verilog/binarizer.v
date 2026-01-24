// ATOMiK Binarizer Module
// Tile-energy thresholding for 25x25 grid binarization
// Target: Tang Nano 9K (Gowin GW1NR-9)

`timescale 1ns / 1ps

module binarizer (
    input  wire        clk,           // 27 MHz system clock
    input  wire        rst_n,         // Active-low reset
    input  wire [7:0]  pixel_value,   // 8-bit grayscale pixel input
    input  wire [7:0]  tile_mean,     // 8-bit tile mean (threshold)
    input  wire        pixel_valid,   // Input pixel is valid
    output reg         binary_out,    // 1-bit binarized output
    output reg         output_valid   // Output is valid
);

    // Binarization threshold comparison
    // Output 1 if pixel > tile_mean, else 0
    
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            binary_out <= 1'b0;
            output_valid <= 1'b0;
        end else begin
            if (pixel_valid) begin
                // Simple threshold comparison
                binary_out <= (pixel_value > tile_mean);
                output_valid <= 1'b1;
            end else begin
                output_valid <= 1'b0;
            end
        end
    end

endmodule


// Tile Mean Calculator - computes running average for threshold
module tile_mean_calculator #(
    parameter TILE_SIZE = 16  // 4x4 = 16 pixels per tile
)(
    input  wire        clk,
    input  wire        rst_n,
    input  wire [7:0]  pixel_value,
    input  wire        pixel_valid,
    input  wire        tile_start,     // Start of new tile
    output reg  [7:0]  tile_mean,
    output reg         mean_valid
);

    reg [11:0] accumulator;  // Sum of pixel values (max 16*255 = 4080)
    reg [4:0]  pixel_count;  // Count of pixels in current tile
    
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            accumulator <= 12'b0;
            pixel_count <= 5'b0;
            tile_mean <= 8'd128;  // Default threshold
            mean_valid <= 1'b0;
        end else begin
            if (tile_start) begin
                // Reset for new tile
                accumulator <= 12'b0;
                pixel_count <= 5'b0;
                mean_valid <= 1'b0;
            end else if (pixel_valid) begin
                accumulator <= accumulator + {4'b0, pixel_value};
                pixel_count <= pixel_count + 1;
                
                if (pixel_count == TILE_SIZE - 1) begin
                    // Compute mean (divide by TILE_SIZE = 16, i.e., shift right by 4)
                    tile_mean <= accumulator[11:4];
                    mean_valid <= 1'b1;
                end else begin
                    mean_valid <= 1'b0;
                end
            end else begin
                mean_valid <= 1'b0;
            end
        end
    end

endmodule
