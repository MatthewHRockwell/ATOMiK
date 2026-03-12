// =============================================================================
// ATOMiK Zynq Clock Module
//
// Module:      atomik_zynq_clk
// Description: Clock generation for ATOMiK on Zynq PL.
//              Uses MMCME2_BASE to generate a high-speed ATOMiK clock from
//              the PS FCLK_CLK0 (100 MHz).
//
//              Default configuration:
//                VCO = 100 MHz * 10.0 = 1000 MHz
//                CLKOUT0 = 1000 / ATOMIK_CLK_DIV = ATOMiK clock
//                CLKOUT1 = 1000 / 10 = 100 MHz (AXI passthrough)
//
//              To sweep Fmax, change ATOMIK_CLK_DIV parameter:
//                5.0 → 200 MHz    4.0 → 250 MHz    3.0 → 333 MHz
//                2.5 → 400 MHz    (push until timing fails)
//
//              For iverilog simulation, define SIMULATION to use a behavioral
//              clock divider instead of the Xilinx MMCM primitive.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_zynq_clk #(
    parameter real FCLK_PERIOD     = 10.0,   // FCLK_CLK0 period in ns (100 MHz)
    parameter real VCO_MULT        = 10.0,   // VCO multiplier (VCO = 100 * 10 = 1000 MHz)
    parameter real ATOMIK_CLK_DIV  = 5.0,    // ATOMiK clock divider (1000 / 5 = 200 MHz)
    parameter integer AXI_CLK_DIV  = 10      // AXI clock divider (1000 / 10 = 100 MHz)
) (
    input  wire fclk_clk0,    // PS fabric clock (100 MHz)
    input  wire rst_n,        // Active-low reset

    output wire axi_clk,      // AXI bus clock (100 MHz)
    output wire atomik_clk,   // ATOMiK core clock (200+ MHz)
    output wire locked        // MMCM locked indicator
);

`ifdef SIMULATION
    // =========================================================================
    // Simulation: behavioral clock generation
    // =========================================================================
    // In simulation, both clocks derive from fclk_clk0.
    // atomik_clk runs at a faster rate using a counter-based divider.
    // For functional verification, the exact frequency ratio matters less
    // than having two independent, asynchronous-ish clock edges.

    assign axi_clk = fclk_clk0;

    // Generate a faster clock by toggling at half the period
    // For ATOMIK_CLK_DIV=5.0 (200 MHz = 2.5ns half-period):
    reg sim_atomik_clk = 0;
    reg sim_locked = 0;

    // Compute half-period: VCO_period/2 * ATOMIK_CLK_DIV
    // VCO = 1 GHz → 1ns period → 0.5ns half. CLKOUT0 = VCO/DIV → half = DIV * 0.5ns
    localparam real SIM_ATOMIK_HALF_PERIOD = (FCLK_PERIOD * ATOMIK_CLK_DIV) / (VCO_MULT * 2.0);

    always #(SIM_ATOMIK_HALF_PERIOD) sim_atomik_clk = ~sim_atomik_clk;

    // Simulate lock delay (~10 fclk cycles)
    integer lock_cnt = 0;
    always @(posedge fclk_clk0) begin
        if (!rst_n) begin
            lock_cnt   <= 0;
            sim_locked <= 1'b0;
        end else if (lock_cnt < 10) begin
            lock_cnt   <= lock_cnt + 1;
            sim_locked <= 1'b0;
        end else begin
            sim_locked <= 1'b1;
        end
    end

    assign atomik_clk = sim_atomik_clk;
    assign locked     = sim_locked;

`else
    // =========================================================================
    // Synthesis: Xilinx MMCME2_BASE
    // =========================================================================
    wire clkfb;
    wire clkout0_unbuf;
    wire clkout1_unbuf;

    MMCME2_BASE #(
        .CLKIN1_PERIOD   (FCLK_PERIOD),
        .CLKFBOUT_MULT_F (VCO_MULT),       // VCO = FCLK * MULT = 1000 MHz
        .CLKOUT0_DIVIDE_F(ATOMIK_CLK_DIV), // ATOMiK clock
        .CLKOUT1_DIVIDE  (AXI_CLK_DIV),    // AXI clock (passthrough)
        .DIVCLK_DIVIDE   (1),
        .STARTUP_WAIT    ("FALSE")
    ) u_mmcm (
        .CLKIN1   (fclk_clk0),
        .RST      (~rst_n),
        .PWRDWN   (1'b0),

        .CLKFBOUT (clkfb),
        .CLKFBIN  (clkfb),

        .CLKOUT0  (clkout0_unbuf),
        .CLKOUT1  (clkout1_unbuf),

        // Unused outputs
        .CLKOUT0B (),
        .CLKOUT1B (),
        .CLKOUT2  (),
        .CLKOUT2B (),
        .CLKOUT3  (),
        .CLKOUT3B (),
        .CLKOUT4  (),
        .CLKOUT5  (),
        .CLKOUT6  (),
        .CLKFBOUTB(),
        .LOCKED   (locked)
    );

    // Global clock buffers
    BUFG u_bufg_atomik (.I(clkout0_unbuf), .O(atomik_clk));
    BUFG u_bufg_axi    (.I(clkout1_unbuf), .O(axi_clk));

`endif

endmodule
