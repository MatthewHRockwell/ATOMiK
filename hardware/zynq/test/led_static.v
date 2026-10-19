// Absolute minimum: hardwired LED outputs, no clock, no logic
module led_static(
    output wire [3:0] led
);
    // Active-low: 0 = LED ON, 1 = LED OFF
    assign led = 4'b0000;  // All LEDs ON
endmodule
