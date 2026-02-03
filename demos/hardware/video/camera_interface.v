// =============================================================================
// ATOMiK Video Demo - Camera / Frame Source Interface
//
// Module:      camera_interface
// Description: Simulated camera frame source for the video domain demo.
//              Generates synthetic 256-bit frame delta values at a
//              configurable rate.  When a real camera is connected via DVP
//              (cam_vsync, cam_href, cam_data), the module could be extended
//              to ingest actual pixel data.  In stand-alone mode it produces
//              deterministic test patterns for verification.
//
// Pattern Generation:
//   - Frame counter increments on every frame tick
//   - Delta value is derived from the frame counter via a simple LFSR-style
//     expansion so that successive deltas are non-trivial and non-zero
//   - cam_ready pulses high for one clock cycle when a new delta is available
//
// Target: Gowin GW1NR-LV9QN88C6/I5 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   January 27, 2026
// =============================================================================

`timescale 1ns / 1ps

module camera_interface #(
    parameter DATA_WIDTH = 256,
    parameter FRAME_DIV  = 27_000   // Clock divider for frame rate
                                    // 27 MHz / 27000 = 1000 fps (demo)
)(
    // =========================================================================
    // Clock and Reset
    // =========================================================================
    input  wire                    clk,
    input  wire                    rst_n,

    // =========================================================================
    // Camera DVP Signals (directly mapped, directly active when connected)
    // =========================================================================
    input  wire                    cam_vsync,
    input  wire                    cam_href,
    input  wire [7:0]              cam_data,

    // =========================================================================
    // Frame Delta Output
    // =========================================================================
    output reg  [DATA_WIDTH-1:0]   frame_delta,
    output reg                     cam_ready
);

    // =========================================================================
    // Frame Rate Divider
    // =========================================================================
    reg [31:0] div_cnt;
    reg        frame_tick;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            div_cnt    <= 32'd0;
            frame_tick <= 1'b0;
        end
        else begin
            frame_tick <= 1'b0;
            if (div_cnt >= FRAME_DIV - 1) begin
                div_cnt    <= 32'd0;
                frame_tick <= 1'b1;
            end
            else begin
                div_cnt <= div_cnt + 32'd1;
            end
        end
    end

    // =========================================================================
    // Frame Counter
    // =========================================================================
    reg [31:0] frame_count;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            frame_count <= 32'd0;
        else if (frame_tick)
            frame_count <= frame_count + 32'd1;
    end

    // =========================================================================
    // Synthetic Delta Pattern Generator
    // =========================================================================
    // Expands the 32-bit frame counter into a DATA_WIDTH-bit delta value.
    // Uses XOR-shift mixing to produce non-trivial, non-zero patterns that
    // exercise the full data width of the ATOMiK core.
    //
    // Pattern:  delta = { fc ^ (fc <<  5),
    //                     fc ^ (fc >> 3),
    //                     fc ^ (fc << 11),
    //                     fc ^ (fc >> 7),
    //                     fc ^ (fc << 2),
    //                     fc ^ (fc >> 13),
    //                     fc ^ (fc << 17),
    //                     fc ^ (fc >> 1) }
    // where fc is the current frame_count value (each slice is 32 bits,
    // yielding 256 bits total for DATA_WIDTH=256).

    wire [31:0] fc;
    assign fc = frame_count;

    wire [DATA_WIDTH-1:0] pattern;
    assign pattern = { fc ^ (fc << 5),
                       fc ^ (fc >> 3),
                       fc ^ (fc << 11),
                       fc ^ (fc >> 7),
                       fc ^ (fc << 2),
                       fc ^ (fc >> 13),
                       fc ^ (fc << 17),
                       fc ^ (fc >> 1) };

    // =========================================================================
    // Output Register
    // =========================================================================
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            frame_delta <= {DATA_WIDTH{1'b0}};
            cam_ready   <= 1'b0;
        end
        else begin
            cam_ready <= 1'b0;

            if (frame_tick) begin
                frame_delta <= pattern;
                cam_ready   <= 1'b1;
            end
        end
    end

    // =========================================================================
    // Future Extension Point
    // =========================================================================
    // When a real camera is connected, cam_vsync rising edge can trigger
    // frame capture.  The cam_href / cam_data bus would shift in pixel bytes
    // and accumulate them into a DATA_WIDTH-bit register.  A simple way to
    // bridge the 8-bit bus to 256 bits is to XOR each incoming byte into a
    // rotating position within the delta register, producing a hash-like
    // frame delta from the actual pixel stream.

endmodule
