// Simple LED blink test - pins 15, 17, 18
module led_blink (
    input clk,
    output reg pin15,  // Debug LED
    output reg pin17,  // UART TX
    output reg pin18   // UART RX
);

reg [24:0] counter;

always @(posedge clk) begin
    counter <= counter + 1;
end

// Very distinctive patterns to identify pins
always @(posedge clk) begin
    // Pin 15: Always ON (steady)
    pin15 <= 1'b1;

    // Pin 17: Slow blink (1 Hz - 0.5s on, 0.5s off)
    pin17 <= counter[24];

    // Pin 18: Fast flicker (13 Hz)
    pin18 <= counter[21];
end

endmodule
