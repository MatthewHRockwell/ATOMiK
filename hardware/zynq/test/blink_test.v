// Absolute minimum blink test for AX7020
// No PLL, no CPU, just a counter on the raw 50 MHz clock
module blink_test(
    input  wire clk50,     // U18 - 50 MHz oscillator
    output wire [3:0] led  // M14, M15, K16, J16 - active low
);

reg [25:0] counter = 0;

always @(posedge clk50)
    counter <= counter + 1;

// LEDs active LOW on AX7020
assign led[0] = ~counter[23]; // ~3 Hz blink
assign led[1] = ~counter[24]; // ~1.5 Hz blink
assign led[2] = ~counter[25]; // ~0.75 Hz blink
assign led[3] = ~counter[22]; // ~6 Hz blink

endmodule
