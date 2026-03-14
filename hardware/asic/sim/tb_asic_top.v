// =============================================================================
// ATOMiK ASIC Top-Level Testbench
//
// Exercises:
//   1. Reset synchronizer behavior
//   2. BIST self-test (10/10 tests)
//   3. Functional mode operations (LOAD, ACCUM, READ, SWAP)
//   4. Scan chain connectivity
//
// ATOMiK Project — March 2026
// =============================================================================

`timescale 1ns / 1ps

module tb_asic_top;

    // =========================================================================
    // Parameters
    // =========================================================================
    parameter CLK_PERIOD = 5.0;  // 200 MHz
    parameter ADDR_WIDTH = 8;
    parameter DATA_WIDTH = 64;

    // =========================================================================
    // DUT Signals
    // =========================================================================
    reg                    clk;
    reg                    rst_n;
    reg                    op_valid;
    reg  [1:0]             op_code;
    reg  [ADDR_WIDTH-1:0]  op_addr;
    reg  [DATA_WIDTH-1:0]  op_data;
    wire                   op_ready;
    wire                   result_valid;
    wire [DATA_WIDTH-1:0]  result_data;
    reg                    bist_start;
    wire                   bist_done;
    wire                   bist_pass;
    wire [7:0]             bist_count;
    reg                    scan_enable;
    reg                    scan_in;
    wire                   scan_out;

    // =========================================================================
    // Test Tracking
    // =========================================================================
    integer test_num;
    integer pass_count;
    integer fail_count;

    // =========================================================================
    // DUT
    // =========================================================================
    atomik_asic_top #(
        .ADDR_WIDTH   (ADDR_WIDTH),
        .DATA_WIDTH   (DATA_WIDTH),
        .READ_LATENCY (1)
    ) dut (
        .clk          (clk),
        .rst_n        (rst_n),
        .op_valid     (op_valid),
        .op_code      (op_code),
        .op_addr      (op_addr),
        .op_data      (op_data),
        .op_ready     (op_ready),
        .result_valid (result_valid),
        .result_data  (result_data),
        .bist_start   (bist_start),
        .bist_done    (bist_done),
        .bist_pass    (bist_pass),
        .bist_count   (bist_count),
        .scan_enable  (scan_enable),
        .scan_in      (scan_in),
        .scan_out     (scan_out)
    );

    // =========================================================================
    // Clock Generation
    // =========================================================================
    initial clk = 0;
    always #(CLK_PERIOD/2) clk = ~clk;

    // =========================================================================
    // VCD Dump
    // =========================================================================
`ifdef VCD_OUTPUT
    initial begin
        $dumpfile("tb_asic_top.vcd");
        $dumpvars(0, tb_asic_top);
    end
