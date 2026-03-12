// =============================================================================
// ATOMiK CDC Wrapper Testbench — Dual-Clock Domain Validation
//
// Tests the full AXI4-Lite → CDC bridge → ATOMiK core path with independent
// AXI (100 MHz) and ATOMiK (200 MHz) clocks.
//
// Uses SIMULATION define so atomik_zynq_clk provides behavioral clocks
// instead of Xilinx MMCM primitives.
//
// Test Groups:
//   1. Reset + MMCM lock sequence
//   2. STATUS register (version, banks, acc_zero)
//   3. CONFIG enable/disable
//   4. LOAD single address across CDC
//   5. LOAD multiple addresses
//   6. ACCUM single across CDC
//   7. ACCUM multiple
//   8. ACCUM commutativity (order independence across clock domains)
//   9. SWAP across CDC
//  10. State snapshot atomicity
//  11. 64-bit boundary patterns
//  12. Self-inverse
//  13. Rapid back-to-back operations (stress CDC handshake)
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module tb_cdc_wrapper;

    // Clock and reset
    reg fclk_clk0;
    reg fclk_reset_n;

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

    wire        locked;

    // DUT — full top-level with MMCM + CDC + core
    atomik_zynq_top #(
        .ADDR_WIDTH    (6),
        .DATA_WIDTH    (32),
        .ATOMIK_CLK_DIV(5.0)   // 200 MHz ATOMiK clock in simulation
    ) uut (
        .fclk_clk0     (fclk_clk0),
        .fclk_reset_n  (fclk_reset_n),
        .locked        (locked),

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

    // FCLK_CLK0: 100 MHz (10 ns period)
    always #5 fclk_clk0 = ~fclk_clk0;

    // =========================================================================
    // Test infrastructure
    // =========================================================================
    integer pass_count = 0;
    integer fail_count = 0;
    integer test_num = 0;

    reg [31:0] rd_data;

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
    // AXI4-Lite BFM Tasks (same as Phase 1 testbench)
    // =========================================================================

    task axi_write(input [5:0] addr, input [31:0] data);
    begin
        @(posedge fclk_clk0);
        awaddr  <= addr;
        awvalid <= 1'b1;
        wdata   <= data;
        wstrb   <= 4'hF;
        wvalid  <= 1'b1;
        bready  <= 1'b1;

        fork
            begin : aw_wait
                while (!(awvalid && awready)) @(posedge fclk_clk0);
                @(posedge fclk_clk0);
                awvalid <= 1'b0;
            end
            begin : w_wait
                while (!(wvalid && wready)) @(posedge fclk_clk0);
                @(posedge fclk_clk0);
                wvalid <= 1'b0;
            end
        join

        // Wait for BRESP
        while (!bvalid) @(posedge fclk_clk0);
        @(posedge fclk_clk0);
        bready <= 1'b0;
    end
    endtask

    task axi_read(input [5:0] addr);
    begin
        @(posedge fclk_clk0);
        araddr  <= addr;
        arvalid <= 1'b1;
        rready  <= 1'b1;

        while (!(arvalid && arready)) @(posedge fclk_clk0);
        @(posedge fclk_clk0);
        arvalid <= 1'b0;

        while (!rvalid) @(posedge fclk_clk0);
        rd_data = rdata;
        @(posedge fclk_clk0);
        rready <= 1'b0;
    end
    endtask

    // =========================================================================
    // ATOMiK operation helpers
    // =========================================================================
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

    task atomik_load(input [7:0] addr, input [63:0] data);
    begin
        axi_write(LOAD_ADDR, {24'h0, addr});
        axi_write(LOAD_DATA_LO, data[31:0]);
        axi_write(LOAD_DATA_HI, data[63:32]);
        // Extra settle time for CDC round-trip + BSRAM pipeline
        repeat (4) @(posedge fclk_clk0);
    end
    endtask

    task atomik_accum(input [63:0] delta);
    begin
        axi_write(ACCUM_LO, delta[31:0]);
        axi_write(ACCUM_HI, delta[63:32]);
        repeat (2) @(posedge fclk_clk0);
    end
    endtask

    reg [63:0] state_result;
    task atomik_read_state;
    begin
        axi_read(STATE_LO);
        state_result[31:0] = rd_data;
        axi_read(STATE_HI);
        state_result[63:32] = rd_data;
    end
    endtask

    task atomik_swap(input [7:0] addr);
    begin
        axi_write(SWAP_ADDR, {24'h0, addr});
        repeat (4) @(posedge fclk_clk0);
    end
    endtask

    // =========================================================================
    // Main test sequence
    // =========================================================================
    initial begin
        $dumpfile("tb_cdc_wrapper.vcd");
        $dumpvars(0, tb_cdc_wrapper);

        // Initialize
        fclk_clk0    = 0;
        fclk_reset_n = 0;
        awaddr  = 0;
        awvalid = 0;
        wdata   = 0;
        wstrb   = 0;
        wvalid  = 0;
        bready  = 0;
        araddr  = 0;
        arvalid = 0;
        rready  = 0;

        // =================================================================
        $display("\n=== Test 1: Reset + MMCM Lock Sequence ===");
        // =================================================================
        // Hold reset for 8 cycles
        repeat (8) @(posedge fclk_clk0);
        check1(1'b0, locked, "locked=0 during reset");

        // Release reset
        fclk_reset_n = 1;

        // Wait for simulated MMCM lock (~10 fclk cycles)
        repeat (15) @(posedge fclk_clk0);
        check1(1'b1, locked, "locked=1 after MMCM settle");

        // Extra settle
        repeat (4) @(posedge fclk_clk0);

        // =================================================================
        $display("\n=== Test 2: STATUS Register ===");
        // =================================================================
        axi_read(STATUS);
        // VERSION=0x02 (CDC-capable), N_BANKS=0x01, acc_zero=1
        check(32'h00020101, rd_data, "STATUS: v2, 1 bank, acc_zero=1");

        // =================================================================
        $display("\n=== Test 3: CONFIG Enable/Disable ===");
        // =================================================================
        axi_read(CONFIG);
        check(32'h00000001, rd_data, "CONFIG resets to enable=1");

        axi_write(CONFIG, 32'h00000000);
        axi_read(CONFIG);
        check(32'h00000000, rd_data, "CONFIG: write 0, read 0");

        axi_write(CONFIG, 32'h00000001);
        axi_read(CONFIG);
        check(32'h00000001, rd_data, "CONFIG: write 1, read 1");

        // =================================================================
        $display("\n=== Test 4: LOAD Single Address Across CDC ===");
        // =================================================================
        atomik_load(8'd0, 64'hDEADBEEFCAFEBABE);

        atomik_read_state;
        check(32'hCAFEBABE, state_result[31:0], "LOAD CDC: state_lo");
        check(32'hDEADBEEF, state_result[63:32], "LOAD CDC: state_hi");

        // =================================================================
        $display("\n=== Test 5: LOAD Multiple Addresses ===");
        // =================================================================
        atomik_load(8'd0, 64'h1111111111111111);
        atomik_load(8'd1, 64'h2222222222222222);
        atomik_load(8'd2, 64'h3333333333333333);

        // Verify addr 2 (currently active)
        atomik_read_state;
        check(32'h33333333, state_result[31:0], "addr2: state_lo");
        check(32'h33333333, state_result[63:32], "addr2: state_hi");

        // Switch to addr 0
        atomik_swap(8'd0);
        atomik_read_state;
        check(32'h11111111, state_result[31:0], "addr0: state_lo after swap");
        check(32'h11111111, state_result[63:32], "addr0: state_hi after swap");

        // Switch to addr 1
        atomik_swap(8'd1);
        atomik_read_state;
        check(32'h22222222, state_result[31:0], "addr1: state_lo after swap");
        check(32'h22222222, state_result[63:32], "addr1: state_hi after swap");

        // =================================================================
        $display("\n=== Test 6: ACCUM Single Across CDC ===");
        // =================================================================
        atomik_load(8'd0, 64'hDEADBEEFCAFEBABE);
        atomik_accum(64'h00000000_00FF00FF);

        atomik_read_state;
        check(32'hCA01BA41, state_result[31:0], "ACCUM CDC: state_lo");
        check(32'hDEADBEEF, state_result[63:32], "ACCUM CDC: state_hi");

        // =================================================================
        $display("\n=== Test 7: ACCUM Multiple ===");
        // =================================================================
        atomik_load(8'd0, 64'h0000000000000000);
        atomik_accum(64'h0000000000000001);
        atomik_accum(64'h0000000000000002);
        atomik_accum(64'h0000000000000004);
        // acc = 1^2^4 = 7
        atomik_read_state;
        check(32'h00000007, state_result[31:0], "ACCUM 3x: 1^2^4=7");

        // =================================================================
        $display("\n=== Test 8: ACCUM Commutativity Across CDC ===");
        // =================================================================
        atomik_load(8'd0, 64'h0000000000000000);
        atomik_accum(64'hAAAAAAAA55555555);
        atomik_accum(64'h55555555AAAAAAAA);
        atomik_accum(64'h1234567890ABCDEF);
        atomik_read_state;

        begin : comm_block
            reg [63:0] result_a;
            result_a = state_result;

            // Reverse order
            atomik_load(8'd0, 64'h0000000000000000);
            atomik_accum(64'h1234567890ABCDEF);
            atomik_accum(64'hAAAAAAAA55555555);
            atomik_accum(64'h55555555AAAAAAAA);
            atomik_read_state;

            check(result_a[31:0], state_result[31:0], "commutativity CDC: lo");
            check(result_a[63:32], state_result[63:32], "commutativity CDC: hi");
        end

        // =================================================================
        $display("\n=== Test 9: SWAP Across CDC ===");
        // =================================================================
        atomik_load(8'd0, 64'h0000000000001111);
        atomik_accum(64'h0000000000000001);

        atomik_read_state;
        check(32'h00001110, state_result[31:0], "pre-swap: state=0x1110");

        atomik_swap(8'd0);
        atomik_read_state;
        check(32'h00001110, state_result[31:0], "post-swap: ref updated");

        atomik_accum(64'h0000000000000001);
        atomik_read_state;
        check(32'h00001111, state_result[31:0], "post-swap accum: 0x1111");

        // =================================================================
        $display("\n=== Test 10: State Snapshot Atomicity ===");
        // =================================================================
        atomik_load(8'd0, 64'hFFFF0000FFFF0000);

        axi_read(STATE_LO);
        check(32'hFFFF0000, rd_data, "snapshot: LO read");

        atomik_accum(64'hFFFFFFFFFFFFFFFF);

        axi_read(STATE_HI);
        check(32'hFFFF0000, rd_data, "snapshot: HI from latch");

        atomik_read_state;
        check(32'h0000FFFF, state_result[31:0], "fresh read: lo after accum");
        check(32'h0000FFFF, state_result[63:32], "fresh read: hi after accum");

        // =================================================================
        $display("\n=== Test 11: 64-bit Boundary Patterns ===");
        // =================================================================
        atomik_load(8'd0, 64'hFFFFFFFFFFFFFFFF);
        atomik_read_state;
        check(32'hFFFFFFFF, state_result[31:0], "all-ones: lo");
        check(32'hFFFFFFFF, state_result[63:32], "all-ones: hi");

        atomik_load(8'd0, 64'h8000000000000000);
        atomik_read_state;
        check(32'h00000000, state_result[31:0], "MSB only: lo=0");
        check(32'h80000000, state_result[63:32], "MSB only: hi");

        atomik_load(8'd0, 64'h0000000000000001);
        atomik_read_state;
        check(32'h00000001, state_result[31:0], "LSB only: lo=1");
        check(32'h00000000, state_result[63:32], "LSB only: hi=0");

        // =================================================================
        $display("\n=== Test 12: Self-Inverse ===");
        // =================================================================
        atomik_load(8'd0, 64'h0000000000000000);
        atomik_accum(64'hFEDCBA9876543210);
        atomik_accum(64'hFEDCBA9876543210);

        atomik_read_state;
        check(32'h00000000, state_result[31:0], "self-inverse: lo=0");
        check(32'h00000000, state_result[63:32], "self-inverse: hi=0");

        // =================================================================
        $display("\n=== Test 13: Rapid Back-to-Back Operations (CDC Stress) ===");
        // =================================================================
        // This stresses the toggle-handshake by issuing operations as fast
        // as the AXI wrapper will accept them.
        atomik_load(8'd0, 64'h0000000000000000);

        // 8 rapid accumulates
        atomik_accum(64'h0000000000000001);
        atomik_accum(64'h0000000000000002);
        atomik_accum(64'h0000000000000004);
        atomik_accum(64'h0000000000000008);
        atomik_accum(64'h0000000000000010);
        atomik_accum(64'h0000000000000020);
        atomik_accum(64'h0000000000000040);
        atomik_accum(64'h0000000000000080);
        // Expected: 0xFF (1+2+4+8+16+32+64+128)
        atomik_read_state;
        check(32'h000000FF, state_result[31:0], "rapid 8x accum: 0xFF");
        check(32'h00000000, state_result[63:32], "rapid 8x accum: hi=0");

        // Verify all XOR correctly (self-inverse all 8)
        atomik_accum(64'h00000000000000FF);
        atomik_read_state;
        check(32'h00000000, state_result[31:0], "rapid cancel: 0xFF^0xFF=0");

        // =================================================================
        // Summary
        // =================================================================
        $display("\n==============================================");
        $display("CDC Test Summary: %0d/%0d passed", pass_count, pass_count + fail_count);
        $display("==============================================");

        if (fail_count > 0)
            $display("*** %0d TESTS FAILED ***", fail_count);
        else
            $display("*** ALL TESTS PASSED ***");

        $finish;
    end

    // Watchdog timer (longer for CDC latency)
    initial begin
        #2000000;
        $display("TIMEOUT: Simulation exceeded 2ms");
        $finish;
    end

endmodule
