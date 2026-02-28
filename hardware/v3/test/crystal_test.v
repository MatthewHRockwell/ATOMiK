// Crystal Oscillation Test
// Bypasses PLL - directly divides external clock to blink LED
// If LED blinks → crystal is oscillating
// If LED stays off → crystal circuit is broken

module crystal_test (
    input wire clk,          // External crystal (pin 52)
    output wire led_blink,   // Blink from divided crystal
    output wire led_toggle   // Fast toggle (visual check)
);

// Counter driven directly by external clock
reg [25:0] counter = 0;

always @(posedge clk) begin
    counter <= counter + 1;
end

// Slow blink (visible)
assign led_blink = counter[25];  // ~1.5 Hz at 25-27 MHz

// Fast toggle (should appear dimly lit if crystal works)
assign led_toggle = counter[20]; // ~24-27 Hz

endmodule
