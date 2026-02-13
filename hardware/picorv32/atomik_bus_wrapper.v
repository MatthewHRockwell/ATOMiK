// =============================================================================
// ATOMiK Bus Wrapper for PicoRV32 — Dual-Clock, 4-Bank Parallel
//
// Module:      atomik_bus_wrapper
// Description: Wraps atomik_parallel_acc (N=4, 32-bit) with PicoRV32's
//              valid/ready bus protocol. Uses atomik_cdc_bridge for clock
//              domain crossing between bus (25.2 MHz) and core (81 MHz).
//
// Register Map (unchanged from single-bank version):
//   0x00  LOAD   (W)  - Load initial state, clears all 4 bank accumulators
//   0x04  ACCUM  (W)  - Accumulate delta via XOR (round-robin across banks)
//   0x08  STATE  (R)  - Read reconstructed state (initial XOR merged banks)
//   0x0C  STATUS (R)  - Bit 0: accumulator_zero (merged)
//   0x10  CONFIG (W)  - Bit 0: soft reset
//   0x14  INIT   (R)  - Read initial state (debug)
//   0x18  DELTA  (R)  - Read merged accumulator (debug)
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   February 12, 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_bus_wrapper (
    input  wire        clk,             // Bus clock (25.2 MHz)
    input  wire        clk_core,        // ATOMiK core clock (81 MHz)
    input  wire        core_pll_lock,   // Core PLL lock signal
    input  wire        resetn,          // Active-low reset (PicoRV32 convention)

    // PicoRV32 Bus Interface
    input  wire        mem_valid,
    input  wire [31:0] mem_addr,
    input  wire [31:0] mem_wdata,
    input  wire [3:0]  mem_wstrb,
    output wire        mem_ready,
    output wire [31:0] mem_rdata
);

    // =========================================================================
    // Reset Synchronization into Core Domain
    // =========================================================================

    // Core domain reset: synchronized via 3FF chain from bus domain
    // Asserted when: bus reset OR PLL not locked
    wire rst_bus_n = resetn;
    wire core_rst_base_n = resetn & core_pll_lock;

    reg core_rst_sync_0, core_rst_sync_1, core_rst_sync_2;
    always @(posedge clk_core or negedge core_rst_base_n) begin
        if (!core_rst_base_n) begin
            core_rst_sync_0 <= 1'b0;
            core_rst_sync_1 <= 1'b0;
            core_rst_sync_2 <= 1'b0;
        end else begin
            core_rst_sync_0 <= 1'b1;
            core_rst_sync_1 <= core_rst_sync_0;
            core_rst_sync_2 <= core_rst_sync_1;
        end
    end

    // Soft reset: CDC bridge pulses cdc_soft_reset for 1 core cycle.
    // Stretch it via a 2-bit shift register to ensure stable reset.
    wire cdc_soft_reset;
    reg [1:0] soft_reset_stretch;
    always @(posedge clk_core or negedge core_rst_base_n) begin
        if (!core_rst_base_n)
            soft_reset_stretch <= 2'b0;
        else if (cdc_soft_reset)
            soft_reset_stretch <= 2'b11;
        else
            soft_reset_stretch <= {1'b0, soft_reset_stretch[1]};
    end

    wire core_rst_n = core_rst_sync_2 & ~soft_reset_stretch[0];

    // =========================================================================
    // CDC Bridge ↔ Parallel Accumulator Signals
    // =========================================================================

    wire [31:0] cdc_initial_state_in;
    wire        cdc_load_initial;
    wire [31:0] cdc_delta_in;
    wire        cdc_delta_valid;
    wire [31:0] acc_current_state;
    wire [31:0] acc_merged_accumulator;
    wire        acc_accumulator_zero;
    wire [31:0] acc_initial_state_out;

    // =========================================================================
    // ATOMiK Parallel Accumulator (N=4, 32-bit, core clock domain)
    // =========================================================================

    atomik_parallel_acc #(
        .DELTA_WIDTH(32),
        .N_BANKS(4)
    ) u_parallel_acc (
        .clk                  (clk_core),
        .rst_n                (core_rst_n),

        // Single-stream round-robin input
        .delta_in             (cdc_delta_in),
        .delta_valid          (cdc_delta_valid),

        // Parallel input — unused, tied off
        .delta_parallel_in    (128'b0),
        .delta_parallel_valid (4'b0),

        // Round-robin mode
        .parallel_mode        (1'b0),

        // Initial state
        .initial_state_in     (cdc_initial_state_in),
        .load_initial         (cdc_load_initial),

        // Outputs
        .current_state        (acc_current_state),
        .merged_accumulator   (acc_merged_accumulator),
        .accumulator_zero     (acc_accumulator_zero),
        .current_bank         (),  // Not needed
        .bank_active          ()   // Not needed
    );

    // Read initial_state from the parallel_acc's internal register
    // The parallel_acc exposes this as part of current_state reconstruction;
    // we can derive it: initial_state = current_state ^ merged_accumulator
    assign acc_initial_state_out = acc_current_state ^ acc_merged_accumulator;

    // =========================================================================
    // CDC Bridge (bus ↔ core domain)
    // =========================================================================

    atomik_cdc_bridge #(
        .DATA_WIDTH(32),
        .N_BANKS(4)
    ) u_cdc_bridge (
        // Bus-side (25.2 MHz)
        .clk_bus              (clk),
        .rst_bus_n            (rst_bus_n),
        .mem_valid            (mem_valid),
        .mem_addr             (mem_addr),
        .mem_wdata            (mem_wdata),
        .mem_wstrb            (mem_wstrb),
        .mem_ready            (mem_ready),
        .mem_rdata            (mem_rdata),

        // Core-side (81 MHz)
        .clk_core             (clk_core),
        .rst_core_n           (core_rst_n),
        .core_initial_state_in (cdc_initial_state_in),
        .core_load_initial    (cdc_load_initial),
        .core_delta_in        (cdc_delta_in),
        .core_delta_valid     (cdc_delta_valid),
        .core_soft_reset      (cdc_soft_reset),
        .core_current_state   (acc_current_state),
        .core_merged_accumulator(acc_merged_accumulator),
        .core_accumulator_zero(acc_accumulator_zero),
        .core_initial_state_out(acc_initial_state_out)
    );

endmodule
