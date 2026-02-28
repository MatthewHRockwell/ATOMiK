// PLL Lock Diagnostic Test
// Routes PLL lock signal to LED to verify 27 MHz crystal + PLL works
// If LED is ON → PLL locks successfully
// If LED is OFF → Crystal not oscillating or PLL not locking

module pll_lock_test (
    input wire clk,        // 27 MHz external crystal
    output wire led_lock,  // PLL lock indicator
    output wire led_blink  // Blink from PLL output (proves clock works)
);

// PLL: 27 MHz → 126 MHz (same config as v2/v3)
wire clk_pll;
wire pll_lock;

Gowin_rPLL u_pll (
    .clkin(clk),
    .clkout(clk_pll),
    .lock(pll_lock)
);

// Test both polarities - if inverted lock lights up, LED is active-low
assign led_lock = ~pll_lock;  // Inverted for testing

// Counter for blink (using PLL output clock, no gating)
reg [24:0] counter = 0;

always @(posedge clk_pll) begin
    counter <= counter + 1;  // Always increment (removed pll_lock gating)
end

// Route actual lock to blink LED for verification
assign led_blink = pll_lock;  // Direct lock signal

endmodule