`endif

    // =========================================================================
    // Helper Tasks
    // =========================================================================

    task check(
        input [511:0] name,
        input [DATA_WIDTH-1:0] got,
        input [DATA_WIDTH-1:0] exp
    );
    begin
        test_num = test_num + 1;
        if (got === exp) begin
            pass_count = pass_count + 1;
            $display("  [PASS] Test %0d: %0s (0x%016h)", test_num, name, got);
        end else begin
            fail_count = fail_count + 1;
            $display("  [FAIL] Test %0d: %0s — got 0x%016h, expected 0x%016h",
                     test_num, name, got, exp);
        end
    end
    endtask

    task do_op(
        input [1:0] code,
        input [ADDR_WIDTH-1:0] addr,
        input [DATA_WIDTH-1:0] data,
        output [DATA_WIDTH-1:0] result
    );
    begin : do_op_block
        integer timeout;
        @(posedge clk);
        op_valid <= 1;
        op_code  <= code;
        op_addr  <= addr;
        op_data  <= data;
        @(posedge clk);
        op_valid <= 0;
        // Wait for result
        timeout = 0;
        while (!result_valid && timeout < 100) begin
            @(posedge clk);
            timeout = timeout + 1;
        end
        result = result_data;
        @(posedge clk);  // Let result_valid deassert
        if (timeout >= 100) begin
            $display("  [ERROR] Timeout waiting for result_valid");
        end
    end
    endtask

    // =========================================================================
    // Test Sequence
    // =========================================================================
    reg [DATA_WIDTH-1:0] captured;

    initial begin
        test_num   = 0;
        pass_count = 0;
        fail_count = 0;

        // Initialize
        rst_n       = 0;
        op_valid    = 0;
        op_code     = 0;
        op_addr     = 0;
        op_data     = 0;
        bist_start  = 0;
        scan_enable = 0;
        scan_in     = 0;

        $display("");
        $display("=== ATOMiK ASIC Top-Level Testbench ===");
        $display("");

        // =====================================================================
        // Phase 1: Reset
        // =====================================================================
        $display("--- Phase 1: Reset Synchronizer ---");
        #(CLK_PERIOD * 2);
        rst_n = 1;
        #(CLK_PERIOD * 10);  // Wait for 4-stage sync + margin

        check("Reset deasserted, op_ready=1", {63'd0, op_ready}, 64'd1);

        // =====================================================================
        // Phase 2: BIST Self-Test
        // =====================================================================
        $display("");
        $display("--- Phase 2: BIST Self-Test (10 tests) ---");

        @(posedge clk);
        bist_start <= 1;
        @(posedge clk);
        bist_start <= 0;

        // Wait for BIST to complete
        begin : bist_wait
            integer bist_timeout;
            bist_timeout = 0;
            while (!bist_done && bist_timeout < 5000) begin
                @(posedge clk);
                bist_timeout = bist_timeout + 1;
            end
            if (bist_timeout >= 5000) begin
                $display("  [ERROR] BIST timeout after 5000 cycles");
            end
        end

        check("BIST complete", {63'd0, bist_done}, 64'd1);
        check("BIST all pass", {63'd0, bist_pass}, 64'd1);
        check("BIST count=10", {56'd0, bist_count}, 64'd10);

        // =====================================================================
        // Phase 3: Functional Mode Operations
        // =====================================================================
        $display("");
        $display("--- Phase 3: Functional Mode ---");

        // Verify op_ready is back after BIST
        check("op_ready after BIST", {63'd0, op_ready}, 64'd1);

        // LOAD + READ
        do_op(2'b00, 8'd42, 64'hCAFE_BABE_DEAD_BEEF, captured);  // LOAD

        do_op(2'b10, 8'd42, 64'd0, captured);  // READ
        check("LOAD+READ addr=42", captured, 64'hCAFE_BABE_DEAD_BEEF);

        // ACCUM + READ
        do_op(2'b01, 8'd42, 64'h00FF_00FF_00FF_00FF, captured);  // ACCUM

        do_op(2'b10, 8'd42, 64'd0, captured);  // READ
        check("ACCUM+READ", captured, 64'hCAFE_BABE_DEAD_BEEF ^ 64'h00FF_00FF_00FF_00FF);

        // SWAP
        do_op(2'b11, 8'd42, 64'd0, captured);  // SWAP
        check("SWAP returns state", captured, 64'hCAFE_BABE_DEAD_BEEF ^ 64'h00FF_00FF_00FF_00FF);

        // READ after SWAP — accumulator should be reset
        do_op(2'b10, 8'd42, 64'd0, captured);  // READ
        check("READ after SWAP", captured, 64'hCAFE_BABE_DEAD_BEEF ^ 64'h00FF_00FF_00FF_00FF);

        // =====================================================================
        // Phase 4: Scan Chain Connectivity
        // =====================================================================
        $display("");
        $display("--- Phase 4: Scan Chain ---");

        scan_enable = 1;
        // Shift 8 bits through scan chain
        scan_in = 1; @(posedge clk);
        scan_in = 0; @(posedge clk);
        scan_in = 1; @(posedge clk);
        scan_in = 0; @(posedge clk);
        scan_in = 1; @(posedge clk);
        scan_in = 0; @(posedge clk);
        scan_in = 1; @(posedge clk);
        scan_in = 0; @(posedge clk);

        // After 8 shifts, scan_out should be the first bit shifted in (1)
        check("Scan chain output", {63'd0, scan_out}, 64'd1);
        scan_enable = 0;

        // =====================================================================
        // Results
        // =====================================================================
        $display("");
        $display("=== Results: %0d/%0d PASS ===", pass_count, test_num);
        if (fail_count > 0) begin
            $display("*** %0d FAILURES ***", fail_count);
            $finish;
        end else begin
            $display("All tests passed.");
        end
        $display("");
        $finish;
    end

    // Watchdog
    initial begin
        #(CLK_PERIOD * 100000);
        $display("[ERROR] Global timeout");
        $finish;
    end

endmodule
