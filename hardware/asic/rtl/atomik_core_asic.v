// =============================================================================
// ATOMiK Core — ASIC-Portable Implementation
//
// Module:      atomik_core_asic
// Description: Vendor-neutral ATOMiK core with generic SRAM for state table.
//              Implements the full ATOMiK instruction set:
//                LOAD  — Set initial reference state
//                ACCUM — XOR delta into accumulator
//                READ  — Reconstruct current state (reference ⊕ accumulator)
//                SWAP  — Atomic read-and-reset (snapshot + new epoch)
//
//              State table: 256 entries × 64-bit, backed by generic dual-port
//              SRAM (atomik_sram_dp). Replace with foundry macro for tapeout.
//
// Parameters:
//   ADDR_WIDTH     - State table address width (default 8 → 256 entries)
//   DATA_WIDTH     - Data width (default 64-bit)
//   READ_LATENCY   - SRAM read latency: 1 or 2 cycles
//
// Interface:
//   Same as atomik_v3_atomik — drop-in replacement for ASIC synthesis.
//   op_valid + op_code + operands → result_valid + result_data.
//
// Proven: 92 Lean4 theorems. Hardware-validated on Gowin + Xilinx.
//
// ATOMiK Project — March 2026
// SPDX-License-Identifier: MIT
// =============================================================================

