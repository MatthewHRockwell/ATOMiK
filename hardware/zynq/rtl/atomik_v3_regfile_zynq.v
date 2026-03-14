// =============================================================================
// ATOMiK v3 Register File — Zynq Port (Distributed RAM)
//
// Module:      atomik_v3_regfile_zynq
// Description: 32 x 64-bit register file for RV64I on Xilinx 7-series.
//              Dual-copy SDP architecture using distributed RAM.
//              Drop-in replacement for atomik_v3_regfile (Gowin BSRAM).
//
// Distributed RAM Mapping:
//   Copy A (RS1): 32x64 = 128 LUT6 (via Xilinx RAM32M/RAM64M)
//   Copy B (RS2): 32x64 = 128 LUT6
//   Total: ~256 LUT6
//
// Timing:
//   Reads are registered — address in cycle N, data valid at cycle N+1.
//   Same timing as Gowin BSRAM version (DECODE→EXECUTE boundary).
//   Distributed RAM provides combinational read; the output register
//   adds the required pipeline stage for timing closure at 100+ MHz.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

/* verilator lint_off UNUSEDSIGNAL */

module atomik_v3_regfile_zynq (
    input  wire        clk,
    input  wire        rst_n,      // Unused; kept for interface compatibility

    // Read port 1
    input  wire [4:0]  rs1_addr,
    output wire [63:0] rs1_data,

    // Read port 2
    input  wire [4:0]  rs2_addr,
    output wire [63:0] rs2_data,

    // Write port
    input  wire [4:0]  rd_addr,
    input  wire [63:0] rd_data,
    input  wire        rd_wen
);

    // =========================================================================
    // Copy A: RS1 read path
    // Distributed RAM: combinational read, synchronous write
    // =========================================================================
    (* ram_style = "distributed" *) reg [63:0] regs_a [0:31];
    reg [63:0] rs1_data_reg;

    // Write port A
    always @(posedge clk) begin
        if (rd_wen && rd_addr != 5'b0)
            regs_a[rd_addr] <= rd_data;
    end

    // Read port A (registered output — matches BSRAM timing)
    always @(posedge clk) begin
        rs1_data_reg <= regs_a[rs1_addr];
    end

    // =========================================================================
    // Copy B: RS2 read path
    // =========================================================================
    (* ram_style = "distributed" *) reg [63:0] regs_b [0:31];
    reg [63:0] rs2_data_reg;

    // Write port B
    always @(posedge clk) begin
        if (rd_wen && rd_addr != 5'b0)
            regs_b[rd_addr] <= rd_data;
    end

    // Read port B (registered output)
    always @(posedge clk) begin
        rs2_data_reg <= regs_b[rs2_addr];
    end

    // =========================================================================
    // x0 hardwire: force output to 0 when reading register x0
    // =========================================================================
    assign rs1_data = (rs1_addr == 5'b0) ? 64'b0 : rs1_data_reg;
    assign rs2_data = (rs2_addr == 5'b0) ? 64'b0 : rs2_data_reg;

    // =========================================================================
    // Simulation-only initialization
    // =========================================================================
    // synthesis translate_off
    integer i;
    initial begin
        for (i = 0; i < 32; i = i + 1) begin
            regs_a[i] = 64'b0;
            regs_b[i] = 64'b0;
        end
        rs1_data_reg = 64'b0;
        rs2_data_reg = 64'b0;
    end
    // synthesis translate_on

endmodule
