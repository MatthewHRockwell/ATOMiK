// Test Reset_Sync logic
// Pin 15: sys_resetn (should go HIGH after 15 clock cycles)
// Pin 16: Clock toggle (should blink)

module reset_test (
    input wire clk,
    input wire resetn,
    output wire led_reset_n,
    output wire led_clk
);

wire clk_div;
wire sys_resetn;

// CLKDIV: 27 MHz → 13.5 MHz
Gowin_CLKDIV u_div (
    .clkout(clk_div),
    .hclkin(clk),
    .resetn(1'b1)
);

// Reset synchronizer (from picoperipheral.v)
Reset_Sync u_reset (
    .clk(clk_div),
    .ext_reset(resetn),
    .resetn(sys_resetn)
);

// Counter on divided clock
reg [24:0] counter = 0;
always @(posedge clk_div) begin
    counter <= counter + 1;
end

assign led_reset_n = sys_resetn;   // HIGH when reset released
assign led_clk = counter[24];      // Blink at ~0.8 Hz

endmodule
