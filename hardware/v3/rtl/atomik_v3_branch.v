// =============================================================================
// ATOMiK v3 Branch Comparator
//
// Module:      atomik_v3_branch
// Description: Branch condition evaluation for RV64I.
//              Evaluates BEQ, BNE, BLT, BGE, BLTU, BGEU.
//              Pure combinational.
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_v3_branch (
    input  wire [63:0] rs1_val,     // RS1 register value
    input  wire [63:0] rs2_val,     // RS2 register value
    input  wire [2:0]  funct3,      // Branch type

    output reg         branch_taken // 1 = branch condition met
);

    // Branch funct3 encoding
    localparam BEQ  = 3'b000;
    localparam BNE  = 3'b001;
    localparam BLT  = 3'b100;
    localparam BGE  = 3'b101;
    localparam BLTU = 3'b110;
    localparam BGEU = 3'b111;

    always @(*) begin
        case (funct3)
            BEQ:     branch_taken = (rs1_val == rs2_val);
            BNE:     branch_taken = (rs1_val != rs2_val);
            BLT:     branch_taken = ($signed(rs1_val) < $signed(rs2_val));
            BGE:     branch_taken = ($signed(rs1_val) >= $signed(rs2_val));
            BLTU:    branch_taken = (rs1_val < rs2_val);
            BGEU:    branch_taken = (rs1_val >= rs2_val);
            default: branch_taken = 1'b0;
        endcase
    end

endmodule
