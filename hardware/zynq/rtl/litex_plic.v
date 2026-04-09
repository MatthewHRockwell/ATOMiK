// Minimal RISC-V PLIC (Platform Level Interrupt Controller) for single-core
//
// Supports up to 32 interrupt sources, 1 context (hart 0 M-mode + S-mode).
// Wishbone slave interface, compatible with LiteX bus.
//
// Register map (byte offsets, SiFive PLIC layout):
//   0x000000-0x00007C  priority[0..31]    (source 0 reserved)
//   0x001000           pending[0]         (bits [31:0])
//   0x002000           enable_m[0]        (M-mode, context 0)
//   0x002080           enable_s[0]        (S-mode, context 1)
//   0x200000           threshold_m        (context 0)
//   0x200004           claim_m            (context 0)
//   0x201000           threshold_s        (context 1)
//   0x201004           claim_s            (context 1)
//
// 4MB address space (22-bit word address).

module litex_plic #(
    parameter NUM_SOURCES = 32
) (
    input  wire        clk,
    input  wire        rst,

    // Wishbone slave
    input  wire [21:0] adr,
    input  wire [31:0] dat_w,
    output reg  [31:0] dat_r,
    input  wire        we,
    input  wire        cyc,
    input  wire        stb,
    output reg         ack,

    // Interrupt sources (active high)
    input  wire [NUM_SOURCES-1:0] irq_sources,

    // Interrupt outputs (active high)
    output wire        meip,
    output wire        seip
);

    reg [2:0] priority [0:NUM_SOURCES-1];
    reg [NUM_SOURCES-1:0] pending;
    reg [NUM_SOURCES-1:0] enable_m;
    reg [NUM_SOURCES-1:0] enable_s;
    reg [2:0] threshold_m;
    reg [2:0] threshold_s;
    reg [NUM_SOURCES-1:0] claimed_m;
    reg [NUM_SOURCES-1:0] claimed_s;

    wire [NUM_SOURCES-1:0] active_m = pending & enable_m & ~claimed_m;
    wire [NUM_SOURCES-1:0] active_s = pending & enable_s & ~claimed_s;

    // Interrupt pending logic
    reg meip_r, seip_r;
    integer k;
    always @(*) begin
        meip_r = 1'b0;
        seip_r = 1'b0;
        for (k = 1; k < NUM_SOURCES; k = k + 1) begin
            if (active_m[k] && priority[k] > threshold_m) meip_r = 1'b1;
            if (active_s[k] && priority[k] > threshold_s) seip_r = 1'b1;
        end
    end
    assign meip = meip_r;
    assign seip = seip_r;

    // Find best candidate for claim
    function [4:0] find_claim;
        input [NUM_SOURCES-1:0] active;
        input [2:0] thresh;
        integer j;
        reg [2:0] best_pri;
        begin
            find_claim = 5'd0;
            best_pri   = 3'd0;
            for (j = 1; j < NUM_SOURCES; j = j + 1) begin
                if (active[j] && priority[j] > thresh && priority[j] > best_pri) begin
                    find_claim = j[4:0];
                    best_pri   = priority[j];
                end
            end
        end
    endfunction

    wire wb_access = cyc & stb & ~ack;

    // Single always block — no multi-driver on pending
    always @(posedge clk) begin
        if (rst) begin
            pending     <= {NUM_SOURCES{1'b0}};
            ack         <= 1'b0;
            dat_r       <= 32'd0;
            enable_m    <= {NUM_SOURCES{1'b0}};
            enable_s    <= {NUM_SOURCES{1'b0}};
            threshold_m <= 3'd0;
            threshold_s <= 3'd0;
            claimed_m   <= {NUM_SOURCES{1'b0}};
            claimed_s   <= {NUM_SOURCES{1'b0}};
        end else begin
            // Level-sensitive: latch pending from irq_sources
            pending <= pending | irq_sources;

            ack <= 1'b0;

            if (wb_access) begin
                ack   <= 1'b1;
                dat_r <= 32'd0;

                casez (adr)
                    // Priority registers
                    22'b0000_0000_0000_0000_0????? : begin
                        if (adr[4:0] < NUM_SOURCES) begin
                            dat_r <= {29'd0, priority[adr[4:0]]};
                            if (we) priority[adr[4:0]] <= dat_w[2:0];
                        end
                    end

                    // Pending (read-only)
                    22'h00400: dat_r <= pending;

                    // Enable M-mode
                    22'h00800: begin
                        dat_r <= enable_m;
                        if (we) enable_m <= dat_w[NUM_SOURCES-1:0];
                    end

                    // Enable S-mode
                    22'h00820: begin
                        dat_r <= enable_s;
                        if (we) enable_s <= dat_w[NUM_SOURCES-1:0];
                    end

                    // Threshold M-mode
                    22'h80000: begin
                        dat_r <= {29'd0, threshold_m};
                        if (we) threshold_m <= dat_w[2:0];
                    end

                    // Claim/Complete M-mode
                    22'h80001: begin
                        if (we) begin
                            if (dat_w[4:0] < NUM_SOURCES)
                                claimed_m[dat_w[4:0]] <= 1'b0;
                        end else begin
                            dat_r <= {27'd0, find_claim(active_m, threshold_m)};
                            claimed_m[find_claim(active_m, threshold_m)] <= 1'b1;
                            pending[find_claim(active_m, threshold_m)] <= 1'b0;
                        end
                    end

                    // Threshold S-mode
                    22'h80400: begin
                        dat_r <= {29'd0, threshold_s};
                        if (we) threshold_s <= dat_w[2:0];
                    end

                    // Claim/Complete S-mode
                    22'h80401: begin
                        if (we) begin
                            if (dat_w[4:0] < NUM_SOURCES)
                                claimed_s[dat_w[4:0]] <= 1'b0;
                        end else begin
                            dat_r <= {27'd0, find_claim(active_s, threshold_s)};
                            claimed_s[find_claim(active_s, threshold_s)] <= 1'b1;
                            pending[find_claim(active_s, threshold_s)] <= 1'b0;
                        end
                    end

                    default: dat_r <= 32'd0;
                endcase
            end
        end
    end

    // Initialize priority array
    integer i;
    initial begin
        for (i = 0; i < NUM_SOURCES; i = i + 1)
            priority[i] = 3'd0;
    end

endmodule
