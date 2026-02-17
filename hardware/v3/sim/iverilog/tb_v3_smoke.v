// =============================================================================
// ATOMiK v3 Smoke Testbench (iverilog)
//
// Module:      tb_v3_smoke
// Description: Smoke tests for the v3 stub module. Validates that the
//              toolchain compiles v3-style Verilog and exercises basic
//              delta algebra properties (reset, accumulate, self-inverse).
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

module tb_v3_smoke;

    // =========================================================================
    // Parameters
    // =========================================================================
    parameter DW         = 64;
    parameter CLK_PERIOD = 10.0;  // 100 MHz

    // =========================================================================
    // DUT Signals
    // =========================================================================
    reg              clk;
    reg              rst_n;
    reg  [DW-1:0]    delta_in;
    reg              delta_valid;
    wire [DW-1:0]    accumulator;
    wire             accumulator_zero;

    // =========================================================================
    // Test Infrastructure
    // =========================================================================
    integer test_count;
    integer pass_count;
    integer fail_count;

    // =========================================================================
    // DUT Instantiation
    // =========================================================================
    atomik_v3_stub #(
        .DW(DW)
    ) dut (
        .clk              (clk),
        .rst_n            (rst_n),
        .delta_in         (delta_in),
        .delta_valid      (delta_valid),
        .accumulator      (accumulator),
        .accumulator_zero (accumulator_zero)
    );

    // =========================================================================
    // Clock Generation
    // =========================================================================
    initial begin
        clk = 0;
        forever #(CLK_PERIOD/2) clk = ~clk;
    end

    // =========================================================================
    // VCD Dump
    // =========================================================================
    `ifdef VCD_OUTPUT
    initial begin
        $dumpfile("tb_v3_smoke.vcd");
        $dumpvars(0, tb_v3_smoke);
    end
    `endif

    // =========================================================================
    // Test Tasks
    // =========================================================================
    task reset_dut;
        begin
            rst_n = 0;
            delta_in = 0;
            delta_valid = 0;
            repeat(3) @(posedge clk);
            #1;
            rst_n = 1;
            @(posedge clk);
            #1;
        end
    endtask

    task accumulate;
        input [DW-1:0] delta;
        begin
            @(negedge clk);       // Setup on falling edge
            delta_in = delta;
            delta_valid = 1;
            @(posedge clk);       // DUT samples here
            #1;
            delta_valid = 0;
            delta_in = 0;
        end
    endtask

    task check;
        input [DW-1:0]  exp_accum;
        input            exp_zero;
        input [255:0]    test_name;
        begin
            test_count = test_count + 1;
            @(negedge clk);       // Read after posedge NBA resolves
            if (accumulator !== exp_accum) begin
                $display("FAIL [%0s]: accumulator = %h, expected %h",
                         test_name, accumulator, exp_accum);
                fail_count = fail_count + 1;
            end
            else if (accumulator_zero !== exp_zero) begin
                $display("FAIL [%0s]: accumulator_zero = %b, expected %b",
                         test_name, accumulator_zero, exp_zero);
                fail_count = fail_count + 1;
            end
            else begin
                $display("PASS [%0s]", test_name);
                pass_count = pass_count + 1;
            end
        end
    endtask

    // =========================================================================
    // Main Test Sequence
    // =========================================================================
    initial begin
        $display("==============================================");
        $display("ATOMiK v3 Smoke Test (iverilog)");
        $display("==============================================");
        $display("");

        test_count = 0;
        pass_count = 0;
        fail_count = 0;

        // -----------------------------------------------------------------
        // Test 1: Reset clears accumulator
        // -----------------------------------------------------------------
        $display("--- Test 1: Reset ---");
        reset_dut();
        check(64'h0, 1'b1, "Reset clears accumulator");

        // -----------------------------------------------------------------
        // Test 2: Single accumulate
        // -----------------------------------------------------------------
        $display("--- Test 2: Single accumulate ---");
        accumulate(64'hDEADBEEFCAFEBABE);
        check(64'hDEADBEEFCAFEBABE, 1'b0, "Single accumulate");

        // -----------------------------------------------------------------
        // Test 3: Self-inverse (delta XOR delta = 0)
        // -----------------------------------------------------------------
        $display("--- Test 3: Self-inverse ---");
        reset_dut();
        accumulate(64'hAAAABBBBCCCCDDDD);
        accumulate(64'hAAAABBBBCCCCDDDD);
        check(64'h0, 1'b1, "Self-inverse cancels");

        // -----------------------------------------------------------------
        // Test 4: Commutativity
        // -----------------------------------------------------------------
        $display("--- Test 4: Commutativity ---");
        reset_dut();
        accumulate(64'hFF00FF00FF00FF00);
        accumulate(64'h00FF00FF00FF00FF);
        // FF00FF00FF00FF00 ^ 00FF00FF00FF00FF = FFFFFFFFFFFF FFFF
        check(64'hFFFFFFFFFFFFFFFF, 1'b0, "Commutativity (order A)");

        reset_dut();
        accumulate(64'h00FF00FF00FF00FF);
        accumulate(64'hFF00FF00FF00FF00);
        check(64'hFFFFFFFFFFFFFFFF, 1'b0, "Commutativity (order B)");

        // -----------------------------------------------------------------
        // Summary
        // -----------------------------------------------------------------
        $display("");
        $display("==============================================");
        $display("Test Summary: %0d/%0d passed", pass_count, test_count);
        $display("==============================================");

        if (fail_count == 0) begin
            $display("*** ALL TESTS PASSED ***");
        end else begin
            $display("*** %0d TESTS FAILED ***", fail_count);
        end

        $display("");
        $finish;
    end

    // =========================================================================
    // Timeout Watchdog
    // =========================================================================
    initial begin
        #100000;
        $display("ERROR: Test timeout!");
        $finish;
    end

endmodule
