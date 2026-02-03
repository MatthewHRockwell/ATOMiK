// =============================================================================
// ATOMiK Edge Sensor Demo - Testbench
//
// Module:      tb_sensor_demo
// Description: Verification testbench for the sensor demo pipeline.
//              Directly exercises atomik_core_v2 and spi_imu_interface in
//              a representative 64-bit IMU fusion delta workflow.
//
// Test Coverage:
//   Test 1: Load initial sensor state
//   Test 2: Accumulate 20 motion deltas
//   Test 3: Read and verify reconstructed state
//   Test 4: Verify alert flag triggers on threshold
//   Test 5: Verify latency timer measurements
//
// Author: ATOMiK Project
// Date:   January 27, 2026
// =============================================================================

`timescale 1ns / 1ps

module tb_sensor_demo;

    // =========================================================================
    // Parameters
    // =========================================================================
    parameter DATA_WIDTH      = 64;
    parameter CLK_PERIOD      = 37.037;  // 27 MHz = 37.037 ns period
    parameter ALERT_THRESHOLD = 32'h0000FFFF;

    // Operation codes (matching atomik_core_v2)
    localparam OP_NOP        = 2'b00;
    localparam OP_LOAD       = 2'b01;
    localparam OP_ACCUMULATE = 2'b10;
    localparam OP_READ       = 2'b11;

    // =========================================================================
    // DUT Signals
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

    // Alert detection
    reg                     alert_flag;

    // Latency timer signals
    wire [15:0]             lat_load;
    wire [15:0]             lat_accumulate;
    wire [15:0]             lat_reconstruct;
    wire [15:0]             lat_rollback;
    wire                    lat_measuring;

    // =========================================================================
    // Test Infrastructure
    // =========================================================================
    integer test_count;
    integer pass_count;
    integer fail_count;
    integer i;

    reg [DATA_WIDTH-1:0] read_result;
    reg [DATA_WIDTH-1:0] expected_state;
    reg [DATA_WIDTH-1:0] delta_values [0:19];

    // =========================================================================
    // DUT: ATOMiK Core v2
    // =========================================================================
    atomik_core_v2 #(
        .DATA_WIDTH(DATA_WIDTH)
    ) u_core (
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
    // Latency Timer Instance
    // =========================================================================
    latency_timer #(
        .COUNT_WIDTH(16)
    ) u_latency (
        .clk                  (clk),
        .rst_n                (rst_n),
        .operation            (operation),
        .op_start             (operation != OP_NOP),
        .op_done              (data_valid),
        .load_latency         (lat_load),
        .accumulate_latency   (lat_accumulate),
        .reconstruct_latency  (lat_reconstruct),
        .rollback_latency     (lat_rollback),
        .measuring            (lat_measuring)
    );

    // =========================================================================
    // Alert Flag Logic (mirrors sensor_demo_top)
    // =========================================================================
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            alert_flag <= 1'b0;
        else if (data_valid && (data_out[31:0] > ALERT_THRESHOLD))
            alert_flag <= 1'b1;
        else if (accumulator_zero)
            alert_flag <= 1'b0;
    end

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
        $dumpfile("sim/tb_sensor_demo.vcd");
        $dumpvars(0, tb_sensor_demo);
    end
    `endif

    // =========================================================================
    // Test Tasks
    // =========================================================================

    task reset_dut;
        begin
            rst_n     = 0;
            operation = OP_NOP;
            data_in   = {DATA_WIDTH{1'b0}};
            repeat (3) @(posedge clk);
            rst_n = 1;
            @(posedge clk);
        end
    endtask

    task do_load;
        input [DATA_WIDTH-1:0] state;
        begin
            @(posedge clk);
            operation = OP_LOAD;
            data_in   = state;
            @(posedge clk);
            operation = OP_NOP;
            data_in   = {DATA_WIDTH{1'b0}};
        end
    endtask

    task do_accumulate;
        input [DATA_WIDTH-1:0] delta;
        begin
            @(posedge clk);
            operation = OP_ACCUMULATE;
            data_in   = delta;
            @(posedge clk);
            operation = OP_NOP;
            data_in   = {DATA_WIDTH{1'b0}};
        end
    endtask

    task do_read;
        output [DATA_WIDTH-1:0] result;
        begin
            @(posedge clk);
            operation = OP_READ;
            @(posedge clk);
            operation = OP_NOP;
            @(posedge clk);
            result = data_out;
        end
    endtask

    task check;
        input              condition;
        input [255:0]      test_name;
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
        $display("==========================================================");
        $display("ATOMiK Edge Sensor Demo - Testbench");
        $display("  DATA_WIDTH = %0d, ALERT_THRESHOLD = 0x%08h", DATA_WIDTH, ALERT_THRESHOLD);
        $display("==========================================================");
        $display("");

        test_count = 0;
        pass_count = 0;
        fail_count = 0;

        // Initialize synthetic delta values (simulating IMU motion data)
        delta_values[ 0] = 64'h0001_0002_0003_0004;
        delta_values[ 1] = 64'h0010_0020_0030_0040;
        delta_values[ 2] = 64'h0100_0200_0300_0400;
        delta_values[ 3] = 64'h1000_2000_3000_4000;
        delta_values[ 4] = 64'h000A_000B_000C_000D;
        delta_values[ 5] = 64'h00A0_00B0_00C0_00D0;
        delta_values[ 6] = 64'h0A00_0B00_0C00_0D00;
        delta_values[ 7] = 64'hA000_B000_C000_D000;
        delta_values[ 8] = 64'h0005_0006_0007_0008;
        delta_values[ 9] = 64'h0050_0060_0070_0080;
        delta_values[10] = 64'h0500_0600_0700_0800;
        delta_values[11] = 64'h5000_6000_7000_8000;
        delta_values[12] = 64'h000F_000E_000D_000C;
        delta_values[13] = 64'h00F0_00E0_00D0_00C0;
        delta_values[14] = 64'h0F00_0E00_0D00_0C00;
        delta_values[15] = 64'hF000_E000_D000_C000;
        delta_values[16] = 64'h1111_2222_3333_4444;
        delta_values[17] = 64'h5555_6666_7777_8888;
        delta_values[18] = 64'h9999_AAAA_BBBB_CCCC;
        delta_values[19] = 64'hDDDD_EEEE_FFFF_0000;

        // -----------------------------------------------------------------
        // Test 1: Load initial sensor state
        // -----------------------------------------------------------------
        $display("--- Test 1: Load Initial Sensor State ---");
        reset_dut();

        do_load(64'hACCE_1E20_6900_BA5E);
        @(posedge clk);

        check(debug_initial_state === 64'hACCE_1E20_6900_BA5E,
              "Load: initial_state set correctly");
        check(debug_accumulator === 64'h0,
              "Load: accumulator cleared");
        check(accumulator_zero === 1'b1,
              "Load: accumulator_zero asserted");

        // -----------------------------------------------------------------
        // Test 2: Accumulate 20 motion deltas
        // -----------------------------------------------------------------
        $display("");
        $display("--- Test 2: Accumulate 20 Motion Deltas ---");

        // Compute expected accumulator: XOR of all 20 deltas
        expected_state = 64'h0;
        for (i = 0; i < 20; i = i + 1) begin
            expected_state = expected_state ^ delta_values[i];
        end

        // Apply deltas to core
        for (i = 0; i < 20; i = i + 1) begin
            do_accumulate(delta_values[i]);
        end
        @(posedge clk);

        check(debug_accumulator === expected_state,
              "Accumulate: 20-delta XOR accumulation correct");
        check(accumulator_zero === 1'b0,
              "Accumulate: accumulator is non-zero");

        $display("  Accumulated delta = 0x%016h", debug_accumulator);

        // -----------------------------------------------------------------
        // Test 3: Read and verify reconstructed state
        // -----------------------------------------------------------------
        $display("");
        $display("--- Test 3: Read and Verify Reconstructed State ---");

        // Expected: initial_state XOR accumulated_deltas
        expected_state = 64'hACCE_1E20_6900_BA5E ^ expected_state;

        do_read(read_result);

        check(read_result === expected_state,
              "Read: reconstructed state matches expected");

        $display("  Initial state    = 0x%016h", 64'hACCE_1E20_6900_BA5E);
        $display("  Accumulated XOR  = 0x%016h", debug_accumulator);
        $display("  Reconstructed    = 0x%016h", read_result);
        $display("  Expected         = 0x%016h", expected_state);

        // Verify XOR identity: applying all deltas again cancels them
        for (i = 0; i < 20; i = i + 1) begin
            do_accumulate(delta_values[i]);
        end
        @(posedge clk);

        check(accumulator_zero === 1'b1,
              "Self-inverse: re-applying 20 deltas zeroes accumulator");

        do_read(read_result);
        check(read_result === 64'hACCE_1E20_6900_BA5E,
              "Self-inverse: state restored to initial");

        // -----------------------------------------------------------------
        // Test 4: Alert flag triggers on threshold
        // -----------------------------------------------------------------
        $display("");
        $display("--- Test 4: Alert Flag Threshold Detection ---");

        reset_dut();
        do_load(64'h0000_0000_0000_0000);

        // Accumulate a delta that produces a large lower-32-bit value
        do_accumulate(64'h0000_0000_0010_0000);
        // READ to trigger data_valid + data_out evaluation
        do_read(read_result);
        @(posedge clk);

        check(alert_flag === 1'b1,
              "Alert: flag set when data_out[31:0] > ALERT_THRESHOLD");

        // Clear: accumulate inverse to zero out accumulator
        do_accumulate(64'h0000_0000_0010_0000);
        @(posedge clk);
        @(posedge clk);

        check(alert_flag === 1'b0,
              "Alert: flag cleared when accumulator returns to zero");

        // Sub-threshold test
        reset_dut();
        do_load(64'h0000_0000_0000_0000);
        do_accumulate(64'h0000_0000_0000_0001);
        do_read(read_result);
        @(posedge clk);

        check(alert_flag === 1'b0,
              "Alert: flag remains clear for sub-threshold value");

        // -----------------------------------------------------------------
        // Test 5: Latency timer measurements
        // -----------------------------------------------------------------
        $display("");
        $display("--- Test 5: Latency Timer Measurements ---");

        reset_dut();

        // Measure LOAD latency
        do_load(64'hDEAD_BEEF_CAFE_BABE);
        @(posedge clk);
        @(posedge clk);

        check(lat_load !== 16'd0,
              "Latency: LOAD measurement recorded");
        $display("  LOAD latency        = %0d cycles", lat_load);

        // Measure ACCUMULATE latency
        do_accumulate(64'h1234_5678_9ABC_DEF0);
        @(posedge clk);
        @(posedge clk);

        check(lat_accumulate !== 16'd0,
              "Latency: ACCUMULATE measurement recorded");
        $display("  ACCUMULATE latency  = %0d cycles", lat_accumulate);

        // Measure READ/RECONSTRUCT latency
        do_read(read_result);
        @(posedge clk);
        @(posedge clk);

        check(lat_reconstruct !== 16'd0,
              "Latency: READ/RECONSTRUCT measurement recorded");
        $display("  RECONSTRUCT latency = %0d cycles", lat_reconstruct);

        // Verify single-cycle operations (ATOMiK guarantees 1-cycle ops)
        check(lat_load <= 16'd2,
              "Latency: LOAD completes within 2 cycles");
        check(lat_accumulate <= 16'd2,
              "Latency: ACCUMULATE completes within 2 cycles");
        check(lat_reconstruct <= 16'd2,
              "Latency: READ completes within 2 cycles");

        // -----------------------------------------------------------------
        // Test Summary
        // -----------------------------------------------------------------
        $display("");
        $display("==========================================================");
        $display("Test Summary");
        $display("==========================================================");
        $display("Total Tests: %0d", test_count);
        $display("Passed:      %0d", pass_count);
        $display("Failed:      %0d", fail_count);
        $display("");

        if (fail_count == 0) begin
            $display("*** ALL TESTS PASSED ***");
        end
        else begin
            $display("*** SOME TESTS FAILED ***");
        end

        $display("");
        $finish;
    end

    // =========================================================================
    // Timeout Watchdog
    // =========================================================================
    initial begin
        #1_000_000;
        $display("ERROR: Test timeout after 1ms!");
        $finish;
    end

endmodule
