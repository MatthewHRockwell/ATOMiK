// =============================================================================
// ATOMiK v3 ALU
//
// Module:      atomik_v3_alu
// Description: 64-bit ALU for RV64I base integer ISA.
//              Supports all RV64I arithmetic/logic ops and W-variants.
//              Pure combinational — no clock, no state.
//
//              Shift optimization: single shared right-shift barrel shifter.
//              SLL is implemented as: reverse(reverse(input) >> shamt).
//              SRA uses sign-bit fill; SRL/SLL use zero fill.
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
    wire        slt_result;
    wire        sltu_result;

    // Shift amounts: for W-variants use lower 5 bits, else lower 6 bits
    wire [5:0] shamt = is_word_op ? {1'b0, operand_b[4:0]} : operand_b[5:0];

    // Adder / subtractor
    assign sum  = operand_a + operand_b;
    assign diff = operand_a - operand_b;

    // =========================================================================
    // Shared barrel shifter (SLL, SRL, SRA unified)
    //
    // Strategy: one right-shift barrel shifter with configurable fill bit.
    // SLL is implemented via bit-reversal: reverse input, right-shift, reverse output.
    // Bit reversal is free in hardware (routing only, no LUT cost).
    // =========================================================================

    wire is_sll = (alu_op == ALU_SLL);
    wire is_sra = (alu_op == ALU_SRA);

    // For W-variants: operate on lower 32 bits only
    wire [63:0] shift_base = is_word_op ? {32'b0, operand_a[31:0]} : operand_a;

    // For SRA: sign-extend from bit 31 (W-variant) or bit 63
    wire [63:0] sra_input = is_word_op ?
        {{32{operand_a[31]}}, operand_a[31:0]} : operand_a;

    // Prepare shift input: bit-reverse for SLL, sign-extend for SRA, normal for SRL
    wire [63:0] shift_in;
    genvar g;
    generate
        for (g = 0; g < 64; g = g + 1) begin : gen_shift_in
            assign shift_in[g] = is_sll ? shift_base[63-g] :
                                 is_sra ? sra_input[g] :
                                          shift_base[g];
        end
    endgenerate

    // Fill bit: sign bit for SRA, zero for SRL/SLL
    wire fill = is_sra & shift_in[63];

    // 6-stage right-shift barrel shifter with configurable fill
    wire [63:0] s5, s4, s3, s2, s1, s0;
    assign s5 = shamt[5] ? {{32{fill}}, shift_in[63:32]} : shift_in;
    assign s4 = shamt[4] ? {{16{fill}}, s5[63:16]}       : s5;
    assign s3 = shamt[3] ? {{8{fill}},  s4[63:8]}        : s4;
    assign s2 = shamt[2] ? {{4{fill}},  s3[63:4]}        : s3;
    assign s1 = shamt[1] ? {{2{fill}},  s2[63:2]}        : s2;
    assign s0 = shamt[0] ? {{1{fill}},  s1[63:1]}        : s1;

    // Post-process: bit-reverse output for SLL
    wire [63:0] shift_result;
    generate
        for (g = 0; g < 64; g = g + 1) begin : gen_shift_out
            assign shift_result[g] = is_sll ? s0[63-g] : s0[g];
        end
    endgenerate

    // Comparisons (always 64-bit, even for W-variants)
    assign slt_result  = ($signed(operand_a) < $signed(operand_b)) ? 1'b1 : 1'b0;
    assign sltu_result = (operand_a < operand_b) ? 1'b1 : 1'b0;

    // Result mux
    reg [63:0] alu_raw;

    always @(*) begin
        case (alu_op)
            ALU_ADD:    alu_raw = sum;
            ALU_SUB:    alu_raw = diff;
            ALU_SLL:    alu_raw = shift_result;
            ALU_SLT:    alu_raw = {63'b0, slt_result};
            ALU_SLTU:   alu_raw = {63'b0, sltu_result};
            ALU_XOR:    alu_raw = operand_a ^ operand_b;
            ALU_SRL:    alu_raw = shift_result;
            ALU_SRA:    alu_raw = shift_result;
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
