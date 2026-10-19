// Drive ALL 167 CLG484 PL I/O pins LOW
// If any LED turns on, we know the package and JTAG works
module all_low(
    output wire [166:0] p
);
    assign p = 167'b0;
endmodule
