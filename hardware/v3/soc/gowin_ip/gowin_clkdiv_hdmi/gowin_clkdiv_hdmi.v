// HDMI Clock Divider: 126 MHz ÷ 5 → 25.2 MHz pixel clock
// Standard 640×480 @ 60 Hz HDMI timing
// Part Number: GW1NR-LV9QN88PC6/I5

module Gowin_CLKDIV_HDMI (clkout, hclkin, resetn);

output clkout;
input hclkin;
input resetn;

wire gw_gnd;

assign gw_gnd = 1'b0;

CLKDIV clkdiv_inst (
    .CLKOUT(clkout),
    .HCLKIN(hclkin),
    .RESETN(resetn),
    .CALIB(gw_gnd)
);

defparam clkdiv_inst.DIV_MODE = "5";
defparam clkdiv_inst.GSREN = "false";

endmodule
