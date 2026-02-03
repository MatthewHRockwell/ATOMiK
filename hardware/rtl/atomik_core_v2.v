// =============================================================================
// ATOMiK Core v2 - Delta Architecture Top-Level Module
// 
// Module:      atomik_core_v2
// Description: Top-level integration of delta accumulator and state reconstructor
// 
// Key Features:
//   - Unified control interface with 2-bit operation code
//   - Single-cycle LOAD and ACCUMULATE operations
//   - Combinational state reconstruction (registered output)
//   - Status flags for debugging and verification
//   - Configurable data width (64-bit default)
//
// Architecture:
//   ┌─────────────────────────────────────────────────────────────┐
//   │                     atomik_core_v2                          │
//   │                                                             │
//   │  ┌─────────────────────┐    ┌─────────────────────────┐    │
//   │  │  atomik_delta_acc   │    │   atomik_state_rec      │    │
//   │  │                     │    │                         │    │
//   │  │  initial_state ─────┼────┼──► XOR ──► current_state│    │
//   │  │  delta_accumulator ─┼────┼──►     │                │    │
//   │  │                     │    │        │                │    │
//   │  └─────────────────────┘    └────────┼────────────────┘    │
//   │                                      │                     │
//   │                                      ▼                     │
//   │                                 [data_out]                 │
//   └─────────────────────────────────────────────────────────────┘
//
// Operation Encoding:
//   2'b00 (NOP):        No operation, hold state
//   2'b01 (LOAD):       Load initial_state ← data_in
//   2'b10 (ACCUMULATE): Accumulate delta: accumulator ← accumulator ⊕ data_in
//   2'b11 (READ):       Output current_state to data_out
//
// Performance:
//   - All operations complete in 1 clock cycle
//   - State reconstruction is combinational (0 added latency)
//   - Throughput: 1 operation per cycle @ target frequency
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   January 25, 2026
// =============================================================================

