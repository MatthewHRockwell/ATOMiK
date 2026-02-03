// =============================================================================
// ATOMiK Video Demo Testbench
//
// Module:      tb_video_demo
// Description: Comprehensive testbench for the video domain hardware
//              demonstrator.  Directly exercises atomik_core_v2 at
//              DATA_WIDTH=256, verifying the H.264 delta pipeline with
//              synthetic frame data and performance instrumentation.
//
// Test Plan:
//   Test 1 - Load initial 256-bit frame state
//   Test 2 - Accumulate 10 frame deltas
//   Test 3 - Read and verify reconstructed state (initial XOR all deltas)
//   Test 4 - Verify performance counter recorded operations
//   Test 5 - Verify throughput monitor captured ops count
//
// Author: ATOMiK Project
// Date:   January 27, 2026
// =============================================================================

`timescale 1ns / 1ps

// Operation code definitions (must match atomik_core_v2)
`define OP_NOP        2'b00
`define OP_LOAD       2'b01
`define OP_ACCUMULATE 2'b10
`define OP_READ       2'b11

module tb_video_demo;

    // =========================================================================
    // Parameters
    // =========================================================================
    parameter DATA_WIDTH  = 256;
    parameter CLK_PERIOD  = 37.037;  // 27 MHz = 37.037 ns period
    parameter NUM_DELTAS  = 10;

    // =========================================================================
    // DUT Signals - ATOMiK Core v2
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
    // DUT Signals - Performance Counter
    // =========================================================================
    reg                     perf_start;
    reg                     perf_stop;
    reg                     perf_clear;
    wire [31:0]             perf_count;
    wire                    perf_running;
    wire                    perf_done;

    // =========================================================================
    // DUT Signals - Throughput Monitor
    // =========================================================================
    reg                     tput_enable;
    wire [31:0]             tput_ops_count;
    wire [31:0]             tput_ops_result;
    wire [31:0]             tput_window_count;
    wire                    tput_window_done;

    // =========================================================================
    // Test Infrastructure
    // =========================================================================
    integer                 test_count;
    integer                 pass_count;
    integer                 fail_count;
    integer                 i;
    reg                     perf_done_captured;

    reg [DATA_WIDTH-1:0]    initial_frame;
    reg [DATA_WIDTH-1:0]    deltas [0:NUM_DELTAS-1];
    reg [DATA_WIDTH-1:0]    expected_state;
    reg [DATA_WIDTH-1:0]    read_result;

    // =========================================================================
    // DUT Instantiation - ATOMiK Core v2 (256-bit)
    // =========================================================================
    atomik_core_v2 #(
        .DATA_WIDTH(DATA_WIDTH)
    ) u_core (
        .clk                (clk),
        .rst_n              (rst_n),
        .operation          (operation),
        .data_in            (data_in),
        .data_out           (data_out),
        .data_valid         (data_valid),
        .accumulator_zero   (accumulator_zero),
        .debug_initial_state(debug_initial_state),
        .debug_accumulator  (debug_accumulator)
    );

    // =========================================================================
    // DUT Instantiation - Performance Counter
    // =========================================================================
    perf_counter #(
        .COUNT_WIDTH(32)
    ) u_perf (
        .clk     (clk),
        .rst_n   (rst_n),
        .start   (perf_start),
        .stop    (perf_stop),
        .clear   (perf_clear),
        .count   (perf_count),
        .running (perf_running),
        .done    (perf_done)
    );

    // =========================================================================
    // DUT Instantiation - Throughput Monitor
    // =========================================================================
    throughput_monitor #(
        .WINDOW_CYCLES(100),    // Short window for testbench (100 cycles)
        .COUNT_WIDTH  (32)
    ) u_throughput (
        .clk          (clk),
        .rst_n        (rst_n),
        .enable       (tput_enable),
        .op_valid     (data_valid),
        .ops_count    (tput_ops_count),
        .ops_result   (tput_ops_result),
        .window_count (tput_window_count),
        .window_done  (tput_window_done)
    );

    // =========================================================================
    // Clock Generation (27 MHz)
    // =========================================================================
    initial begin
        clk = 0;
        forever #(CLK_PERIOD / 2) clk = ~clk;
    end

    // =========================================================================
    // VCD Dump
    // =========================================================================
    `ifdef VCD_OUTPUT
    initial begin
        $dumpfile("sim/video_demo.vcd");
        $dumpvars(0, tb_video_demo);
    end
    `endif

    // =========================================================================
    // Test Tasks
    // =========================================================================

    // Reset all DUTs
    task reset_all;
        begin
            rst_n      = 0;
            operation  = `OP_NOP;
            data_in    = {DATA_WIDTH{1'b0}};
            perf_start = 0;
            perf_stop  = 0;
            perf_clear = 0;
            tput_enable = 0;
            repeat (5) @(posedge clk);
            rst_n = 1;
            @(posedge clk);
        end
    endtask

    // Execute LOAD operation
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

    // Execute ACCUMULATE operation
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

    // Execute READ operation and capture result
    task do_read;
        output [DATA_WIDTH-1:0] result;
        begin
            @(posedge clk);
            operation = `OP_READ;
            @(posedge clk);
            operation = `OP_NOP;
            @(posedge clk);  // Wait for output to be registered
            result = data_out;
        end
    endtask

    // Check a condition and report PASS/FAIL
    task check;
        input              condition;
        input [1023:0]     test_name;
        begin
            test_count = test_count + 1;
            if (condition) begin
                $display("  PASS [%0s]", test_name);
                pass_count = pass_count + 1;
            end
            else begin
                $display("  FAIL [%0s]", test_name);
                fail_count = fail_count + 1;
            end
        end
    endtask

    // =========================================================================
    // Main Test Sequence
    // =========================================================================
    initial begin
        $display("");
        $display("==========================================================");
        $display(" ATOMiK Video Domain Hardware Demonstrator - Testbench");
        $display(" DATA_WIDTH = %0d, NUM_DELTAS = %0d", DATA_WIDTH, NUM_DELTAS);
        $display("==========================================================");
        $display("");

        test_count = 0;
        pass_count = 0;
        fail_count = 0;

        // -----------------------------------------------------------------
        // Initialize test data
        // -----------------------------------------------------------------
        // 256-bit initial frame state (synthetic H.264 macroblock data)
        initial_frame = 256'hA5A5_A5A5_DEAD_BEEF_CAFE_BABE_1234_5678_9ABC_DEF0_FACE_FEED_B00B_C0DE_F00D_BEAD;

        // 10 synthetic frame deltas (non-trivial 256-bit values)
        deltas[0] = 256'h0101_0101_0101_0101_0101_0101_0101_0101_0101_0101_0101_0101_0101_0101_0101_0101;
        deltas[1] = 256'h0202_0202_0202_0202_0202_0202_0202_0202_0202_0202_0202_0202_0202_0202_0202_0202;
        deltas[2] = 256'h0404_0404_0404_0404_0404_0404_0404_0404_0404_0404_0404_0404_0404_0404_0404_0404;
        deltas[3] = 256'h0808_0808_0808_0808_0808_0808_0808_0808_0808_0808_0808_0808_0808_0808_0808_0808;
        deltas[4] = 256'h1010_1010_1010_1010_1010_1010_1010_1010_1010_1010_1010_1010_1010_1010_1010_1010;
        deltas[5] = 256'h2020_2020_2020_2020_2020_2020_2020_2020_2020_2020_2020_2020_2020_2020_2020_2020;
        deltas[6] = 256'h4040_4040_4040_4040_4040_4040_4040_4040_4040_4040_4040_4040_4040_4040_4040_4040;
        deltas[7] = 256'h8080_8080_8080_8080_8080_8080_8080_8080_8080_8080_8080_8080_8080_8080_8080_8080;
        deltas[8] = 256'hFF00_FF00_FF00_FF00_FF00_FF00_FF00_FF00_FF00_FF00_FF00_FF00_FF00_FF00_FF00_FF00;
        deltas[9] = 256'h00FF_00FF_00FF_00FF_00FF_00FF_00FF_00FF_00FF_00FF_00FF_00FF_00FF_00FF_00FF_00FF;

        // Compute expected state: initial_frame XOR all deltas
        expected_state = initial_frame;
        for (i = 0; i < NUM_DELTAS; i = i + 1) begin
            expected_state = expected_state ^ deltas[i];
        end

        // -----------------------------------------------------------------
        // Reset
        // -----------------------------------------------------------------
        reset_all();

        // =================================================================
        // Test 1: Load Initial Frame State (256-bit)
        // =================================================================
        $display("--- Test 1: Load initial frame state (256-bit) ---");
        do_load(initial_frame);
        @(posedge clk);

        check(debug_initial_state === initial_frame,
              "LOAD: initial_state matches 256-bit frame");
        check(debug_accumulator === {DATA_WIDTH{1'b0}},
              "LOAD: accumulator is zero after load");
        check(accumulator_zero === 1'b1,
              "LOAD: accumulator_zero flag asserted");
        $display("");

        // =================================================================
        // Test 2: Accumulate 10 Frame Deltas
        // =================================================================
        $display("--- Test 2: Accumulate %0d frame deltas ---", NUM_DELTAS);

        // Start performance counter before accumulation burst
        @(posedge clk);
        perf_clear = 1'b1;
        @(posedge clk);
        perf_clear = 1'b0;

        @(posedge clk);
        perf_start = 1'b1;
        tput_enable = 1'b1;
        @(posedge clk);
        perf_start = 1'b0;

        for (i = 0; i < NUM_DELTAS; i = i + 1) begin
            do_accumulate(deltas[i]);
        end

        // Stop performance counter and capture the done pulse
        @(posedge clk);
        perf_stop = 1'b1;
        @(posedge clk);
        perf_done_captured = perf_done;
        perf_stop = 1'b0;

        // Allow one cycle for accumulator_zero flag to propagate
        @(posedge clk);

        // Verify accumulator is non-zero after non-cancelling deltas
        check(accumulator_zero === 1'b0,
              "ACCUMULATE: accumulator non-zero after 10 deltas");

        // Verify accumulator content: XOR of all deltas
        begin : check_accum_block
            reg [DATA_WIDTH-1:0] expected_accum;
            expected_accum = {DATA_WIDTH{1'b0}};
            for (i = 0; i < NUM_DELTAS; i = i + 1) begin
                expected_accum = expected_accum ^ deltas[i];
            end
            check(debug_accumulator === expected_accum,
                  "ACCUMULATE: accumulator equals XOR of all deltas");
        end
        $display("");

        // =================================================================
        // Test 3: Read and Verify Reconstructed State
        // =================================================================
        $display("--- Test 3: Read and verify reconstructed state ---");
        do_read(read_result);

        $display("  Expected: %064h", expected_state);
        $display("  Got:      %064h", read_result);

        check(read_result === expected_state,
              "READ: reconstructed state = initial XOR all deltas");
        $display("");

        // =================================================================
        // Test 4: Verify Performance Counter Recorded Operations
        // =================================================================
        $display("--- Test 4: Verify performance counter recorded operations ---");

        check(perf_done_captured === 1'b1,
              "PERF: done flag asserted after stop");
        check(perf_running === 1'b0,
              "PERF: counter stopped");
        check(perf_count > 0,
              "PERF: cycle count is non-zero");
        $display("  Performance counter recorded %0d cycles", perf_count);
        $display("");

        // =================================================================
        // Test 5: Verify Throughput Monitor Captured Ops Count
        // =================================================================
        $display("--- Test 5: Verify throughput monitor captured ops count ---");

        // The throughput monitor counts data_valid pulses.
        // We had 10 ACCUMULATEs, each producing a data_valid pulse.
        check(tput_ops_count >= NUM_DELTAS,
              "THROUGHPUT: ops_count >= number of deltas accumulated");
        $display("  Throughput monitor ops_count = %0d", tput_ops_count);

        // Wait for a measurement window to complete (100 cycles in TB)
        repeat (120) @(posedge clk);
        check(tput_ops_result > 0,
              "THROUGHPUT: ops_result captured in measurement window");
        $display("  Throughput monitor ops_result = %0d (per window)", tput_ops_result);
        $display("");

        // =================================================================
        // Bonus: Self-Inverse Property at 256-bit Width
        // =================================================================
        $display("--- Bonus: Self-inverse property (256-bit) ---");
        reset_all();
        do_load(initial_frame);
        do_accumulate(256'hDEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF);
        do_accumulate(256'hDEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF_DEAD_BEEF);
        do_read(read_result);

        check(read_result === initial_frame,
              "SELF-INVERSE: delta XOR delta = identity (state restored)");
        check(accumulator_zero === 1'b1,
              "SELF-INVERSE: accumulator returns to zero");
        $display("");

        // =================================================================
        // Test Summary
        // =================================================================
        $display("==========================================================");
        $display(" Test Summary");
        $display("==========================================================");
        $display(" Total Tests: %0d", test_count);
        $display(" Passed:      %0d", pass_count);
        $display(" Failed:      %0d", fail_count);
        $display("");

        if (fail_count == 0) begin
            $display(" *** ALL TESTS PASSED ***");
        end
        else begin
            $display(" *** SOME TESTS FAILED ***");
        end

        $display("");
        $finish;
    end

    // =========================================================================
    // Timeout Watchdog
    // =========================================================================
    initial begin
        #1_000_000;  // 1 ms timeout
        $display("ERROR: Testbench timeout!");
        $finish;
    end

endmodule
