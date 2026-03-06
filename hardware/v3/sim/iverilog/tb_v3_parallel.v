// =============================================================================
// ATOMiK v3 Parallel Accumulator Testbench (iverilog)
//
// Tests: N=1 and N=4 configurations. Verifies round-robin distribution,
// parallel mode, XOR merge tree, commutativity, BSRAM state table, SWAP.
// =============================================================================

`timescale 1ns / 1ps

module tb_v3_parallel;

    // --- Test infrastructure ---
    integer test_count = 0;
    integer pass_count = 0;
    integer fail_count = 0;

    task check64;
        input [63:0] got;
        input [63:0] expected;
        input [255:0] name;
        begin
            test_count = test_count + 1;
            if (got === expected) begin
                pass_count = pass_count + 1;
                $display("PASS [%0s]", name);
            end else begin
                fail_count = fail_count + 1;
                $display("FAIL [%0s]: got %016x, expected %016x", name, got, expected);
            end
        end
    endtask

    task check1;
        input got;
        input expected;
        input [255:0] name;
        begin
            test_count = test_count + 1;
            if (got === expected) begin
                pass_count = pass_count + 1;
                $display("PASS [%0s]", name);
            end else begin
                fail_count = fail_count + 1;
                $display("FAIL [%0s]: got %0b, expected %0b", name, got, expected);
            end
        end
    endtask

    // =========================================================================
    // N=1 instance (baseline — should behave identically to atomik_v3_atomik)
    // =========================================================================
    reg         clk, rst_n;
    reg         load_en, accum_en, swap_en;
    reg  [7:0]  addr_in;
    reg  [63:0] delta_in, init_data;
    wire [63:0] cs1;
    wire        az1;

    atomik_v3_parallel #(.N_BANKS(1)) u_n1 (
        .clk(clk), .rst_n(rst_n),
        .load_en(load_en), .accum_en(accum_en), .swap_en(swap_en),
        .addr_in(addr_in), .delta_in(delta_in), .init_data(init_data),
        .delta_parallel_in(64'b0), .delta_parallel_valid(1'b0), .parallel_mode(1'b0),
        .current_state(cs1), .acc_zero(az1),
        .current_bank(), .bank_active()
    );

    // =========================================================================
    // N=4 instance
    // =========================================================================
    reg  [4*64-1:0] par_delta_in;
    reg  [3:0]      par_valid;
    reg             par_mode;
    wire [63:0]     cs4;
    wire            az4;
    wire [1:0]      bank4;
    wire [3:0]      active4;

    atomik_v3_parallel #(.N_BANKS(4)) u_n4 (
        .clk(clk), .rst_n(rst_n),
        .load_en(load_en), .accum_en(accum_en), .swap_en(swap_en),
        .addr_in(addr_in), .delta_in(delta_in), .init_data(init_data),
        .delta_parallel_in(par_delta_in), .delta_parallel_valid(par_valid),
        .parallel_mode(par_mode),
        .current_state(cs4), .acc_zero(az4),
        .current_bank(bank4), .bank_active(active4)
    );

    localparam CLK_PERIOD = 10.0;
    always #(CLK_PERIOD/2) clk = ~clk;

    // Helper: one clock cycle
    task tick;
        begin @(posedge clk); #1; end
    endtask

    // Helper: LOAD operation
    task do_load;
        input [7:0]  addr;
        input [63:0] data;
        begin
            @(negedge clk);
            load_en  = 1;
            addr_in  = addr;
            init_data = data;
            @(posedge clk); #1;
            load_en = 0;
            // Wait for BSRAM read to register
            tick;
        end
    endtask

    // Helper: ACCUM operation
    task do_accum;
        input [63:0] delta;
        begin
            @(negedge clk);
            accum_en = 1;
            delta_in = delta;
            @(posedge clk); #1;
            accum_en = 0;
        end
    endtask

    // Helper: SWAP operation
    task do_swap;
        input [7:0] addr;
        begin
            @(negedge clk);
            swap_en = 1;
            addr_in = addr;
            @(posedge clk); #1;
            swap_en = 0;
            // Wait for swap_pending to clear acc (1 cycle delay)
            tick;
            // Wait for BSRAM read to register new value
            tick;
        end
    endtask

    initial begin
        $display("==============================================");
        $display("ATOMiK v3 Parallel Accumulator Testbench");
        $display("==============================================");
        $display("");

        // Initialize
        clk = 0; rst_n = 0;
        load_en = 0; accum_en = 0; swap_en = 0;
        addr_in = 0; delta_in = 0; init_data = 0;
        par_delta_in = 0; par_valid = 0; par_mode = 0;

        // Reset
        tick; tick;
        rst_n = 1;
        tick;

        // =================================================================
        // Test 1: N=1 basic — LOAD, ACCUM, READ
        // =================================================================
        $display("--- N=1 Basic Tests ---");
        do_load(8'd0, 64'hAAAA_BBBB_CCCC_DDDD);
        check64(cs1, 64'hAAAA_BBBB_CCCC_DDDD, "N1: load initial state");
        check1(az1, 1'b1, "N1: acc zero after load");

        do_accum(64'h1111_1111_1111_1111);
        tick;  // Wait for BSRAM read to update
        check64(cs1, 64'hAAAA_BBBB_CCCC_DDDD ^ 64'h1111_1111_1111_1111, "N1: accum one delta");
        check1(az1, 1'b0, "N1: acc not zero after accum");

        // Self-inverse
        do_accum(64'h1111_1111_1111_1111);
        tick;
        check64(cs1, 64'hAAAA_BBBB_CCCC_DDDD, "N1: self-inverse (XOR cancel)");
        check1(az1, 1'b1, "N1: acc zero after cancel");

        // =================================================================
        // Test 2: N=4 round-robin distribution
        // =================================================================
        $display("");
        $display("--- N=4 Round-Robin Tests ---");
        par_mode = 0;  // round-robin

        do_load(8'd0, 64'hDEAD_BEEF_CAFE_BABE);
        check64(cs4, 64'hDEAD_BEEF_CAFE_BABE, "N4: load initial state");
        check1(az4, 1'b1, "N4: acc zero after load");

        // Four ACCUM operations go to banks 0, 1, 2, 3
        do_accum(64'h0000_0000_0000_0001);  // bank 0
        do_accum(64'h0000_0000_0000_0002);  // bank 1
        do_accum(64'h0000_0000_0000_0004);  // bank 2
        do_accum(64'h0000_0000_0000_0008);  // bank 3
        tick;

        // Merged = 0x1 ^ 0x2 ^ 0x4 ^ 0x8 = 0xF
        check64(cs4, 64'hDEAD_BEEF_CAFE_BABE ^ 64'h000F,
                "N4: round-robin 4 deltas merged");

        // =================================================================
        // Test 3: N=4 parallel mode
        // =================================================================
        $display("");
        $display("--- N=4 Parallel Mode Tests ---");
        par_mode = 1;

        do_load(8'd1, 64'hFFFF_FFFF_FFFF_FFFF);
        check64(cs4, 64'hFFFF_FFFF_FFFF_FFFF, "N4 par: load all-ones");

        // Feed all 4 banks simultaneously
        @(negedge clk);
        accum_en = 0;  // parallel_mode uses delta_parallel_valid
        par_delta_in = {64'hF000_0000_0000_0000,   // bank 3
                        64'h0F00_0000_0000_0000,    // bank 2
                        64'h00F0_0000_0000_0000,    // bank 1
                        64'h000F_0000_0000_0000};   // bank 0
        par_valid = 4'b1111;
        @(posedge clk); #1;
        par_valid = 4'b0000;
        tick;

        // Merged = 0xF000... ^ 0x0F00... ^ 0x00F0... ^ 0x000F... = 0xFFFF_0000_0000_0000
        check64(cs4, 64'hFFFF_FFFF_FFFF_FFFF ^ 64'hFFFF_0000_0000_0000,
                "N4 par: 4 parallel deltas merged");

        // =================================================================
        // Test 4: Commutativity — order doesn't matter
        // =================================================================
        $display("");
        $display("--- Commutativity Tests ---");
        par_mode = 0;

        // Run same 3 deltas in forward order
        do_load(8'd2, 64'h0);
        do_accum(64'h1111_1111_1111_1111);
        do_accum(64'h2222_2222_2222_2222);
        do_accum(64'h4444_4444_4444_4444);
        tick;

        begin : comm_block
            reg [63:0] forward_result;
            forward_result = cs4;

            // Now reverse order
            do_load(8'd2, 64'h0);
            do_accum(64'h4444_4444_4444_4444);
            do_accum(64'h2222_2222_2222_2222);
            do_accum(64'h1111_1111_1111_1111);
            tick;

            check64(cs4, forward_result, "N4: commutativity (reverse order)");
        end

        // =================================================================
        // Test 5: SWAP — updates reference, clears accumulator
        // =================================================================
        $display("");
        $display("--- SWAP Tests ---");
        par_mode = 0;

        do_load(8'd3, 64'hAAAA_0000_0000_0000);
        do_accum(64'h5555_0000_0000_0000);
        tick;
        // current_state = 0xAAAA ^ 0x5555 = 0xFFFF_0000_0000_0000
        check64(cs4, 64'hFFFF_0000_0000_0000, "N4: pre-swap state");

        do_swap(8'd3);
        // After SWAP: ref updated to current_state, acc cleared
        // current_state = new_ref ^ 0 = 0xFFFF_0000_0000_0000
        check64(cs4, 64'hFFFF_0000_0000_0000, "N4: state unchanged after swap");
        check1(az4, 1'b1, "N4: acc zero after swap");

        // Now accum from new reference
        do_accum(64'h0000_FFFF_0000_0000);
        tick;
        check64(cs4, 64'hFFFF_FFFF_0000_0000, "N4: accum from new reference");

        // =================================================================
        // Test 6: Multi-context — different addresses
        // =================================================================
        $display("");
        $display("--- Multi-Context Tests ---");
        par_mode = 0;

        do_load(8'd10, 64'hAAAA_AAAA_AAAA_AAAA);
        do_accum(64'h1111_1111_1111_1111);
        tick;
        check64(cs4, 64'hBBBB_BBBB_BBBB_BBBB, "N4: context 10");

        do_load(8'd20, 64'h5555_5555_5555_5555);
        do_accum(64'h2222_2222_2222_2222);
        tick;
        check64(cs4, 64'h7777_7777_7777_7777, "N4: context 20");

        // Switch back to context 10 via SWAP — but note SWAP clears acc
        // and writes current state as new ref. So we need to LOAD to
        // just read a different context without modifying it.
        // Actually, use LOAD with the old value to switch back:
        do_load(8'd10, 64'hBBBB_BBBB_BBBB_BBBB);
        check64(cs4, 64'hBBBB_BBBB_BBBB_BBBB, "N4: switch back to context 10");

        // =================================================================
        // Test 7: N=1 and N=4 equivalence for sequential deltas
        // =================================================================
        $display("");
        $display("--- N=1 vs N=4 Equivalence ---");
        par_mode = 0;

        // Load same initial state into both
        do_load(8'd0, 64'hDEAD_BEEF_DEAD_BEEF);

        // Accumulate same deltas
        do_accum(64'h0101_0101_0101_0101);
        do_accum(64'h0202_0202_0202_0202);
        do_accum(64'h0404_0404_0404_0404);
        do_accum(64'h0808_0808_0808_0808);
        tick;

        // Both should produce the same state
        // N=1: sequential XOR
        // N=4: round-robin distributed, then merged
        check64(cs1, cs4, "N1 vs N4: same result for same deltas");

        // =================================================================
        // Summary
        // =================================================================
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
