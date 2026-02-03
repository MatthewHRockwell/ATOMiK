// =============================================================================
// ATOMiK Finance Demo Testbench
//
// Module:      tb_finance_demo
// Description: Comprehensive testbench for the finance domain hardware
//              demonstrator. Exercises the 64-bit price tick delta pipeline
//              through five test scenarios covering load, high-throughput
//              burst accumulation, read-back, deep rollback, and multi-field
//              accumulation.
//
// Test Coverage:
//   Test 1: Load initial price state
//   Test 2: Accumulate 100 price tick deltas (high-throughput burst)
//   Test 3: Read and verify reconstructed state
//   Test 4: Deep rollback test (verify rollback depth 4096)
//   Test 5: Three-field accumulation (price, volume, flags in 64-bit)
//
// Author: ATOMiK Project
// Date:   January 2026
// =============================================================================

`timescale 1ns / 1ps

`define OP_NOP        2'b00
`define OP_LOAD       2'b01
`define OP_ACCUMULATE 2'b10
`define OP_READ       2'b11

module tb_finance_demo;

    // =========================================================================
    // Parameters
    // =========================================================================
    parameter DATA_WIDTH  = 64;
    parameter CLK_PERIOD  = 37.037;  // 27 MHz = 37.037 ns period

    // =========================================================================
    // DUT Signals (direct core connection for testbench)
    // =========================================================================
    reg                     clk;
    reg                     rst_n;
    reg  [1:0]              operation;
    reg  [DATA_WIDTH-1:0]   data_in;
    wire [DATA_WIDTH-1:0]   data_out;
    wire                    data_valid;
    wire                    accumulator_zero;
    wire [DATA_WIDTH-1:0]   debug_initial_state;
    wire [DATA_WIDTH-1:0]   debug_accumulator;

    // =========================================================================
    // Test Infrastructure
    // =========================================================================
    integer test_num;
    integer pass_count;
    integer fail_count;
    integer i;

    reg [DATA_WIDTH-1:0] read_result;
    reg [DATA_WIDTH-1:0] expected;
    reg [DATA_WIDTH-1:0] running_xor;

    // =========================================================================
    // DUT: atomik_core_v2 (64-bit for finance demo)
    // =========================================================================
    atomik_core_v2 #(
        .DATA_WIDTH(DATA_WIDTH)
    ) dut (
        .clk               (clk),
        .rst_n             (rst_n),
        .operation         (operation),
        .data_in           (data_in),
        .data_out          (data_out),
        .data_valid        (data_valid),
        .accumulator_zero  (accumulator_zero),
        .debug_initial_state(debug_initial_state),
        .debug_accumulator (debug_accumulator)
    );

    // =========================================================================
    // Clock Generation (27 MHz)
    // =========================================================================
    initial begin
        clk = 0;
        forever #(CLK_PERIOD / 2.0) clk = ~clk;
    end

    // =========================================================================
    // VCD Dump
    // =========================================================================
    `ifdef VCD_OUTPUT
    initial begin
        $dumpfile("tb_finance_demo.vcd");
        $dumpvars(0, tb_finance_demo);
    end
    `endif

    // =========================================================================
    // Helper Tasks
    // =========================================================================

    task reset_dut;
        begin
            rst_n     = 0;
            operation = `OP_NOP;
            data_in   = {DATA_WIDTH{1'b0}};
            repeat (5) @(posedge clk);
            rst_n = 1;
            @(posedge clk);
        end
    endtask

    task do_load;
        input [DATA_WIDTH-1:0] state;
        begin
            @(posedge clk);
            operation = `OP_LOAD;
            data_in   = state;
            @(posedge clk);
            operation = `OP_NOP;
            data_in   = {DATA_WIDTH{1'b0}};
        end
    endtask

    task do_accumulate;
        input [DATA_WIDTH-1:0] delta;
        begin
            @(posedge clk);
            operation = `OP_ACCUMULATE;
            data_in   = delta;
            @(posedge clk);
            operation = `OP_NOP;
            data_in   = {DATA_WIDTH{1'b0}};
        end
    endtask

    task do_read;
        output [DATA_WIDTH-1:0] result;
        begin
            @(posedge clk);
            operation = `OP_READ;
            @(posedge clk);
            operation = `OP_NOP;
            @(posedge clk);
            result = data_out;
        end
    endtask

    task check;
        input              condition;
        input [639:0]      msg;
        begin
            if (condition) begin
                pass_count = pass_count + 1;
                $display("  PASS: %0s", msg);
            end else begin
                fail_count = fail_count + 1;
                $display("  FAIL: %0s", msg);
            end
        end
    endtask

    // =========================================================================
    // Main Test Sequence
    // =========================================================================
    initial begin
        $display("==============================================================");
        $display("ATOMiK Finance Domain Hardware Demonstrator - Testbench");
        $display("  DATA_WIDTH = %0d, Clock = 27 MHz", DATA_WIDTH);
        $display("==============================================================");
        $display("");

        pass_count = 0;
        fail_count = 0;

        // -----------------------------------------------------------------
        // Test 1: Load initial price state
        // -----------------------------------------------------------------
        test_num = 1;
        $display("--- Test %0d: Load Initial Price State ---", test_num);
        reset_dut();

        do_load(64'h0000_0100_0000_0000);  // Initial price = 256.0 (fixed point)
        @(posedge clk);

        check(debug_initial_state === 64'h0000_0100_0000_0000,
              "Initial price state loaded correctly");
        check(debug_accumulator === 64'h0,
              "Accumulator cleared after LOAD");
        check(accumulator_zero === 1'b1,
              "accumulator_zero flag set after LOAD");

        // -----------------------------------------------------------------
        // Test 2: Accumulate 100 price tick deltas (high-throughput burst)
        // -----------------------------------------------------------------
        test_num = 2;
        $display("");
        $display("--- Test %0d: Accumulate 100 Price Tick Deltas ---", test_num);
        reset_dut();

        do_load(64'hDEAD_BEEF_CAFE_BABE);

        // Accumulate 100 deltas using back-to-back streaming
        // Each delta = index value (0x01, 0x02, ... 0x64)
        running_xor = {DATA_WIDTH{1'b0}};
        @(posedge clk);
        for (i = 1; i <= 100; i = i + 1) begin
            operation = `OP_ACCUMULATE;
            data_in   = {DATA_WIDTH{1'b0}} | i[DATA_WIDTH-1:0];
            running_xor = running_xor ^ ({DATA_WIDTH{1'b0}} | i[DATA_WIDTH-1:0]);
            @(posedge clk);
        end
        operation = `OP_NOP;
        data_in   = {DATA_WIDTH{1'b0}};
        @(posedge clk);

        check(debug_accumulator === running_xor,
              "100-delta burst: accumulator matches expected XOR");

        // Verify reconstructed state
        do_read(read_result);
        expected = 64'hDEAD_BEEF_CAFE_BABE ^ running_xor;
        check(read_result === expected,
              "100-delta burst: reconstructed state correct");
        $display("    Accumulated XOR = 0x%016h", running_xor);
        $display("    Expected state  = 0x%016h", expected);
        $display("    Read result     = 0x%016h", read_result);

        // -----------------------------------------------------------------
        // Test 3: Read and verify reconstructed state (multi-step)
        // -----------------------------------------------------------------
        test_num = 3;
        $display("");
        $display("--- Test %0d: Read and Verify Reconstructed State ---", test_num);
        reset_dut();

        // Load a known initial price
        do_load(64'hAAAA_5555_AAAA_5555);

        // Apply three specific price deltas
        do_accumulate(64'h0000_FFFF_0000_FFFF);
        do_accumulate(64'hFFFF_0000_FFFF_0000);
        do_accumulate(64'h1234_5678_9ABC_DEF0);

        // Expected accumulator: 0x0000FFFF0000FFFF ^ 0xFFFF0000FFFF0000 ^ 0x123456789ABCDEF0
        //   Step 1: 0x0000FFFF0000FFFF ^ 0xFFFF0000FFFF0000 = 0xFFFFFFFFFFFFFFFF
        //   Step 2: 0xFFFFFFFFFFFFFFFF ^ 0x123456789ABCDEF0 = 0xEDCBA9876543210F
        // Expected state: 0xAAAA5555AAAA5555 ^ 0xEDCBA9876543210F = 0x4761FCD2CFE9745A

        do_read(read_result);
        expected = 64'hAAAA_5555_AAAA_5555 ^ 64'hEDCB_A987_6543_210F;
        check(read_result === expected,
              "Three-delta reconstruction verified");
        $display("    Expected = 0x%016h, Got = 0x%016h", expected, read_result);

        // -----------------------------------------------------------------
        // Test 4: Deep rollback test (verify rollback depth 4096)
        //
        // The rollback property of XOR-based delta accumulation:
        // Accumulating the same delta twice cancels it out (self-inverse).
        // We verify this works even after 4096 accumulated deltas.
        // -----------------------------------------------------------------
        test_num = 4;
        $display("");
        $display("--- Test %0d: Deep Rollback Test (depth 4096) ---", test_num);
        reset_dut();

        do_load(64'hFEED_FACE_0BAD_CAFE);

        // Accumulate 4096 unique deltas
        running_xor = {DATA_WIDTH{1'b0}};
        @(posedge clk);
        for (i = 1; i <= 4096; i = i + 1) begin
            operation = `OP_ACCUMULATE;
            // Generate pseudo-random deltas: i XOR shifted pattern
            data_in   = {i[15:0], i[15:0] ^ 16'hA5A5, i[15:0] ^ 16'h5A5A, i[15:0] ^ 16'hFFFF};
            running_xor = running_xor ^ {i[15:0], i[15:0] ^ 16'hA5A5, i[15:0] ^ 16'h5A5A, i[15:0] ^ 16'hFFFF};
            @(posedge clk);
        end
        operation = `OP_NOP;
        @(posedge clk);

        // Verify state after 4096 deltas
        do_read(read_result);
        expected = 64'hFEED_FACE_0BAD_CAFE ^ running_xor;
        check(read_result === expected,
              "4096-delta deep state correct");

        // Now "rollback" by re-accumulating the same 4096 deltas
        // (XOR self-inverse: applying same deltas again cancels them)
        @(posedge clk);
        for (i = 1; i <= 4096; i = i + 1) begin
            operation = `OP_ACCUMULATE;
            data_in   = {i[15:0], i[15:0] ^ 16'hA5A5, i[15:0] ^ 16'h5A5A, i[15:0] ^ 16'hFFFF};
            @(posedge clk);
        end
        operation = `OP_NOP;
        @(posedge clk);

        // After rollback, accumulator should be zero (all deltas cancelled)
        check(accumulator_zero === 1'b1,
              "4096-delta rollback: accumulator returned to zero");

        // State should equal original initial state
        do_read(read_result);
        check(read_result === 64'hFEED_FACE_0BAD_CAFE,
              "4096-delta rollback: state restored to initial");
        $display("    Restored state = 0x%016h", read_result);

        // -----------------------------------------------------------------
        // Test 5: Three-field accumulation (price, volume, flags in 64-bit)
        //
        // Finance use case: pack three fields into 64 bits:
        //   [63:32] price delta (32-bit)
        //   [31:16] volume delta (16-bit)
        //   [15:0]  flags/metadata (16-bit)
        //
        // XOR accumulation preserves field independence since XOR has
        // no carry propagation across bit boundaries.
        // -----------------------------------------------------------------
        test_num = 5;
        $display("");
        $display("--- Test %0d: Three-Field Accumulation ---", test_num);
        reset_dut();

        // Initial state: price=0x00010000, volume=0x0100, flags=0x000F
        do_load(64'h0001_0000_0100_000F);

        // Delta 1: price_delta=+0x00000005, volume_delta=+0x0003, flags_delta=0x0001
        do_accumulate(64'h0000_0005_0003_0001);

        // Delta 2: price_delta=+0x0000000A, volume_delta=+0x0007, flags_delta=0x0002
        do_accumulate(64'h0000_000A_0007_0002);

        do_read(read_result);

        // Expected accumulator: 0x00000005_00030001 ^ 0x0000000A_00070002
        //   = 0x0000000F_00040003
        // Expected state: 0x00010000_0100000F ^ 0x0000000F_0004_0003
        //   = 0x0001000F_0104_000C
        expected = 64'h0001_0000_0100_000F ^ 64'h0000_000F_0004_0003;

        // Verify full 64-bit result
        check(read_result === expected,
              "Three-field full 64-bit result correct");

        // Verify individual fields
        check(read_result[63:32] === expected[63:32],
              "Price field (bits 63:32) correct");
        check(read_result[31:16] === expected[31:16],
              "Volume field (bits 31:16) correct");
        check(read_result[15:0] === expected[15:0],
              "Flags field (bits 15:0) correct");
        $display("    Result: price=0x%08h vol=0x%04h flags=0x%04h",
                 read_result[63:32], read_result[31:16], read_result[15:0]);

        // -----------------------------------------------------------------
        // Test Summary
        // -----------------------------------------------------------------
        $display("");
        $display("==============================================================");
        $display("Finance Demo Testbench Summary");
        $display("==============================================================");
        $display("  Tests passed: %0d", pass_count);
        $display("  Tests failed: %0d", fail_count);
        $display("");

        if (fail_count == 0) begin
            $display("*** ALL TESTS PASSED ***");
        end else begin
            $display("*** %0d TEST(S) FAILED ***", fail_count);
        end

        $display("");
        $finish;
    end

    // =========================================================================
    // Timeout Watchdog
    // =========================================================================
    initial begin
        #50_000_000;  // 50 ms timeout
        $display("ERROR: Testbench timeout!");
        $finish;
    end

endmodule
