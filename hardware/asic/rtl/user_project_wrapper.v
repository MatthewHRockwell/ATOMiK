// =============================================================================
// ATOMiK — Caravel User Project Wrapper
//
// Module:      user_project_wrapper
// Description: Top-level integration of ATOMiK delta-state core into the
//              chipIgnite Caravel SoC harness. Single clock domain (wb_clk_i).
//
// Architecture:
//   Caravel PicoRV32 → Wishbone → atomik_wb_wrapper → atomik_v3_atomik
//
// Address: 0x30000000 + register offsets (see atomik_wb_wrapper.v)
//
// GPIO Usage:
//   io_out[5]  = acc_zero (delta accumulator is zero — state converged)
//   io_out[6]  = core_enable status
//
// LA Usage:
//   la_data_out[63:0]   = current_state (live readout)
//   la_data_out[64]     = acc_zero
//   la_data_out[65]     = core_enable
//   la_data_in[64]      = clock override (when la_oenb[64]=0)
//   la_data_in[65]      = reset override (when la_oenb[65]=0)
//
// IRQ:
//   user_irq[0] = acc_zero transition (state converged notification)
//
// Target:  chipIgnite Caravel SoC (SKY130 130nm)
// Author:  ATOMiK Project
// Date:    March 2026
// =============================================================================

`timescale 1ns / 1ps
`default_nettype none

module user_project_wrapper #(
    parameter BITS = 32
)(
`ifdef USE_POWER_PINS
    inout vdda1,
    inout vdda2,
    inout vssa1,
    inout vssa2,
    inout vccd1,
    inout vccd2,
    inout vssd1,
    inout vssd2,
`endif

    // Wishbone Slave ports (WB MI A)
    input         wb_clk_i,
    input         wb_rst_i,
    input         wbs_stb_i,
    input         wbs_cyc_i,
    input         wbs_we_i,
    input  [3:0]  wbs_sel_i,
    input  [31:0] wbs_dat_i,
    input  [31:0] wbs_adr_i,
    output        wbs_ack_o,
    output [31:0] wbs_dat_o,

    // Logic Analyzer Signals
    input  [127:0] la_data_in,
    output [127:0] la_data_out,
    input  [127:0] la_oenb,

    // IOs
    input  [`MPRJ_IO_PADS-1:0] io_in,
    output [`MPRJ_IO_PADS-1:0] io_out,
    output [`MPRJ_IO_PADS-1:0] io_oeb,

    // Analog
    inout [`MPRJ_IO_PADS-10:0] analog_io,

    // Independent clock
    input  user_clock2,

    // User maskable interrupt signals
    output [2:0] user_irq
);

    // =========================================================================
    // Clock and Reset (with LA override for testability)
    // =========================================================================
    wire clk = (~la_oenb[64]) ? la_data_in[64] : wb_clk_i;
    wire rst = (~la_oenb[65]) ? la_data_in[65] : wb_rst_i;

    // Active-low reset for ATOMiK core (Caravel uses active-high)
    wire rst_n = ~rst;

    // =========================================================================
    // Internal Wires: Wishbone Wrapper ↔ ATOMiK Core
    // =========================================================================
    wire        core_load_en;
    wire        core_accum_en;
    wire        core_swap_en;
    wire [7:0]  core_addr;
    wire [63:0] core_delta;
    wire [63:0] core_init_data;
    wire [63:0] core_current_state;
    wire        core_acc_zero;

    // =========================================================================
    // Wishbone Slave Wrapper
    // =========================================================================
    atomik_wb_wrapper u_wb (
        .wb_clk_i       (clk),
        .wb_rst_i       (rst),
        .wbs_stb_i      (wbs_stb_i),
        .wbs_cyc_i      (wbs_cyc_i),
        .wbs_we_i       (wbs_we_i),
        .wbs_sel_i      (wbs_sel_i),
        .wbs_dat_i      (wbs_dat_i),
        .wbs_adr_i      (wbs_adr_i),
        .wbs_ack_o      (wbs_ack_o),
        .wbs_dat_o      (wbs_dat_o),

        .core_load_en   (core_load_en),
        .core_accum_en  (core_accum_en),
        .core_swap_en   (core_swap_en),
        .core_addr      (core_addr),
        .core_delta     (core_delta),
        .core_init_data (core_init_data),
        .core_current_state (core_current_state),
        .core_acc_zero  (core_acc_zero)
    );

    // =========================================================================
    // ATOMiK Delta-State Core (single bank)
    // =========================================================================
    atomik_v3_atomik u_core (
        .clk            (clk),
        .rst_n          (rst_n),

        .load_en        (core_load_en),
        .accum_en       (core_accum_en),
        .swap_en        (core_swap_en),

        .addr_in        (core_addr),
        .delta_in       (core_delta),
        .init_data      (core_init_data),

        .current_state  (core_current_state),
        .acc_zero       (core_acc_zero)
    );

    // =========================================================================
    // Logic Analyzer Output
    // =========================================================================
    assign la_data_out[63:0]   = core_current_state;   // Live state readout
    assign la_data_out[64]     = core_acc_zero;         // Accumulator zero flag
    assign la_data_out[65]     = 1'b1;                  // Core enabled
    assign la_data_out[127:66] = 62'b0;                 // Reserved

    // =========================================================================
    // GPIO Output
    // =========================================================================
    // io_oeb: 0 = output enabled, 1 = hi-Z
    // Drive acc_zero on GPIO[5] and core_enable on GPIO[6]
    assign io_out[4:0]  = 5'b0;                         // Reserved (Caravel-managed)
    assign io_out[5]    = core_acc_zero;                 // State convergence indicator
    assign io_out[6]    = 1'b1;                          // Core active indicator
    assign io_out[`MPRJ_IO_PADS-1:7] = {(`MPRJ_IO_PADS-7){1'b0}};

    assign io_oeb[4:0]  = 5'b11111;                     // Hi-Z (Caravel-managed)
    assign io_oeb[5]    = 1'b0;                          // Output enabled
    assign io_oeb[6]    = 1'b0;                          // Output enabled
    assign io_oeb[`MPRJ_IO_PADS-1:7] = {(`MPRJ_IO_PADS-7){1'b1}};  // Hi-Z

    // =========================================================================
    // IRQ: signal state convergence (acc_zero rising edge)
    // =========================================================================
    reg acc_zero_prev;
    always @(posedge clk) begin
        if (rst)
            acc_zero_prev <= 1'b0;
        else
            acc_zero_prev <= core_acc_zero;
    end

    assign user_irq[0] = core_acc_zero & ~acc_zero_prev;  // Rising edge
    assign user_irq[2:1] = 2'b00;

endmodule

`default_nettype wire
