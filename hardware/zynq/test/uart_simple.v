module uart_simple(output wire uart_tx, output wire [3:0] leds);

// Ring oscillator as clock source (no external clock needed)
(* DONT_TOUCH = "true" *) wire [4:0] ring;
assign ring[0] = ~ring[4];
genvar i;
generate for (i = 0; i < 4; i = i + 1) begin : delay
    (* DONT_TOUCH = "true" *) LUT1 #(.INIT(2'b10)) lut (.I0(ring[i]), .O(ring[i+1]));
end endgenerate

// Divide ring osc down to ~1 MHz range, then to baud rate
reg [31:0] cnt = 0;
always @(posedge ring[0]) cnt <= cnt + 1;

// Heartbeat on header pins
assign leds[0] = cnt[20];
assign leds[1] = cnt[21];
assign leds[2] = cnt[22];
assign leds[3] = cnt[23];

// Simple TX: just toggle uart_tx as heartbeat for now (square wave)
// If we see it on a scope/meter, PL I/O + clock confirmed
assign uart_tx = cnt[15]; // some frequency on the TX pin

endmodule
