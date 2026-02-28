// Test if resetn pin is actually HIGH
// Pin 15: Direct copy of resetn input (should be HIGH if pullup works)
// Pin 16: Inverted resetn (should be LOW if pullup works)

module pin_test (
    input wire resetn,
    output wire led_resetn,
    output wire led_resetn_inv
);

assign led_resetn = resetn;      // Should be HIGH (pullup)
assign led_resetn_inv = ~resetn; // Should be LOW

endmodule
