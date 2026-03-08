// =============================================================================
// ATOMiK Zynq Clock Module
//
// Module:      atomik_zynq_clk
// Description: Clock generation for ATOMiK on Zynq PL.
//              Phase 1 (SINGLE_CLOCK): Passthrough — both AXI and ATOMiK
//              clocks are FCLK_CLK0 from PS. No MMCM needed.
//              Phase 2: Replace body with MMCM for higher ATOMiK clock.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_zynq_clk (
    input  wire fclk_clk0,    // PS fabric clock (50-100 MHz)

    output wire axi_clk,      // AXI bus clock
    output wire atomik_clk,   // ATOMiK core clock
    output wire locked        // Clock locked indicator
);

    // Phase 1: Single clock domain — passthrough
    assign axi_clk    = fclk_clk0;
    assign atomik_clk = fclk_clk0;
    assign locked      = 1'b1;

    // Phase 2: Replace with MMCM instantiation:
    //   MMCME2_BASE #(
    //       .CLKIN1_PERIOD  (10.0),  // 100 MHz input
    //       .CLKFBOUT_MULT_F(10.0),  // VCO = 1000 MHz
    //       .CLKOUT0_DIVIDE_F(5.0),  // 200 MHz ATOMiK clock
    //       .CLKOUT1_DIVIDE  (10)    // 100 MHz AXI clock (passthrough)
    //   ) u_mmcm ( ... );

endmodule
