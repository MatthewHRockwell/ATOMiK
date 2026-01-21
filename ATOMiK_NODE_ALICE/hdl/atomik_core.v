// =============================================================================
// ATOMiK Core Logic (v3.4 - Fmax + Determinism)
// - Robust end-of-execution detection using data_valid falling edge
// - Optional timer gating to reduce switching when idle
// =============================================================================

module atomik_core (
    input  wire        clk,
    input  wire        rst_n,

    // Configuration Inputs (From BIOS)
    input  wire [31:0] scramble_threshold,
    input  wire [31:0] polymorph_seed,
    input  wire        otp_en,

    // Data Interfaces
    input  wire [31:0] data_in,
    input  wire        data_valid,
    output reg  [31:0] data_out,
    output reg         data_ready
);

    // --- STATE ---
    reg [31:0] current_seed;
    reg [31:0] timer;

    reg        dv_prev;             // previous data_valid
    reg        processing_active;    // "in-transaction" indicator

    // --- XorShift next seed (combinational) ---
    wire [31:0] seed_s1 = current_seed ^ (current_seed << 13);
    wire [31:0] seed_s2 = seed_s1      ^ (seed_s1 >> 17);
    wire [31:0] seed_s3 = seed_s2      ^ (seed_s2 << 5);

    wire        seed_is_zero = ~(|seed_s3);
    wire [31:0] next_seed    = seed_is_zero ? 32'hFFFF_FFFF : seed_s3;

    // Falling edge indicates "transaction ended"
    wire execution_done = dv_prev && !data_valid;

    // Time-based scramble condition (only meaningful if threshold > 0)
    wire time_scramble = (scramble_threshold != 32'd0) && (timer >= scramble_threshold);

    // --- SYNCHRONOUS LOGIC ---
    always @(posedge clk) begin
        if (!rst_n) begin
            current_seed       <= 32'd0;          // will load on first active cycle
            timer              <= 32'd0;
            dv_prev            <= 1'b0;
            processing_active  <= 1'b0;
            data_out           <= 32'd0;
            data_ready         <= 1'b0;
        end else begin
            // Track previous data_valid
            dv_prev <= data_valid;

            // Initialize seed on first cycle after reset (deterministic)
            // If you prefer: load this when core_enable asserts (handled in top via rst_n)
            if (current_seed == 32'd0) begin
                current_seed <= (polymorph_seed != 32'd0) ? polymorph_seed : 32'hFFFF_FFFF;
            end

            // 1) DATA PROCESSING
            if (data_valid) begin
                processing_active <= 1'b1;
                data_out          <= data_in ^ current_seed;
                data_ready        <= 1'b1;
            end else begin
                data_ready <= 1'b0;

                // Clear processing_active only after we observe the falling edge
                if (execution_done)
                    processing_active <= 1'b0;
            end

            // 2) POLYMORPHISM: rotate seed on time threshold OR OTP end-of-transaction
            if ( time_scramble || (otp_en && execution_done) ) begin
                current_seed <= next_seed;
                timer        <= 32'd0;
            end else begin
                // Timer policy:
                // Option A (recommended): count only when processing_active or data_valid
                if (scramble_threshold != 32'd0) begin
                    if (processing_active || data_valid)
                        timer <= timer + 32'd1;
                end
            end
        end
    end

endmodule
