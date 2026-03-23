// =============================================================================
// ATOMiK v3 Datapath Unit Test
//
// Tests: LOAD, ACCUM, READ, SWAP, XOR cancellation, commutativity,
//        context switch patterns.
// =============================================================================

`timescale 1ns / 1ps

module tb_v3_atomik;

    reg        clk;
    reg        rst_n;
    reg        load_en;
    reg        accum_en;
    reg        swap_en;
    reg  [7:0] addr_in;
    reg  [63:0] delta_in;
    reg  [63:0] init_data;
    wire [63:0] current_state;
    wire        acc_zero;

    atomik_v3_atomik uut (
        .clk           (clk),
        .rst_n         (rst_n),
        .load_en       (load_en),
        .accum_en      (accum_en),
        .swap_en       (swap_en),
        .addr_in       (addr_in),
        .delta_in      (delta_in),
        .init_data     (init_data),
        .current_state (current_state),
        .acc_zero      (acc_zero)
    );

    // Clock generation
    always #5 clk = ~clk;

    integer pass_count = 0;
    integer fail_count = 0;
    integer test_num = 0;

    task check(input [63:0] expected, input [63:0] actual, input [255:0] name);
    begin
        test_num = test_num + 1;
        if (actual === expected) begin
            $display("  PASS [%0d]: %0s = 0x%016h", test_num, name, actual);
            pass_count = pass_count + 1;
        end else begin
            $display("  FAIL [%0d]: %0s = 0x%016h (expected 0x%016h)", test_num, name, actual, expected);
            fail_count = fail_count + 1;
        end
    end
    endtask

    task check1(input expected, input actual, input [255:0] name);
    begin
        test_num = test_num + 1;
        if (actual === expected) begin
            $display("  PASS [%0d]: %0s = %0b", test_num, name, actual);
            pass_count = pass_count + 1;
        end else begin
            $display("  FAIL [%0d]: %0s = %0b (expected %0b)", test_num, name, actual, expected);
            fail_count = fail_count + 1;
        end
    end
    endtask

    // Deassert all control signals
    task idle;
    begin
        load_en  = 0;
        accum_en = 0;
        swap_en  = 0;
        addr_in  = 0;
        delta_in = 0;
        init_data = 0;
    end
    endtask

    // Execute ATOMIK.LOAD: write init_data to state_table[addr], clear acc
    task do_load(input [7:0] addr, input [63:0] data);
    begin
        load_en   = 1;
        addr_in   = addr;
        init_data = data;
        @(posedge clk);
        #1;
        idle;
        // Wait one more cycle for BSRAM read to register
        @(posedge clk);
        #1;
    end
    endtask

    // Execute ATOMIK.ACCUM: acc ^= delta
    task do_accum(input [63:0] delta);
    begin
        accum_en = 1;
        delta_in = delta;
        @(posedge clk);
        #1;
        idle;
    end
    endtask

    // Execute ATOMIK.SWAP: change active address
    task do_swap(input [7:0] addr);
    begin
        swap_en = 1;
        addr_in = addr;
        @(posedge clk);
        #1;
        idle;
        // Wait one cycle for BSRAM read to register
        @(posedge clk);
        #1;
    end
    endtask

    initial begin
        $dumpfile("tb_v3_atomik.vcd");
        $dumpvars(0, tb_v3_atomik);

        clk   = 0;
        rst_n = 0;
        idle;

        // Reset
        repeat (4) @(posedge clk);
        rst_n = 1;
        @(posedge clk);
        #1;

        // =================================================================
        $display("\n=== Test Group 1: LOAD and READ ===");
        // =================================================================

        // LOAD initial state 0xDEADBEEF_CAFEBABE at address 0
        do_load(8'd0, 64'hDEADBEEFCAFEBABE);

        // current_state should be initial_state ^ 0 = initial_state
        check(64'hDEADBEEFCAFEBABE, current_state, "LOAD+READ: initial state");
        check1(1'b1, acc_zero, "acc_zero after LOAD");

        // =================================================================
        $display("\n=== Test Group 2: ACCUM and READ ===");
        // =================================================================

        // ACCUM with delta 0x00000000_00FF00FF
        do_accum(64'h00000000_00FF00FF);
        check1(1'b0, acc_zero, "acc_zero after ACCUM");

        // current_state = 0xDEADBEEFCAFEBABE ^ 0x0000000000FF00FF
        //               = 0xDEADBEEFCA01BA41
        check(64'hDEADBEEFCA01BA41, current_state, "ACCUM+READ: xor delta");

        // =================================================================
        $display("\n=== Test Group 3: XOR Cancellation ===");
        // =================================================================

        // ACCUM same delta again — should cancel
        do_accum(64'h00000000_00FF00FF);
        check(64'hDEADBEEFCAFEBABE, current_state, "XOR cancel: back to initial");
        check1(1'b1, acc_zero, "acc_zero after cancel");

        // =================================================================
        $display("\n=== Test Group 4: Commutativity ===");
        // =================================================================

        // d1 ^ d2 ^ d3 should equal d3 ^ d1 ^ d2
        do_load(8'd0, 64'hDEADBEEFCAFEBABE);

        do_accum(64'h1111111111111111);  // d1
        do_accum(64'h2222222222222222);  // d2
        do_accum(64'h3333333333333333);  // d3

        // acc = d1 ^ d2 ^ d3 = 0x1111...^0x2222...^0x3333... = 0x0000...
        // Wait, 1^2=3, 3^3=0. So acc = 0? Let me recalculate.
        // 0x1111...^0x2222... = 0x3333..., 0x3333...^0x3333... = 0x0000...
        // current_state = initial ^ 0 = initial
        check(64'hDEADBEEFCAFEBABE, current_state, "commutativity d1^d2^d3 (=0)");

        // Now try d3 ^ d1 ^ d2
        do_load(8'd0, 64'hDEADBEEFCAFEBABE);
        do_accum(64'h3333333333333333);  // d3
        do_accum(64'h1111111111111111);  // d1
        do_accum(64'h2222222222222222);  // d2
        check(64'hDEADBEEFCAFEBABE, current_state, "commutativity d3^d1^d2 (=0)");

        // Non-trivial commutativity: d1 ^ d2 vs d2 ^ d1
        do_load(8'd0, 64'h0000000000000000);
        do_accum(64'hAAAAAAAAAAAAAAAA);
        do_accum(64'h5555555555555555);
        // acc = 0xAAAA...^0x5555... = 0xFFFF...
        check(64'hFFFFFFFFFFFFFFFF, current_state, "commutativity d1^d2");

        do_load(8'd0, 64'h0000000000000000);
        do_accum(64'h5555555555555555);
        do_accum(64'hAAAAAAAAAAAAAAAA);
        check(64'hFFFFFFFFFFFFFFFF, current_state, "commutativity d2^d1");

        // =================================================================
        $display("\n=== Test Group 5: SWAP (Context Switch) ===");
        // =================================================================

        // Setup context 0 with initial state and deltas
        do_load(8'd0, 64'h0000000000001111);
        do_accum(64'h0000000000000001);
        // acc=1, state=0x1111^1=0x1110
        check(64'h0000000000001110, current_state, "ctx0: initial^delta");

        // Setup context 1 with different initial state
        do_load(8'd1, 64'h0000000000002222);
        // acc=0 (cleared by LOAD), state=0x2222
        check(64'h0000000000002222, current_state, "ctx1: loaded fresh");

        // SWAP back to context 0 — accumulator persists!
        do_swap(8'd0);
        // acc=0 (was cleared by last LOAD), state=0x1111^0=0x1111
        check(64'h0000000000001111, current_state, "ctx0 after SWAP: acc=0 from LOAD");

        // =================================================================
        $display("\n=== Test Group 6: Multiple Contexts ===");
        // =================================================================

        // Load 4 contexts
        do_load(8'd0, 64'h1000000000000000);
        do_load(8'd1, 64'h2000000000000000);
        do_load(8'd2, 64'h3000000000000000);
        do_load(8'd3, 64'h4000000000000000);

        // Accumulate in context 3 (currently active after last LOAD)
        do_accum(64'h0000000000000001);
        check(64'h4000000000000001, current_state, "ctx3: acc=1");

        // SWAP to context 0 — acc cleared by SWAP, ctx0 had no accumulation
        do_swap(8'd0);
        check(64'h1000000000000000, current_state, "ctx0: clean after SWAP (no acc)");

        // SWAP to context 2 — acc cleared by SWAP, ctx2 had no accumulation
        do_swap(8'd2);
        check(64'h3000000000000000, current_state, "ctx2: clean after SWAP (no acc)");

        // =================================================================
        $display("\n=== Test Group 7: All-bits Patterns ===");
        // =================================================================

        do_load(8'd0, 64'hFFFFFFFFFFFFFFFF);
        check(64'hFFFFFFFFFFFFFFFF, current_state, "all-ones initial");

        do_accum(64'hFFFFFFFFFFFFFFFF);
        // acc = 0 ^ 0xFFFF... = 0xFFFF..., current = 0xFFFF... ^ 0xFFFF... = 0
        check(64'h0000000000000000, current_state, "all-ones XOR all-ones = 0");
        check1(1'b0, acc_zero, "acc_zero: acc=0xFFFF, not zero");

        // Now cancel it
        do_accum(64'hFFFFFFFFFFFFFFFF);
        check(64'hFFFFFFFFFFFFFFFF, current_state, "cancel back to all-ones");
        check1(1'b1, acc_zero, "acc_zero after cancel");

        // =================================================================
        // Summary
        // =================================================================
        $display("\n==============================================");
        $display("Test Summary: %0d/%0d passed", pass_count, pass_count + fail_count);
        $display("==============================================");

        if (fail_count > 0)
            $display("*** %0d TESTS FAILED ***", fail_count);
        else
            $display("*** ALL TESTS PASSED ***");

        $finish;
    end

endmodule
