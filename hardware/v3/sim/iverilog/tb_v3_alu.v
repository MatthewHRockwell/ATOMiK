// =============================================================================
// ATOMiK v3 ALU Testbench (iverilog)
//
// Tests all RV64I ALU operations and W-variants.
// =============================================================================

`timescale 1ns / 1ps

module tb_v3_alu;

    reg  [63:0] operand_a;
    reg  [63:0] operand_b;
    reg  [4:0]  alu_op;
    reg         is_word_op;
    wire [63:0] result;

    atomik_v3_alu uut (
        .operand_a  (operand_a),
        .operand_b  (operand_b),
        .alu_op     (alu_op),
        .is_word_op (is_word_op),
        .result     (result)
    );

    // ALU op encoding
    localparam ALU_ADD    = 5'd0;
    localparam ALU_SUB    = 5'd1;
    localparam ALU_SLL    = 5'd2;
    localparam ALU_SLT    = 5'd3;
    localparam ALU_SLTU   = 5'd4;
    localparam ALU_XOR    = 5'd5;
    localparam ALU_SRL    = 5'd6;
    localparam ALU_SRA    = 5'd7;
    localparam ALU_OR     = 5'd8;
    localparam ALU_AND    = 5'd9;
    localparam ALU_PASS_B = 5'd10;

    integer test_count = 0;
    integer pass_count = 0;
    integer fail_count = 0;

    task check;
        input [63:0] expected;
        input [255:0] name; // String
        begin
            test_count = test_count + 1;
            #1;
            if (result === expected) begin
                pass_count = pass_count + 1;
                $display("PASS [%0s]: got %016x", name, result);
            end else begin
                fail_count = fail_count + 1;
                $display("FAIL [%0s]: got %016x, expected %016x", name, result, expected);
            end
        end
    endtask

    initial begin
        $display("==============================================");
        $display("ATOMiK v3 ALU Testbench");
        $display("==============================================");
        $display("");

        is_word_op = 0;

        // --- ADD ---
        operand_a = 64'h0000_0000_0000_0001;
        operand_b = 64'h0000_0000_0000_0002;
        alu_op = ALU_ADD;
        check(64'h0000_0000_0000_0003, "ADD: 1+2=3");

        operand_a = 64'hFFFF_FFFF_FFFF_FFFF;
        operand_b = 64'h0000_0000_0000_0001;
        check(64'h0000_0000_0000_0000, "ADD: -1+1=0 (overflow)");

        // --- SUB ---
        operand_a = 64'h0000_0000_0000_0005;
        operand_b = 64'h0000_0000_0000_0003;
        alu_op = ALU_SUB;
        check(64'h0000_0000_0000_0002, "SUB: 5-3=2");

        operand_a = 64'h0000_0000_0000_0000;
        operand_b = 64'h0000_0000_0000_0001;
        check(64'hFFFF_FFFF_FFFF_FFFF, "SUB: 0-1=-1");

        // --- SLL ---
        operand_a = 64'h0000_0000_0000_0001;
        operand_b = 64'h0000_0000_0000_003F; // shift 63
        alu_op = ALU_SLL;
        check(64'h8000_0000_0000_0000, "SLL: 1<<63");

        operand_b = 64'h0000_0000_0000_0004; // shift 4
        check(64'h0000_0000_0000_0010, "SLL: 1<<4");

        // --- SLT (signed) ---
        operand_a = 64'hFFFF_FFFF_FFFF_FFFF; // -1
        operand_b = 64'h0000_0000_0000_0000; // 0
        alu_op = ALU_SLT;
        check(64'h0000_0000_0000_0001, "SLT: -1 < 0 = 1");

        operand_a = 64'h0000_0000_0000_0001;
        operand_b = 64'hFFFF_FFFF_FFFF_FFFF;
        check(64'h0000_0000_0000_0000, "SLT: 1 < -1 = 0");

        // --- SLTU (unsigned) ---
        operand_a = 64'hFFFF_FFFF_FFFF_FFFF;
        operand_b = 64'h0000_0000_0000_0000;
        alu_op = ALU_SLTU;
        check(64'h0000_0000_0000_0000, "SLTU: max > 0 = 0");

        operand_a = 64'h0000_0000_0000_0000;
        operand_b = 64'hFFFF_FFFF_FFFF_FFFF;
        check(64'h0000_0000_0000_0001, "SLTU: 0 < max = 1");

        // --- XOR ---
        operand_a = 64'hFF00_FF00_FF00_FF00;
        operand_b = 64'h00FF_00FF_00FF_00FF;
        alu_op = ALU_XOR;
        check(64'hFFFF_FFFF_FFFF_FFFF, "XOR: complementary");

        // --- SRL ---
        operand_a = 64'h8000_0000_0000_0000;
        operand_b = 64'h0000_0000_0000_003F; // shift 63
        alu_op = ALU_SRL;
        check(64'h0000_0000_0000_0001, "SRL: >>63");

        // --- SRA ---
        operand_a = 64'h8000_0000_0000_0000;
        operand_b = 64'h0000_0000_0000_003F; // shift 63
        alu_op = ALU_SRA;
        check(64'hFFFF_FFFF_FFFF_FFFF, "SRA: -x>>63 = -1");

        operand_a = 64'h4000_0000_0000_0000;
        operand_b = 64'h0000_0000_0000_003F;
        check(64'h0000_0000_0000_0000, "SRA: +x>>63 = 0");

        // --- OR ---
        operand_a = 64'hFF00_FF00_0000_0000;
        operand_b = 64'h00FF_00FF_0000_0000;
        alu_op = ALU_OR;
        check(64'hFFFF_FFFF_0000_0000, "OR: complementary high");

        // --- AND ---
        operand_a = 64'hFFFF_0000_FFFF_0000;
        operand_b = 64'h0000_FFFF_0000_FFFF;
        alu_op = ALU_AND;
        check(64'h0000_0000_0000_0000, "AND: disjoint = 0");

        operand_a = 64'hFFFF_FFFF_FFFF_FFFF;
        operand_b = 64'hDEAD_BEEF_CAFE_BABE;
        check(64'hDEAD_BEEF_CAFE_BABE, "AND: all-ones mask");

        // --- PASS_B (LUI) ---
        operand_a = 64'h1234_5678_9ABC_DEF0;
        operand_b = 64'hFFFF_FFFF_DEAD_B000;
        alu_op = ALU_PASS_B;
        check(64'hFFFF_FFFF_DEAD_B000, "PASS_B: LUI immediate");

        // =================================================================
        // W-variant tests (32-bit, sign-extended to 64)
        // =================================================================
        $display("");
        $display("--- W-variant tests ---");
        is_word_op = 1;

        // ADDW: 32-bit add, sign-extend
        operand_a = 64'h0000_0000_7FFF_FFFF;
        operand_b = 64'h0000_0000_0000_0001;
        alu_op = ALU_ADD;
        check(64'hFFFF_FFFF_8000_0000, "ADDW: 0x7FFFFFFF+1 sign-extends");

        // SUBW
        operand_a = 64'h0000_0000_0000_0000;
        operand_b = 64'h0000_0000_0000_0001;
        alu_op = ALU_SUB;
        check(64'hFFFF_FFFF_FFFF_FFFF, "SUBW: 0-1 = -1 (sign-ext)");

        // SLLW: 5-bit shift amount
        operand_a = 64'h0000_0000_0000_0001;
        operand_b = 64'h0000_0000_0000_001F; // shift 31
        alu_op = ALU_SLL;
        check(64'hFFFF_FFFF_8000_0000, "SLLW: 1<<31 sign-extends");

        // SRLW: logical right shift of 32-bit value
        operand_a = 64'hFFFF_FFFF_8000_0000;
        operand_b = 64'h0000_0000_0000_001F; // shift 31
        alu_op = ALU_SRL;
        check(64'h0000_0000_0000_0001, "SRLW: 0x80000000>>31 = 1");

        // SRAW: arithmetic right shift of 32-bit value
        operand_a = 64'h0000_0000_8000_0000;
        operand_b = 64'h0000_0000_0000_001F; // shift 31
        alu_op = ALU_SRA;
        check(64'hFFFF_FFFF_FFFF_FFFF, "SRAW: 0x80000000>>>31 = -1");

        // ADDW with upper bits in operands (should be ignored)
        operand_a = 64'hDEAD_BEEF_0000_0005;
        operand_b = 64'hCAFE_BABE_0000_0003;
        alu_op = ALU_ADD;
        check(64'h0000_0000_0000_0008, "ADDW: ignores upper 32 of result");

        is_word_op = 0;

        // =================================================================
        // Summary
        // =================================================================
        $display("");
        $display("==============================================");
        $display("Test Summary: %0d/%0d passed", pass_count, test_count);
        $display("==============================================");

        if (fail_count == 0)
            $display("*** ALL TESTS PASSED ***");
        else
            $display("*** %0d TESTS FAILED ***", fail_count);

        $finish;
    end

endmodule
