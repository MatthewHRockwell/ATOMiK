// =============================================================================
// ATOMiK Core Logic (v3.5 - Fmax Push)
// Seed update pipelined + registered scramble_fire to reduce timer fanout.
// =============================================================================

module atomik_core (
    input  wire        clk,
    input  wire        rst_n,

    input  wire [31:0] scramble_threshold,
    input  wire [31:0] polymorph_seed,
    input  wire        otp_en,

    input  wire [31:0] data_in,
    input  wire        data_valid,
    output reg  [31:0] data_out,
    output reg         data_ready
);

    // Core state
    reg [31:0] current_seed;
    reg [31:0] timer;
    reg        processing_active;

    // Seed pipeline
    reg [31:0] next_seed_reg;
    reg        req_latched;     // stage-A request latch
    reg        commit_valid;    // stage-B commit pulse

    // Registered periodic trigger
    reg        scramble_fire_r;

    // Xorshift candidate (combinational)
    wire [31:0] seed_s1 = current_seed ^ (current_seed << 13);
    wire [31:0] seed_s2 = seed_s1       ^ (seed_s1       >> 17);
    wire [31:0] seed_s3 = seed_s2       ^ (seed_s2       << 5);

    wire        seed_is_zero   = ~(|seed_s3);
    wire [31:0] seed_candidate = seed_is_zero ? 32'hFFFF_FFFF : seed_s3;

    wire execution_done = (processing_active && !data_valid);

    always @(posedge clk) begin
        if (!rst_n) begin
            current_seed      <= polymorph_seed;
            timer             <= 32'd0;
            processing_active <= 1'b0;

            data_out          <= 32'd0;
            data_ready        <= 1'b0;

            scramble_fire_r   <= 1'b0;
            next_seed_reg     <= 32'd0;
            req_latched       <= 1'b0;
            commit_valid      <= 1'b0;

        end else begin
            // -------------------------------------------------------------
            // 1) DATA PATH
            // -------------------------------------------------------------
            if (data_valid) begin
                processing_active <= 1'b1;
                data_out          <= data_in ^ current_seed;
                data_ready        <= 1'b1;
            end else begin
                processing_active <= 1'b0;
                data_ready        <= 1'b0;
            end

            // -------------------------------------------------------------
            // 2) TIMER
            //   - increments only when threshold enabled
            //   - resets on actual seed commit
            // -------------------------------------------------------------
            if (commit_valid) begin
                timer <= 32'd0;
            end else if (scramble_threshold > 0) begin
                timer <= timer + 32'd1;
            end else begin
                timer <= 32'd0;
            end

            // -------------------------------------------------------------
            // 3) REGISTERED SCRAMBLE FIRE (1-cycle late, timing-friendly)
            // -------------------------------------------------------------
            scramble_fire_r <= (scramble_threshold > 0) && (timer >= scramble_threshold);

            // -------------------------------------------------------------
            // 4) SEED UPDATE PIPELINE CONTROL
            //   Stage A: latch request
            //   Stage B: commit pulse derived from previous request
            // -------------------------------------------------------------
            req_latched  <= (scramble_fire_r || (otp_en && execution_done));
            commit_valid <= req_latched;

            // -------------------------------------------------------------
            // 5) SEED PIPELINE
            //   Stage A: capture candidate
            //   Stage B: commit into current_seed
            // -------------------------------------------------------------
            if (req_latched) begin
                next_seed_reg <= seed_candidate;
            end

            if (commit_valid) begin
                current_seed <= next_seed_reg;
            end
        end
    end

endmodule