`timescale 1ns / 1ps

// Operation code definitions
`define OP_NOP        2'b00
`define OP_LOAD       2'b01
`define OP_ACCUMULATE 2'b10
`define OP_READ       2'b11

module atomik_core_v2 #(
    parameter DATA_WIDTH = 64       // Width of data/state/delta
)(
    // =========================================================================
    // Clock and Reset
    // =========================================================================
    input  wire                    clk,         // System clock
    input  wire                    rst_n,       // Active-low synchronous reset
    
    // =========================================================================
    // Control Interface
    // =========================================================================
    input  wire [1:0]              operation,   // Operation selector
    input  wire [DATA_WIDTH-1:0]   data_in,     // Input data (initial_state or delta)
    output reg  [DATA_WIDTH-1:0]   data_out,    // Output data (current_state on READ)
    output reg                     data_valid,  // Pulsed high when operation completes
    
    // =========================================================================
    // Status Interface
    // =========================================================================
    output wire                    accumulator_zero,     // High when accumulator = 0
    output wire [DATA_WIDTH-1:0]   debug_initial_state,  // Debug: current initial_state
    output wire [DATA_WIDTH-1:0]   debug_accumulator     // Debug: current accumulator
);

    // =========================================================================
    // Internal Signals
    // =========================================================================
    
    // Signals between delta_acc and control logic
    wire [DATA_WIDTH-1:0] initial_state;
    wire [DATA_WIDTH-1:0] delta_accumulator;
    
    // Signals for state reconstruction
    wire [DATA_WIDTH-1:0] current_state;
    
    // Control signals derived from operation code
    wire load_en;
    wire accumulate_en;
    wire read_en;
    
    // =========================================================================
    // Operation Decode (Combinational)
    // =========================================================================
    assign load_en       = (operation == `OP_LOAD);
    assign accumulate_en = (operation == `OP_ACCUMULATE);
    assign read_en       = (operation == `OP_READ);
    
    // =========================================================================
    // Delta Accumulator Instance
    // =========================================================================
    atomik_delta_acc #(
        .DELTA_WIDTH(DATA_WIDTH)
    ) u_delta_acc (
        .clk                  (clk),
        .rst_n                (rst_n),
        
        // Delta input
        .delta_in             (data_in),
        .delta_valid          (accumulate_en),
        
        // Initial state input
        .initial_state_in     (data_in),
        .load_initial         (load_en),
        
        // Outputs
        .initial_state_out    (initial_state),
        .delta_accumulator_out(delta_accumulator),
        .accumulator_zero     (accumulator_zero)
    );
    
    // =========================================================================
    // State Reconstructor Instance
    // =========================================================================
    atomik_state_rec #(
        .STATE_WIDTH(DATA_WIDTH)
    ) u_state_rec (
        .initial_state     (initial_state),
        .delta_accumulator (delta_accumulator),
        .current_state     (current_state)
    );
    
    // =========================================================================
    // Output Register and Control
    // =========================================================================
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            data_out   <= {DATA_WIDTH{1'b0}};
            data_valid <= 1'b0;
        end
        else begin
            // Default: clear valid flag
            data_valid <= 1'b0;
            
            case (operation)
                `OP_NOP: begin
                    // No operation - hold state, no valid pulse
                end
                
                `OP_LOAD: begin
                    // Load operation completes this cycle
                    data_valid <= 1'b1;
                end
                
                `OP_ACCUMULATE: begin
                    // Accumulate operation completes this cycle
                    data_valid <= 1'b1;
                end
                
                `OP_READ: begin
                    // Capture reconstructed state to output register
                    data_out   <= current_state;
                    data_valid <= 1'b1;
                end
                
                default: begin
                    // Should never happen, but safe default
                    data_valid <= 1'b0;
                end
            endcase
        end
    end
    
    // =========================================================================
    // Debug Outputs
    // =========================================================================
    assign debug_initial_state = initial_state;
    assign debug_accumulator   = delta_accumulator;

endmodule

// =============================================================================
// Module Documentation
// =============================================================================
//
// USAGE EXAMPLE:
// --------------
// atomik_core_v2 #(
//     .DATA_WIDTH(64)
// ) u_core (
//     .clk              (clk_94p5m),
//     .rst_n            (sys_rst_n & pll_lock),
//     .operation        (ctrl_operation),
//     .data_in          (ctrl_data_in),
//     .data_out         (core_data_out),
//     .data_valid       (core_data_valid),
//     .accumulator_zero (core_acc_zero),
//     .debug_initial_state(dbg_init),
//     .debug_accumulator(dbg_accum)
// );
//
// OPERATION SEQUENCES:
// --------------------
//
// Initialize and Load:
//   Cycle 1: operation=LOAD, data_in=S_initial
//   Cycle 2: data_valid=1 (load complete)
//
// Accumulate Deltas:
//   Cycle 1: operation=ACCUMULATE, data_in=delta_1
//   Cycle 2: operation=ACCUMULATE, data_in=delta_2, data_valid=1
//   Cycle 3: operation=ACCUMULATE, data_in=delta_3, data_valid=1
//   ...
//
// Read Current State:
//   Cycle 1: operation=READ
//   Cycle 2: data_out=current_state, data_valid=1
//
// TIMING DIAGRAM:
// ---------------
//   clk:       __|‾‾|__|‾‾|__|‾‾|__|‾‾|__|‾‾|__|‾‾|__
//
//   operation:   |LOAD |ACCUM|ACCUM|READ |NOP  |
//   data_in:     |init | δ1  | δ2  | XX  | XX  |
//
//   data_out:    | XX  | XX  | XX  |S_cur| -- held --
//   data_valid:  |  0  |  1  |  1  |  1  |  0  |
//
// RESOURCE ESTIMATES (GW1NR-9):
// -----------------------------
//   From atomik_delta_acc: 128 FFs, ~82 LUTs
//   From atomik_state_rec: 0 FFs, 64 LUTs
//   This module:           64 FFs (data_out), ~15 LUTs (control)
//   ---------------------------------------------------------
//   TOTAL:                 ~192 FFs, ~161 LUTs
//                          (2.96% FF, 1.86% LUT utilization)
//
// =============================================================================
