// HDMI 720p test pattern for HamGeek RK-ZYNQ7020-F (CLG484)
// Uses ALINX rgb2dvi IP (VHDL) + color_bar + direct MMCM instantiation.
// No Xilinx IP catalog dependencies — builds in non-project mode.

module hdmi_test_top(
    inout [14:0] DDR_addr, inout [2:0] DDR_ba, inout DDR_cas_n,
    inout DDR_ck_n, inout DDR_ck_p, inout DDR_cke, inout DDR_cs_n,
    inout [3:0] DDR_dm, inout [31:0] DDR_dq, inout [3:0] DDR_dqs_n,
    inout [3:0] DDR_dqs_p, inout DDR_odt, inout DDR_ras_n,
    inout DDR_reset_n, inout DDR_we_n,
    inout FIXED_IO_ddr_vrn, inout FIXED_IO_ddr_vrp,
    inout [53:0] FIXED_IO_mio, inout FIXED_IO_ps_clk,
    inout FIXED_IO_ps_porb, inout FIXED_IO_ps_srstb,
    output TMDS_clk_p, output TMDS_clk_n,
    output [2:0] TMDS_data_p, output [2:0] TMDS_data_n,
    output hdmi_oen,
    output pl_led1, output pl_led2
);

wire fclk; // 100 MHz from PS FCLK_CLK0
ps_sys_wrapper u_ps (
    .DDR_addr(DDR_addr), .DDR_ba(DDR_ba), .DDR_cas_n(DDR_cas_n),
    .DDR_ck_n(DDR_ck_n), .DDR_ck_p(DDR_ck_p), .DDR_cke(DDR_cke),
    .DDR_cs_n(DDR_cs_n), .DDR_dm(DDR_dm), .DDR_dq(DDR_dq),
    .DDR_dqs_n(DDR_dqs_n), .DDR_dqs_p(DDR_dqs_p), .DDR_odt(DDR_odt),
    .DDR_ras_n(DDR_ras_n), .DDR_reset_n(DDR_reset_n), .DDR_we_n(DDR_we_n),
    .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn), .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
    .FIXED_IO_mio(FIXED_IO_mio), .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
    .FIXED_IO_ps_porb(FIXED_IO_ps_porb), .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
    .fclk(fclk)
);

// AX7020 reference drives HDMI_OUT_EN high. Build variants can override this.
`ifdef HDMI_OUT_EN_LOW
assign hdmi_oen = 1'b0;
`else
assign hdmi_oen = 1'b1;
`endif

// Build variants can choose between the original 75/375 MHz clocks and an
// exact 74.25/371.25 MHz pair for stricter HDMI sinks.

wire pixel_clk, serial_clk, mmcm_locked, mmcm_fb;

`ifdef HDMI_EXACT_74250
MMCME2_BASE #(
    .BANDWIDTH("OPTIMIZED"),
    .CLKFBOUT_MULT_F(37.125),  // 100 * 37.125 / 5 = 742.5 MHz VCO
    .DIVCLK_DIVIDE(5),
    .CLKOUT0_DIVIDE_F(10.0),   // 742.5 / 10 = 74.25 MHz
    .CLKOUT1_DIVIDE(2),        // 742.5 / 2 = 371.25 MHz
    .CLKIN1_PERIOD(10.0)
) u_mmcm (
    .CLKIN1(fclk),
    .CLKFBIN(mmcm_fb),
    .CLKFBOUT(mmcm_fb),
    .CLKOUT0(pixel_clk),
    .CLKOUT1(serial_clk),
    .LOCKED(mmcm_locked),
    .PWRDWN(1'b0),
    .RST(1'b0)
);
`else
MMCME2_BASE #(
    .BANDWIDTH("OPTIMIZED"),
    .CLKFBOUT_MULT_F(7.5),     // VCO = 100 * 7.5 = 750 MHz
    .DIVCLK_DIVIDE(1),
    .CLKOUT0_DIVIDE_F(10.0),   // 750 / 10 = 75 MHz
    .CLKOUT1_DIVIDE(2),        // 750 / 2 = 375 MHz
    .CLKIN1_PERIOD(10.0)
) u_mmcm (
    .CLKIN1(fclk),
    .CLKFBIN(mmcm_fb),
    .CLKFBOUT(mmcm_fb),
    .CLKOUT0(pixel_clk),
    .CLKOUT1(serial_clk),
    .LOCKED(mmcm_locked),
    .PWRDWN(1'b0),
    .RST(1'b0)
);
`endif

// Color bar generator (ALINX reference)
wire video_hs, video_vs, video_de;
wire [7:0] video_r, video_g, video_b;

color_bar u_colorbar (
    .clk(pixel_clk),
    .rst(~mmcm_locked),
    .hs(video_hs),
    .vs(video_vs),
    .de(video_de),
    .rgb_r(video_r),
    .rgb_g(video_g),
    .rgb_b(video_b)
);

// RGB to DVI/TMDS (Digilent rgb2dvi IP — VHDL)
rgb2dvi #(
    .kGenerateSerialClk("FALSE"),  // We provide SerialClk externally
    .kClkRange(2)                  // 74.25 MHz pixel clock range
) u_rgb2dvi (
    .TMDS_Clk_p(TMDS_clk_p),
    .TMDS_Clk_n(TMDS_clk_n),
    .TMDS_Data_p(TMDS_data_p),
    .TMDS_Data_n(TMDS_data_n),
    .aRst_n(mmcm_locked),
    .vid_pData({video_r, video_g, video_b}),
    .vid_pVDE(video_de),
    .vid_pHSync(video_hs),
    .vid_pVSync(video_vs),
    .PixelClk(pixel_clk),
    .SerialClk(serial_clk)
);

// Heartbeat LEDs
reg [25:0] cnt = 0;
always @(posedge fclk) cnt <= cnt + 1;
assign pl_led1 = cnt[24];
assign pl_led2 = mmcm_locked; // LED2 = PLL locked indicator

endmodule
