// =============================================================================
// Generic Single-Port SRAM — ASIC-Portable Memory Wrapper
//
// Module:      atomik_sram_sp
// Description: Behavioral single-port SRAM model for ASIC synthesis.
//              During synthesis, this module is replaced by foundry-specific
//              SRAM macros (via module swapping or wrapper generation).
//
//              For simulation: fully behavioral (reg array).
//              For ASIC synthesis: replace with foundry SRAM compiler output.
//              For FPGA: synthesis tools will infer Block RAM.
//
// Parameters:
//   ADDR_WIDTH  - Address width (depth = 2^ADDR_WIDTH)
//   DATA_WIDTH  - Data width in bits
//   DEPTH       - Memory depth (overrides 2^ADDR_WIDTH if specified)
//
// Interface:
//   Single-clock, synchronous read/write, read-first behavior.
//
// ATOMiK Project — March 2026
// SPDX-License-Identifier: MIT
// =============================================================================

`timescale 1ns / 1ps

module atomik_sram_sp #(
    parameter ADDR_WIDTH = 8,
    parameter DATA_WIDTH = 64,
    parameter DEPTH      = (1 << ADDR_WIDTH)
) (
    input  wire                    clk,
    input  wire                    ce,       // Chip enable (active high)
    input  wire                    we,       // Write enable (active high)
    input  wire [ADDR_WIDTH-1:0]   addr,
    input  wire [DATA_WIDTH-1:0]   din,
    output reg  [DATA_WIDTH-1:0]   dout
);

    // =========================================================================
    // Memory Array
    // =========================================================================
    //
    // For ASIC synthesis, replace this behavioral model with a foundry
    // SRAM macro instantiation. The interface (ce, we, addr, din, dout)
    // is standard across all foundry SRAM compilers.
    //
    // Example foundry replacement:
    //   Sky130:  sky130_sram_1kbyte_1rw1r_8x1024_8
    //   GF180:   gf180mcu_fd_ip_sram__sram256x8m8wm1
    //   TSMC28:  ts28n_sram_sp_hde_256x64m4s1

    (* ram_style = "block" *)
    reg [DATA_WIDTH-1:0] mem [0:DEPTH-1];

    // =========================================================================
    // Synchronous Read-First Operation
    // =========================================================================
    always @(posedge clk) begin
        if (ce) begin
            dout <= mem[addr];       // Read-first: output old value on write
            if (we) begin
                mem[addr] <= din;
            end
        end
    end

    // =========================================================================
    // Optional initialization for simulation
    // =========================================================================
`ifdef SIMULATION
    integer i;
    initial begin
        for (i = 0; i < DEPTH; i = i + 1)
            mem[i] = {DATA_WIDTH{1'b0}};
    end
`endif

endmodule
