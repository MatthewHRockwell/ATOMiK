// =============================================================================
// ATOMiK Co-Processor Top-Level
//
// Module:      atomik_coprocessor_top
// Description: Wraps the v3 RV64I CPU with Zynq integration:
//              - RV64I CPU with direct-wire ATOMiK custom instructions
//              - Local BRAM for hot-loop code (32 KB default)
//              - AXI4 master bridge for DDR3 access (instruction fetch + load/store)
//              - Mailbox for ARM ↔ RV64I communication
//              - Bus mux: routes CPU transactions to BRAM, mailbox, or AXI bridge
//
// Memory Map (RV64I view):
//   0x00000000 - 0x0FFFFFFF  DDR3 via AXI (translated to 0x10000000 physical)
//   0x40000000 - 0x40007FFF  Local BRAM (32 KB, word-addressed)
//   0x80000000 - 0x8000003F  Mailbox registers
//   0x80000100 - 0x800001FF  Reserved (future peripherals)
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

/* verilator lint_off UNUSEDSIGNAL */
/* verilator lint_off UNUSEDPARAM */

module atomik_coprocessor_top #(
    parameter RESET_PC       = 64'h0000_0000_0000_0000,  // Boot from DDR3 (ARM loads firmware)
    parameter BRAM_ADDR_BITS = 13,                        // 2^13 = 8K words = 32 KB
    parameter [31:0] DDR3_BASE_OFFSET = 32'h10000000,    // CPU 0x0 → DDR3 0x10000000
    parameter AXI_ID_WIDTH   = 4
) (
    input  wire        clk,          // Co-processor clock (100 MHz from MMCM)
    input  wire        ext_rst_n,    // External reset (from mailbox CTRL[0])

    // =========================================================================
    // AXI4 Master Interface (to Zynq S_AXI_HP0 for DDR3 access)
    // =========================================================================
    output wire [AXI_ID_WIDTH-1:0] m_axi_awid,
    output wire [31:0]             m_axi_awaddr,
    output wire [7:0]              m_axi_awlen,
    output wire [2:0]              m_axi_awsize,
    output wire [1:0]              m_axi_awburst,
    output wire                    m_axi_awlock,
    output wire [3:0]              m_axi_awcache,
    output wire [2:0]              m_axi_awprot,
    output wire [3:0]              m_axi_awqos,
    output wire                    m_axi_awvalid,
    input  wire                    m_axi_awready,

    output wire [31:0]             m_axi_wdata,
    output wire [3:0]              m_axi_wstrb,
    output wire                    m_axi_wlast,
    output wire                    m_axi_wvalid,
    input  wire                    m_axi_wready,

    input  wire [AXI_ID_WIDTH-1:0] m_axi_bid,
    input  wire [1:0]              m_axi_bresp,
    input  wire                    m_axi_bvalid,
    output wire                    m_axi_bready,

    output wire [AXI_ID_WIDTH-1:0] m_axi_arid,
    output wire [31:0]             m_axi_araddr,
    output wire [7:0]              m_axi_arlen,
    output wire [2:0]              m_axi_arsize,
    output wire [1:0]              m_axi_arburst,
    output wire                    m_axi_arlock,
    output wire [3:0]              m_axi_arcache,
    output wire [2:0]              m_axi_arprot,
    output wire [3:0]              m_axi_arqos,
    output wire                    m_axi_arvalid,
    input  wire                    m_axi_arready,

    input  wire [AXI_ID_WIDTH-1:0] m_axi_rid,
    input  wire [31:0]             m_axi_rdata,
    input  wire [1:0]              m_axi_rresp,
    input  wire                    m_axi_rlast,
    input  wire                    m_axi_rvalid,
    output wire                    m_axi_rready,

    // =========================================================================
    // Mailbox AXI4-Lite Slave (from ARM via AXI interconnect)
    // =========================================================================
    input  wire [5:0]              s_axi_mbox_awaddr,
    input  wire                    s_axi_mbox_awvalid,
    output wire                    s_axi_mbox_awready,
    input  wire [31:0]             s_axi_mbox_wdata,
    input  wire [3:0]              s_axi_mbox_wstrb,
    input  wire                    s_axi_mbox_wvalid,
    output wire                    s_axi_mbox_wready,
    output wire [1:0]              s_axi_mbox_bresp,
    output wire                    s_axi_mbox_bvalid,
    input  wire                    s_axi_mbox_bready,
    input  wire [5:0]              s_axi_mbox_araddr,
    input  wire                    s_axi_mbox_arvalid,
    output wire                    s_axi_mbox_arready,
    output wire [31:0]             s_axi_mbox_rdata,
    output wire [1:0]              s_axi_mbox_rresp,
    output wire                    s_axi_mbox_rvalid,
    input  wire                    s_axi_mbox_rready,

    // =========================================================================
    // Interrupt output
    // =========================================================================
    output wire                    irq_to_arm,

    // =========================================================================
    // Debug
    // =========================================================================
    output wire [31:0]             debug_pc
);

    // =========================================================================
    // CPU bus signals
    // =========================================================================
    wire        cpu_mem_valid;
    wire        cpu_mem_ready;
    wire [31:0] cpu_mem_addr;
    wire [31:0] cpu_mem_wdata;
    wire [3:0]  cpu_mem_wstrb;
    wire [31:0] cpu_mem_rdata;

    // =========================================================================
    // Reset from mailbox
    // =========================================================================
    wire        mailbox_rst_n;
    wire        cpu_rst_n = ext_rst_n & mailbox_rst_n;

    // =========================================================================
    // Bus destination decode (addr[31:28])
    // =========================================================================
    // 0x0_______ → DDR3 via AXI
    // 0x4_______ → Local BRAM
    // 0x8_______ → Mailbox / peripherals
    wire sel_ddr3    = (cpu_mem_addr[31:28] == 4'h0);
    wire sel_bram    = (cpu_mem_addr[31:28] == 4'h4);
    wire sel_mailbox = (cpu_mem_addr[31:28] == 4'h8);

    // =========================================================================
    // Local BRAM (32 KB)
    // =========================================================================
    reg [31:0] bram [0:(1 << BRAM_ADDR_BITS)-1];
    reg [31:0] bram_rdata;
    reg        bram_ready;

    wire [BRAM_ADDR_BITS-1:0] bram_word_addr = cpu_mem_addr[BRAM_ADDR_BITS+1:2];

    always @(posedge clk) begin
        if (!cpu_rst_n) begin
            bram_ready <= 1'b0;
        end else begin
            bram_ready <= 1'b0;

            if (cpu_mem_valid && sel_bram && !bram_ready) begin
                bram_ready <= 1'b1;

                if (|cpu_mem_wstrb) begin
                    // Byte-lane writes
                    if (cpu_mem_wstrb[0]) bram[bram_word_addr][ 7: 0] <= cpu_mem_wdata[ 7: 0];
                    if (cpu_mem_wstrb[1]) bram[bram_word_addr][15: 8] <= cpu_mem_wdata[15: 8];
                    if (cpu_mem_wstrb[2]) bram[bram_word_addr][23:16] <= cpu_mem_wdata[23:16];
                    if (cpu_mem_wstrb[3]) bram[bram_word_addr][31:24] <= cpu_mem_wdata[31:24];
                end

                bram_rdata <= bram[bram_word_addr];
            end
        end
    end

    // =========================================================================
    // Mailbox bus signals
    // =========================================================================
    wire        mbox_rv_ready;
    wire [31:0] mbox_rv_rdata;

    // =========================================================================
    // AXI bridge bus signals
    // =========================================================================
    wire        axi_cpu_ready;
    wire [31:0] axi_cpu_rdata;

    // =========================================================================
    // Bus mux — route CPU transactions
    // =========================================================================
    assign cpu_mem_ready = sel_ddr3    ? axi_cpu_ready :
                           sel_bram    ? bram_ready    :
                           sel_mailbox ? mbox_rv_ready :
                           1'b1;  // Unknown address: ack immediately (prevents hang)

    assign cpu_mem_rdata = sel_ddr3    ? axi_cpu_rdata :
                           sel_bram    ? bram_rdata    :
                           sel_mailbox ? mbox_rv_rdata :
                           32'hDEAD_BEEF;

    // =========================================================================
    // RV64I CPU (v3 architecture, Zynq-ported)
    // =========================================================================
    // Note: atomik_v3_cpu instantiates atomik_v3_regfile and atomik_v3_atomik
    // internally. For Zynq, we use the _zynq variants which must be bound
    // at the project level (add atomik_v3_regfile_zynq.v and
    // atomik_v3_atomik_zynq.v as sources, with module names matching the
    // originals, OR modify the CPU to parameterize the submodule selection).
    //
    // Strategy: Use atomik_v3_cpu.v unmodified. At Vivado project level,
    // add the _zynq variants with wrapper modules that match the original
    // module names. See atomik_v3_cpu_zynq.v for the adapted top-level.
    atomik_v3_cpu_zynq #(
        .RESET_PC(RESET_PC)
    ) u_cpu (
        .clk       (clk),
        .rst_n     (cpu_rst_n),
        .mem_valid (cpu_mem_valid),
        .mem_ready (cpu_mem_ready),
        .mem_addr  (cpu_mem_addr),
        .mem_wdata (cpu_mem_wdata),
        .mem_wstrb (cpu_mem_wstrb),
        .mem_rdata (cpu_mem_rdata)
    );

    // Expose PC for debug (fetch unit's PC is internal; use addr as proxy)
    // The actual PC is exposed via a debug port on the Zynq CPU wrapper
    assign debug_pc = cpu_mem_addr;

    // =========================================================================
    // AXI4 Master Bridge (CPU bus → DDR3 via S_AXI_HP0)
    // =========================================================================
    atomik_v3_bus_to_axi #(
        .ADDR_WIDTH    (32),
        .DATA_WIDTH    (32),
        .ID_WIDTH      (AXI_ID_WIDTH),
        .BASE_OFFSET   (DDR3_BASE_OFFSET)
    ) u_axi_bridge (
        .clk           (clk),
        .rst_n         (cpu_rst_n),

        .cpu_valid     (cpu_mem_valid & sel_ddr3),
        .cpu_ready     (axi_cpu_ready),
        .cpu_addr      (cpu_mem_addr),
        .cpu_wdata     (cpu_mem_wdata),
        .cpu_wstrb     (cpu_mem_wstrb),
        .cpu_rdata     (axi_cpu_rdata),

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
        .m_axi_rready  (m_axi_rready)
    );

    // =========================================================================
    // Mailbox (ARM ↔ RV64I)
    // =========================================================================
    atomik_mailbox #(
        .ADDR_WIDTH (6),
        .DATA_WIDTH (32)
    ) u_mailbox (
        // AXI side (ARM)
        .s_axi_aclk    (clk),
        .s_axi_aresetn (ext_rst_n),

        .s_axi_awaddr  (s_axi_mbox_awaddr),
        .s_axi_awvalid (s_axi_mbox_awvalid),
        .s_axi_awready (s_axi_mbox_awready),
        .s_axi_wdata   (s_axi_mbox_wdata),
        .s_axi_wstrb   (s_axi_mbox_wstrb),
        .s_axi_wvalid  (s_axi_mbox_wvalid),
        .s_axi_wready  (s_axi_mbox_wready),
        .s_axi_bresp   (s_axi_mbox_bresp),
        .s_axi_bvalid  (s_axi_mbox_bvalid),
        .s_axi_bready  (s_axi_mbox_bready),
        .s_axi_araddr  (s_axi_mbox_araddr),
        .s_axi_arvalid (s_axi_mbox_arvalid),
        .s_axi_arready (s_axi_mbox_arready),
        .s_axi_rdata   (s_axi_mbox_rdata),
        .s_axi_rresp   (s_axi_mbox_rresp),
        .s_axi_rvalid  (s_axi_mbox_rvalid),
        .s_axi_rready  (s_axi_mbox_rready),

        // RV64I side (same clock — CPU and mailbox on co-processor clock)
        .rv_clk        (clk),
        .rv_rst_n      (cpu_rst_n),
        .rv_valid      (cpu_mem_valid & sel_mailbox),
        .rv_ready      (mbox_rv_ready),
        .rv_addr       (cpu_mem_addr),
        .rv_wdata      (cpu_mem_wdata),
        .rv_wstrb      (cpu_mem_wstrb),
        .rv_rdata      (mbox_rv_rdata),

        // Control
        .rv64i_reset_n (mailbox_rst_n),
        .irq_to_arm    (irq_to_arm),
        .debug_pc      (debug_pc)
    );

    // =========================================================================
    // Simulation-only BRAM initialization
    // =========================================================================
    // synthesis translate_off
    integer i;
    initial begin
        for (i = 0; i < (1 << BRAM_ADDR_BITS); i = i + 1)
            bram[i] = 32'b0;
    end
    // synthesis translate_on

endmodule
