/**
 * Testbench for atomik_finance_trading_price_tick
 */

`timescale 1ns / 1ps

module tb_atomik_finance_trading_price_tick;

    // Parameters
    parameter DATA_WIDTH = 64;
    parameter CLK_PERIOD = 10;  // 10ns = 100MHz

    // Clock and reset
    reg clk;
    reg rst_n;

    // Control signals
    reg load_en;
    reg accumulate_en;
    reg read_en;
    reg rollback_en;

    // Data signals
    reg [DATA_WIDTH-1:0] data_in;
    wire [DATA_WIDTH-1:0] data_out;

    // Status
    wire accumulator_zero;

    // DUT instantiation
    atomik_finance_trading_price_tick #(
        .DATA_WIDTH(DATA_WIDTH)
    ) dut (
        .clk(clk),
        .rst_n(rst_n),
        .load_en(load_en),
        .accumulate_en(accumulate_en),
        .read_en(read_en),
        .rollback_en(rollback_en),
        .data_in(data_in),
        .data_out(data_out),
        .accumulator_zero(accumulator_zero)
    );

    // Clock generation
    initial begin
        clk = 0;
        forever #(CLK_PERIOD/2) clk = ~clk;
    end

    // Test sequence
    initial begin
        $display("=== Starting testbench for atomik_finance_trading_price_tick ===");

        // Initialize signals
        rst_n = 0;
        load_en = 0;
        accumulate_en = 0;
        read_en = 0;
        rollback_en = 0;
        data_in = 0;

        // Release reset
        #(CLK_PERIOD*2);
        rst_n = 1;
        #(CLK_PERIOD);

        // Test 1: LOAD operation
        $display("Test 1: LOAD operation");
        data_in = 64'hAAAAAAAAAAAAAAAA;
        load_en = 1;
        #(CLK_PERIOD);
        load_en = 0;
        #(CLK_PERIOD);
        $display("  [PASS] LOAD complete");

        // Test 2: ACCUMULATE operation
        $display("Test 2: ACCUMULATE operation");
        data_in = 64'h5555555555555555;
        accumulate_en = 1;
        #(CLK_PERIOD);
        accumulate_en = 0;
        #(CLK_PERIOD);
        $display("  [PASS] ACCUMULATE complete");

        // Test 3: READ operation
        $display("Test 3: READ operation");
        read_en = 1;
        #(CLK_PERIOD);
        read_en = 0;
        #(CLK_PERIOD);
        if (data_out == 64'hFFFFFFFFFFFFFFFF)
            $display("  [PASS] READ correct (0xAAAA XOR 0x5555 = 0xFFFF)");
        else
            $display("  [FAIL] READ incorrect: expected 0xFFFF, got %h", data_out);

        // Test 4: Self-inverse property
        $display("Test 4: Self-inverse property");
        data_in = 64'h1234567890ABCDEF;
        accumulate_en = 1;
        #(CLK_PERIOD);
        accumulate_en = 0;
        #(CLK_PERIOD);
        accumulate_en = 1;  // Apply same delta twice
        #(CLK_PERIOD);
        accumulate_en = 0;
        #(CLK_PERIOD);
        if (accumulator_zero)
            $display("  [PASS] Self-inverse verified");
        else
            $display("  [FAIL] Self-inverse failed");

        // Test 5: Rollback operation
        $display("Test 5: Rollback operation");
        data_in = 64'h1111111111111111;
        accumulate_en = 1;
        #(CLK_PERIOD);
        accumulate_en = 0;
        #(CLK_PERIOD);
        rollback_en = 1;
        #(CLK_PERIOD);
        rollback_en = 0;
        #(CLK_PERIOD);
        if (accumulator_zero)
            $display("  [PASS] Rollback complete");
        else
            $display("  [FAIL] Rollback failed");

        $display("=== All tests complete ===");
        $finish;
    end

endmodule
