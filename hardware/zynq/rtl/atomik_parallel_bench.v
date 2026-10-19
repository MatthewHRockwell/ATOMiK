// =============================================================================
// ATOMiK Parallel-Bank Throughput Benchmark Engine  (Wishbone MMIO)
//
// Makes the N-bank parallel XOR accumulator REAL and MEASURABLE on hardware.
// A self-contained engine that accumulates `count` deterministic deltas across
// `active_banks` parallel banks, feeding `active_banks` deltas every cycle, and
// reports the exact number of hardware cycles the accumulation loop took.
//
// HONESTY / VERIFIABILITY:
//   Each delta is a STATELESS function of its global index:
//       d(i) = xorshift3( seed ^ i )       (no DSP, pure shift/XOR)
//   The merged result is  XOR over i=0..count-1 of d(i).  This is INDEPENDENT
//   of active_banks — only the cycle count changes (ceil(count/active_banks)).
//   So software can:
//     (a) recompute the same XOR in C and confirm the hardware result, and
//     (b) sweep active_banks 1->8 and watch CYCLES drop ~1/active_banks,
//   i.e. a real, reproducible "more banks = proportionally fewer cycles"
//   throughput curve measured on silicon — not a modeled number.
//
// This wraps atomik_parallel_acc (N-port parallel mode). It is a SEPARATE MMIO
// region from the validated single-bank adapter at 0xF0020000 — it does not
// touch that path.
//
// Register map (32-bit word-addressed from base, base = 0xF0021000):
//   0x00 CTRL      W bit0=start, bit1=clear   R {30'b0, done, busy}
//   0x04 COUNT     W/R  number of deltas to accumulate (32-bit)
//   0x08 ACTIVE    W/R  active bank count, clamped to 1..N_BANKS
//   0x0C SEED_LO   W/R  LFSR/mix seed [31:0]
//   0x10 SEED_HI   W/R  LFSR/mix seed [63:32]
//   0x14 CYCLES    R    hardware cycles the last run's accumulate loop took
//   0x18 RESULT_LO R    merged_accumulator[31:0]
//   0x1C RESULT_HI R    merged_accumulator[63:32]
//   0x20 STATUS    R    {VERSION[7:0], N_BANKS[7:0], 14'b0, done, busy}
//
// ATOMiK Project — May 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_parallel_bench #(
    parameter N_BANKS     = 8,
    parameter DELTA_WIDTH = 64,
    parameter VERSION     = 2
) (
    input  wire        clk,
    input  wire        rst,        // active-high (LiteX ResetSignal)

    // Wishbone slave
    input  wire [4:0]  adr,        // word address (32 regs)
    input  wire [31:0] dat_w,
    output reg  [31:0] dat_r,
    input  wire        we,
    input  wire        cyc,
    input  wire        stb,
    output reg         ack
);
    localparam BSEL_W = (N_BANKS > 1) ? $clog2(N_BANKS) : 1;

    // Sized views of the integer params (Vivado forbids part-selecting params)
    wire [7:0] ver_b   = VERSION;
    wire [7:0] banks_b = N_BANKS;

    // ── Configuration / status registers ─────────────────────────────
    reg [31:0] reg_count;
    reg [31:0] reg_active;          // clamped on use
    reg [63:0] reg_seed;
    reg [31:0] reg_cycles;
    reg [63:0] reg_result;
    reg        busy, done;

    // ── Run-time state ────────────────────────────────────────────────
    reg [63:0] idx_base;            // global index of lane 0 this cycle
    reg [31:0] cyc_count;
    reg [1:0]  state;
    localparam S_IDLE  = 2'd0;
    localparam S_CLEAR = 2'd1;
    localparam S_RUN   = 2'd2;
    localparam S_DONE  = 2'd3;

    // active banks, clamped 1..N_BANKS
    wire [31:0] active_c = (reg_active == 0)        ? 32'd1 :
                           (reg_active >  N_BANKS)  ? N_BANKS : reg_active;

    // ── Deterministic stateless delta generator ──────────────────────
    //   d(i) = xorshift3(seed ^ i)
    function [63:0] dmix;
        input [63:0] x_in;
        reg   [63:0] x;
        begin
            x = x_in;
            x = x ^ (x << 13);
            x = x ^ (x >> 7);
            x = x ^ (x << 17);
            dmix = x;
        end
    endfunction

    // Per-lane delta + valid for this cycle (combinational)
    wire [N_BANKS*DELTA_WIDTH-1:0] lane_delta;
    wire [N_BANKS-1:0]             lane_valid;

    genvar gi;
    generate
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : lanes
            wire [63:0] gidx = idx_base + gi;           // global index for this lane
            assign lane_delta[(gi+1)*DELTA_WIDTH-1 -: DELTA_WIDTH] =
                       dmix(reg_seed ^ gidx);
            // lane is live only while running, within active bank count,
            // and below the requested delta count.
            assign lane_valid[gi] = (state == S_RUN)
                                  && (gi < active_c)
                                  && (gidx < {32'd0, reg_count});
        end
    endgenerate

    // ── N-bank parallel accumulator ──────────────────────────────────
    wire [DELTA_WIDTH-1:0] merged_acc;
    reg                    load_clear;

    atomik_parallel_acc #(
        .DELTA_WIDTH (DELTA_WIDTH),
        .N_BANKS     (N_BANKS)
    ) u_acc (
        .clk                  (clk),
        .rst_n                (~rst),
        .delta_in             ({DELTA_WIDTH{1'b0}}),
        .delta_valid          (1'b0),
        .delta_parallel_in    (lane_delta),
        .delta_parallel_valid (lane_valid),
        .parallel_mode        (1'b1),
        .initial_state_in     ({DELTA_WIDTH{1'b0}}),
        .load_initial         (load_clear),
        /* verilator lint_off PINCONNECTEMPTY */
        .current_state        (),
        .accumulator_zero     (),
        .current_bank         (),
        .bank_active          (),
        /* verilator lint_on PINCONNECTEMPTY */
        .merged_accumulator   (merged_acc)
    );

    // ── Engine FSM + Wishbone ─────────────────────────────────────────
    wire wb_access = cyc & stb & ~ack;

    always @(posedge clk) begin
        if (rst) begin
            reg_count  <= 32'd0;
            reg_active <= 32'd1;
            reg_seed   <= 64'd1;
            reg_cycles <= 32'd0;
            reg_result <= 64'd0;
            busy       <= 1'b0;
            done       <= 1'b0;
            idx_base   <= 64'd0;
            cyc_count  <= 32'd0;
            state      <= S_IDLE;
            load_clear <= 1'b0;
            ack        <= 1'b0;
            dat_r      <= 32'd0;
        end else begin
            ack        <= 1'b0;
            load_clear <= 1'b0;

            // ── accumulate FSM ───────────────────────────────────────
            case (state)
                S_IDLE: begin
                    // wait for start (set via CTRL write below)
                end

                S_CLEAR: begin
                    // load_clear was pulsed on entry; banks are zeroed now.
                    idx_base  <= 64'd0;
                    cyc_count <= 32'd0;
                    if (reg_count == 32'd0) begin
                        state <= S_DONE;        // nothing to do
                    end else begin
                        state <= S_RUN;
                    end
                end

                S_RUN: begin
                    // lanes are presented combinationally this cycle and
                    // register into the banks on this edge.
                    cyc_count <= cyc_count + 32'd1;
                    if ((idx_base + active_c) >= {32'd0, reg_count}) begin
                        // this is the final (possibly partial) cycle
                        state <= S_DONE;
                    end else begin
                        idx_base <= idx_base + {32'd0, active_c};
                    end
                end

                S_DONE: begin
                    // banks have absorbed the final deltas; capture results.
                    reg_result <= merged_acc;
                    reg_cycles <= cyc_count;
                    busy       <= 1'b0;
                    done       <= 1'b1;
                    state      <= S_IDLE;
                end

                default: state <= S_IDLE;
            endcase

            // ── Wishbone register access ─────────────────────────────
            if (wb_access) begin
                ack <= 1'b1;
                if (we) begin
                    case (adr)
                        5'h00: begin                    // CTRL
                            if (dat_w[0] && state == S_IDLE) begin
                                busy       <= 1'b1;
                                done       <= 1'b0;
                                load_clear <= 1'b1;     // zero the banks
                                state      <= S_CLEAR;
                            end
                            if (dat_w[1]) begin          // explicit clear
                                load_clear <= 1'b1;
                                done       <= 1'b0;
                            end
                        end
                        5'h01: reg_count       <= dat_w;
                        5'h02: reg_active      <= dat_w;
                        5'h03: reg_seed[31:0]  <= dat_w;
                        5'h04: reg_seed[63:32] <= dat_w;
                        default: ;
                    endcase
                end
                // read mux
                case (adr)
                    5'h00: dat_r <= {30'd0, done, busy};
                    5'h01: dat_r <= reg_count;
                    5'h02: dat_r <= active_c;
                    5'h03: dat_r <= reg_seed[31:0];
                    5'h04: dat_r <= reg_seed[63:32];
                    5'h05: dat_r <= reg_cycles;
                    5'h06: dat_r <= reg_result[31:0];
                    5'h07: dat_r <= reg_result[63:32];
                    // STATUS: {VERSION[7:0], N_BANKS[7:0], 14'b0, done, busy}
                    5'h08: dat_r <= {ver_b, banks_b, 14'd0, done, busy};
                    default: dat_r <= 32'd0;
                endcase
            end
        end
    end

endmodule
