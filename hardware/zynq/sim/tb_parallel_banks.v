// =============================================================================
// ATOMiK Zynq Parallel Bank Testbench — Multi-Bank Verification
//
// Tests N_BANKS=4 parallel accumulators through the full CDC path.
// Verifies round-robin distribution, XOR merge, and SWAP across banks.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module tb_parallel_banks;

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

    // DUT: 4-bank parallel
    atomik_zynq_top #(
        .N_BANKS        (4),
        .ATOMIK_CLK_DIV (5.0)
    ) dut (
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

    // 100 MHz AXI clock
    always #5 fclk_clk0 = ~fclk_clk0;

    // Test counters
    integer pass_count = 0;
    integer fail_count = 0;
    integer test_num   = 0;

    // Read data register
    reg [31:0] rd_data;

    // Register addresses
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

    // Timeout watchdog
    initial begin
        #200000;
        $display("TIMEOUT: Simulation exceeded 200us");
        $display("\n==============================================");
        $display("Parallel Bank Test Summary: %0d/%0d passed", pass_count, pass_count + fail_count);
        $display("==============================================");
        if (fail_count > 0 || pass_count == 0)
            $display("*** TESTS FAILED ***");
        $finish;
    end

    // =========================================================================
    // AXI BFM tasks
    // =========================================================================
    task axi_write(input [5:0] addr, input [31:0] data);
    begin
        @(posedge fclk_clk0);
        awaddr  <= addr;
        awvalid <= 1;
        wdata   <= data;
        wstrb   <= 4'hF;
        wvalid  <= 1;
        bready  <= 1;
        // Wait for both channels
        fork
            begin wait(awready) @(posedge fclk_clk0); awvalid <= 0; end
            begin wait(wready)  @(posedge fclk_clk0); wvalid  <= 0; end
        join
        // Wait for BRESP
        wait(bvalid) @(posedge fclk_clk0);
        bready <= 0;
    end
    endtask

    task axi_read(input [5:0] addr);
    begin
        @(posedge fclk_clk0);
        araddr  <= addr;
        arvalid <= 1;
        rready  <= 1;
        wait(arready) @(posedge fclk_clk0);
        arvalid <= 0;
        wait(rvalid) @(posedge fclk_clk0);
        rd_data = rdata;
        rready  <= 0;
    end
    endtask

    // Convenience tasks
    task atomik_load(input [7:0] addr, input [63:0] data);
    begin
        axi_write(LOAD_ADDR, {24'h0, addr});
        axi_write(LOAD_DATA_LO, data[31:0]);
        axi_write(LOAD_DATA_HI, data[63:32]);
    end
    endtask

    task atomik_accum(input [63:0] delta);
    begin
        axi_write(ACCUM_LO, delta[31:0]);
        axi_write(ACCUM_HI, delta[63:32]);
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
    end
    endtask

    task check(input [63:0] expected, input [63:0] got, input [255:0] msg);
    begin
        test_num = test_num + 1;
        if (got === expected) begin
            $display("  PASS [%0d]: %0s = 0x%016x", test_num, msg, got);
            pass_count = pass_count + 1;
        end else begin
            $display("  FAIL [%0d]: %0s = 0x%016x (expected 0x%016x)", test_num, msg, got, expected);
            fail_count = fail_count + 1;
        end
    end
    endtask

    // =========================================================================
    // Main test sequence
    // =========================================================================
    initial begin
        $dumpfile("tb_parallel_banks.vcd");
        $dumpvars(0, tb_parallel_banks);

        fclk_clk0   = 0;
        fclk_reset_n = 0;
        awaddr  = 0; awvalid = 0;
        wdata   = 0; wstrb   = 0; wvalid = 0;
        bready  = 0;
        araddr  = 0; arvalid = 0;
        rready  = 0;

        // Release reset after MMCM "lock"
        repeat (20) @(posedge fclk_clk0);
        fclk_reset_n = 1;
        repeat (10) @(posedge fclk_clk0);

        // Wait for lock
        wait(locked);
        repeat (5) @(posedge fclk_clk0);

        // =================================================================
        $display("\n=== Test Group 1: LOAD + READ (4-bank) ===");
        // =================================================================
        atomik_load(8'd0, 64'hAAAAAAAA_BBBBBBBB);
        atomik_read_state;
        check(64'hAAAAAAAA_BBBBBBBB, state_result, "LOAD addr0");

        // =================================================================
        $display("\n=== Test Group 2: Round-Robin ACCUM across 4 banks ===");
        // =================================================================
        // 4 ACCUMs → one per bank → merged = d1^d2^d3^d4
        atomik_accum(64'h0000000000000001);   // bank 0
        atomik_accum(64'h0000000000000002);   // bank 1
        atomik_accum(64'h0000000000000004);   // bank 2
        atomik_accum(64'h0000000000000008);   // bank 3

        atomik_read_state;
        // merged = 1^2^4^8 = 0xF
        // state = 0xAAAAAAAABBBBBBBB ^ 0xF = 0xAAAAAAAABBBBBBB4
        check(64'hAAAAAAAA_BBBBBBB4, state_result, "4 deltas round-robin merged");

        // =================================================================
        $display("\n=== Test Group 3: Self-inverse across banks ===");
        // =================================================================
        // Cancel each bank: same 4 deltas again → each bank XORs to zero
        atomik_accum(64'h0000000000000001);   // bank 0: 1^1=0
        atomik_accum(64'h0000000000000002);   // bank 1: 2^2=0
        atomik_accum(64'h0000000000000004);   // bank 2: 4^4=0
        atomik_accum(64'h0000000000000008);   // bank 3: 8^8=0

        atomik_read_state;
        check(64'hAAAAAAAA_BBBBBBBB, state_result, "self-inverse: back to initial");

        // =================================================================
        $display("\n=== Test Group 4: Commutativity (order independence) ===");
        // =================================================================
        // Apply 4 deltas in order A
        atomik_accum(64'h1111111111111111);
        atomik_accum(64'h2222222222222222);
        atomik_accum(64'h4444444444444444);
        atomik_accum(64'h8888888888888888);

        atomik_read_state;
        // merged = 0x1111...^0x2222...^0x4444...^0x8888... = 0xFFFF...
        check(64'h55555555_44444444, state_result, "order A: merged=FFFF^init");

        // Reset, reload, apply in different order
        atomik_load(8'd0, 64'hAAAAAAAA_BBBBBBBB);
        atomik_accum(64'h8888888888888888);
        atomik_accum(64'h4444444444444444);
        atomik_accum(64'h2222222222222222);
        atomik_accum(64'h1111111111111111);

        atomik_read_state;
        check(64'h55555555_44444444, state_result, "order B: same result (commutativity)");

        // =================================================================
        $display("\n=== Test Group 5: SWAP with multi-bank ===");
        // =================================================================
        atomik_load(8'd0, 64'h1111111111111111);
        atomik_accum(64'h0000000000000010);   // bank 0
        atomik_accum(64'h0000000000000020);   // bank 1

        atomik_read_state;
        // merged = 0x10 ^ 0x20 = 0x30
        check(64'h1111111111111121, state_result, "pre-swap: init^0x30");

        // SWAP to addr 1 → writes state_table[1] ^ merged_acc to addr 1, clears all banks
        // addr 1 was never loaded, so state_table[1]=0
        // write: state_table[1] = 0 ^ 0x30 = 0x30
        atomik_swap(8'd1);

        // After SWAP: active_addr=1, all acc cleared, state_table[1]=0x30
        atomik_read_state;
        check(64'h0000000000000030, state_result, "post-swap: acc folded into addr1");

        // Swap back to addr 0
        atomik_swap(8'd0);
        atomik_read_state;
        // addr 0 still has original init = 0x1111111111111111 (LOAD wrote it)
        // acc = 0 → state = init
        check(64'h1111111111111111, state_result, "swap back to addr0: original init");

        // =================================================================
        $display("\n=== Test Group 6: Burst accumulation (16 deltas, 4 per bank) ===");
        // =================================================================
        atomik_load(8'd0, 64'h0000000000000000);

        // 16 identical deltas of 0xFF → 4 per bank
        // Each bank: 0xFF ^ 0xFF ^ 0xFF ^ 0xFF = 0 (even number of XORs)
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);

        atomik_read_state;
        // 4 per bank, each bank does 4 XORs of 0xFF = 0
        // merged = 0^0^0^0 = 0
        check(64'h0000000000000000, state_result, "16 deltas: 4/bank, all cancel");

        // 15 deltas → 3 per bank for banks 0-2, 4 per bank 3 → uneven
        // Actually: 15 deltas round-robin across 4 banks:
        //   bank0: deltas 0,4,8,12 → 4 deltas
        //   bank1: deltas 1,5,9,13 → 4 deltas
        //   bank2: deltas 2,6,10,14 → 4 deltas
        //   bank3: deltas 3,7,11    → 3 deltas
        atomik_load(8'd0, 64'h0000000000000000);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);
        atomik_accum(64'h00000000000000FF);

        atomik_read_state;
        // bank0: 4 XORs=0, bank1: 4=0, bank2: 4=0, bank3: 3 XORs=0xFF
        // Wait, 15 deltas: bank_sel starts at 0 (reset by LOAD)
        // delta  0 → bank 0
        // delta  1 → bank 1
        // delta  2 → bank 2
        // delta  3 → bank 3
        // delta  4 → bank 0
        // ...
        // delta 12 → bank 0
        // delta 13 → bank 1
        // delta 14 → bank 2
        // bank0: 0,4,8,12 = 4 XORs = 0
        // bank1: 1,5,9,13 = 4 XORs = 0
        // bank2: 2,6,10,14 = 4 XORs = 0
        // bank3: 3,7,11 = 3 XORs = 0xFF
        // merged = 0^0^0^0xFF = 0xFF
        check(64'h00000000000000FF, state_result, "15 deltas: bank3 has odd count");

        // =================================================================
        // Summary
        // =================================================================
        $display("\n==============================================");
        $display("Parallel Bank Test Summary: %0d/%0d passed", pass_count, pass_count + fail_count);
        $display("==============================================");
        if (fail_count == 0)
            $display("*** ALL TESTS PASSED ***");
        else
            $display("*** TESTS FAILED ***");
        $finish;
    end

endmodule
