// =============================================================================
// ATOMiK v3 Decoder Testbench (iverilog)
//
// Tests RV64I instruction decoding: all opcode types, immediate extraction,
// custom-0 detect.
// =============================================================================

`timescale 1ns / 1ps

module tb_v3_decode;

    reg  [31:0] instr;
    wire [4:0]  rd, rs1, rs2;
    wire [63:0] imm;
    wire [4:0]  alu_op;
    wire        alu_src_b_imm, is_word_op;
    wire        is_lui, is_auipc, is_jal, is_jalr;
    wire        is_branch, is_load, is_store;
    wire        is_alu_reg, is_alu_imm;
    wire        is_system, is_fence, is_custom0;
    wire [2:0]  mem_size, funct3;
    wire [6:0]  funct7;
    wire        illegal_instr;

    atomik_v3_decode uut (
        .instr          (instr),
        .rd             (rd),
        .rs1            (rs1),
        .rs2            (rs2),
        .imm            (imm),
        .alu_op         (alu_op),
        .alu_src_b_imm  (alu_src_b_imm),
        .is_word_op     (is_word_op),
        .is_lui         (is_lui),
        .is_auipc       (is_auipc),
        .is_jal         (is_jal),
        .is_jalr        (is_jalr),
        .is_branch      (is_branch),
        .is_load        (is_load),
        .is_store       (is_store),
        .is_alu_reg     (is_alu_reg),
        .is_alu_imm     (is_alu_imm),
        .is_system      (is_system),
        .is_fence       (is_fence),
        .is_custom0     (is_custom0),
        .mem_size       (mem_size),
        .funct3         (funct3),
        .funct7         (funct7),
        .illegal_instr  (illegal_instr)
    );

    integer test_count = 0;
    integer pass_count = 0;
    integer fail_count = 0;

    task check_flag;
        input flag_val;
        input expected;
        input [255:0] name;
        begin
            test_count = test_count + 1;
            if (flag_val === expected) begin
                pass_count = pass_count + 1;
                $display("PASS [%0s]", name);
            end else begin
                fail_count = fail_count + 1;
                $display("FAIL [%0s]: got %0d, expected %0d", name, flag_val, expected);
            end
        end
    endtask

    task check_val;
        input [63:0] got;
        input [63:0] expected;
        input [255:0] name;
        begin
            test_count = test_count + 1;
            if (got === expected) begin
                pass_count = pass_count + 1;
                $display("PASS [%0s]: %016x", name, got);
            end else begin
                fail_count = fail_count + 1;
                $display("FAIL [%0s]: got %016x, expected %016x", name, got, expected);
            end
        end
    endtask

    initial begin
        $display("==============================================");
        $display("ATOMiK v3 Decoder Testbench");
        $display("==============================================");
        $display("");

        // --- LUI x1, 0xDEADB ---
        // LUI: imm[31:12]=0xDEADB, rd=1, opcode=0110111
        instr = {20'hDEADB, 5'd1, 7'b0110111};
        #1;
        check_flag(is_lui, 1, "LUI: is_lui");
        check_val({59'd0, rd}, {59'd0, 5'd1}, "LUI: rd=1");
        check_val(imm, 64'hFFFF_FFFF_DEAD_B000, "LUI: imm sign-ext");

        // --- AUIPC x2, 0x12345 ---
        instr = {20'h12345, 5'd2, 7'b0010111};
        #1;
        check_flag(is_auipc, 1, "AUIPC: is_auipc");
        check_val({59'd0, rd}, {59'd0, 5'd2}, "AUIPC: rd=2");
        check_val(imm, 64'h0000_0000_1234_5000, "AUIPC: imm");

        // --- ADDI x3, x1, -1 ---
        // I-type: imm=0xFFF, rs1=1, funct3=000, rd=3, opcode=0010011
        instr = {12'hFFF, 5'd1, 3'b000, 5'd3, 7'b0010011};
        #1;
        check_flag(is_alu_imm, 1, "ADDI: is_alu_imm");
        check_val({59'd0, rs1}, {59'd0, 5'd1}, "ADDI: rs1=1");
        check_val({59'd0, rd}, {59'd0, 5'd3}, "ADDI: rd=3");
        check_val(imm, 64'hFFFF_FFFF_FFFF_FFFF, "ADDI: imm=-1");
        check_flag(alu_src_b_imm, 1, "ADDI: src_b_imm");

        // --- ADD x5, x3, x4 (R-type) ---
        instr = {7'b0000000, 5'd4, 5'd3, 3'b000, 5'd5, 7'b0110011};
        #1;
        check_flag(is_alu_reg, 1, "ADD: is_alu_reg");
        check_val({59'd0, rs1}, {59'd0, 5'd3}, "ADD: rs1=3");
        check_val({59'd0, rs2}, {59'd0, 5'd4}, "ADD: rs2=4");
        check_val({59'd0, rd}, {59'd0, 5'd5}, "ADD: rd=5");
        check_flag(alu_src_b_imm, 0, "ADD: src_b_reg");

        // --- SUB x6, x3, x4 ---
        instr = {7'b0100000, 5'd4, 5'd3, 3'b000, 5'd6, 7'b0110011};
        #1;
        check_flag(is_alu_reg, 1, "SUB: is_alu_reg");
        check_val({59'd0, alu_op}, {59'd0, 5'd1}, "SUB: alu_op=SUB(1)");

        // --- BEQ x1, x2, offset ---
        // B-type: imm=+8 → imm[12|10:5]=0, imm[4:1|11]=4|0
        // imm = 8 → bit12=0, bits10:5=0, bits4:1=0100, bit11=0
        instr = {1'b0, 6'b000000, 5'd2, 5'd1, 3'b000, 4'b0100, 1'b0, 7'b1100011};
        #1;
        check_flag(is_branch, 1, "BEQ: is_branch");
        check_val(imm, 64'h0000_0000_0000_0008, "BEQ: imm=+8");

        // --- JAL x1, offset ---
        // J-type: imm=+16 → bit20=0, bits10:1=0000001000, bit11=0, bits19:12=00000000
        instr = {1'b0, 10'b0000001000, 1'b0, 8'b00000000, 5'd1, 7'b1101111};
        #1;
        check_flag(is_jal, 1, "JAL: is_jal");
        check_val(imm, 64'h0000_0000_0000_0010, "JAL: imm=+16");

        // --- JALR x1, x2, 0 ---
        instr = {12'h000, 5'd2, 3'b000, 5'd1, 7'b1100111};
        #1;
        check_flag(is_jalr, 1, "JALR: is_jalr");

        // --- LW x3, 4(x1) ---
        instr = {12'h004, 5'd1, 3'b010, 5'd3, 7'b0000011};
        #1;
        check_flag(is_load, 1, "LW: is_load");
        check_val(imm, 64'h0000_0000_0000_0004, "LW: imm=4");
        check_val({61'd0, mem_size}, {61'd0, 3'b010}, "LW: size=W");

        // --- LD x3, 8(x1) ---
        instr = {12'h008, 5'd1, 3'b011, 5'd3, 7'b0000011};
        #1;
        check_flag(is_load, 1, "LD: is_load");
        check_val({61'd0, mem_size}, {61'd0, 3'b011}, "LD: size=D");

        // --- SW x2, 12(x1) ---
        // S-type: imm=12 → imm[11:5]=0, imm[4:0]=01100
        instr = {7'b0000000, 5'd2, 5'd1, 3'b010, 5'b01100, 7'b0100011};
        #1;
        check_flag(is_store, 1, "SW: is_store");
        check_val(imm, 64'h0000_0000_0000_000C, "SW: imm=12");

        // --- ADDIW x1, x2, 5 (W-variant) ---
        instr = {12'h005, 5'd2, 3'b000, 5'd1, 7'b0011011};
        #1;
        check_flag(is_alu_imm, 1, "ADDIW: is_alu_imm");
        check_flag(is_word_op, 1, "ADDIW: is_word_op");

        // --- ADDW x1, x2, x3 (W-variant) ---
        instr = {7'b0000000, 5'd3, 5'd2, 3'b000, 5'd1, 7'b0111011};
        #1;
        check_flag(is_alu_reg, 1, "ADDW: is_alu_reg");
        check_flag(is_word_op, 1, "ADDW: is_word_op");

        // --- ECALL ---
        instr = 32'h00000073;
        #1;
        check_flag(is_system, 1, "ECALL: is_system");
        check_flag(illegal_instr, 0, "ECALL: not illegal");

        // --- FENCE ---
        instr = 32'h0000000F;
        #1;
        check_flag(is_fence, 1, "FENCE: is_fence");

        // --- Custom-0 (ATOMiK) ---
        instr = {7'b0000000, 5'd0, 5'd1, 3'b001, 5'd0, 7'b0001011};
        #1;
        check_flag(is_custom0, 1, "CUSTOM0: is_custom0");

        // --- SLLI x1, x2, 63 (6-bit shamt) ---
        // imm[11:0] = 000000_111111 = 0x03F
        instr = {6'b000000, 6'b111111, 5'd2, 3'b001, 5'd1, 7'b0010011};
        #1;
        check_flag(is_alu_imm, 1, "SLLI64: is_alu_imm");
        // Shamt should be in imm[5:0] = 63
        check_val(imm[5:0], 6'd63, "SLLI64: shamt=63");

        // --- Negative B-immediate ---
        // BEQ x0, x0, -4 → imm = -4 = 0xFFFF...FFFC
        // B-type: imm[12]=1, imm[10:5]=111111, imm[4:1]=1110, imm[11]=1
        instr = {1'b1, 6'b111111, 5'd0, 5'd0, 3'b000, 4'b1110, 1'b1, 7'b1100011};
        #1;
        check_flag(is_branch, 1, "BEQ-neg: is_branch");
        check_val(imm, 64'hFFFF_FFFF_FFFF_FFFC, "BEQ-neg: imm=-4");

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
