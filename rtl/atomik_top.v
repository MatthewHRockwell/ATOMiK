// =============================================================================
// ATOMiK Top-Level (Tang Nano 9K)
// Clean, Gowin-synthesis friendly, PLL-ready, core-gated, core-retained
// =============================================================================

module atomik_top #(
    parameter integer SYS_CLK_HZ = 94_500_000, // set to 27_000_000 when USE_PLL=0
    parameter integer USE_PLL    = 1           // 0 = use sys_clk directly, 1 = use PLL output
)(
    input  wire       sys_clk,     // 27MHz onboard oscillator
    input  wire       sys_rst_n,    // active-low reset
    input  wire       uart_rx,
    output wire       uart_tx,
    output wire [5:0] led
);

    // -------------------------------------------------------------------------
    // CLOCKING (PLL optional)
    // -------------------------------------------------------------------------
    wire clk_int;
    wire pll_lock;

    generate
        if (USE_PLL != 0) begin : gen_pll
            // Gowin IP: atomik_pll_81m
            // Ports: (clkout, lock, reset, clkin)
            atomik_pll_94p5m u_pll (
                .clkin  (sys_clk),
                .reset  (~sys_rst_n), // PLL reset is active-HIGH
                .clkout (clk_int),
                .lock   (pll_lock)
            );
        end else begin : gen_nopll
            assign clk_int  = sys_clk;
            assign pll_lock = 1'b1;
        end
    endgenerate

    // -------------------------------------------------------------------------
    // POWER-ON RESET (POR) - runs off sys_clk so POR timing is stable
    // -------------------------------------------------------------------------
    reg [15:0] por_cnt = 16'd0;

    always @(posedge sys_clk or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            por_cnt <= 16'd0;
        end else if (por_cnt != 16'hFFFF) begin
            por_cnt <= por_cnt + 16'd1;
        end
    end

    wire por_done = (por_cnt == 16'hFFFF);

    // Combine global release conditions (in sys_clk domain conceptually)
    wire release_ok = sys_rst_n && por_done && pll_lock;

    // -------------------------------------------------------------------------
    // RESET SYNC INTO clk_int DOMAIN
    // Avoids async deassert into clk_int which can create odd bring-up behavior.
    // -------------------------------------------------------------------------
    reg [1:0] rst_sync;

    always @(posedge clk_int or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            rst_sync <= 2'b00;
        end else if (!por_done) begin
            rst_sync <= 2'b00;
        end else if (!pll_lock) begin
            rst_sync <= 2'b00;
        end else begin
            rst_sync <= {rst_sync[0], 1'b1};
        end
    end

    wire internal_rst_n = rst_sync[1];

    // -------------------------------------------------------------------------
    // HEARTBEAT (runs on clk_int so it reflects real system speed)
    // -------------------------------------------------------------------------
    reg [23:0] heartbeat_cnt;

    always @(posedge clk_int or negedge internal_rst_n) begin
        if (!internal_rst_n) begin
            heartbeat_cnt <= 24'd0;
        end else begin
            heartbeat_cnt <= heartbeat_cnt + 24'd1;
        end
    end

    // -------------------------------------------------------------------------
    // BIOS / GENOME LOADER (UART RX)
    // -------------------------------------------------------------------------
    wire [31:0]  w_poly_freq;
    wire [31:0]  w_poly_seed;
    wire [255:0] w_dna_storage;
    wire         w_core_enable;
    wire         w_loader_busy;
    wire         w_otp_en;

    uart_genome_loader #(
        .CLK_FREQ (SYS_CLK_HZ),
        .BAUD_RATE(115200)
    ) bios_inst (
        .clk           (clk_int),
        .rst_n         (internal_rst_n),
        .uart_rx       (uart_rx),

        .poly_freq_out (w_poly_freq),
        .poly_seed_out (w_poly_seed),
        .dna_storage   (w_dna_storage),
        .core_enable   (w_core_enable),
        .loader_busy   (w_loader_busy),
        .otp_en        (w_otp_en)
    );

    // -------------------------------------------------------------------------
    // CORE GATING
    // Core held in reset until BIOS asserts core_enable.
    // Force-enable is for timing/bring-up only.
    // -------------------------------------------------------------------------
    localparam integer CORE_DEBUG_FORCE_EN = 0; // set to 0 once genome->core path is live

    wire core_en_effective = w_core_enable | (CORE_DEBUG_FORCE_EN != 0);
    wire core_rst_n        = internal_rst_n & core_en_effective;

    // -------------------------------------------------------------------------
    // CORE STIMULUS (keeps core "real" during PLL/Fmax work)
    // -------------------------------------------------------------------------
    wire [31:0] core_data_in    = {8'd0, heartbeat_cnt}; // changing word
    wire        core_data_valid = heartbeat_cnt[15];     // periodic pulse

    // -------------------------------------------------------------------------
    // ATOMiK CORE (v3.3 interface)
    // -------------------------------------------------------------------------
    wire [31:0] w_data_out;
    wire        w_data_ready;

    atomik_core core_inst (
        .clk               (clk_int),
        .rst_n             (core_rst_n),

        .scramble_threshold(w_poly_freq),
        .polymorph_seed    (w_poly_seed),
        .otp_en            (w_otp_en),

        .data_in           (core_data_in),
        .data_valid        (core_data_valid),

        .data_out          (w_data_out),
        .data_ready        (w_data_ready)
    );

    // -------------------------------------------------------------------------
    // CORE DEBUG TAP -> LED (prevents sweeping)
    // -------------------------------------------------------------------------
    wire core_debug = w_data_ready ^ w_data_out[0];

    // -------------------------------------------------------------------------
    // UART TX (bring-up: loopback)
    // Make it a wire to avoid reg init issues; this is sufficient for link test.
    // -------------------------------------------------------------------------
    assign uart_tx = uart_rx;

    // -------------------------------------------------------------------------
    // LED MAPPING (Tang Nano LEDs are typically active-low)
    // -------------------------------------------------------------------------
    // led[5] = PLL lock indicator (active-low; ON when locked)
    // led[4] = heartbeat (active-low)
    // led[3] = loader busy (active-low; ON when busy)
    // led[2] = core enabled (active-low; ON when enabled)
    // led[1] = OTP enabled (active-low; ON when otp_en=1)
    // led[0] = core debug tap (active-low; ON when core_debug=1)
    assign led[5] = ~pll_lock;
    assign led[4] = ~heartbeat_cnt[23];
    assign led[3] = ~w_loader_busy;
    assign led[2] = ~w_core_enable;
    assign led[1] = ~w_otp_en;
    assign led[0] = ~core_debug;

endmodule
