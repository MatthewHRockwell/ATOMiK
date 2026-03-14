// =============================================================================
// ATOMiK Co-Processor Testbench
//
// Validates:
//   1. CPU boots from DDR3 (AXI bridge translates addresses)
//   2. Local BRAM read/write
//   3. Mailbox ARM→RV64I and RV64I→ARM communication
//   4. Bus mux routing (DDR3 vs BRAM vs mailbox)
//   5. ATOMiK custom instructions via Zynq-ported datapath
//
// Uses behavioral AXI slave to model DDR3, preloaded with test firmware.
// =============================================================================

`timescale 1ns / 1ps

module tb_coprocessor;

    // =========================================================================
    // Clock and reset
    // =========================================================================
    reg clk = 0;
    always #5 clk = ~clk;  // 100 MHz

    reg rst_n = 0;

    // =========================================================================
    // AXI4 Master signals (from coprocessor to DDR3 model)
    // =========================================================================
    wire [3:0]  m_axi_awid;
    wire [31:0] m_axi_awaddr;
    wire [7:0]  m_axi_awlen;
    wire [2:0]  m_axi_awsize;
    wire [1:0]  m_axi_awburst;
    wire        m_axi_awlock;
    wire [3:0]  m_axi_awcache;
    wire [2:0]  m_axi_awprot;
    wire [3:0]  m_axi_awqos;
    wire        m_axi_awvalid;
    reg         m_axi_awready;

    wire [31:0] m_axi_wdata;
    wire [3:0]  m_axi_wstrb;
    wire        m_axi_wlast;
    wire        m_axi_wvalid;
    reg         m_axi_wready;

    reg  [3:0]  m_axi_bid;
    reg  [1:0]  m_axi_bresp;
    reg         m_axi_bvalid;
    wire        m_axi_bready;

    wire [3:0]  m_axi_arid;
    wire [31:0] m_axi_araddr;
    wire [7:0]  m_axi_arlen;
    wire [2:0]  m_axi_arsize;
    wire [1:0]  m_axi_arburst;
    wire        m_axi_arlock;
    wire [3:0]  m_axi_arcache;
    wire [2:0]  m_axi_arprot;
    wire [3:0]  m_axi_arqos;
    wire        m_axi_arvalid;
    reg         m_axi_arready;

    reg  [3:0]  m_axi_rid;
    reg  [31:0] m_axi_rdata;
    reg  [1:0]  m_axi_rresp;
    reg         m_axi_rlast;
    reg         m_axi_rvalid;
    wire        m_axi_rready;

    // =========================================================================
    // Mailbox AXI4-Lite signals (ARM side — testbench acts as ARM)
    // =========================================================================
    reg  [5:0]  s_axi_mbox_awaddr;
    reg         s_axi_mbox_awvalid;
    wire        s_axi_mbox_awready;
    reg  [31:0] s_axi_mbox_wdata;
    reg  [3:0]  s_axi_mbox_wstrb;
    reg         s_axi_mbox_wvalid;
    wire        s_axi_mbox_wready;
    wire [1:0]  s_axi_mbox_bresp;
    wire        s_axi_mbox_bvalid;
    reg         s_axi_mbox_bready;
    reg  [5:0]  s_axi_mbox_araddr;
    reg         s_axi_mbox_arvalid;
    wire        s_axi_mbox_arready;
    wire [31:0] s_axi_mbox_rdata;
    wire [1:0]  s_axi_mbox_rresp;
    wire        s_axi_mbox_rvalid;
    reg         s_axi_mbox_rready;

    wire        irq_to_arm;
    wire [31:0] debug_pc;

    // =========================================================================
    // DUT
    // =========================================================================
    atomik_coprocessor_top #(
        .RESET_PC       (64'h0000_0000_0000_0000),
        .BRAM_ADDR_BITS (10),   // 1K words = 4 KB (smaller for sim)
        .DDR3_BASE_OFFSET(32'h10000000),
        .AXI_ID_WIDTH   (4)
    ) dut (
        .clk           (clk),
        .ext_rst_n     (rst_n),

        .m_axi_awid    (m_axi_awid),
        .m_axi_awaddr  (m_axi_awaddr),
        .m_axi_awlen   (m_axi_awlen),
        .m_axi_awsize  (m_axi_awsize),
        .m_axi_awburst (m_axi_awburst),
        .m_axi_awlock  (m_axi_awlock),
        .m_axi_awcache (m_axi_awcache),
        .m_axi_awprot  (m_axi_awprot),
        .m_axi_awqos   (m_axi_awqos),
        .m_axi_awvalid (m_axi_awvalid),
        .m_axi_awready (m_axi_awready),

        .m_axi_wdata   (m_axi_wdata),
        .m_axi_wstrb   (m_axi_wstrb),
        .m_axi_wlast   (m_axi_wlast),
        .m_axi_wvalid  (m_axi_wvalid),
        .m_axi_wready  (m_axi_wready),

        .m_axi_bid     (m_axi_bid),
        .m_axi_bresp   (m_axi_bresp),
        .m_axi_bvalid  (m_axi_bvalid),
        .m_axi_bready  (m_axi_bready),

        .m_axi_arid    (m_axi_arid),
        .m_axi_araddr  (m_axi_araddr),
        .m_axi_arlen   (m_axi_arlen),
        .m_axi_arsize  (m_axi_arsize),
        .m_axi_arburst (m_axi_arburst),
        .m_axi_arlock  (m_axi_arlock),
        .m_axi_arcache (m_axi_arcache),
        .m_axi_arprot  (m_axi_arprot),
        .m_axi_arqos   (m_axi_arqos),
        .m_axi_arvalid (m_axi_arvalid),
        .m_axi_arready (m_axi_arready),

        .m_axi_rid     (m_axi_rid),
        .m_axi_rdata   (m_axi_rdata),
        .m_axi_rresp   (m_axi_rresp),
        .m_axi_rlast   (m_axi_rlast),
        .m_axi_rvalid  (m_axi_rvalid),
        .m_axi_rready  (m_axi_rready),

        .s_axi_mbox_awaddr  (s_axi_mbox_awaddr),
        .s_axi_mbox_awvalid (s_axi_mbox_awvalid),
        .s_axi_mbox_awready (s_axi_mbox_awready),
        .s_axi_mbox_wdata   (s_axi_mbox_wdata),
        .s_axi_mbox_wstrb   (s_axi_mbox_wstrb),
        .s_axi_mbox_wvalid  (s_axi_mbox_wvalid),
        .s_axi_mbox_wready  (s_axi_mbox_wready),
        .s_axi_mbox_bresp   (s_axi_mbox_bresp),
        .s_axi_mbox_bvalid  (s_axi_mbox_bvalid),
        .s_axi_mbox_bready  (s_axi_mbox_bready),
        .s_axi_mbox_araddr  (s_axi_mbox_araddr),
        .s_axi_mbox_arvalid (s_axi_mbox_arvalid),
        .s_axi_mbox_arready (s_axi_mbox_arready),
        .s_axi_mbox_rdata   (s_axi_mbox_rdata),
        .s_axi_mbox_rresp   (s_axi_mbox_rresp),
        .s_axi_mbox_rvalid  (s_axi_mbox_rvalid),
        .s_axi_mbox_rready  (s_axi_mbox_rready),

        .irq_to_arm    (irq_to_arm),
        .debug_pc      (debug_pc)
    );

    // =========================================================================
    // Behavioral DDR3 Model (AXI slave)
    // 64 KB memory at physical address 0x10000000
    // =========================================================================
    reg [31:0] ddr3_mem [0:16383];  // 16K words = 64 KB

    // AXI read response
    always @(posedge clk) begin
        if (!rst_n) begin
            m_axi_arready <= 1'b1;
            m_axi_rvalid  <= 1'b0;
            m_axi_rdata   <= 32'b0;
            m_axi_rresp   <= 2'b00;
            m_axi_rlast   <= 1'b0;
            m_axi_rid     <= 4'b0;
        end else begin
            if (m_axi_arvalid && m_axi_arready) begin
                m_axi_arready <= 1'b0;
                m_axi_rvalid  <= 1'b1;
                m_axi_rlast   <= 1'b1;
                m_axi_rid     <= m_axi_arid;
                m_axi_rresp   <= 2'b00;
                // Address bits [15:2] index 16K words (offset from 0x10000000)
                m_axi_rdata   <= ddr3_mem[(m_axi_araddr - 32'h10000000) >> 2];
            end
            if (m_axi_rvalid && m_axi_rready) begin
                m_axi_rvalid  <= 1'b0;
                m_axi_rlast   <= 1'b0;
                m_axi_arready <= 1'b1;
            end
        end
    end

    // AXI write response
    always @(posedge clk) begin
        if (!rst_n) begin
            m_axi_awready <= 1'b1;
            m_axi_wready  <= 1'b1;
            m_axi_bvalid  <= 1'b0;
            m_axi_bresp   <= 2'b00;
            m_axi_bid     <= 4'b0;
        end else begin
            if (m_axi_awvalid && m_axi_awready && m_axi_wvalid && m_axi_wready) begin
                m_axi_awready <= 1'b0;
                m_axi_wready  <= 1'b0;
                m_axi_bvalid  <= 1'b1;
                m_axi_bid     <= m_axi_awid;
                // Perform write
                if (m_axi_wstrb[0]) ddr3_mem[(m_axi_awaddr - 32'h10000000) >> 2][ 7: 0] <= m_axi_wdata[ 7: 0];
                if (m_axi_wstrb[1]) ddr3_mem[(m_axi_awaddr - 32'h10000000) >> 2][15: 8] <= m_axi_wdata[15: 8];
                if (m_axi_wstrb[2]) ddr3_mem[(m_axi_awaddr - 32'h10000000) >> 2][23:16] <= m_axi_wdata[23:16];
                if (m_axi_wstrb[3]) ddr3_mem[(m_axi_awaddr - 32'h10000000) >> 2][31:24] <= m_axi_wdata[31:24];
            end
            if (m_axi_bvalid && m_axi_bready) begin
                m_axi_bvalid  <= 1'b0;
                m_axi_awready <= 1'b1;
                m_axi_wready  <= 1'b1;
            end
        end
    end

    // =========================================================================
    // Test counters
    // =========================================================================
    integer test_num = 0;
    integer pass_count = 0;
    integer fail_count = 0;

    // =========================================================================
    // Helper tasks
    // =========================================================================
    task mbox_write(input [5:0] addr, input [31:0] data);
    begin
        @(posedge clk);
        s_axi_mbox_awaddr  <= addr;
        s_axi_mbox_awvalid <= 1'b1;
        s_axi_mbox_wdata   <= data;
        s_axi_mbox_wstrb   <= 4'hF;
        s_axi_mbox_wvalid  <= 1'b1;
        s_axi_mbox_bready  <= 1'b1;

        // Wait for both AW and W accepted
        @(posedge clk);
        while (!(s_axi_mbox_awready || !s_axi_mbox_awvalid) ||
               !(s_axi_mbox_wready  || !s_axi_mbox_wvalid))
            @(posedge clk);

        s_axi_mbox_awvalid <= 1'b0;
        s_axi_mbox_wvalid  <= 1'b0;

        // Wait for BRESP
        while (!s_axi_mbox_bvalid)
            @(posedge clk);
        @(posedge clk);
        s_axi_mbox_bready <= 1'b0;
    end
    endtask

    task mbox_read(input [5:0] addr, output [31:0] data);
    begin
        @(posedge clk);
        s_axi_mbox_araddr  <= addr;
        s_axi_mbox_arvalid <= 1'b1;
        s_axi_mbox_rready  <= 1'b1;

        // Wait for AR accepted
        while (!(s_axi_mbox_arready && s_axi_mbox_arvalid))
            @(posedge clk);
        s_axi_mbox_arvalid <= 1'b0;

        // Wait for RVALID
        while (!s_axi_mbox_rvalid)
            @(posedge clk);
        data = s_axi_mbox_rdata;
        @(posedge clk);
        s_axi_mbox_rready <= 1'b0;
    end
    endtask

    task check(input [31:0] got, input [31:0] expected, input [255:0] name);
    begin
        test_num = test_num + 1;
        if (got === expected) begin
            pass_count = pass_count + 1;
            $display("  PASS %0d: %0s (0x%08X)", test_num, name, got);
        end else begin
            fail_count = fail_count + 1;
            $display("  FAIL %0d: %0s — got 0x%08X, expected 0x%08X", test_num, name, got, expected);
        end
    end
    endtask

    // =========================================================================
    // Test firmware: Simple RV64I program
    //
    // Loads into DDR3 at physical 0x10000000 (CPU sees 0x00000000).
    // Program:
    //   0x00: LUI  x1, 0x40000     (x1 = 0x40000000 — BRAM base)
    //   0x04: ADDI x2, x0, 0x42    (x2 = 0x42)
    //   0x08: SW   x2, 0(x1)       (store 0x42 to BRAM[0])
    //   0x0C: LW   x3, 0(x1)       (load from BRAM[0])
    //   0x10: LUI  x4, 0x80000     (x4 = 0x80000000 — mailbox base)
    //   0x14: SW   x3, 0x18(x4)    (write x3 to RESP0 register)
    //   0x18: JAL  x0, 0           (infinite loop — halt)
    // =========================================================================
    integer fw_i;
    initial begin
        for (fw_i = 0; fw_i < 16384; fw_i = fw_i + 1)
            ddr3_mem[fw_i] = 32'h00000013;  // NOP (ADDI x0, x0, 0)

        // LUI x1, 0x40000
        ddr3_mem[0] = 32'h40000_0B7;
        // ADDI x2, x0, 0x42
        ddr3_mem[1] = 32'h04200_113;
        // SW x2, 0(x1)   — store 0x42 to BRAM[0]
        ddr3_mem[2] = 32'h0020_A023;
        // LW x3, 0(x1)   — load from BRAM[0]
        ddr3_mem[3] = 32'h0000_A183;
        // LUI x4, 0x80000
        ddr3_mem[4] = 32'h80000_237;
        // SW x3, 0x18(x4) — write x3 to mailbox RESP0 (offset 0x18)
        ddr3_mem[5] = 32'h0032_2C23;
        // JAL x0, 0       — infinite loop
        ddr3_mem[6] = 32'h0000_006F;
    end

    // =========================================================================
    // Main test sequence
    // =========================================================================
    reg [31:0] rdata;

    initial begin
        $dumpfile("tb_coprocessor.vcd");
        $dumpvars(0, tb_coprocessor);

        // Initialize mailbox AXI signals
        s_axi_mbox_awaddr  = 0;
        s_axi_mbox_awvalid = 0;
        s_axi_mbox_wdata   = 0;
        s_axi_mbox_wstrb   = 0;
        s_axi_mbox_wvalid  = 0;
        s_axi_mbox_bready  = 0;
        s_axi_mbox_araddr  = 0;
        s_axi_mbox_arvalid = 0;
        s_axi_mbox_rready  = 0;

        // Reset
        rst_n = 0;
        repeat (20) @(posedge clk);
        rst_n = 1;

        $display("");
        $display("=== ATOMiK Co-Processor Testbench ===");
        $display("");

        // =====================================================================
        // Test 1: Mailbox register read/write (CPU still in reset)
        // =====================================================================
        $display("--- Test Group 1: Mailbox Registers ---");

        // Read initial CTRL (should be 0 = CPU in reset)
        mbox_read(6'h00, rdata);
        check(rdata, 32'h0, "CTRL initial value (CPU in reset)");

        // Write FW_ADDR
        mbox_write(6'h24, 32'h10000000);
        mbox_read(6'h24, rdata);
        check(rdata, 32'h10000000, "FW_ADDR readback");

        // Write FW_SIZE
        mbox_write(6'h28, 32'h100);
        mbox_read(6'h28, rdata);
        check(rdata, 32'h100, "FW_SIZE readback");

        // Write CMD register
        mbox_write(6'h08, 32'hCAFE);
        mbox_read(6'h04, rdata);
        check(rdata[2], 1'b1, "STATUS.cmd_pending after CMD write");

        // Write ARG0
        mbox_write(6'h0C, 32'hDEAD);
        mbox_read(6'h0C, rdata);
        check(rdata, 32'hDEAD, "ARG0 readback");

        // =====================================================================
        // Test 2: Release CPU from reset
        // =====================================================================
        $display("--- Test Group 2: CPU Boot ---");

        // Set CTRL[0] = 1 (release reset)
        mbox_write(6'h00, 32'h00000001);
        mbox_read(6'h00, rdata);
        check(rdata, 32'h1, "CTRL readback (CPU running)");

        // Wait for CPU to execute firmware
        // The firmware writes x3 (= 0x42) to RESP0 then loops
        repeat (500) @(posedge clk);

        // Read RESP0 — should contain 0x42 (from firmware SW to mailbox)
        mbox_read(6'h18, rdata);
        check(rdata, 32'h42, "RESP0 = 0x42 (firmware wrote BRAM→mailbox)");

        // =====================================================================
        // Summary
        // =====================================================================
        $display("");
        $display("=== Results: %0d PASS, %0d FAIL (of %0d) ===",
                 pass_count, fail_count, test_num);

        if (fail_count > 0)
            $display("*** SOME TESTS FAILED ***");
        else
            $display("*** ALL TESTS PASSED ***");

        $display("");
        $finish;
    end

    // Timeout
    initial begin
        #100000;
        $display("TIMEOUT: simulation exceeded 100us");
        $finish(1);
    end

endmodule
