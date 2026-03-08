// =============================================================================
// ATOMiK AXI4-Lite Wrapper Testbench
//
// Tests the AXI4-Lite slave interface to ATOMiK core.
// Behavioral AXI4-Lite BFM (bus functional model) for iverilog simulation.
//
// Test Groups:
//   1. Reset state
//   2. STATUS register
//   3. CONFIG write/readback
//   4. CONFIG enable=0 holds core in reset
//   5. LOAD single address
//   6. LOAD multiple addresses
//   7. ACCUM single
//   8. ACCUM multiple
//   9. ACCUM commutativity
//  10. SWAP
//  11. State snapshot atomicity
//  12. 64-bit boundary patterns
//  13. Address boundary
//  14. Identity (accumulate zero)
//  15. Self-inverse
//  16. Write to read-only register
//  17. Read from write-only register
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module tb_axi4lite_wrapper;

    // Clock and reset
    reg clk;
    reg resetn;

    // AXI4-Lite signals
    reg  [5:0]  awaddr;
    reg         awvalid;
    wire        awready;

    reg  [31:0] wdata;
    reg  [3:0]  wstrb;
    reg         wvalid;
    wire        wready;

    wire [1:0]  bresp;
    wire        bvalid;
    reg         bready;

    reg  [5:0]  araddr;
    reg         arvalid;
    wire        arready;

    wire [31:0] rdata;
    wire [1:0]  rresp;
    wire        rvalid;
    reg         rready;

    // DUT
    atomik_axi4lite_wrapper #(
        .ADDR_WIDTH(6),
        .DATA_WIDTH(32)
    ) uut (
        .s_axi_aclk    (clk),
        .s_axi_aresetn (resetn),
        .s_axi_awaddr  (awaddr),
        .s_axi_awvalid (awvalid),
        .s_axi_awready (awready),
        .s_axi_wdata   (wdata),
        .s_axi_wstrb   (wstrb),
        .s_axi_wvalid  (wvalid),
        .s_axi_wready  (wready),
        .s_axi_bresp   (bresp),
        .s_axi_bvalid  (bvalid),
        .s_axi_bready  (bready),
        .s_axi_araddr  (araddr),
        .s_axi_arvalid (arvalid),
        .s_axi_arready (arready),
        .s_axi_rdata   (rdata),
        .s_axi_rresp   (rresp),
        .s_axi_rvalid  (rvalid),
        .s_axi_rready  (rready)
    );

    // Clock generation: 100 MHz (10 ns period)
    always #5 clk = ~clk;

    // =========================================================================
    // Test infrastructure
    // =========================================================================
    integer pass_count = 0;
    integer fail_count = 0;
    integer test_num = 0;

    reg [31:0] rd_data;  // Captured read data

    task check(input [31:0] expected, input [31:0] actual, input [255:0] name);
    begin
        test_num = test_num + 1;
        if (actual === expected) begin
            $display("  PASS [%0d]: %0s = 0x%08h", test_num, name, actual);
            pass_count = pass_count + 1;
        end else begin
            $display("  FAIL [%0d]: %0s = 0x%08h (expected 0x%08h)", test_num, name, actual, expected);
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

    // =========================================================================
    // AXI4-Lite BFM Tasks
    // =========================================================================

    // AXI write: drive AW+W simultaneously, wait for BRESP
    task axi_write(input [5:0] addr, input [31:0] data);
    begin
        @(posedge clk);
        awaddr  <= addr;
        awvalid <= 1'b1;
        wdata   <= data;
        wstrb   <= 4'hF;
        wvalid  <= 1'b1;
        bready  <= 1'b1;

        // Wait for both AW and W accepted
        fork
            begin : aw_wait
                while (!(awvalid && awready)) @(posedge clk);
                @(posedge clk);
                awvalid <= 1'b0;
            end
            begin : w_wait
                while (!(wvalid && wready)) @(posedge clk);
                @(posedge clk);
                wvalid <= 1'b0;
            end
        join

        // Wait for BRESP
        while (!bvalid) @(posedge clk);
        @(posedge clk);
        bready <= 1'b0;
    end
    endtask

    // AXI read: drive AR, wait for RDATA, store in rd_data
    task axi_read(input [5:0] addr);
    begin
        @(posedge clk);
        araddr  <= addr;
        arvalid <= 1'b1;
        rready  <= 1'b1;

        // Wait for AR accepted
        while (!(arvalid && arready)) @(posedge clk);
        @(posedge clk);
        arvalid <= 1'b0;

        // Wait for RVALID
        while (!rvalid) @(posedge clk);
        rd_data = rdata;
        @(posedge clk);
        rready <= 1'b0;
    end
    endtask

    // =========================================================================
    // ATOMiK operation helpers (via AXI register writes)
    // =========================================================================

    // Register offsets
    localparam LOAD_ADDR    = 6'h00;
    localparam LOAD_DATA_LO = 6'h04;
    localparam LOAD_DATA_HI = 6'h08;
    localparam ACCUM_LO     = 6'h0C;
    localparam ACCUM_HI     = 6'h10;
    localparam STATE_LO     = 6'h14;
    localparam STATE_HI     = 6'h18;
    localparam STATUS       = 6'h1C;
    localparam SWAP_ADDR    = 6'h20;
    localparam CONFIG       = 6'h24;

    // LOAD: set addr, write LO, write HI (triggers)
    task atomik_load(input [7:0] addr, input [63:0] data);
    begin
        axi_write(LOAD_ADDR, {24'h0, addr});
        axi_write(LOAD_DATA_LO, data[31:0]);
        axi_write(LOAD_DATA_HI, data[63:32]);
        // Wait for BSRAM read pipeline
        @(posedge clk);
        @(posedge clk);
    end
    endtask

    // ACCUM: write LO, write HI (triggers)
    task atomik_accum(input [63:0] delta);
    begin
        axi_write(ACCUM_LO, delta[31:0]);
        axi_write(ACCUM_HI, delta[63:32]);
    end
    endtask

    // READ STATE: read LO (latches snapshot), read HI, return 64-bit result
    reg [63:0] state_result;
    task atomik_read_state;
    begin
        axi_read(STATE_LO);
        state_result[31:0] = rd_data;
        axi_read(STATE_HI);
        state_result[63:32] = rd_data;
    end
    endtask

    // SWAP: write addr (triggers)
    task atomik_swap(input [7:0] addr);
    begin
        axi_write(SWAP_ADDR, {24'h0, addr});
        // Wait for BSRAM read pipeline
        @(posedge clk);
        @(posedge clk);
    end
    endtask

    // =========================================================================
    // Main test sequence
    // =========================================================================
    initial begin
        $dumpfile("tb_axi4lite_wrapper.vcd");
        $dumpvars(0, tb_axi4lite_wrapper);

        // Initialize
        clk     = 0;
        resetn  = 0;
        awaddr  = 0;
        awvalid = 0;
        wdata   = 0;
        wstrb   = 0;
        wvalid  = 0;
        bready  = 0;
        araddr  = 0;
        arvalid = 0;
        rready  = 0;

        // Reset
        repeat (8) @(posedge clk);
        resetn = 1;
        repeat (4) @(posedge clk);

        // =================================================================
        $display("\n=== Test 1: Reset State ===");
        // =================================================================
        check1(1'b0, bvalid, "bvalid deasserted after reset");
        check1(1'b0, rvalid, "rvalid deasserted after reset");

        axi_read(CONFIG);
        check(32'h00000001, rd_data, "CONFIG resets to enable=1");

        // =================================================================
        $display("\n=== Test 2: STATUS Register ===");
        // =================================================================
        axi_read(STATUS);
        // Expect: {8'h00, version=8'h01, n_banks=8'h01, 7'h00, acc_zero=1}
        check(32'h00010101, rd_data, "STATUS: v1, 1 bank, acc_zero=1");

        // =================================================================
        $display("\n=== Test 3: CONFIG Write/Readback ===");
        // =================================================================
        axi_write(CONFIG, 32'h00000000);
        axi_read(CONFIG);
        check(32'h00000000, rd_data, "CONFIG: write 0, read 0");

        axi_write(CONFIG, 32'h00000001);
        axi_read(CONFIG);
        check(32'h00000001, rd_data, "CONFIG: write 1, read 1");

        axi_write(CONFIG, 32'hFFFFFFFF);
        axi_read(CONFIG);
        check(32'h00000001, rd_data, "CONFIG: only bit 0 stored");

        // =================================================================
        $display("\n=== Test 4: CONFIG Enable=0 Holds Core in Reset ===");
        // =================================================================
        // Load a known state
        atomik_load(8'd0, 64'hDEADBEEFCAFEBABE);
        atomik_accum(64'h0000000000FF00FF);

        // Verify state changed
        atomik_read_state;
        check(32'hCA01BA41, state_result[31:0], "state_lo before disable");

        // Disable core
        axi_write(CONFIG, 32'h00000000);
        repeat (4) @(posedge clk);

        // Re-enable — core comes out of reset, accumulator cleared
        axi_write(CONFIG, 32'h00000001);
        repeat (4) @(posedge clk);

        // After re-enable, acc should be zero (core was reset)
        axi_read(STATUS);
        check1(1'b1, rd_data[0], "acc_zero=1 after re-enable");

        // =================================================================
        $display("\n=== Test 5: LOAD Single Address ===");
        // =================================================================
        atomik_load(8'd0, 64'hDEADBEEFCAFEBABE);

        atomik_read_state;
        check(32'hCAFEBABE, state_result[31:0], "LOAD: state_lo");
        check(32'hDEADBEEF, state_result[63:32], "LOAD: state_hi");

        // =================================================================
        $display("\n=== Test 6: LOAD Multiple Addresses ===");
        // =================================================================
        atomik_load(8'd0, 64'h1111111111111111);
        atomik_load(8'd1, 64'h2222222222222222);
        atomik_load(8'd2, 64'h3333333333333333);

        // Verify addr 2 (currently active after last LOAD)
        atomik_read_state;
        check(32'h33333333, state_result[31:0], "addr2: state_lo");
        check(32'h33333333, state_result[63:32], "addr2: state_hi");

        // Switch to addr 0 and verify
        atomik_swap(8'd0);
        atomik_read_state;
        check(32'h11111111, state_result[31:0], "addr0: state_lo after swap");
        check(32'h11111111, state_result[63:32], "addr0: state_hi after swap");

        // Switch to addr 1 and verify
        atomik_swap(8'd1);
        atomik_read_state;
        check(32'h22222222, state_result[31:0], "addr1: state_lo after swap");
        check(32'h22222222, state_result[63:32], "addr1: state_hi after swap");

        // =================================================================
        $display("\n=== Test 7: ACCUM Single ===");
        // =================================================================
        atomik_load(8'd0, 64'hDEADBEEFCAFEBABE);
        atomik_accum(64'h00000000_00FF00FF);

        // current = 0xDEADBEEFCAFEBABE ^ 0x0000000000FF00FF = 0xDEADBEEFCA01BA41
        atomik_read_state;
        check(32'hCA01BA41, state_result[31:0], "ACCUM: state_lo = init^delta");
        check(32'hDEADBEEF, state_result[63:32], "ACCUM: state_hi unchanged");

        // =================================================================
        $display("\n=== Test 8: ACCUM Multiple ===");
        // =================================================================
        atomik_load(8'd0, 64'h0000000000000000);
        atomik_accum(64'h0000000000000001);
        atomik_accum(64'h0000000000000002);
        atomik_accum(64'h0000000000000004);
        // acc = 1^2^4 = 7
        atomik_read_state;
        check(32'h00000007, state_result[31:0], "ACCUM 3x: 1^2^4=7");

        // =================================================================
        $display("\n=== Test 9: ACCUM Commutativity ===");
        // =================================================================
        // Order A: d1, d2, d3
        atomik_load(8'd0, 64'h0000000000000000);
        atomik_accum(64'hAAAAAAAA55555555);
        atomik_accum(64'h55555555AAAAAAAA);
        atomik_accum(64'h1234567890ABCDEF);
        atomik_read_state;

        begin : comm_block
            reg [63:0] result_a;
            result_a = state_result;

            // Order B: d3, d1, d2
            atomik_load(8'd0, 64'h0000000000000000);
            atomik_accum(64'h1234567890ABCDEF);
            atomik_accum(64'hAAAAAAAA55555555);
            atomik_accum(64'h55555555AAAAAAAA);
            atomik_read_state;

            check(result_a[31:0], state_result[31:0], "commutativity: lo matches");
            check(result_a[63:32], state_result[63:32], "commutativity: hi matches");
        end

        // =================================================================
        $display("\n=== Test 10: SWAP ===");
        // =================================================================
        atomik_load(8'd0, 64'h0000000000001111);
        atomik_accum(64'h0000000000000001);
        // state = 0x1111 ^ 0x0001 = 0x1110

        atomik_read_state;
        check(32'h00001110, state_result[31:0], "pre-swap: state=0x1110");

        // SWAP updates reference: new ref = current_state (0x1110)
        // Then accumulator clears (delayed one cycle)
        atomik_swap(8'd0);
        // After swap: ref=0x1110, acc=0 → state=0x1110^0=0x1110
        atomik_read_state;
        check(32'h00001110, state_result[31:0], "post-swap: state=0x1110 (ref updated)");

        // Accumulate again — now relative to new reference
        atomik_accum(64'h0000000000000001);
        atomik_read_state;
        check(32'h00001111, state_result[31:0], "post-swap accum: 0x1110^0x0001=0x1111");

        // =================================================================
        $display("\n=== Test 11: State Snapshot Atomicity ===");
        // =================================================================
        atomik_load(8'd0, 64'hFFFF0000FFFF0000);

        // Read STATE_LO — this should latch the full 64-bit snapshot
        axi_read(STATE_LO);
        check(32'hFFFF0000, rd_data, "snapshot: LO read");

        // Now accumulate — this changes current_state, but snapshot is latched
        atomik_accum(64'hFFFFFFFFFFFFFFFF);

        // Read STATE_HI — should return the latched value, NOT the new state
        axi_read(STATE_HI);
        check(32'hFFFF0000, rd_data, "snapshot: HI from latch (not live)");

        // Now do a fresh read — should see the new state
        atomik_read_state;
        check(32'h0000FFFF, state_result[31:0], "fresh read: lo after accum");
        check(32'h0000FFFF, state_result[63:32], "fresh read: hi after accum");

        // =================================================================
        $display("\n=== Test 12: 64-bit Boundary Patterns ===");
        // =================================================================
        // All ones
        atomik_load(8'd0, 64'hFFFFFFFFFFFFFFFF);
        atomik_read_state;
        check(32'hFFFFFFFF, state_result[31:0], "all-ones: lo");
        check(32'hFFFFFFFF, state_result[63:32], "all-ones: hi");

        // Alternating
        atomik_load(8'd0, 64'hAAAAAAAA55555555);
        atomik_read_state;
        check(32'h55555555, state_result[31:0], "alternating: lo=0x5555");
        check(32'hAAAAAAAA, state_result[63:32], "alternating: hi=0xAAAA");

        // MSB only
        atomik_load(8'd0, 64'h8000000000000000);
        atomik_read_state;
        check(32'h00000000, state_result[31:0], "MSB only: lo=0");
        check(32'h80000000, state_result[63:32], "MSB only: hi=0x8000_0000");

        // LSB only
        atomik_load(8'd0, 64'h0000000000000001);
        atomik_read_state;
        check(32'h00000001, state_result[31:0], "LSB only: lo=1");
        check(32'h00000000, state_result[63:32], "LSB only: hi=0");

        // =================================================================
        $display("\n=== Test 13: Address Boundary ===");
        // =================================================================
        atomik_load(8'd0,   64'hAAAAAAAAAAAAAAAA);
        atomik_load(8'd127, 64'hBBBBBBBBBBBBBBBB);
        atomik_load(8'd255, 64'hCCCCCCCCCCCCCCCC);

        // Verify addr 255 (currently active)
        atomik_read_state;
        check(32'hCCCCCCCC, state_result[31:0], "addr255: lo");

        // Verify addr 0
        atomik_swap(8'd0);
        atomik_read_state;
        check(32'hAAAAAAAA, state_result[31:0], "addr0: lo");

        // Verify addr 127
        atomik_swap(8'd127);
        atomik_read_state;
        check(32'hBBBBBBBB, state_result[31:0], "addr127: lo");

        // =================================================================
        $display("\n=== Test 14: Identity (Accumulate Zero) ===");
        // =================================================================
        atomik_load(8'd0, 64'hDEADBEEFCAFEBABE);
        atomik_accum(64'h0000000000000000);  // Zero delta

        atomik_read_state;
        check(32'hCAFEBABE, state_result[31:0], "identity: lo unchanged");
        check(32'hDEADBEEF, state_result[63:32], "identity: hi unchanged");

        axi_read(STATUS);
        check1(1'b1, rd_data[0], "identity: acc_zero=1");

        // =================================================================
        $display("\n=== Test 15: Self-Inverse ===");
        // =================================================================
        atomik_load(8'd0, 64'h0000000000000000);
        atomik_accum(64'hFEDCBA9876543210);
        atomik_accum(64'hFEDCBA9876543210);  // Same delta twice → cancels

        atomik_read_state;
        check(32'h00000000, state_result[31:0], "self-inverse: lo=0");
        check(32'h00000000, state_result[63:32], "self-inverse: hi=0");

        axi_read(STATUS);
        check1(1'b1, rd_data[0], "self-inverse: acc_zero=1");

        // =================================================================
        $display("\n=== Test 16: Write to Read-Only Register ===");
        // =================================================================
        // Write to STATUS (read-only) — should be silently ignored
        axi_write(STATUS, 32'hDEADDEAD);
        axi_read(STATUS);
        check(32'h00010101, rd_data, "STATUS unchanged after write");

        // Write to STATE_LO (read-only) — should be silently ignored
        atomik_load(8'd0, 64'h1234567890ABCDEF);
        axi_write(STATE_LO, 32'hFFFFFFFF);
        atomik_read_state;
        check(32'h90ABCDEF, state_result[31:0], "STATE_LO unchanged after write");

        // =================================================================
        $display("\n=== Test 17: Read from Write-Only Register ===");
        // =================================================================
        axi_read(LOAD_ADDR);
        check(32'h00000000, rd_data, "read LOAD_ADDR returns 0");

        axi_read(LOAD_DATA_LO);
        check(32'h00000000, rd_data, "read LOAD_DATA_LO returns 0");

        axi_read(ACCUM_LO);
        check(32'h00000000, rd_data, "read ACCUM_LO returns 0");

        axi_read(SWAP_ADDR);
        check(32'h00000000, rd_data, "read SWAP_ADDR returns 0");

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

    // Watchdog timer
    initial begin
        #500000;
        $display("TIMEOUT: Simulation exceeded 500us");
        $finish;
    end

endmodule
