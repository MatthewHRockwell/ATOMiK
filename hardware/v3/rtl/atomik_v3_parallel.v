// =============================================================================
// ATOMiK v3 Parallel Accumulator Banks
//
// Module:      atomik_v3_parallel
// Description: N-bank parallel XOR accumulator with BSRAM state table for
//              v3 custom instruction integration. Drops in as a replacement
//              for atomik_v3_atomik.v with N_BANKS > 1.
//
// Architecture:
//   - N parallel accumulators fed via round-robin or parallel ports
//   - Combinational XOR merge tree (log2(N) depth, no carry propagation)
//   - Shared BSRAM state table (256x64-bit)
//   - State reconstruction: current_state = state_table[addr] ^ merged_acc
//
// Mathematical Foundation (108 Lean4 theorems):
//   delta_comm:  d1 ^ d2 = d2 ^ d1         (bank distribution is transparent)
//   delta_assoc: (d1 ^ d2) ^ d3 = d1 ^ (d2 ^ d3)  (merge order irrelevant)
//
// Throughput: N_BANKS x ops/cycle (linear scaling, constant latency)
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_v3_parallel #(
    parameter N_BANKS    = 1,
    parameter BANK_SEL_W = (N_BANKS > 1) ? $clog2(N_BANKS) : 1
)(
    input  wire        clk,
    input  wire        rst_n,

    // Control (active for one cycle in EXECUTE state)
    input  wire        load_en,     // ATOMIK.LOAD: clear all acc, write state table, set addr
    input  wire        accum_en,    // ATOMIK.ACCUM: acc[bank_sel] ^= delta_in
    input  wire        swap_en,     // ATOMIK.SWAP: update ref, clear all acc

    // Data inputs
    input  wire [7:0]  addr_in,     // Address for LOAD/SWAP (rs1[7:0])
    input  wire [63:0] delta_in,    // Delta for ACCUM (rs1_data)
    input  wire [63:0] init_data,   // Initial state data for LOAD (rs2_data)

    // Parallel input mode (optional — active when parallel_mode=1)
    input  wire [N_BANKS*64-1:0]    delta_parallel_in,
    input  wire [N_BANKS-1:0]       delta_parallel_valid,
    input  wire                     parallel_mode,

    // Outputs
    output wire [63:0] current_state,   // state_table[addr] ^ merged_acc
    output wire        acc_zero,        // Merged accumulator is all zeros

    // Bank status
    output wire [BANK_SEL_W-1:0]    current_bank,
    output wire [N_BANKS-1:0]       bank_active
);

    // =========================================================================
    // Round-Robin Bank Selector
    // =========================================================================
    reg [BANK_SEL_W-1:0] bank_sel;

    always @(posedge clk) begin
        if (!rst_n)
            bank_sel <= {BANK_SEL_W{1'b0}};
        else if (load_en)
            bank_sel <= {BANK_SEL_W{1'b0}};
        else if (!parallel_mode && accum_en) begin
            if (bank_sel == N_BANKS[BANK_SEL_W-1:0] - 1'b1)
                bank_sel <= {BANK_SEL_W{1'b0}};
            else
                bank_sel <= bank_sel + 1'b1;
        end
    end

    // =========================================================================
    // Per-Bank Delta Valid and Data Signals
    // =========================================================================
    wire [N_BANKS-1:0] bank_accum_en;
    wire [63:0] bank_delta [0:N_BANKS-1];

    genvar gi;
    generate
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : bank_input_mux
            assign bank_accum_en[gi] = parallel_mode
                ? delta_parallel_valid[gi]
                : (accum_en && (bank_sel == gi[BANK_SEL_W-1:0]));

            assign bank_delta[gi] = parallel_mode
                ? delta_parallel_in[(gi+1)*64-1 -: 64]
                : delta_in;
        end
    endgenerate

    // =========================================================================
    // N Parallel Accumulator Banks (XOR only — no initial_state)
    // =========================================================================
    (* syn_preserve = 1 *) reg [63:0] bank_acc [0:N_BANKS-1];

    // Delayed clear for SWAP (accumulator must hold through WRITEBACK)
    reg swap_pending;

    always @(posedge clk) begin
        if (!rst_n)
            swap_pending <= 1'b0;
        else
            swap_pending <= swap_en;
    end

    generate
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : banks
            always @(posedge clk) begin
                if (!rst_n)
                    bank_acc[gi] <= 64'b0;
                else if (load_en)
                    bank_acc[gi] <= 64'b0;          // Clear all on LOAD
                else if (swap_pending)
                    bank_acc[gi] <= 64'b0;          // Clear all after SWAP writeback
                else if (bank_accum_en[gi])
                    bank_acc[gi] <= bank_acc[gi] ^ bank_delta[gi];
            end
        end
    endgenerate

    // =========================================================================
    // XOR-Reduce Merge Tree (Combinational, Binary Tree)
    // =========================================================================
    //
    // Merges all bank accumulators via XOR in a balanced binary tree:
    //   Indices [0..N-1]:     leaf nodes (bank outputs)
    //   Indices [N..2N-2]:    internal nodes (pair-wise XOR)
    //   Root at [2N-2]:       final merged result
    //
    // Depth: log2(N) XOR levels, NO carry propagation, NO ALU inference.

    wire [63:0] merge_node [0:2*N_BANKS-2];

    generate
        // Leaf nodes: bank accumulator outputs
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : merge_leaf
            assign merge_node[gi] = bank_acc[gi];
        end

        // Internal nodes: pair-wise XOR
        for (gi = 0; gi < N_BANKS - 1; gi = gi + 1) begin : merge_inner
            assign merge_node[N_BANKS + gi] = merge_node[2*gi] ^ merge_node[2*gi + 1];
        end
    endgenerate

    // Merged accumulator — syn_keep prevents ALU carry chain inference
    wire [63:0] merged_acc /* synthesis syn_keep=1 */;
    assign merged_acc = merge_node[2*N_BANKS - 2];

    // =========================================================================
    // Active Address Register
    // =========================================================================
    reg [7:0] active_addr;

    always @(posedge clk) begin
        if (!rst_n)
            active_addr <= 8'b0;
        else if (load_en || swap_en)
            active_addr <= addr_in;
    end

    // =========================================================================
    // BSRAM State Table (256 x 64-bit via dual 256x32 SDP)
    // =========================================================================
    (* syn_ramstyle = "block_ram" *) reg [63:0] state_table [0:255];
    reg [63:0] state_read_reg;

    // Write port: LOAD writes initial_state, SWAP updates reference
    always @(posedge clk) begin
        if (load_en)
            state_table[addr_in] <= init_data;
        else if (swap_en)
            state_table[active_addr] <= state_read_reg ^ merged_acc;
    end

    // Read port: registered output
    always @(posedge clk) begin
        state_read_reg <= state_table[active_addr];
    end

    // =========================================================================
    // State Reconstructor: current_state = initial_state XOR merged_acc
    // =========================================================================
    (* syn_keep = 1 *) wire [63:0] reconstructed;
    assign reconstructed = state_read_reg ^ merged_acc;
    assign current_state = reconstructed;

    // =========================================================================
    // Output Assignments
    // =========================================================================
    assign acc_zero     = ~(|merged_acc);
    assign current_bank = bank_sel;
    assign bank_active  = bank_accum_en;

    // =========================================================================
    // Simulation-only initialization
    // =========================================================================
    // synthesis translate_off
    integer i;
    initial begin
        bank_sel = {BANK_SEL_W{1'b0}};
        active_addr = 8'b0;
        state_read_reg = 64'b0;
        swap_pending = 1'b0;
        for (i = 0; i < N_BANKS; i = i + 1)
            bank_acc[i] = 64'b0;
        for (i = 0; i < 256; i = i + 1)
            state_table[i] = 64'b0;
    end
    // synthesis translate_on

endmodule
