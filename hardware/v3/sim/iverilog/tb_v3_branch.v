// =============================================================================
// ATOMiK v3 Branch Comparator Testbench (iverilog)
// =============================================================================

`timescale 1ns / 1ps

module tb_v3_branch;

    reg  [63:0] rs1_val, rs2_val;
    reg  [2:0]  funct3;
    wire        branch_taken;

    atomik_v3_branch uut (
        .rs1_val      (rs1_val),
        .rs2_val      (rs2_val),
        .funct3       (funct3),
        .branch_taken (branch_taken)
    );

    integer test_count = 0;
    integer pass_count = 0;
    integer fail_count = 0;

    task check;
        input expected;
        input [255:0] name;
        begin
            test_count = test_count + 1;
            #1;
            if (branch_taken === expected) begin
                pass_count = pass_count + 1;
                $display("PASS [%0s]", name);
            end else begin
                fail_count = fail_count + 1;
                $display("FAIL [%0s]: got %0d, expected %0d", name, branch_taken, expected);
            end
        end
    endtask

    initial begin
        $display("==============================================");
        $display("ATOMiK v3 Branch Comparator Testbench");
        $display("==============================================");
        $display("");

        // BEQ (funct3=000)
        funct3 = 3'b000;
        rs1_val = 64'd42; rs2_val = 64'd42;
        check(1, "BEQ: equal");
        rs1_val = 64'd42; rs2_val = 64'd43;
        check(0, "BEQ: not equal");

        // BNE (funct3=001)
        funct3 = 3'b001;
        rs1_val = 64'd1; rs2_val = 64'd2;
        check(1, "BNE: not equal");
        rs1_val = 64'd5; rs2_val = 64'd5;
        check(0, "BNE: equal");

        // BLT (funct3=100) — signed
        funct3 = 3'b100;
        rs1_val = 64'hFFFF_FFFF_FFFF_FFFF; rs2_val = 64'd0; // -1 < 0
        check(1, "BLT: -1 < 0");
        rs1_val = 64'd0; rs2_val = 64'hFFFF_FFFF_FFFF_FFFF;
        check(0, "BLT: 0 >= -1");
        rs1_val = 64'd5; rs2_val = 64'd5;
        check(0, "BLT: equal");

        // BGE (funct3=101) — signed
        funct3 = 3'b101;
        rs1_val = 64'd0; rs2_val = 64'hFFFF_FFFF_FFFF_FFFF;
        check(1, "BGE: 0 >= -1");
        rs1_val = 64'd5; rs2_val = 64'd5;
        check(1, "BGE: equal");
        rs1_val = 64'hFFFF_FFFF_FFFF_FFFF; rs2_val = 64'd0;
        check(0, "BGE: -1 < 0");

        // BLTU (funct3=110) — unsigned
        funct3 = 3'b110;
        rs1_val = 64'd0; rs2_val = 64'hFFFF_FFFF_FFFF_FFFF;
        check(1, "BLTU: 0 < max");
        rs1_val = 64'hFFFF_FFFF_FFFF_FFFF; rs2_val = 64'd0;
        check(0, "BLTU: max >= 0");

        // BGEU (funct3=111) — unsigned
        funct3 = 3'b111;
        rs1_val = 64'hFFFF_FFFF_FFFF_FFFF; rs2_val = 64'd0;
        check(1, "BGEU: max >= 0");
        rs1_val = 64'd5; rs2_val = 64'd5;
        check(1, "BGEU: equal");
        rs1_val = 64'd0; rs2_val = 64'd1;
        check(0, "BGEU: 0 < 1");

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
