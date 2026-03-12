// =============================================================================
// ATOMiK Zynq PL Top-Level
//
// Module:      atomik_zynq_top
// Description: Structural wrapper for ATOMiK on Zynq PL fabric.
//              Dual-clock architecture:
//                - AXI domain: 100 MHz (FCLK_CLK0 via MMCM passthrough)
//                - ATOMiK domain: 200+ MHz (MMCM CLKOUT0, parameterizable)
//              Toggle-handshake CDC bridge between domains.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_zynq_top #(
    parameter ADDR_WIDTH     = 6,
    parameter DATA_WIDTH     = 32,
    parameter real ATOMIK_CLK_DIV = 5.0   // 1000/5 = 200 MHz (sweep: 4.0→250, 3.0→333)
) (
    // PS interface
    input  wire                    fclk_clk0,       // PS fabric clock (100 MHz)
    input  wire                    fclk_reset_n,    // PS fabric reset (active low)

    // Clock status
    output wire                    locked,           // MMCM locked

    // AXI4-Lite Slave Interface
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
    // Internal clocks and resets
    // =========================================================================
    wire axi_clk;
    wire atomik_clk;

    wire core_enable;    // From AXI wrapper CONFIG register
    wire core_rst_n;     // Combined: fclk_reset_n & locked & core_enable

    // Hold core in reset until MMCM locks AND wrapper enables it
    assign core_rst_n = fclk_reset_n & locked & core_enable;

    // AXI reset: fclk_reset_n & locked
    wire axi_rst_n;
    assign axi_rst_n = fclk_reset_n & locked;

    // =========================================================================
    // Clock Module (MMCM)
    // =========================================================================
    atomik_zynq_clk #(
        .FCLK_PERIOD    (10.0),
        .VCO_MULT       (10.0),
        .ATOMIK_CLK_DIV (ATOMIK_CLK_DIV),
        .AXI_CLK_DIV    (10)
    ) u_clk (
        .fclk_clk0  (fclk_clk0),
        .rst_n      (fclk_reset_n),
        .axi_clk    (axi_clk),
        .atomik_clk (atomik_clk),
        .locked     (locked)
    );

    // =========================================================================
    // CDC Bridge signals
    // =========================================================================
    wire        cdc_req_valid;
    wire        cdc_req_ready;
    wire        cdc_req_load_en;
    wire        cdc_req_accum_en;
    wire        cdc_req_swap_en;
    wire [7:0]  cdc_req_addr;
    wire [63:0] cdc_req_data;

    wire [63:0] cdc_resp_state;
    wire        cdc_resp_acc_zero;
    wire        cdc_resp_valid;

    // Core control signals (core clock domain)
    wire        core_load_en;
    wire        core_accum_en;
    wire        core_swap_en;
    wire [7:0]  core_addr;
    wire [63:0] core_delta;
    wire [63:0] core_init_data;
    wire [63:0] core_current_state;
    wire        core_acc_zero;

    // =========================================================================
    // AXI4-Lite Wrapper (AXI clock domain)
    // =========================================================================
    atomik_axi4lite_wrapper #(
        .ADDR_WIDTH (ADDR_WIDTH),
        .DATA_WIDTH (DATA_WIDTH)
    ) u_wrapper (
        .s_axi_aclk    (axi_clk),
        .s_axi_aresetn (axi_rst_n),

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
        .s_axi_rready  (s_axi_rready),

        // CDC bridge interface
        .cdc_req_valid    (cdc_req_valid),
        .cdc_req_ready    (cdc_req_ready),
        .cdc_req_load_en  (cdc_req_load_en),
        .cdc_req_accum_en (cdc_req_accum_en),
        .cdc_req_swap_en  (cdc_req_swap_en),
        .cdc_req_addr     (cdc_req_addr),
        .cdc_req_data     (cdc_req_data),

        .cdc_resp_state    (cdc_resp_state),
        .cdc_resp_acc_zero (cdc_resp_acc_zero),
        .cdc_resp_valid    (cdc_resp_valid),

        .core_enable (core_enable)
    );

    // =========================================================================
    // CDC Bridge (AXI ↔ ATOMiK clock domains)
    // =========================================================================
    atomik_cdc_bridge u_cdc (
        // AXI side
        .clk_axi       (axi_clk),
        .rst_axi_n     (axi_rst_n),

        .req_valid     (cdc_req_valid),
        .req_ready     (cdc_req_ready),
        .req_load_en   (cdc_req_load_en),
        .req_accum_en  (cdc_req_accum_en),
        .req_swap_en   (cdc_req_swap_en),
        .req_addr      (cdc_req_addr),
        .req_data      (cdc_req_data),

        .resp_state    (cdc_resp_state),
        .resp_acc_zero (cdc_resp_acc_zero),
        .resp_valid    (cdc_resp_valid),

        // Core side
        .clk_core      (atomik_clk),
        .rst_core_n    (core_rst_n),

        .core_load_en      (core_load_en),
        .core_accum_en     (core_accum_en),
        .core_swap_en      (core_swap_en),
        .core_addr         (core_addr),
        .core_delta        (core_delta),
        .core_init_data    (core_init_data),
        .core_current_state(core_current_state),
        .core_acc_zero     (core_acc_zero)
    );

    // =========================================================================
    // ATOMiK Core (ATOMiK clock domain — 200+ MHz)
    // Zynq-optimized: BRAM state table + pipelined SWAP write-back
    // =========================================================================
    atomik_core_zynq u_atomik (
        .clk           (atomik_clk),
        .rst_n         (core_rst_n),
        .load_en       (core_load_en),
        .accum_en      (core_accum_en),
        .swap_en       (core_swap_en),
        .addr_in       (core_addr),
        .delta_in      (core_delta),
        .init_data     (core_init_data),
        .current_state (core_current_state),
        .acc_zero      (core_acc_zero)
    );

endmodule
