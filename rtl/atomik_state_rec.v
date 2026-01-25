// =============================================================================
// ATOMiK State Reconstructor Module
// 
// Module:      atomik_state_rec
// Description: Combinational state reconstruction from initial state and 
//              accumulated delta via XOR
// 
// Key Features:
//   - Pure combinational logic (0-cycle latency)
//   - 64-bit parallel XOR (no carry propagation)
//   - Always-available output (no enable signal needed)
//   - Configurable state width
//
// Mathematical Foundation:
//   current_state = initial_state ⊕ delta_accumulator
//
//   This implements the fundamental ATOMiK reconstruction equation.
//   Because XOR is its own inverse:
//     S_current = S_initial ⊕ (δ₁ ⊕ δ₂ ⊕ ... ⊕ δₙ)
//
// Performance:
//   - Latency: 0 cycles (combinational)
//   - Delay: ~1.5ns on GW1NR-9 (single LUT level)
//   - This eliminates the O(N) reconstruction penalty from Phase 2
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   January 25, 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_state_rec #(
    parameter STATE_WIDTH = 64      // Width of state (matches Phase 1 proofs)
)(
    // Input Interface
    input  wire [STATE_WIDTH-1:0]   initial_state,      // Base state S₀
    input  wire [STATE_WIDTH-1:0]   delta_accumulator,  // Accumulated deltas
    
    // Output Interface
    output wire [STATE_WIDTH-1:0]   current_state       // Reconstructed state
);

    // =========================================================================
    // State Reconstruction
    // =========================================================================
    //
    // The core ATOMiK equation: current_state = initial_state ⊕ delta_accumulator
    //
    // This is a pure combinational operation:
    //   - 64 independent XOR gates (one per bit)
    //   - No carry chain (unlike addition)
    //   - Single LUT delay (~1.5ns on GW1NR-9)
    //
    // This O(1) operation replaces the O(N) reconstruction loop from Phase 2:
    //   Python: for delta in history: result ^= delta  # O(N)
    //   Verilog: current_state = initial ^ accumulator  # O(1)
    //
    assign current_state = initial_state ^ delta_accumulator;

endmodule

// =============================================================================
// Module Documentation
// =============================================================================
//
// USAGE EXAMPLE:
// --------------
// atomik_state_rec #(
//     .STATE_WIDTH(64)
// ) u_state_rec (
//     .initial_state     (reg_initial_state),
//     .delta_accumulator (reg_delta_accumulator),
//     .current_state     (wire_current_state)
// );
//
// TIMING CHARACTERISTICS:
// -----------------------
// This module is purely combinational. The output changes immediately
// when either input changes (after propagation delay).
//
//   initial_state:      ══╤═══════════════════════════════════
//                         │  0x1234567890ABCDEF
//                         └───────────────────────────────────
//
//   delta_accumulator:  ══════════╤═══════════════════════════
//                                 │  0x00000000FFFFFFFF
//                                 └───────────────────────────
//
//   current_state:      ══╤═══════╤═══════════════════════════
//                         │ init  │  0x12345678655432EF
//                         └───────┴───────────────────────────
//                                 ↑
//                          ~1.5ns propagation delay
//
// RESOURCE ESTIMATES (GW1NR-9):
// -----------------------------
//   Flip-Flops: 0 (combinational only)
//   LUTs:       64 (one LUT2 per bit for XOR)
//   Fmax:       N/A (combinational, limited by connected registers)
//
// INTEGRATION NOTES:
// ------------------
// In a typical design, the output should be registered before use:
//
//   always @(posedge clk) begin
//       data_out <= current_state;  // Register the combinational output
//   end
//
// This adds 1 cycle of latency but ensures clean timing closure.
//
// =============================================================================
