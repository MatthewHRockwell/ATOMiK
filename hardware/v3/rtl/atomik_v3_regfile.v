// =============================================================================
// ATOMiK v3 Register File
//
// Module:      atomik_v3_regfile
// Description: 32 x 64-bit register file for RV64I.
//              Behavioral model with combinational read, synchronous write.
//              x0 is hardwired to zero.
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_v3_regfile (
    input  wire        clk,
    input  wire        rst_n,

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

    // 32 x 64-bit register array
    reg [63:0] regs [1:31];  // x0 is not stored — always zero

    // Combinational read with x0 hardwire
    assign rs1_data = (rs1_addr == 5'b0) ? 64'b0 : regs[rs1_addr];
    assign rs2_data = (rs2_addr == 5'b0) ? 64'b0 : regs[rs2_addr];

    // Synchronous write (x0 writes are ignored)
    integer i;
    always @(posedge clk) begin
        if (!rst_n) begin
            for (i = 1; i < 32; i = i + 1)
                regs[i] <= 64'b0;
        end else if (rd_wen && rd_addr != 5'b0) begin
            regs[rd_addr] <= rd_data;
        end
    end

endmodule
