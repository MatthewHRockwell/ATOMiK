// =============================================================================
// ATOMiK Zynq PL Top-Level
//
// Module:      atomik_zynq_top
// Description: Structural wrapper for ATOMiK on Zynq PL fabric.
//              Instantiates clock module + AXI4-Lite wrapper.
//              Exposes AXI4-Lite slave port for PS GP AXI master connection.
//
//              Phase 1: Single clock domain (FCLK_CLK0 for both AXI and core).
//              Phase 2: Clock module generates separate ATOMiK clock via MMCM.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_zynq_top #(
    parameter ADDR_WIDTH = 6,
    parameter DATA_WIDTH = 32
) (
    // PS interface
    input  wire                    fclk_clk0,       // PS fabric clock
    input  wire                    fclk_reset_n,    // PS fabric reset (active low)

    // Clock status
    output wire                    locked,           // Clock PLL/MMCM locked

    // AXI4-Lite Slave Interface (directly connected to PS GP AXI)
    input  wire [ADDR_WIDTH-1:0]   s_axi_awaddr,
    input  wire                    s_axi_awvalid,
    output wire                    s_axi_awready,

    input  wire [DATA_WIDTH-1:0]   s_axi_wdata,
    input  wire [DATA_WIDTH/8-1:0] s_axi_wstrb,
    input  wire                    s_axi_wvalid,
    output wire                    s_axi_wready,

    output wire [1:0]              s_axi_bresp,
    output wire                    s_axi_bvalid,
    input  wire                    s_axi_bready,

    input  wire [ADDR_WIDTH-1:0]   s_axi_araddr,
    input  wire                    s_axi_arvalid,
    output wire                    s_axi_arready,

    output wire [DATA_WIDTH-1:0]   s_axi_rdata,
    output wire [1:0]              s_axi_rresp,
    output wire                    s_axi_rvalid,
    input  wire                    s_axi_rready
);

    // =========================================================================
    // Internal clocks
    // =========================================================================
    wire axi_clk;
    wire atomik_clk;

    // =========================================================================
    // Clock Module
    // =========================================================================
    atomik_zynq_clk u_clk (
        .fclk_clk0  (fclk_clk0),
        .axi_clk    (axi_clk),
        .atomik_clk (atomik_clk),   // Phase 1: same as axi_clk
        .locked     (locked)
    );

    // =========================================================================
    // AXI4-Lite Wrapper (includes ATOMiK core)
    // =========================================================================
    atomik_axi4lite_wrapper #(
        .ADDR_WIDTH (ADDR_WIDTH),
        .DATA_WIDTH (DATA_WIDTH)
    ) u_wrapper (
        .s_axi_aclk    (axi_clk),
        .s_axi_aresetn (fclk_reset_n),

        .s_axi_awaddr  (s_axi_awaddr),
        .s_axi_awvalid (s_axi_awvalid),
        .s_axi_awready (s_axi_awready),

        .s_axi_wdata   (s_axi_wdata),
        .s_axi_wstrb   (s_axi_wstrb),
        .s_axi_wvalid  (s_axi_wvalid),
        .s_axi_wready  (s_axi_wready),

        .s_axi_bresp   (s_axi_bresp),
        .s_axi_bvalid  (s_axi_bvalid),
        .s_axi_bready  (s_axi_bready),

        .s_axi_araddr  (s_axi_araddr),
        .s_axi_arvalid (s_axi_arvalid),
        .s_axi_arready (s_axi_arready),

        .s_axi_rdata   (s_axi_rdata),
        .s_axi_rresp   (s_axi_rresp),
        .s_axi_rvalid  (s_axi_rvalid),
        .s_axi_rready  (s_axi_rready)
    );

endmodule
