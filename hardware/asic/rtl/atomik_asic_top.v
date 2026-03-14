// =============================================================================
// ATOMiK ASIC Top-Level — Vendor-Neutral Chip Wrapper
//
// Module:      atomik_asic_top
// Description: Top-level ASIC wrapper for ATOMiK IP core. Contains:
//              - Clock and reset infrastructure (external clock, no PLL)
//              - ATOMiK core with generic SRAM
//              - BIST controller for production test
//              - DFT scan chain wrapper
//              - Simple bus interface for integration
//
//              This module is the synthesis target for ASIC flows.
//              No vendor-specific primitives — pure behavioral Verilog.
//
// Clock Strategy:
//   External clock input (no on-chip PLL). The integrator provides the
//   clock at the target frequency. This simplifies ASIC design and avoids
//   PLL analog IP licensing costs for initial tapeout.
//
// Reset Strategy:
//   Asynchronous assert, synchronous deassert. 4-stage synchronizer
//   ensures clean reset release on clock edge.
//
// ATOMiK Project — March 2026
// SPDX-License-Identifier: MIT
// =============================================================================

`timescale 1ns / 1ps

module atomik_asic_top #(
    parameter ADDR_WIDTH   = 8,
    parameter DATA_WIDTH   = 64,
    parameter READ_LATENCY = 1
) (
    // =========================================================================
    // Clock and Reset
    // =========================================================================
    input  wire                    clk,        // External clock input
    input  wire                    rst_n,      // Async reset (active low)

    // =========================================================================
    // ATOMiK Operation Interface
    // =========================================================================
    input  wire                    op_valid,
    input  wire [1:0]              op_code,    // 00=LOAD, 01=ACCUM, 10=READ, 11=SWAP
    input  wire [ADDR_WIDTH-1:0]   op_addr,
    input  wire [DATA_WIDTH-1:0]   op_data,
    output wire                    op_ready,   // Core ready for next operation

    output wire                    result_valid,
    output wire [DATA_WIDTH-1:0]   result_data,

    // =========================================================================
    // BIST Interface
    // =========================================================================
    input  wire                    bist_start, // Assert to begin self-test
    output wire                    bist_done,  // Asserted when test completes
    output wire                    bist_pass,  // All tests passed
    output wire [7:0]              bist_count, // Number of tests passed

    // =========================================================================
    // DFT Interface
    // =========================================================================
    input  wire                    scan_enable,
    input  wire                    scan_in,
    output wire                    scan_out
);

    // =========================================================================
    // Reset Synchronizer
    // =========================================================================
    //
    // Asynchronous assert, synchronous deassert.
    // 4-stage shift register for metastability resolution.

    reg [3:0] rst_sync;
    wire      rst_n_sync;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            rst_sync <= 4'b0000;
        else
            rst_sync <= {rst_sync[2:0], 1'b1};
    end

    assign rst_n_sync = rst_sync[3];

    // =========================================================================
    // BIST / Functional Mode Mux
    // =========================================================================
    //
    // When BIST is active, the BIST controller drives the ATOMiK core.
    // When idle, the external interface drives the core.

    wire                   core_op_valid;
    wire [1:0]             core_op_code;
    wire [ADDR_WIDTH-1:0]  core_op_addr;
    wire [DATA_WIDTH-1:0]  core_op_data;
    wire                   core_result_valid;
    wire [DATA_WIDTH-1:0]  core_result_data;

    wire                   bist_op_valid;
    wire [1:0]             bist_op_code;
    wire [ADDR_WIDTH-1:0]  bist_op_addr;
    wire [DATA_WIDTH-1:0]  bist_op_data;
    wire                   bist_active;

    assign core_op_valid = bist_active ? bist_op_valid : op_valid;
    assign core_op_code  = bist_active ? bist_op_code  : op_code;
    assign core_op_addr  = bist_active ? bist_op_addr  : op_addr;
    assign core_op_data  = bist_active ? bist_op_data  : op_data;

    // External results only valid when BIST is not active
    assign result_valid = bist_active ? 1'b0 : core_result_valid;
    assign result_data  = core_result_data;
    assign op_ready     = ~bist_active;

    // =========================================================================
    // ATOMiK Core (vendor-neutral)
    // =========================================================================

    atomik_core_asic #(
        .ADDR_WIDTH   (ADDR_WIDTH),
        .DATA_WIDTH   (DATA_WIDTH),
        .READ_LATENCY (READ_LATENCY)
    ) u_core (
        .clk          (clk),
        .rst_n        (rst_n_sync),
        .op_valid     (core_op_valid),
        .op_code      (core_op_code),
        .op_addr      (core_op_addr),
        .op_data      (core_op_data),
        .result_valid (core_result_valid),
        .result_data  (core_result_data)
    );

    // =========================================================================
    // BIST Controller
    // =========================================================================

    atomik_bist #(
        .ADDR_WIDTH (ADDR_WIDTH),
        .DATA_WIDTH (DATA_WIDTH)
    ) u_bist (
        .clk          (clk),
        .rst_n        (rst_n_sync),
        .bist_start   (bist_start),
        .bist_done    (bist_done),
        .bist_pass    (bist_pass),
        .bist_count   (bist_count),
        .bist_active  (bist_active),
        .op_valid     (bist_op_valid),
        .op_code      (bist_op_code),
        .op_addr      (bist_op_addr),
        .op_data      (bist_op_data),
        .result_valid (core_result_valid),
        .result_data  (core_result_data)
    );

    // =========================================================================
    // DFT Scan Chain
    // =========================================================================
    //
    // Minimal scan wrapper for initial tapeout. In a real DFT flow, Synopsys
    // DFT Compiler or Cadence Modus inserts scan chains automatically.
    // This stub provides the I/O ports and basic mux structure.

    atomik_dft_wrapper u_dft (
        .clk         (clk),
        .rst_n       (rst_n_sync),
        .scan_enable (scan_enable),
        .scan_in     (scan_in),
        .scan_out    (scan_out)
    );

endmodule
