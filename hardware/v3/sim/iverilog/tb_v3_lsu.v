// =============================================================================
// ATOMiK v3 LSU Testbench
//
// Standalone test of atomik_v3_lsu module.
// Tests: all load/store types, byte offsets, LD/SD two-transaction path,
//        multi-cycle bus delay, back-to-back transactions.
// =============================================================================

`timescale 1ns / 1ps

module tb_v3_lsu;

    reg         clk, rst_n;
    reg         lsu_start, lsu_is_store;
    reg  [2:0]  lsu_size;
    reg  [63:0] lsu_addr, lsu_wdata;
    wire [63:0] lsu_rdata;
    wire        lsu_done;
    wire        bus_valid;
    reg         bus_ready;
    wire [31:0] bus_addr, bus_wdata;
    wire [3:0]  bus_wstrb;
    reg  [31:0] bus_rdata;

    // Size encoding
    localparam SIZE_B  = 3'b000;
    localparam SIZE_H  = 3'b001;
    localparam SIZE_W  = 3'b010;
    localparam SIZE_D  = 3'b011;
    localparam SIZE_BU = 3'b100;
    localparam SIZE_HU = 3'b101;
    localparam SIZE_WU = 3'b110;

    atomik_v3_lsu uut (
        .clk       (clk),
        .rst_n     (rst_n),
        .lsu_start (lsu_start),
        .lsu_is_store(lsu_is_store),
        .lsu_size  (lsu_size),
        .lsu_addr  (lsu_addr),
        .lsu_wdata (lsu_wdata),
        .lsu_rdata (lsu_rdata),
        .lsu_done  (lsu_done),
        .bus_valid (bus_valid),
        .bus_ready (bus_ready),
        .bus_addr  (bus_addr),
        .bus_wdata (bus_wdata),
        .bus_wstrb (bus_wstrb),
        .bus_rdata (bus_rdata)
    );

    // Clock: 10ns period
    always #5 clk = ~clk;

    integer pass_count = 0;
    integer fail_count = 0;
    integer test_count = 0;

    // Simple memory model (word-addressed, 256 words)
    reg [31:0] memory [0:255];

    task check(input [127:0] name, input [63:0] got, input [63:0] expected);
    begin
        test_count = test_count + 1;
        if (got === expected) begin
            $display("PASS [%0s]: got %016x", name, got);
            pass_count = pass_count + 1;
        end else begin
            $display("FAIL [%0s]: got %016x, expected %016x", name, got, expected);
            fail_count = fail_count + 1;
        end
    end
    endtask

    // =========================================================================
    // Bus responder: combinational ready (matches compliance_runner.cpp model)
    // Reads are combinational; writes update memory on posedge after handshake.
    // bus_delay > 0 adds wait cycles before responding.
    // =========================================================================
    integer bus_delay;
    integer wait_count;

    // Combinational response — same timing as compliance_runner's mem_model
    always @(*) begin
        bus_ready = 1'b0;
        bus_rdata = 32'b0;
        if (bus_valid) begin
            if (bus_delay == 0 || wait_count >= bus_delay) begin
                bus_ready = 1'b1;
                if (bus_wstrb == 4'b0)
                    bus_rdata = memory[bus_addr[9:2]];
            end
        end
    end

    // Registered: wait counter and memory writes
    always @(posedge clk) begin
        if (!rst_n) begin
            wait_count <= 0;
        end else if (bus_valid && bus_ready) begin
            // Handshake completed — reset wait counter, process writes
            wait_count <= 0;
            if (bus_wstrb[0]) memory[bus_addr[9:2]][7:0]   <= bus_wdata[7:0];
            if (bus_wstrb[1]) memory[bus_addr[9:2]][15:8]  <= bus_wdata[15:8];
            if (bus_wstrb[2]) memory[bus_addr[9:2]][23:16] <= bus_wdata[23:16];
            if (bus_wstrb[3]) memory[bus_addr[9:2]][31:24] <= bus_wdata[31:24];
        end else if (bus_valid) begin
            wait_count <= wait_count + 1;
        end else begin
            wait_count <= 0;
        end
    end

    task do_load(
        input [2:0] size,
        input [63:0] addr
    );
    begin
        @(negedge clk);
        lsu_start <= 1'b1;
        lsu_is_store <= 1'b0;
        lsu_size <= size;
        lsu_addr <= addr;
        lsu_wdata <= 64'b0;
        @(negedge clk);
        lsu_start <= 1'b0;
        // Wait for done
        wait(lsu_done);
        @(negedge clk);
    end
    endtask

    task do_store(
        input [2:0] size,
        input [63:0] addr,
        input [63:0] data
    );
    begin
        @(negedge clk);
        lsu_start <= 1'b1;
        lsu_is_store <= 1'b1;
        lsu_size <= size;
        lsu_addr <= addr;
        lsu_wdata <= data;
        @(negedge clk);
        lsu_start <= 1'b0;
        wait(lsu_done);
        @(negedge clk);
    end
    endtask

    integer i;

    initial begin
        $dumpfile("tb_v3_lsu.vcd");
        $dumpvars(0, tb_v3_lsu);

        clk = 0;
        rst_n = 0;
        lsu_start = 0;
        lsu_is_store = 0;
        lsu_size = 0;
        lsu_addr = 0;
        lsu_wdata = 0;
        bus_ready = 0;
        bus_rdata = 0;
        bus_delay = 0;
        wait_count = 0;

        // Init memory
        for (i = 0; i < 256; i = i + 1)
            memory[i] = 32'h0;

        // Pre-load some test data
        memory[0] = 32'hDEADBEEF;  // addr 0x000
        memory[1] = 32'hCAFEBABE;  // addr 0x004
        memory[2] = 32'h12345678;  // addr 0x008
        memory[3] = 32'h9ABCDEF0;  // addr 0x00C

        // Reset
        repeat(5) @(posedge clk);
        rst_n = 1;
        @(posedge clk);

        $display("==============================================");
        $display("ATOMiK v3 LSU Testbench");
        $display("==============================================");
        $display("");

        // ==================================================
        // Test group 1: Word loads
        // ==================================================
        $display("--- Word loads ---");

        do_load(SIZE_W, 64'h0);
        check("LW addr 0x000", lsu_rdata, 64'hFFFFFFFFDEADBEEF);  // sign-ext

        do_load(SIZE_W, 64'h4);
        check("LW addr 0x004", lsu_rdata, 64'hFFFFFFFFCAFEBABE);

        do_load(SIZE_WU, 64'h0);
        check("LWU addr 0x000", lsu_rdata, 64'h00000000DEADBEEF);

        do_load(SIZE_WU, 64'h8);
        check("LWU addr 0x008", lsu_rdata, 64'h0000000012345678);

        // ==================================================
        // Test group 2: Byte loads at all offsets
        // ==================================================
        $display("--- Byte loads at offsets ---");
        // memory[0] = 0xDEADBEEF (little-endian bytes: EF, BE, AD, DE)

        do_load(SIZE_B, 64'h0);
        check("LB offset 0", lsu_rdata, 64'hFFFFFFFFFFFFFFEF);

        do_load(SIZE_B, 64'h1);
        check("LB offset 1", lsu_rdata, 64'hFFFFFFFFFFFFFFBE);

        do_load(SIZE_B, 64'h2);
        check("LB offset 2", lsu_rdata, 64'hFFFFFFFFFFFFFFAD);

        do_load(SIZE_B, 64'h3);
        check("LB offset 3", lsu_rdata, 64'hFFFFFFFFFFFFFFDE);

        do_load(SIZE_BU, 64'h0);
        check("LBU offset 0", lsu_rdata, 64'h00000000000000EF);

        do_load(SIZE_BU, 64'h3);
        check("LBU offset 3", lsu_rdata, 64'h00000000000000DE);

        // ==================================================
        // Test group 3: Halfword loads at offsets
        // ==================================================
        $display("--- Halfword loads at offsets ---");

        do_load(SIZE_H, 64'h0);
        check("LH offset 0", lsu_rdata, 64'hFFFFFFFFFFFFBEEF);

        do_load(SIZE_H, 64'h2);
        check("LH offset 2", lsu_rdata, 64'hFFFFFFFFFFFFDEAD);

        do_load(SIZE_HU, 64'h0);
        check("LHU offset 0", lsu_rdata, 64'h000000000000BEEF);

        do_load(SIZE_HU, 64'h2);
        check("LHU offset 2", lsu_rdata, 64'h000000000000DEAD);

        // ==================================================
        // Test group 4: Doubleword load (two transactions)
        // ==================================================
        $display("--- Doubleword load (LD) ---");
        // memory[0] = 0xDEADBEEF (low), memory[1] = 0xCAFEBABE (high)

        do_load(SIZE_D, 64'h0);
        check("LD addr 0x000", lsu_rdata, 64'hCAFEBABE_DEADBEEF);

        do_load(SIZE_D, 64'h8);
        check("LD addr 0x008", lsu_rdata, 64'h9ABCDEF0_12345678);

        // ==================================================
        // Test group 5: Word store + readback
        // ==================================================
        $display("--- Word store + readback ---");

        do_store(SIZE_W, 64'h40, 64'hAAAAAAAA_11223344);
        do_load(SIZE_WU, 64'h40);
        check("SW/LWU roundtrip", lsu_rdata, 64'h0000000011223344);

        // ==================================================
        // Test group 6: Byte store at all offsets
        // ==================================================
        $display("--- Byte stores at offsets ---");
        memory[32] = 32'h0;  // addr 0x80

        do_store(SIZE_B, 64'h80, 64'hAA);
        check("SB offset 0", {32'b0, memory[32]}, 64'h000000AA);

        do_store(SIZE_B, 64'h81, 64'hBB);
        check("SB offset 1", {32'b0, memory[32]}, 64'h0000BBAA);

        do_store(SIZE_B, 64'h82, 64'hCC);
        check("SB offset 2", {32'b0, memory[32]}, 64'h00CCBBAA);

        do_store(SIZE_B, 64'h83, 64'hDD);
        check("SB offset 3", {32'b0, memory[32]}, 64'hDDCCBBAA);

        // ==================================================
        // Test group 7: Halfword store at offsets
        // ==================================================
        $display("--- Halfword stores ---");
        memory[33] = 32'h0;  // addr 0x84

        do_store(SIZE_H, 64'h84, 64'hBEEF);
        check("SH offset 0", {32'b0, memory[33]}, 64'h0000BEEF);

        do_store(SIZE_H, 64'h86, 64'hCAFE);
        check("SH offset 2", {32'b0, memory[33]}, 64'hCAFEBEEF);

        // ==================================================
        // Test group 8: Doubleword store (two transactions)
        // ==================================================
        $display("--- Doubleword store (SD) ---");
        memory[34] = 32'h0;  // addr 0x88
        memory[35] = 32'h0;  // addr 0x8C

        do_store(SIZE_D, 64'h88, 64'hFEDCBA98_76543210);
        check("SD low word", {32'b0, memory[34]}, 64'h76543210);
        check("SD high word", {32'b0, memory[35]}, 64'hFEDCBA98);

        // Read back with LD
        do_load(SIZE_D, 64'h88);
        check("SD/LD roundtrip", lsu_rdata, 64'hFEDCBA98_76543210);

        // ==================================================
        // Test group 9: Multi-cycle bus delay (3 cycles)
        // ==================================================
        $display("--- Multi-cycle bus delay ---");
        bus_delay = 3;

        do_load(SIZE_W, 64'h0);
        check("LW 3-cycle delay", lsu_rdata, 64'hFFFFFFFFDEADBEEF);

        do_store(SIZE_W, 64'h100, 64'h99887766);
        do_load(SIZE_WU, 64'h100);
        check("SW/LWU delay", lsu_rdata, 64'h0000000099887766);

        bus_delay = 0;

        // ==================================================
        // Test group 10: LD with bus delay (both halves delayed)
        // ==================================================
        $display("--- LD with bus delay ---");
        bus_delay = 2;

        do_load(SIZE_D, 64'h0);
        check("LD 2-cycle delay", lsu_rdata, 64'hCAFEBABE_DEADBEEF);

        bus_delay = 0;

        // ==================================================
        // Test group 11: SD with bus delay
        // ==================================================
        $display("--- SD with bus delay ---");
        bus_delay = 1;
        memory[36] = 32'h0;
        memory[37] = 32'h0;

        do_store(SIZE_D, 64'h90, 64'h1122334455667788);
        check("SD delay low", {32'b0, memory[36]}, 64'h55667788);
        check("SD delay high", {32'b0, memory[37]}, 64'h11223344);

        bus_delay = 0;

        // ==================================================
        // Test group 12: Back-to-back transactions
        // ==================================================
        $display("--- Back-to-back transactions ---");

        do_store(SIZE_W, 64'h200, 64'h11111111);
        do_store(SIZE_W, 64'h204, 64'h22222222);
        do_store(SIZE_W, 64'h208, 64'h33333333);

        do_load(SIZE_WU, 64'h200);
        check("B2B load 1", lsu_rdata, 64'h0000000011111111);
        do_load(SIZE_WU, 64'h204);
        check("B2B load 2", lsu_rdata, 64'h0000000022222222);
        do_load(SIZE_WU, 64'h208);
        check("B2B load 3", lsu_rdata, 64'h0000000033333333);

        // ==================================================
        // Summary
        // ==================================================
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
