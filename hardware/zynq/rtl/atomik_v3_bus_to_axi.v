// =============================================================================
// ATOMiK v3 Bus-to-AXI4 Master Bridge
//
// Module:      atomik_v3_bus_to_axi
// Description: Translates the v3 CPU's 32-bit valid/ready bus protocol to
//              AXI4 master transactions for DDR3 access via Zynq S_AXI_HP0.
//
//              The v3 CPU bus is a simple single-beat protocol:
//                - mem_valid: CPU asserts transaction
//                - mem_ready: Bridge acknowledges completion
//                - mem_wstrb: Non-zero = write, zero = read
//                - mem_addr:  32-bit byte address
//                - mem_wdata: 32-bit write data
//                - mem_rdata: 32-bit read data
//
//              AXI4 master interface:
//                - 32-bit data bus (matches CPU width)
//                - Single-beat transactions (AWLEN/ARLEN = 0)
//                - Address translation: CPU addr + BASE_OFFSET → physical DDR3
//
// Address Translation:
//   CPU address 0x00000000 → DDR3 physical 0x10000000 (configurable BASE_OFFSET)
//   This allows ARM to load firmware at a reserved DDR3 region.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_v3_bus_to_axi #(
    parameter ADDR_WIDTH = 32,
    parameter DATA_WIDTH = 32,
    parameter ID_WIDTH   = 4,
    parameter [31:0] BASE_OFFSET = 32'h10000000  // CPU 0x0 maps to DDR3 0x10000000
) (
    input  wire                    clk,
    input  wire                    rst_n,

    // =========================================================================
    // v3 CPU Bus Interface (slave — CPU is master)
    // =========================================================================
    input  wire                    cpu_valid,
    output reg                     cpu_ready,
    input  wire [31:0]             cpu_addr,
    input  wire [31:0]             cpu_wdata,
    input  wire [3:0]              cpu_wstrb,
    output reg  [31:0]             cpu_rdata,

    // =========================================================================
    // AXI4 Master Interface (to Zynq S_AXI_HP0)
    // =========================================================================
    // Write address channel
    output reg  [ID_WIDTH-1:0]     m_axi_awid,
    output reg  [ADDR_WIDTH-1:0]   m_axi_awaddr,
    output wire [7:0]              m_axi_awlen,
    output wire [2:0]              m_axi_awsize,
    output wire [1:0]              m_axi_awburst,
    output wire                    m_axi_awlock,
    output wire [3:0]              m_axi_awcache,
    output wire [2:0]              m_axi_awprot,
    output wire [3:0]              m_axi_awqos,
    output reg                     m_axi_awvalid,
    input  wire                    m_axi_awready,

    // Write data channel
    output reg  [DATA_WIDTH-1:0]   m_axi_wdata,
    output reg  [DATA_WIDTH/8-1:0] m_axi_wstrb,
    output wire                    m_axi_wlast,
    output reg                     m_axi_wvalid,
    input  wire                    m_axi_wready,

    // Write response channel
    input  wire [ID_WIDTH-1:0]     m_axi_bid,
    input  wire [1:0]              m_axi_bresp,
    input  wire                    m_axi_bvalid,
    output reg                     m_axi_bready,

    // Read address channel
    output reg  [ID_WIDTH-1:0]     m_axi_arid,
    output reg  [ADDR_WIDTH-1:0]   m_axi_araddr,
    output wire [7:0]              m_axi_arlen,
    output wire [2:0]              m_axi_arsize,
    output wire [1:0]              m_axi_arburst,
    output wire                    m_axi_arlock,
    output wire [3:0]              m_axi_arcache,
    output wire [2:0]              m_axi_arprot,
    output wire [3:0]              m_axi_arqos,
    output reg                     m_axi_arvalid,
    input  wire                    m_axi_arready,

    // Read data channel
    input  wire [ID_WIDTH-1:0]     m_axi_rid,
    input  wire [DATA_WIDTH-1:0]   m_axi_rdata,
    input  wire [1:0]              m_axi_rresp,
    input  wire                    m_axi_rlast,
    input  wire                    m_axi_rvalid,
    output reg                     m_axi_rready
);

    // =========================================================================
    // AXI4 constant signals (single-beat, non-cacheable)
    // =========================================================================
    assign m_axi_awlen   = 8'b0;         // Single beat
    assign m_axi_awsize  = 3'b010;       // 4 bytes
    assign m_axi_awburst = 2'b01;        // INCR (don't care for len=0)
    assign m_axi_awlock  = 1'b0;
    assign m_axi_awcache = 4'b0011;      // Normal non-cacheable bufferable
    assign m_axi_awprot  = 3'b000;       // Unprivileged, secure, data
    assign m_axi_awqos   = 4'b0;
    assign m_axi_wlast   = 1'b1;         // Always last (single beat)

    assign m_axi_arlen   = 8'b0;
    assign m_axi_arsize  = 3'b010;
    assign m_axi_arburst = 2'b01;
    assign m_axi_arlock  = 1'b0;
    assign m_axi_arcache = 4'b0011;
    assign m_axi_arprot  = 3'b000;
    assign m_axi_arqos   = 4'b0;

    // =========================================================================
    // Address translation
    // =========================================================================
    wire [31:0] translated_addr = cpu_addr + BASE_OFFSET;

    // =========================================================================
    // FSM
    // =========================================================================
    localparam S_IDLE      = 3'd0;
    localparam S_WR_ADDR   = 3'd1;
    localparam S_WR_DATA   = 3'd2;
    localparam S_WR_RESP   = 3'd3;
    localparam S_RD_ADDR   = 3'd4;
    localparam S_RD_DATA   = 3'd5;

    reg [2:0] state;

    always @(posedge clk) begin
        if (!rst_n) begin
            state          <= S_IDLE;
            cpu_ready      <= 1'b0;
            cpu_rdata      <= 32'b0;

            m_axi_awid     <= {ID_WIDTH{1'b0}};
            m_axi_awaddr   <= 32'b0;
            m_axi_awvalid  <= 1'b0;

            m_axi_wdata    <= 32'b0;
            m_axi_wstrb    <= 4'b0;
            m_axi_wvalid   <= 1'b0;

            m_axi_bready   <= 1'b0;

            m_axi_arid     <= {ID_WIDTH{1'b0}};
            m_axi_araddr   <= 32'b0;
            m_axi_arvalid  <= 1'b0;

            m_axi_rready   <= 1'b0;
        end else begin
            // Default: deassert single-cycle signals
            cpu_ready <= 1'b0;

            case (state)
                S_IDLE: begin
                    if (cpu_valid && !cpu_ready) begin
                        if (|cpu_wstrb) begin
                            // Write transaction
                            m_axi_awaddr  <= translated_addr;
                            m_axi_awvalid <= 1'b1;
                            m_axi_wdata   <= cpu_wdata;
                            m_axi_wstrb   <= cpu_wstrb;
                            m_axi_wvalid  <= 1'b1;
                            state         <= S_WR_ADDR;
                        end else begin
                            // Read transaction
                            m_axi_araddr  <= translated_addr;
                            m_axi_arvalid <= 1'b1;
                            state         <= S_RD_ADDR;
                        end
                    end
                end

                // ---------------------------------------------------------
                // Write path: issue AW and W simultaneously, wait for B
                // ---------------------------------------------------------
                S_WR_ADDR: begin
                    // AW accepted?
                    if (m_axi_awvalid && m_axi_awready)
                        m_axi_awvalid <= 1'b0;

                    // W accepted?
                    if (m_axi_wvalid && m_axi_wready)
                        m_axi_wvalid <= 1'b0;

                    // Both accepted → wait for response
                    if ((!m_axi_awvalid || m_axi_awready) &&
                        (!m_axi_wvalid  || m_axi_wready)) begin
                        m_axi_awvalid <= 1'b0;
                        m_axi_wvalid  <= 1'b0;
                        m_axi_bready  <= 1'b1;
                        state         <= S_WR_RESP;
                    end
                end

                S_WR_RESP: begin
                    if (m_axi_bvalid && m_axi_bready) begin
                        m_axi_bready <= 1'b0;
                        cpu_ready    <= 1'b1;
                        state        <= S_IDLE;
                    end
                end

                // ---------------------------------------------------------
                // Read path: issue AR, wait for R
                // ---------------------------------------------------------
                S_RD_ADDR: begin
                    if (m_axi_arvalid && m_axi_arready) begin
                        m_axi_arvalid <= 1'b0;
                        m_axi_rready  <= 1'b1;
                        state         <= S_RD_DATA;
                    end
                end

                S_RD_DATA: begin
                    if (m_axi_rvalid && m_axi_rready) begin
                        m_axi_rready <= 1'b0;
                        cpu_rdata    <= m_axi_rdata;
                        cpu_ready    <= 1'b1;
                        state        <= S_IDLE;
                    end
                end

                default: state <= S_IDLE;
            endcase
        end
    end

endmodule
