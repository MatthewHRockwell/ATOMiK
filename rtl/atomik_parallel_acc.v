// =============================================================================
// ATOMiK Parallel Accumulator Banks
//
// Module:      atomik_parallel_acc
// Description: N-bank parallel XOR accumulator with dual input modes for
//              linear throughput scaling. Demonstrates that N parallel XOR
//              accumulator banks achieve N× throughput with constant latency.
//
// Key Features:
//   - N parallel atomik_delta_acc banks (configurable via N_BANKS)
//   - Dual input modes: round-robin (single-stream) and N-port parallel
//   - Combinational XOR-reduce merge tree (log₂(N) depth, no carry)
//   - Constant 1-cycle latency regardless of bank count
//   - Linear throughput scaling: N banks = N× ops/sec
//
// Mathematical Foundation (from Phase 1 Lean4 proofs):
//   - delta_comm:  δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁ (bank distribution is transparent)
//   - delta_assoc: (δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃) (merge order irrelevant)
//   - XOR merge tree is semantically equivalent to sequential accumulation
//
// Architecture:
//   delta_in ──► Round-Robin Distributor ──► Bank[0..N-1] ──► XOR Merge Tree
//   delta_parallel_in[N] ──► Direct N-port ──► Bank[0..N-1] ──► XOR Merge Tree
//
// Performance (estimated @ 94.5 MHz, GW1NR-9):
//   N=1: 94.5 Mops/s (1.0×)    N=2: 189.0 Mops/s (2.0×)
//   N=4: 378.0 Mops/s (4.0×)   N=8: 756.0 Mops/s (8.0×)
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   January 27, 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_parallel_acc #(
    parameter DELTA_WIDTH = 64,     // Width of delta and state
    parameter N_BANKS     = 4,      // Number of parallel accumulator banks
    parameter BANK_SEL_W  = (N_BANKS > 1) ? $clog2(N_BANKS) : 1  // Bank selector width (min 1)
)(
    // Clock and Reset
    input  wire                              clk,
    input  wire                              rst_n,

    // Single-Stream Delta Input (round-robin distribution)
    input  wire [DELTA_WIDTH-1:0]            delta_in,
    input  wire                              delta_valid,

    // N-Port Parallel Delta Input (direct to banks)
    input  wire [N_BANKS*DELTA_WIDTH-1:0]    delta_parallel_in,
    input  wire [N_BANKS-1:0]                delta_parallel_valid,

    // Mode Select
    //   0 = round-robin: single delta_in distributed across banks
    //   1 = parallel:    N independent delta ports feed banks directly
    input  wire                              parallel_mode,

    // Initial State Interface
    input  wire [DELTA_WIDTH-1:0]            initial_state_in,
    input  wire                              load_initial,

    // Output Interface
    output wire [DELTA_WIDTH-1:0]            current_state,
    output wire [DELTA_WIDTH-1:0]            merged_accumulator,
    output wire                              accumulator_zero,

    // Bank Status
    output wire [BANK_SEL_W-1:0]             current_bank,
    output wire [N_BANKS-1:0]                bank_active
);

    // =========================================================================
    // Internal State
    // =========================================================================

    // Initial state register (shared across all banks)
    reg [DELTA_WIDTH-1:0] initial_state;

    // Round-robin bank selector
    reg [BANK_SEL_W-1:0] bank_sel;

    // =========================================================================
    // Initial State Register
    // =========================================================================

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            initial_state <= {DELTA_WIDTH{1'b0}};
        end
        else if (load_initial) begin
            initial_state <= initial_state_in;
        end
    end

    // =========================================================================
    // Round-Robin Bank Selector
    // =========================================================================

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            bank_sel <= {BANK_SEL_W{1'b0}};
        end
        else if (load_initial) begin
            bank_sel <= {BANK_SEL_W{1'b0}};
        end
        else if (!parallel_mode && delta_valid) begin
            if (bank_sel == N_BANKS - 1)
                bank_sel <= {BANK_SEL_W{1'b0}};
            else
                bank_sel <= bank_sel + 1'b1;
        end
    end

    // =========================================================================
    // Per-Bank Delta Valid Signals
    // =========================================================================

    wire [N_BANKS-1:0] bank_delta_valid;
    wire [DELTA_WIDTH-1:0] bank_delta_in [0:N_BANKS-1];

    genvar gi;
    generate
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : bank_input_mux
            // Each bank's valid: either round-robin select or parallel port
            assign bank_delta_valid[gi] = parallel_mode
                ? delta_parallel_valid[gi]
                : (delta_valid && (bank_sel == gi));

            // Each bank's delta: either shared delta_in or parallel port slice
            assign bank_delta_in[gi] = parallel_mode
                ? delta_parallel_in[(gi+1)*DELTA_WIDTH-1 -: DELTA_WIDTH]
                : delta_in;
        end
    endgenerate

    // =========================================================================
    // N Parallel Accumulator Banks
    // =========================================================================

    wire [DELTA_WIDTH-1:0] bank_acc [0:N_BANKS-1];

    generate
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : banks
            atomik_delta_acc #(
                .DELTA_WIDTH(DELTA_WIDTH)
            ) u_bank (
                .clk                  (clk),
                .rst_n                (rst_n),
                .delta_in             (bank_delta_in[gi]),
                .delta_valid          (bank_delta_valid[gi]),
                .initial_state_in     ({DELTA_WIDTH{1'b0}}),
                .load_initial         (load_initial),
                .initial_state_out    (),  // Not used; shared initial_state
                .delta_accumulator_out(bank_acc[gi]),
                .accumulator_zero     ()   // Checked on merged result
            );
        end
    endgenerate

    // =========================================================================
    // XOR-Reduce Merge Tree (Combinational, Binary Tree)
    // =========================================================================
    //
    // Merges all bank accumulators via XOR using an explicit binary tree:
    //   merged = bank_acc[0] ^ bank_acc[1] ^ ... ^ bank_acc[N-1]
    //
    // Tree structure (2*N_BANKS-1 nodes):
    //   Indices [0..N-1]:     leaf nodes (bank accumulator outputs)
    //   Indices [N..2N-2]:    internal nodes (pair-wise XOR)
    //   Root at [2N-2]:       final merged result
    //
    // Depth: log₂(N) XOR levels, NO carry propagation, NO ALU inference.
    // This avoids the serial chain that results from a for-loop.

    wire [DELTA_WIDTH-1:0] merge_node [0:2*N_BANKS-2];

    generate
        // Leaf nodes: directly connected to bank accumulator outputs
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : merge_leaf
            assign merge_node[gi] = bank_acc[gi];
        end

        // Internal nodes: pair-wise XOR building a balanced binary tree
        //   node[N+k] = node[2k] ^ node[2k+1]
        // This gives exactly log₂(N) combinational levels.
        for (gi = 0; gi < N_BANKS - 1; gi = gi + 1) begin : merge_inner
            assign merge_node[N_BANKS + gi] = merge_node[2*gi] ^ merge_node[2*gi + 1];
        end
    endgenerate

    wire [DELTA_WIDTH-1:0] merged_acc_comb = merge_node[2*N_BANKS - 2];

    // =========================================================================
    // Output Assignments
    // =========================================================================

    // State reconstruction: initial_state XOR merged accumulator
    assign current_state      = initial_state ^ merged_acc_comb;

    // Merged accumulator output
    assign merged_accumulator = merged_acc_comb;

    // Zero detection on merged result
    assign accumulator_zero   = ~(|merged_acc_comb);

    // Bank status outputs
    assign current_bank = bank_sel;
    assign bank_active  = bank_delta_valid;

endmodule

// =============================================================================
// Module Documentation
// =============================================================================
//
// USAGE EXAMPLE:
// --------------
// atomik_parallel_acc #(
//     .DELTA_WIDTH(64),
//     .N_BANKS(4)
// ) u_parallel_acc (
//     .clk                  (clk),
//     .rst_n                (rst_n),
//     .delta_in             (delta_value),
//     .delta_valid          (delta_strobe),
//     .delta_parallel_in    ({delta_3, delta_2, delta_1, delta_0}),
//     .delta_parallel_valid (4'b1111),
//     .parallel_mode        (1'b0),        // 0=round-robin, 1=N-port
//     .initial_state_in     (new_initial),
//     .load_initial         (load_strobe),
//     .current_state        (current_state),
//     .merged_accumulator   (merged_acc),
//     .accumulator_zero     (acc_zero),
//     .current_bank         (bank_idx),
//     .bank_active          (active_mask)
// );
//
// THROUGHPUT SCALING:
// -------------------
//   N_BANKS=1:  1 delta/cycle  →  94.5 Mops/s @ 94.5 MHz  (1.0×)
//   N_BANKS=2:  2 deltas/cycle → 189.0 Mops/s @ 94.5 MHz  (2.0×)
//   N_BANKS=4:  4 deltas/cycle → 378.0 Mops/s @ 94.5 MHz  (4.0×)
//   N_BANKS=8:  8 deltas/cycle → 756.0 Mops/s @ 94.5 MHz  (8.0×)
//
// RESOURCE ESTIMATES (GW1NR-9):
// -----------------------------
//   N=1:  ~161 LUTs (1.9%)   N=2:  ~307 LUTs (3.6%)
//   N=4:  ~599 LUTs (6.9%)   N=8: ~1183 LUTs (13.7%)
//
// KEY PROOF: XOR merge tree has log₂(N) gate depth with NO carry propagation.
// Adder merge tree has O(W·log₂(N)) delay due to carry chains.
// This means Fmax is independent of N_BANKS for XOR — linear scaling is free.
//
// =============================================================================
