// =============================================================================
// ATOMiK Delta Accumulator Testbench
// 
// Module:      tb_delta_acc
// Description: Comprehensive testbench for atomik_delta_acc module
// 
// Test Coverage:
//   - Reset behavior
//   - Load initial state
//   - Single delta accumulation
//   - Back-to-back delta accumulation
//   - Delta algebra properties (identity, self-inverse, commutativity)
//   - Zero detection
//   - Corner cases (all zeros, all ones, single bits)
//
// Author: ATOMiK Project
// Date:   January 25, 2026
// =============================================================================

`timescale 1ns / 1ps

module tb_delta_acc;

    // =========================================================================
    // Parameters
    // =========================================================================
    parameter DELTA_WIDTH = 64;
    parameter CLK_PERIOD  = 10.582;  // 94.5 MHz (current PLL config)
    
    // =========================================================================
    // DUT Signals
    // =========================================================================
    reg                     clk;
    reg                     rst_n;
    reg  [DELTA_WIDTH-1:0]  delta_in;
    reg                     delta_valid;
    reg  [DELTA_WIDTH-1:0]  initial_state_in;
    reg                     load_initial;
    wire [DELTA_WIDTH-1:0]  initial_state_out;
    wire [DELTA_WIDTH-1:0]  delta_accumulator_out;
    wire                    accumulator_zero;
    
    // =========================================================================
    // Test Infrastructure
    // =========================================================================
    integer test_count;
    integer pass_count;
    integer fail_count;
    
    // Variables for commutativity and associativity tests
    reg [DELTA_WIDTH-1:0] result_a;
    reg [DELTA_WIDTH-1:0] result_seq;
    
    // =========================================================================
    // DUT Instantiation
    // =========================================================================
    atomik_delta_acc #(
        .DELTA_WIDTH(DELTA_WIDTH)
    ) dut (
        .clk                  (clk),
        .rst_n                (rst_n),
        .delta_in             (delta_in),
        .delta_valid          (delta_valid),
        .initial_state_in     (initial_state_in),
        .load_initial         (load_initial),
        .initial_state_out    (initial_state_out),
        .delta_accumulator_out(delta_accumulator_out),
        .accumulator_zero     (accumulator_zero)
    );
    
    // =========================================================================
    // Clock Generation
    // =========================================================================
    initial begin
        clk = 0;
        forever #(CLK_PERIOD/2) clk = ~clk;
    end
    
    // =========================================================================
    // VCD Dump (for waveform viewing)
    // =========================================================================
    `ifdef VCD_OUTPUT
    initial begin
        $dumpfile("sim/atomik_delta_acc.vcd");
        $dumpvars(0, tb_delta_acc);
    end
    `endif
    
    // =========================================================================
    // Test Tasks
    // =========================================================================
    
    // Reset the DUT
    task reset_dut;
        begin
            rst_n = 0;
            delta_in = 0;
            delta_valid = 0;
            initial_state_in = 0;
            load_initial = 0;
            repeat(3) @(posedge clk);
            rst_n = 1;
            @(posedge clk);
        end
    endtask
    
    // Load initial state
    task load_state;
        input [DELTA_WIDTH-1:0] state;
        begin
            @(posedge clk);
            initial_state_in = state;
            load_initial = 1;
            @(posedge clk);
            load_initial = 0;
            initial_state_in = 0;
        end
    endtask
    
    // Accumulate a delta
    task accumulate_delta;
        input [DELTA_WIDTH-1:0] delta;
        begin
            @(posedge clk);
            delta_in = delta;
            delta_valid = 1;
            @(posedge clk);
            delta_valid = 0;
            delta_in = 0;
        end
    endtask
    
    // Check expected values
    task check_values;
        input [DELTA_WIDTH-1:0] exp_initial;
        input [DELTA_WIDTH-1:0] exp_accum;
        input                   exp_zero;
        input [255:0]           test_name;
        begin
            test_count = test_count + 1;
            @(posedge clk);  // Allow one cycle for output to settle
            
            if (initial_state_out !== exp_initial) begin
                $display("FAIL [%0s]: initial_state_out = %h, expected %h",
                         test_name, initial_state_out, exp_initial);
                fail_count = fail_count + 1;
            end
            else if (delta_accumulator_out !== exp_accum) begin
                $display("FAIL [%0s]: delta_accumulator_out = %h, expected %h",
                         test_name, delta_accumulator_out, exp_accum);
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
        $display("ATOMiK Delta Accumulator Testbench");
        $display("==============================================");
        $display("");
        
        test_count = 0;
        pass_count = 0;
        fail_count = 0;
        result_a = 0;
        result_seq = 0;
        
        // ---------------------------------------------------------------------
        // Test 1: Reset Behavior
        // ---------------------------------------------------------------------
        $display("--- Test 1: Reset Behavior ---");
        reset_dut();
        check_values(64'h0, 64'h0, 1'b1, "Reset clears registers");
        
        // ---------------------------------------------------------------------
        // Test 2: Load Initial State
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 2: Load Initial State ---");
        load_state(64'h1234567890ABCDEF);
        check_values(64'h1234567890ABCDEF, 64'h0, 1'b1, "Load initial state");
        
        // ---------------------------------------------------------------------
        // Test 3: Single Delta Accumulation
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 3: Single Delta Accumulation ---");
        reset_dut();
        accumulate_delta(64'hAAAAAAAAAAAAAAAA);
        check_values(64'h0, 64'hAAAAAAAAAAAAAAAA, 1'b0, "Single delta");
        
        // ---------------------------------------------------------------------
        // Test 4: Back-to-Back Accumulation
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 4: Back-to-Back Accumulation ---");
        reset_dut();
        accumulate_delta(64'h1111111111111111);
        accumulate_delta(64'h2222222222222222);
        // 0x1111... XOR 0x2222... = 0x3333...
        check_values(64'h0, 64'h3333333333333333, 1'b0, "Back-to-back deltas");
        
        // ---------------------------------------------------------------------
        // Test 5: Identity Property (δ = 0)
        // From Properties.lean: delta_zero_right
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 5: Identity Property (delta_zero_right) ---");
        reset_dut();
        load_state(64'hDEADBEEFCAFEBABE);
        accumulate_delta(64'h0000000000000000);  // Zero delta
        check_values(64'hDEADBEEFCAFEBABE, 64'h0, 1'b1, "Zero delta = identity");
        
        // ---------------------------------------------------------------------
        // Test 6: Self-Inverse Property (δ ⊕ δ = 0)
        // From Properties.lean: delta_self_inverse
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 6: Self-Inverse Property (delta_self_inverse) ---");
        reset_dut();
        accumulate_delta(64'h5555555555555555);
        accumulate_delta(64'h5555555555555555);  // Same delta again
        check_values(64'h0, 64'h0, 1'b1, "Same delta twice = zero");
        
        // ---------------------------------------------------------------------
        // Test 7: Commutativity (δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁)
        // From Properties.lean: delta_comm
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 7: Commutativity Property (delta_comm) ---");
        
        // Order A: δ₁ then δ₂
        reset_dut();
        accumulate_delta(64'hFF00FF00FF00FF00);
        accumulate_delta(64'h00FF00FF00FF00FF);
        @(posedge clk);
        result_a = delta_accumulator_out;
        
        // Order B: δ₂ then δ₁
        reset_dut();
        accumulate_delta(64'h00FF00FF00FF00FF);
        accumulate_delta(64'hFF00FF00FF00FF00);
        @(posedge clk);
        
        test_count = test_count + 1;
        if (delta_accumulator_out === result_a) begin
            $display("PASS [Commutativity]: Order A = Order B = %h", result_a);
            pass_count = pass_count + 1;
        end
        else begin
            $display("FAIL [Commutativity]: Order A = %h, Order B = %h",
                     result_a, delta_accumulator_out);
            fail_count = fail_count + 1;
        end
        
        // ---------------------------------------------------------------------
        // Test 8: Associativity ((δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃))
        // From Properties.lean: delta_assoc
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 8: Associativity Property (delta_assoc) ---");
        
        // Sequential: δ₁, δ₂, δ₃
        reset_dut();
        accumulate_delta(64'h123456789ABCDEF0);
        accumulate_delta(64'hFEDCBA9876543210);
        accumulate_delta(64'h0F0F0F0F0F0F0F0F);
        @(posedge clk);
        result_seq = delta_accumulator_out;
        
        // Pre-composed: δ₁, (δ₂ ⊕ δ₃)
        reset_dut();
        accumulate_delta(64'h123456789ABCDEF0);
        // δ₂ ⊕ δ₃ = 0xFEDCBA9876543210 ^ 0x0F0F0F0F0F0F0F0F = 0xF1D3B597795B3D1F
        accumulate_delta(64'hF1D3B597795B3D1F);
        @(posedge clk);
        
        test_count = test_count + 1;
        if (delta_accumulator_out === result_seq) begin
            $display("PASS [Associativity]: Sequential = Pre-composed = %h", result_seq);
            pass_count = pass_count + 1;
        end
        else begin
            $display("FAIL [Associativity]: Sequential = %h, Pre-composed = %h",
                     result_seq, delta_accumulator_out);
            fail_count = fail_count + 1;
        end
        
        // ---------------------------------------------------------------------
        // Test 9: Corner Case - All Ones XOR All Ones
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 9: Corner Case - All Ones XOR All Ones ---");
        reset_dut();
        accumulate_delta(64'hFFFFFFFFFFFFFFFF);
        accumulate_delta(64'hFFFFFFFFFFFFFFFF);
        // All ones XOR all ones = all zeros (self-inverse with all ones)
        check_values(64'h0, 64'h0, 1'b1, "All ones XOR all ones");
        
        // ---------------------------------------------------------------------
        // Test 10: Corner Case - Single Bit Toggle
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 10: Corner Case - Single Bit Toggle ---");
        reset_dut();
        accumulate_delta(64'h0000000000000001);  // Bit 0
        accumulate_delta(64'h8000000000000000);  // Bit 63
        check_values(64'h0, 64'h8000000000000001, 1'b0, "Single bit toggles");
        
        // ---------------------------------------------------------------------
        // Test 11: Load Does Not Clear Accumulator
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 11: Load Preserves Accumulator ---");
        reset_dut();
        accumulate_delta(64'hCAFEBABE12345678);
        load_state(64'h1111111111111111);
        check_values(64'h1111111111111111, 64'hCAFEBABE12345678, 1'b0, 
                     "Load preserves accumulator");
        
        // ---------------------------------------------------------------------
        // Test 12: Multiple Operations Sequence
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 12: Multiple Operations Sequence ---");
        reset_dut();
        load_state(64'hAAAAAAAAAAAAAAAA);
        accumulate_delta(64'h5555555555555555);
        accumulate_delta(64'h0F0F0F0F0F0F0F0F);
        accumulate_delta(64'h5555555555555555);  // Cancel second accumulate
        // Final accumulator: 0x5555... ^ 0x0F0F... ^ 0x5555... = 0x0F0F...
        check_values(64'hAAAAAAAAAAAAAAAA, 64'h0F0F0F0F0F0F0F0F, 1'b0,
                     "Multiple operations");
        
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
        #100000;  // 100us timeout
        $display("ERROR: Test timeout!");
        $finish;
    end

endmodule
