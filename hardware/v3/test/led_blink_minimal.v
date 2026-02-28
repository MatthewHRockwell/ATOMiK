// Minimal LED blink test - uses internal OSC, no PLL, no peripherals
// Tests: FPGA can execute, clock works, LED output path works

module led_blink_minimal (
    output wire led_out
);

// Internal 250 MHz oscillator (GW1N-9C primitive)
wire osc_clk;
OSC osc_inst (
    .OSCOUT(osc_clk)
);
defparam osc_inst.FREQ_DIV = 100; // Divide to ~2.5 MHz

// 25-bit counter for ~1.5 Hz blink at 2.5 MHz
// 2.5 MHz / 2^24 ≈ 0.15 Hz (blink every ~3 seconds, visible to human eye)
reg [24:0] counter = 0;

always @(posedge osc_clk) begin
    counter <= counter + 1;
end

// LED toggles at bit 24 (slow enough to see)
assign led_out = counter[24];

endmodule
