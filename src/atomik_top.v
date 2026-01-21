module atomik_top (
    input wire sys_clk,
    input wire sys_rst_n,
    input wire uart_rx,
    output wire uart_tx,
    output wire [5:0] led
);

    // --- INTERNAL WIRING ---
    wire [31:0] w_poly_freq;
    wire [31:0] w_poly_seed;
    wire [255:0] w_dna_storage;
    wire w_core_enable;
    wire w_loader_busy;
    wire w_otp_en; // NEW: One-Time Pad Wire

    // --- BIOS INSTANTIATION ---
    uart_genome_loader #(
        .CLK_FREQ(27000000), .BAUD_RATE(115200)
    ) bios_inst (
        .clk(sys_clk), .rst_n(sys_rst_n), .uart_rx(uart_rx),
        .poly_freq_out(w_poly_freq), .poly_seed_out(w_poly_seed),
        .dna_storage(w_dna_storage), .core_enable(w_core_enable),
        .loader_busy(w_loader_busy),
        .otp_en(w_otp_en) // NEW
    );

    // --- ENGINE INSTANTIATION ---
    atomik_core engine_inst (
        .clk(sys_clk), .rst_n(w_core_enable),
        .scramble_threshold(w_poly_freq), .polymorph_seed(w_poly_seed),
        .otp_en(w_otp_en), // NEW
        .data_in(32'hAABBCCDD), .data_valid(1'b0), // Loopback/Test inputs
        .data_out(), .data_ready()
    );

    // --- LEDs & IO ---
    assign led[0] = ~sys_rst_n;
    assign led[1] = ~w_loader_busy;
    assign led[2] = ~w_core_enable;
    assign led[3] = ~w_otp_en; // LED 3 indicates OTP Mode Active
    assign led[4] = 1'b1;
    assign led[5] = ~uart_rx;
    assign uart_tx = uart_rx; // Echo loopback

endmodule