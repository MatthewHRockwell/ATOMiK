// =============================================================================
// ATOMiK v3 Register File (BSRAM-Optimized)
//
// Module:      atomik_v3_regfile
// Description: 32 x 64-bit register file for RV64I.
//              Dual-copy SDP architecture for BSRAM inference.
//              Each copy provides one registered read port;
//              both copies share a synchronous write port.
//              x0 is hardwired to zero via output mux.
//
// BSRAM Mapping:
//   Copy A (RS1): 2 BSRAM blocks (32x32-bit low + 32x32-bit high)
//   Copy B (RS2): 2 BSRAM blocks (32x32-bit low + 32x32-bit high)
//   Total: 4 BSRAM blocks (out of 26 available on GW1NR-9)
//
// Timing:
//   Reads are registered — address presented in one cycle, data available
//   at the next posedge. In the multi-cycle FSM, the decoder presents
//   rs1/rs2 addresses during FETCH→DECODE, and the registered output is
//   valid from the start of DECODE through EXECUTE.
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

/* verilator lint_off UNUSEDSIGNAL */

module atomik_v3_regfile (
    input  wire        clk,
    input  wire        rst_n,      // Unused (no reset on BSRAM); kept for interface compat

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
    // SDP pattern: separate write + read always blocks → BSRAM inference
    // =========================================================================
    (* syn_ramstyle = "block_ram" *) reg [63:0] regs_a [0:31];
    reg [63:0] rs1_data_reg;

    // Write port A
    always @(posedge clk) begin
        if (rd_wen && rd_addr != 5'b0)
            regs_a[rd_addr] <= rd_data;
    end

    // Read port A (registered output)
    always @(posedge clk) begin
        rs1_data_reg <= regs_a[rs1_addr];
    end

    // =========================================================================
    // Copy B: RS2 read path
    // =========================================================================
    (* syn_ramstyle = "block_ram" *) reg [63:0] regs_b [0:31];
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
    // rs1_addr/rs2_addr are stable between DECODE and EXECUTE (decoder is
    // combinational from latched instruction), so this mux is correct.
    // =========================================================================
    assign rs1_data = (rs1_addr == 5'b0) ? 64'b0 : rs1_data_reg;
    assign rs2_data = (rs2_addr == 5'b0) ? 64'b0 : rs2_data_reg;

    // =========================================================================
    // Simulation-only initialization
    // BSRAM initial contents are set via INIT_RAM parameters in synthesis.
    // For simulation, initialize to zero so compliance tests don't see X.
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
