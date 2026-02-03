// =============================================================================
// ATOMiK State Reconstructor Testbench
// 
// Module:      tb_state_rec
// Description: Testbench for atomik_state_rec combinational module
// 
// Test Coverage:
//   - Zero inputs
//   - Non-zero reconstruction
//   - Identity (zero accumulator)
//   - All bits toggled
//   - Single bit differences
//   - Combinational timing verification
//
// Author: ATOMiK Project
// Date:   January 25, 2026
// =============================================================================

`timescale 1ns / 1ps

module tb_state_rec;

    // =========================================================================
    // Parameters
    // =========================================================================
    parameter STATE_WIDTH = 64;
    
    // =========================================================================
    // DUT Signals
    // =========================================================================
    reg  [STATE_WIDTH-1:0]  initial_state;
    reg  [STATE_WIDTH-1:0]  delta_accumulator;
    wire [STATE_WIDTH-1:0]  current_state;
    
    // =========================================================================
    // Test Infrastructure
    // =========================================================================
    integer test_count;
    integer pass_count;
    integer fail_count;
    
    // =========================================================================
    // DUT Instantiation
    // =========================================================================
    atomik_state_rec #(
        .STATE_WIDTH(STATE_WIDTH)
    ) dut (
        .initial_state     (initial_state),
        .delta_accumulator (delta_accumulator),
        .current_state     (current_state)
    );
    
    // =========================================================================
    // VCD Dump (for waveform viewing)
    // =========================================================================
    `ifdef VCD_OUTPUT
    initial begin
        $dumpfile("sim/atomik_state_rec.vcd");
        $dumpvars(0, tb_state_rec);
    end
    `endif
    
    // =========================================================================
    // Test Tasks
    // =========================================================================
    
    // Check reconstruction result
    task check_reconstruction;
        input [STATE_WIDTH-1:0] init;
        input [STATE_WIDTH-1:0] accum;
        input [STATE_WIDTH-1:0] expected;
        input [255:0]           test_name;
        begin
            initial_state = init;
            delta_accumulator = accum;
            #1;  // Allow combinational propagation
            
            test_count = test_count + 1;
            
            if (current_state !== expected) begin
                $display("FAIL [%0s]: current_state = %h, expected %h",
                         test_name, current_state, expected);
                $display("      initial_state = %h, delta_accumulator = %h",
                         init, accum);
                fail_count = fail_count + 1;
            end
            else begin
                $display("PASS [%0s]: %h ^ %h = %h",
                         test_name, init, accum, current_state);
                pass_count = pass_count + 1;
            end
        end
    endtask
    
    // =========================================================================
    // Main Test Sequence
    // =========================================================================
    initial begin
        $display("==============================================");
        $display("ATOMiK State Reconstructor Testbench");
        $display("==============================================");
        $display("");
        
        test_count = 0;
        pass_count = 0;
        fail_count = 0;
        
        // Initialize
        initial_state = 0;
        delta_accumulator = 0;
        #10;
        
        // ---------------------------------------------------------------------
        // Test 1: Zero XOR Zero = Zero
        // ---------------------------------------------------------------------
        $display("--- Test 1: Zero Inputs ---");
        check_reconstruction(
            64'h0000000000000000,  // initial
            64'h0000000000000000,  // accum
            64'h0000000000000000,  // expected
            "Zero XOR Zero"
        );
        
        // ---------------------------------------------------------------------
        // Test 2: Identity - Zero Accumulator
        // Verifies: S ⊕ 0 = S
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 2: Identity (Zero Accumulator) ---");
        check_reconstruction(
            64'h1234567890ABCDEF,  // initial
            64'h0000000000000000,  // accum (zero)
            64'h1234567890ABCDEF,  // expected (unchanged)
            "Identity property"
        );
        
        // ---------------------------------------------------------------------
        // Test 3: Basic Reconstruction
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 3: Basic Reconstruction ---");
        check_reconstruction(
            64'hAAAAAAAAAAAAAAAA,  // initial
            64'h5555555555555555,  // accum
            64'hFFFFFFFFFFFFFFFF,  // expected (all ones)
            "Basic reconstruction"
        );
        
        // ---------------------------------------------------------------------
        // Test 4: Self-Inverse - Same Value
        // Verifies: S ⊕ S = 0
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 4: Self-Inverse (Same Value) ---");
        check_reconstruction(
            64'hDEADBEEFCAFEBABE,  // initial
            64'hDEADBEEFCAFEBABE,  // accum (same)
            64'h0000000000000000,  // expected (zero)
            "Self-inverse property"
        );
        
        // ---------------------------------------------------------------------
        // Test 5: All Ones Initial
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 5: All Ones Initial ---");
        check_reconstruction(
            64'hFFFFFFFFFFFFFFFF,  // initial (all ones)
            64'h0F0F0F0F0F0F0F0F,  // accum
            64'hF0F0F0F0F0F0F0F0,  // expected
            "All ones initial"
        );
        
        // ---------------------------------------------------------------------
        // Test 6: Single Bit Difference
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 6: Single Bit Differences ---");
        check_reconstruction(
            64'h0000000000000000,  // initial
            64'h0000000000000001,  // accum (bit 0)
            64'h0000000000000001,  // expected
            "Single bit 0"
        );
        
        check_reconstruction(
            64'h0000000000000000,  // initial
            64'h8000000000000000,  // accum (bit 63)
            64'h8000000000000000,  // expected
            "Single bit 63"
        );
        
        check_reconstruction(
            64'hFFFFFFFFFFFFFFFF,  // initial (all ones)
            64'h0000000000000001,  // accum (bit 0)
            64'hFFFFFFFFFFFFFFFE,  // expected (bit 0 cleared)
            "Toggle bit 0 from all ones"
        );
        
        // ---------------------------------------------------------------------
        // Test 7: Realistic Values
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 7: Realistic Values ---");
        check_reconstruction(
            64'h123456789ABCDEF0,  // initial
            64'hFEDCBA9876543210,  // accum
            64'hECE8ECE0ECE8ECE0,  // expected
            "Realistic values"
        );
        
        // ---------------------------------------------------------------------
        // Test 8: Byte-Aligned Patterns
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 8: Byte-Aligned Patterns ---");
        check_reconstruction(
            64'hFF00FF00FF00FF00,  // initial
            64'h00FF00FF00FF00FF,  // accum
            64'hFFFFFFFFFFFFFFFF,  // expected
            "Alternating bytes"
        );
        
        // ---------------------------------------------------------------------
        // Test 9: Combinational Timing Check
        // Verify output changes immediately with input
        // ---------------------------------------------------------------------
        $display("");
        $display("--- Test 9: Combinational Timing ---");
        initial_state = 64'h0;
        delta_accumulator = 64'h0;
        #1;
        
        // Change input and check output updates immediately
        initial_state = 64'hCAFEBABE12345678;
        #0.1;  // Sub-nanosecond check (combinational)
        
        test_count = test_count + 1;
        if (current_state === 64'hCAFEBABE12345678) begin
            $display("PASS [Combinational timing]: Output follows input immediately");
            pass_count = pass_count + 1;
        end
        else begin
            $display("FAIL [Combinational timing]: Output = %h, expected %h",
                     current_state, 64'hCAFEBABE12345678);
            fail_count = fail_count + 1;
        end
        
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

endmodule