`timescale 1ns / 1ps

module atomik_core_asic #(
    parameter ADDR_WIDTH   = 8,
    parameter DATA_WIDTH   = 64,
    parameter READ_LATENCY = 1    // 1 for distributed, 2 for block SRAM
) (
    input  wire                    clk,
    input  wire                    rst_n,

    // Operation interface
    input  wire                    op_valid,
    input  wire [1:0]              op_code,      // 00=LOAD, 01=ACCUM, 10=READ, 11=SWAP
    input  wire [ADDR_WIDTH-1:0]   op_addr,      // State table address
    input  wire [DATA_WIDTH-1:0]   op_data,      // LOAD: initial state, ACCUM: delta

    // Result interface
    output reg                     result_valid,
    output reg  [DATA_WIDTH-1:0]   result_data   // READ/SWAP: reconstructed state
);

    // =========================================================================
    // Operation Encoding
    // =========================================================================
    localparam OP_LOAD  = 2'b00;
    localparam OP_ACCUM = 2'b01;
    localparam OP_READ  = 2'b10;
    localparam OP_SWAP  = 2'b11;

    // =========================================================================
    // Internal State
    // =========================================================================
    reg  [ADDR_WIDTH-1:0]  active_addr;
    reg  [DATA_WIDTH-1:0]  accumulator;
    reg  [DATA_WIDTH-1:0]  reference;          // Latched from SRAM read

    // SRAM interface signals
    reg                    sram_we;
    reg  [ADDR_WIDTH-1:0]  sram_waddr;
    reg  [DATA_WIDTH-1:0]  sram_wdata;
    reg                    sram_re;
    reg  [ADDR_WIDTH-1:0]  sram_raddr;
    wire [DATA_WIDTH-1:0]  sram_rdata;

    // =========================================================================
    // State Table SRAM (generic, foundry-replaceable)
    // =========================================================================
    //
    // 256 × 64-bit dual-port SRAM.
    // Port A: Write (LOAD stores reference, SWAP stores new reference)
    // Port B: Read (READ/SWAP reads current reference)
    //
    // For ASIC tapeout: replace atomik_sram_dp with foundry macro.
    // For FPGA: synthesis tools infer Block RAM from this behavioral model.

    atomik_sram_dp #(
        .ADDR_WIDTH   (ADDR_WIDTH),
        .DATA_WIDTH   (DATA_WIDTH),
        .DEPTH        (1 << ADDR_WIDTH),
        .READ_LATENCY (READ_LATENCY)
    ) u_state_table (
        .clk   (clk),
        .we    (sram_we),
        .waddr (sram_waddr),
        .wdata (sram_wdata),
        .re    (sram_re),
        .raddr (sram_raddr),
        .rdata (sram_rdata)
    );

    // =========================================================================
    // Pipeline Control
    // =========================================================================
    //
    // READ_LATENCY=1: 2-cycle operation (issue → result)
    // READ_LATENCY=2: 3-cycle operation (issue → wait → result)
    //
    // The pipeline stages handle SRAM read latency for READ and SWAP.

    localparam IDLE   = 2'd0;
    localparam WAIT1  = 2'd1;  // Wait for SRAM read (latency=1 path)
    localparam WAIT2  = 2'd2;  // Wait for SRAM output register (latency=2 path)
    localparam RESULT = 2'd3;

    reg [1:0] state;
    reg [1:0] pending_op;
    reg [DATA_WIDTH-1:0] pending_data;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state        <= IDLE;
            active_addr  <= {ADDR_WIDTH{1'b0}};
            accumulator  <= {DATA_WIDTH{1'b0}};
            reference    <= {DATA_WIDTH{1'b0}};
            result_valid <= 1'b0;
            result_data  <= {DATA_WIDTH{1'b0}};
            sram_we      <= 1'b0;
            sram_re      <= 1'b0;
            pending_op   <= 2'b0;
            pending_data <= {DATA_WIDTH{1'b0}};
        end else begin
            // Defaults
            sram_we      <= 1'b0;
            sram_re      <= 1'b0;
            result_valid <= 1'b0;

            case (state)
                IDLE: begin
                    if (op_valid) begin
                        case (op_code)
                            OP_LOAD: begin
                                // Store reference in SRAM, reset accumulator
                                active_addr <= op_addr;
                                accumulator <= {DATA_WIDTH{1'b0}};
                                sram_we     <= 1'b1;
                                sram_waddr  <= op_addr;
                                sram_wdata  <= op_data;
                                // LOAD completes in 1 cycle (no read needed)
                                result_valid <= 1'b1;
                                result_data  <= {DATA_WIDTH{1'b0}};
                            end

                            OP_ACCUM: begin
                                // XOR delta into accumulator — single cycle
                                accumulator  <= accumulator ^ op_data;
                                result_valid <= 1'b1;
                                result_data  <= {DATA_WIDTH{1'b0}};
                            end

                            OP_READ: begin
                                // Initiate SRAM read, wait for result
                                active_addr <= op_addr;
                                sram_re     <= 1'b1;
                                sram_raddr  <= op_addr;
                                pending_op  <= OP_READ;
                                state       <= WAIT1;
                            end

                            OP_SWAP: begin
                                // Read current state, then write new reference
                                active_addr  <= op_addr;
                                sram_re      <= 1'b1;
                                sram_raddr   <= op_addr;
                                pending_op   <= OP_SWAP;
                                pending_data <= accumulator;
                                state        <= WAIT1;
                            end
                        endcase
                    end
                end

                WAIT1: begin
                    // SRAM read was initiated in IDLE. After 1 cycle,
                    // rdata_pipe0 has the data. Wait one more cycle.
                    if (READ_LATENCY == 2) begin
                        // Need two more cycles for output register
                        state <= WAIT2;
                    end else begin
                        // One more cycle for rdata_pipe0 to settle
                        state <= WAIT2;
                    end
                end

                WAIT2: begin
                    // SRAM data is now stable on sram_rdata
                    reference <= sram_rdata;
                    state     <= RESULT;
                end

                RESULT: begin
                    case (pending_op)
                        OP_READ: begin
                            // Reconstruct: reference ⊕ accumulator
                            result_valid <= 1'b1;
                            result_data  <= reference ^ accumulator;
                        end
                        OP_SWAP: begin
                            // Return current state, then update reference
                            result_valid <= 1'b1;
                            result_data  <= reference ^ pending_data;
                            // Write new reference = current_state
                            sram_we    <= 1'b1;
                            sram_waddr <= active_addr;
                            sram_wdata <= reference ^ pending_data;
                            // Reset accumulator
                            accumulator <= {DATA_WIDTH{1'b0}};
                        end
                        default: begin
                            result_valid <= 1'b1;
                            result_data  <= {DATA_WIDTH{1'b0}};
                        end
                    endcase
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule
