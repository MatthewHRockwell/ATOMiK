// Drive ALL available PL I/O pins LOW to find which ones have LEDs
// CLG484 package has I/O on banks 13, 33, 34, 35
module pin_scan(
    // Bank 13 pins (active on CLG484 only)
    output wire [49:0] bank13,
    // Bank 33 pins (active on CLG484 only)
    output wire [49:0] bank33,
    // Bank 34 pins
    output wire [49:0] bank34,
    // Bank 35 pins
    output wire [49:0] bank35
);
    // Drive everything LOW (active-low LEDs will turn ON)
    assign bank13 = 50'b0;
    assign bank33 = 50'b0;
    assign bank34 = 50'b0;
    assign bank35 = 50'b0;
endmodule
