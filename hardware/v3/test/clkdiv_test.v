// Test CLKDIV output - toggle LED at divided clock rate
// If this works: CLKDIV is functional
// If this fails: CLKDIV or clock routing issue

module clkdiv_test (
    input wire clk,       // 27 MHz crystal
    output wire led_fast, // Direct divided clock (13.5 MHz ÷ 2^24 = ~0.8 Hz)
    output wire led_slow  // Slower blink (~0.4 Hz)
);

wire clk_div;  // 13.5 MHz from CLKDIV

// Instantiate CLKDIV (same as SoC)
Gowin_CLKDIV u_div (
    .clkout(clk_div),
    .hclkin(clk),
    .resetn(1'b1)
);

// Counter on divided clock
reg [24:0] counter = 0;
always @(posedge clk_div) begin
    counter <= counter + 1;
end

assign led_fast = counter[24];  // Bit 24 toggles at ~0.8 Hz (13.5 MHz / 2^25)
assign led_slow = counter[24] & counter[23];  // AND for slower visible rate

endmodule
