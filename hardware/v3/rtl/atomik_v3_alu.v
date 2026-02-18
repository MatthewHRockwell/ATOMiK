// =============================================================================
// ATOMiK v3 ALU
//
// Module:      atomik_v3_alu
// Description: 64-bit ALU for RV64I base integer ISA.
//              Supports all RV64I arithmetic/logic ops and W-variants.
//              Pure combinational — no clock, no state.
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_v3_alu (
    input  wire [63:0] operand_a,   // RS1 value (or PC for AUIPC)
    input  wire [63:0] operand_b,   // RS2 value (or immediate)
    input  wire [4:0]  alu_op,      // ALU operation select
    input  wire        is_word_op,  // 1 = W-variant (32-bit result, sign-extended)

    output reg  [63:0] result       // ALU result
);

    // ALU operation encoding
    localparam ALU_ADD  = 5'd0;
    localparam ALU_SUB  = 5'd1;
    localparam ALU_SLL  = 5'd2;
    localparam ALU_SLT  = 5'd3;
    localparam ALU_SLTU = 5'd4;
    localparam ALU_XOR  = 5'd5;
    localparam ALU_SRL  = 5'd6;
    localparam ALU_SRA  = 5'd7;
    localparam ALU_OR   = 5'd8;
    localparam ALU_AND  = 5'd9;
    localparam ALU_PASS_B = 5'd10;  // Pass operand_b (for LUI)

    // Internal wires
    wire [63:0] sum;
    wire [63:0] diff;
    wire [63:0] sll_result;
    wire [63:0] srl_result;
    wire [63:0] sra_result;
    wire        slt_result;
    wire        sltu_result;

    // Shift amounts: for W-variants use lower 5 bits, else lower 6 bits
    wire [5:0] shamt = is_word_op ? {1'b0, operand_b[4:0]} : operand_b[5:0];

    // Adder / subtractor
    assign sum  = operand_a + operand_b;
    assign diff = operand_a - operand_b;

    // Shifts
    // For W-variants: operate on lower 32 bits only
    wire [63:0] shift_input_a = is_word_op ? {32'b0, operand_a[31:0]} : operand_a;

    assign sll_result = shift_input_a << shamt;

    assign srl_result = shift_input_a >> shamt;

    // Arithmetic right shift: for W-variants, sign-extend from bit 31
    wire [63:0] sra_input = is_word_op ?
        {{32{operand_a[31]}}, operand_a[31:0]} : operand_a;
    assign sra_result = $signed(sra_input) >>> shamt;

    // Comparisons (always 64-bit, even for W-variants)
    assign slt_result  = ($signed(operand_a) < $signed(operand_b)) ? 1'b1 : 1'b0;
    assign sltu_result = (operand_a < operand_b) ? 1'b1 : 1'b0;

    // Result mux
    reg [63:0] alu_raw;

    always @(*) begin
        case (alu_op)
            ALU_ADD:    alu_raw = sum;
            ALU_SUB:    alu_raw = diff;
            ALU_SLL:    alu_raw = sll_result;
            ALU_SLT:    alu_raw = {63'b0, slt_result};
            ALU_SLTU:   alu_raw = {63'b0, sltu_result};
            ALU_XOR:    alu_raw = operand_a ^ operand_b;
            ALU_SRL:    alu_raw = srl_result;
            ALU_SRA:    alu_raw = sra_result;
            ALU_OR:     alu_raw = operand_a | operand_b;
            ALU_AND:    alu_raw = operand_a & operand_b;
            ALU_PASS_B: alu_raw = operand_b;
            default:    alu_raw = 64'b0;
        endcase
    end

    // W-variant sign extension: take lower 32 bits, sign-extend to 64
    always @(*) begin
        if (is_word_op)
            result = {{32{alu_raw[31]}}, alu_raw[31:0]};
        else
            result = alu_raw;
    end

endmodule
