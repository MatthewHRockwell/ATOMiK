// =============================================================================
// ATOMiK Core Logic (v3.4 - Fmax + Bring-up Friendly)
// - Same port/interface as your current v3.3 (drop-in replacement).
// - Keeps the fast OR-reduction zero check (no 32-bit carry-chain equality).
// - Cleans up a couple of corner behaviors for stable bring-up at high Fclk:
//     * data_ready is a 1-cycle pulse when data_valid=1
//     * processing_active is explicitly tracked
//     * polymorphism update happens after data path update (same cycle ordering)
// =============================================================================

module atomik_core (
    input  wire        clk,
    input  wire        rst_n,           // active-low synchronous reset

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

    // -------------------------------------------------------------------------
    // POLYMORPHIC ENGINE (xorshift32)
    // -------------------------------------------------------------------------
    reg  [31:0] current_seed;
    reg  [31:0] timer;

    wire [31:0] seed_s1 = current_seed ^ (current_seed << 13);
    wire [31:0] seed_s2 = seed_s1      ^ (seed_s1      >> 17);
    wire [31:0] seed_s3 = seed_s2      ^ (seed_s2      << 5);

    // Fast zero check (OR-reduction) + guard seed=0
    wire        seed_is_zero = ~(|seed_s3);
    wire [31:0] next_seed    = seed_is_zero ? 32'hFFFF_FFFF : seed_s3;

    // -------------------------------------------------------------------------
    // EXECUTION STATE
    // -------------------------------------------------------------------------
    reg  processing_active;
    wire execution_done = (processing_active && !data_valid);

    // -------------------------------------------------------------------------
    // SYNCHRONOUS LOGIC
    // -------------------------------------------------------------------------
    always @(posedge clk) begin
        if (!rst_n) begin
            current_seed      <= polymorph_seed;
            timer             <= 32'd0;
            processing_active <= 1'b0;
            data_out          <= 32'd0;
            data_ready        <= 1'b0;
        end else begin
            // -----------------------------
            // 1) DATA PROCESSING
            // -----------------------------
            // Treat data_valid as a "do work now" strobe.
            // data_ready pulses high exactly when we accept/process data_valid.
            if (data_valid) begin
                processing_active <= 1'b1;
                data_out          <= data_in ^ current_seed;
                data_ready        <= 1'b1;
            end else begin
                processing_active <= 1'b0;
                data_ready        <= 1'b0;
            end

            // -----------------------------
            // 2) POLYMORPHISM / KEY ROTATION
            // -----------------------------
            // Rotate when:
            //   (A) timer hits threshold (and threshold > 0), OR
            //   (B) OTP enabled AND the transaction ended (execution_done)
            if ( ((scramble_threshold > 0) && (timer >= scramble_threshold)) ||
                 (otp_en && execution_done) ) begin

                current_seed <= next_seed;
                timer        <= 32'd0;

            end else if (scramble_threshold > 0) begin
                timer <= timer + 32'd1;
            end
        end
    end

endmodule
