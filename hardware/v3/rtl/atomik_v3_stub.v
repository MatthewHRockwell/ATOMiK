// =============================================================================
// ATOMiK v3 Stub Module
//
// Module:      atomik_v3_stub
// Description: Minimal parameterized XOR accumulator stub for toolchain
//              verification. Exercises v3 conventions (64-bit default,
//              synchronous reset, delta_valid handshake).
//
//              This module will be replaced by real v3 RTL in Phase 1/2.
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_v3_stub #(
    parameter DW = 64  // Delta/state width (bits)
)(
    input  wire            clk,
    input  wire            rst_n,

    // Delta accumulation interface
    input  wire [DW-1:0]   delta_in,
    input  wire            delta_valid,

    // State output
    output reg  [DW-1:0]   accumulator,
    output wire            accumulator_zero
);

    // Accumulator zero detection
    assign accumulator_zero = (accumulator == {DW{1'b0}});

    // XOR accumulation with synchronous active-low reset
    always @(posedge clk) begin
        if (!rst_n) begin
            accumulator <= {DW{1'b0}};
        end else if (delta_valid) begin
            accumulator <= accumulator ^ delta_in;
        end
    end

endmodule
