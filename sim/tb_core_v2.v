// =============================================================================
// ATOMiK Core v2 Integration Testbench
// 
// Module:      tb_core_v2
// Description: Comprehensive testbench for atomik_core_v2 top-level module
// 
// Test Coverage:
//   - Reset behavior
//   - LOAD operation
//   - ACCUMULATE operation (single and back-to-back)
//   - READ operation
//   - Full operation sequence (load → accumulate → read)
//   - Delta algebra properties verification
//   - Debug output verification
//   - Edge cases
//
// Author: ATOMiK Project
// Date:   January 25, 2026
// =============================================================================

`timescale 1ns / 1ps

// Operation code definitions (must match RTL)
`define OP_NOP        2'b00
`define OP_LOAD       2'b01
`define OP_ACCUMULATE 2'b10
`define OP_READ       2'b11

module tb_core_v2;

    // =========================================================================
    // Parameters
    // =========================================================================
    parameter DATA_WIDTH = 64;
    parameter CLK_PERIOD = 10.582;  // 94.5 MHz
    
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
    
    // =========================================================================
    // Test Infrastructure
    // =========================================================================
    integer test_count;
    integer pass_count;
    integer fail_count;
    
    // Variables for property tests
    reg [DATA_WIDTH-1:0] saved_result;
    reg [DATA_WIDTH-1:0] result_a;
    
    // =========================================================================
    // DUT Instantiation
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
        $dumpfile("sim/atomik_core_v2.vcd");
        $dumpvars(0, tb_core_v2);
    end
    `endif
    
    // =========================================================================
    // Test Tasks
    // =========================================================================
    
    // Reset the DUT
    task reset_dut;
        begin
            rst_n = 0;
            operation = `OP_NOP;
            data_in = 0;
            repeat(3) @(posedge clk);
            rst_n = 1;
            @(posedge clk);
        end
    endtask
    
    // Execute NOP
    task do_nop;
        begin
            @(posedge clk);
            operation = `OP_NOP;
            data_in = 0;
        end
    endtask
    
    // Execute LOAD operation
    task do_load;
        input [DATA_WIDTH-1:0] state;
        begin
            @(posedge clk);
            operation = `OP_LOAD;
            data_in = state;
            @(posedge clk);
            operation = `OP_NOP;
            data_in = 0;
        end
    endtask
    
    // Execute ACCUMULATE operation
    task do_accumulate;
        input [DATA_WIDTH-1:0] delta;
        begin
            @(posedge clk);
            operation = `OP_ACCUMULATE;
            data_in = delta;
            @(posedge clk);
            operation = `OP_NOP;
            data_in = 0;
        end
    endtask
    
    // Execute READ operation and return result
    task do_read;
        output [DATA_WIDTH-1:0] result;
        begin
            @(posedge clk);
            operation = `OP_READ;
            @(posedge clk);
            operation = `OP_NOP;
            @(posedge clk);  // Wait for output to be valid
            result = data_out;
        end
    endtask
    
    // Check a condition and report
    task check;
        input              condition;
        input [255:0]      test_name;
        begin
            test_count = test_count + 1;
            if (condition) begin
                $display("PASS [%0s]", test_name);
                pass_count = pass_count + 1;
            end
            else begin
                $display("FAIL [%0s]", test_name);
                fail_count = fail_count + 1;
            end
        end
    endtask
    
    // =========================================================================
    // Main Test Sequence
    // =========================================================================
    initial begin
        $display("==============================================");
        $display("ATOMiK Core v2 Integration Testbench");
        $display("==============================================");
        $display("");
        
        test_count = 0;
        pass_count = 0;
        fail_count = 0;
        saved_result = 0;
        
        // ---------------------------------------------------------------------
        // Test 1: Reset Behavior
        // ---------------------------------------------------------------------
        $display("--- Test 1: Reset Behavior ---");
        reset_dut();
        @(posedge clk);
        
        check(debug_initial_state === 64'h0, "Reset: initial_state = 0");
        check(debug_accumulator === 64'h0, "Reset: accumulator = 0");
        check(accumulator_zero === 1'b1, "Reset: accumulator_zero = 1");
        check(data_out === 64'h0, "Reset: data_out = 0");
        
        // ---------------------------------------------------------------------
        // Test 2: LOAD Operation
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 2: LOAD Operation ---");
        reset_dut();
        do_load(64'h1234567890ABCDEF);
        @(posedge clk);
        
        check(debug_initial_state === 64'h1234567890ABCDEF, "Load: initial_state updated");
        check(debug_accumulator === 64'h0, "Load: accumulator unchanged");
        check(accumulator_zero === 1'b1, "Load: accumulator still zero");
        
        // ---------------------------------------------------------------------
        // Test 3: ACCUMULATE Operation
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 3: ACCUMULATE Operation ---");
        reset_dut();
        do_accumulate(64'hAAAAAAAAAAAAAAAA);
        @(posedge clk);
        
        check(debug_accumulator === 64'hAAAAAAAAAAAAAAAA, "Accumulate: delta stored");
        check(accumulator_zero === 1'b0, "Accumulate: accumulator_zero = 0");
        
        // ---------------------------------------------------------------------
        // Test 4: READ Operation
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 4: READ Operation ---");
        reset_dut();
        do_load(64'hFFFF0000FFFF0000);
        do_accumulate(64'h0000FFFF0000FFFF);
        do_read(saved_result);
        
        // Expected: 0xFFFF0000FFFF0000 ^ 0x0000FFFF0000FFFF = 0xFFFFFFFFFFFFFFFF
        check(saved_result === 64'hFFFFFFFFFFFFFFFF, "Read: correct reconstruction");
        
        // ---------------------------------------------------------------------
        // Test 5: Full Sequence - Multiple Deltas
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 5: Full Sequence ---");
        reset_dut();
        do_load(64'h0000000000000000);
        do_accumulate(64'h1111111111111111);
        do_accumulate(64'h2222222222222222);
        do_accumulate(64'h4444444444444444);
        do_read(saved_result);
        
        // Expected: 0 ^ 0x1111... ^ 0x2222... ^ 0x4444... = 0x7777...
        check(saved_result === 64'h7777777777777777, "Full sequence: multi-delta");
        
        // ---------------------------------------------------------------------
        // Test 6: Self-Inverse Property
        // From Properties.lean: delta_self_inverse
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 6: Self-Inverse Property ---");
        reset_dut();
        do_load(64'hCAFEBABEDEADBEEF);
        do_accumulate(64'h5555555555555555);
        do_accumulate(64'h5555555555555555);  // Same delta again
        do_read(saved_result);
        
        // After δ ⊕ δ, accumulator = 0, so result = initial_state
        check(saved_result === 64'hCAFEBABEDEADBEEF, "Self-inverse: state restored");
        check(accumulator_zero === 1'b1, "Self-inverse: accumulator = 0");
        
        // ---------------------------------------------------------------------
        // Test 7: Commutativity Property
        // From Properties.lean: delta_comm
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 7: Commutativity Property ---");
        
        // Order A: δ₁ then δ₂
        reset_dut();
        do_load(64'h0);
        do_accumulate(64'hABCD1234ABCD1234);
        do_accumulate(64'h5678FEDC5678FEDC);
        do_read(saved_result);
        result_a = saved_result;
        
        // Order B: δ₂ then δ₁
        reset_dut();
        do_load(64'h0);
        do_accumulate(64'h5678FEDC5678FEDC);
        do_accumulate(64'hABCD1234ABCD1234);
        do_read(saved_result);
        
        check(saved_result === result_a, "Commutativity: order irrelevant");
        $display("       Order A = %h, Order B = %h", result_a, saved_result);
        
        // ---------------------------------------------------------------------
        // Test 8: Identity Property
        // From Properties.lean: delta_zero_right
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 8: Identity Property ---");
        reset_dut();
        do_load(64'h123456789ABCDEF0);
        do_accumulate(64'h0000000000000000);  // Zero delta
        do_read(saved_result);
        
        check(saved_result === 64'h123456789ABCDEF0, "Identity: zero delta preserves state");
        
        // ---------------------------------------------------------------------
        // Test 9: data_valid Signal
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 9: data_valid Signal ---");
        reset_dut();
        
        // NOP should not assert valid
        @(posedge clk);
        operation = `OP_NOP;
        @(posedge clk);
        check(data_valid === 1'b0, "NOP: data_valid = 0");
        
        // LOAD should assert valid on the NEXT cycle
        operation = `OP_LOAD;
        data_in = 64'h1;
        @(posedge clk);  // LOAD is captured here
        check(data_valid === 1'b1, "LOAD: data_valid = 1");
        
        // After returning to NOP, valid should clear
        operation = `OP_NOP;
        @(posedge clk);
        check(data_valid === 1'b0, "After LOAD+NOP: data_valid = 0");
        
        // ---------------------------------------------------------------------
        // Test 10: Back-to-Back Operations
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 10: Back-to-Back Operations ---");
        reset_dut();
        
        // Stream of accumulates without NOPs between
        @(posedge clk);
        operation = `OP_LOAD;
        data_in = 64'h0;
        
        @(posedge clk);
        operation = `OP_ACCUMULATE;
        data_in = 64'h1;
        
        @(posedge clk);
        operation = `OP_ACCUMULATE;
        data_in = 64'h2;
        
        @(posedge clk);
        operation = `OP_ACCUMULATE;
        data_in = 64'h4;
        
        @(posedge clk);
        operation = `OP_READ;
        
        @(posedge clk);
        operation = `OP_NOP;
        
        @(posedge clk);
        // Expected: 0 ^ 1 ^ 2 ^ 4 = 7
        check(data_out === 64'h7, "Back-to-back: result = 7");
        
        // ---------------------------------------------------------------------
        // Test 11: Large Values
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 11: Large Values ---");
        reset_dut();
        do_load(64'hFFFFFFFFFFFFFFFF);
        do_accumulate(64'h0000000000000001);
        do_read(saved_result);
        
        check(saved_result === 64'hFFFFFFFFFFFFFFFE, "Large values: all ones - 1 bit");
        
        // ---------------------------------------------------------------------
        // Test 12: Debug Outputs Track State
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 12: Debug Outputs ---");
        reset_dut();
        do_load(64'hAAAA5555AAAA5555);
        do_accumulate(64'h1234123412341234);
        @(posedge clk);
        
        check(debug_initial_state === 64'hAAAA5555AAAA5555, "Debug: initial_state correct");
        check(debug_accumulator === 64'h1234123412341234, "Debug: accumulator correct");
        
        // ---------------------------------------------------------------------
        // Test Summary
        // ---------------------------------------------------------------------
        $display("");
        $display("==============================================");
        $display("Test Summary");
        $display("==============================================");
        $display("Total Tests: %0d", test_count);
        $display("Passed:      %0d", pass_count);
        $display("Failed:      %0d", fail_count);
        $display("");
        
        if (fail_count == 0) begin
            $display("*** ALL TESTS PASSED ***");
            $display("");
        end
        else begin
            $display("*** SOME TESTS FAILED ***");
            $display("");
        end
        
        $finish;
    end
    
    // =========================================================================
    // Timeout Watchdog
    // =========================================================================
    initial begin
        #500000;  // 500us timeout
        $display("ERROR: Test timeout!");
        $finish;
    end

endmodule
