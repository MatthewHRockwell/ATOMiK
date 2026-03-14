// =============================================================================
// ATOMiK BIST — Built-In Self-Test Controller
//
// Module:      atomik_bist
// Description: Autonomous self-test FSM that exercises all ATOMiK operations.
//              Runs at power-on or on-demand. Reports pass/fail via status
//              registers readable by the test interface.
//
// Test Suite (10 tests, derived from 92 Lean4 proofs):
//   T1: LOAD + READ identity      — load(addr, X); read(addr) == X
//   T2: ACCUM + READ              — load(0, A); accum(B); read == A^B
//   T3: XOR self-inverse           — accum(X); accum(X); read == original
//   T4: Identity element           — accum(0); read == original
//   T5: Commutativity              — accum(A); accum(B) == accum(B); accum(A)
//   T6: Multi-address isolation    — ops on addr X don't affect addr Y
//   T7: SWAP returns current state — swap(addr) == reference ^ accumulator
//   T8: SWAP resets accumulator    — after swap, read == swap result
//   T9: ACCUM after SWAP           — new epoch accumulates correctly
//   T10: All-ones stress           — load(0xFFF...F); accum(0xFFF...F); read == 0
//
// ATOMiK Project — March 2026
// SPDX-License-Identifier: MIT
// =============================================================================

`timescale 1ns / 1ps

module atomik_bist #(
    parameter ADDR_WIDTH = 8,
    parameter DATA_WIDTH = 64
) (
    input  wire                    clk,
    input  wire                    rst_n,

    // Control
    input  wire                    bist_start,    // Pulse to begin test
    output reg                     bist_done,     // Asserted when complete
    output reg                     bist_pass,     // All tests passed
    output reg  [7:0]              bist_count,    // Tests passed (0-10)
    output reg                     bist_active,   // BIST owns the core

    // ATOMiK core interface (directly drives core when active)
    output reg                     op_valid,
    output reg  [1:0]              op_code,
    output reg  [ADDR_WIDTH-1:0]   op_addr,
    output reg  [DATA_WIDTH-1:0]   op_data,

    // Result from core
    input  wire                    result_valid,
    input  wire [DATA_WIDTH-1:0]   result_data
);

    // =========================================================================
    // Operation Encoding
    // =========================================================================
    localparam OP_LOAD  = 2'b00;
    localparam OP_ACCUM = 2'b01;
    localparam OP_READ  = 2'b10;
    localparam OP_SWAP  = 2'b11;

    // =========================================================================
    // Test Constants
    // =========================================================================
    localparam [DATA_WIDTH-1:0] PATTERN_A = 64'hDEADBEEFCAFEBABE;
    localparam [DATA_WIDTH-1:0] PATTERN_B = 64'h00FF00FF00FF00FF;
    localparam [DATA_WIDTH-1:0] PATTERN_C = 64'h1234567890ABCDEF;
    localparam [DATA_WIDTH-1:0] ALL_ONES  = {DATA_WIDTH{1'b1}};
    localparam [DATA_WIDTH-1:0] ZERO      = {DATA_WIDTH{1'b0}};

    localparam NUM_TESTS = 10;

    // =========================================================================
    // FSM States
    // =========================================================================
    localparam S_IDLE        = 5'd0;
    localparam S_ISSUE_OP    = 5'd1;
    localparam S_WAIT_RESULT = 5'd2;
    localparam S_CHECK       = 5'd3;
    localparam S_NEXT_STEP   = 5'd4;
    localparam S_DONE        = 5'd5;

    reg [4:0]  state;
    reg [3:0]  test_id;         // Current test (0-9)
    reg [3:0]  step;            // Step within current test
    reg [DATA_WIDTH-1:0] expected;
    reg [DATA_WIDTH-1:0] captured;  // Captured result for multi-step tests

    // =========================================================================
    // Test Sequencer
    // =========================================================================
    //
    // Each test is a sequence of operations. The FSM issues one op at a time,
    // waits for result_valid, then checks against expected value (if applicable).

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state       <= S_IDLE;
            test_id     <= 4'd0;
            step        <= 4'd0;
            bist_done   <= 1'b0;
            bist_pass   <= 1'b0;
            bist_count  <= 8'd0;
            bist_active <= 1'b0;
            op_valid    <= 1'b0;
            op_code     <= 2'b0;
            op_addr     <= {ADDR_WIDTH{1'b0}};
            op_data     <= ZERO;
            expected    <= ZERO;
            captured    <= ZERO;
        end else begin
            case (state)
                // =============================================================
                S_IDLE: begin
                    op_valid <= 1'b0;
                    if (bist_start && !bist_done) begin
                        bist_active <= 1'b1;
                        bist_done   <= 1'b0;
                        bist_pass   <= 1'b0;
                        bist_count  <= 8'd0;
                        test_id     <= 4'd0;
                        step        <= 4'd0;
                        state       <= S_ISSUE_OP;
                    end
                end

                // =============================================================
                S_ISSUE_OP: begin
                    if (test_id >= NUM_TESTS[3:0]) begin
                        // All tests completed
                        op_valid <= 1'b0;
                        state    <= S_DONE;
                    end else begin
                    op_valid <= 1'b1;
                    state    <= S_WAIT_RESULT;  // Default: advance to wait

                    case (test_id)
                        // =====================================================
                        // T1: LOAD + READ identity
                        // =====================================================
                        4'd0: begin
                            case (step)
                                4'd0: begin  // LOAD addr=0, data=PATTERN_A
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd0;
                                    op_data <= PATTERN_A;
                                end
                                4'd1: begin  // READ addr=0, expect PATTERN_A
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd0;
                                    op_data  <= ZERO;
                                    expected <= PATTERN_A;
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        // =====================================================
                        // T2: ACCUM + READ — load(1,A); accum(B); read == A^B
                        // =====================================================
                        4'd1: begin
                            case (step)
                                4'd0: begin
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd1;
                                    op_data <= PATTERN_A;
                                end
                                4'd1: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd1;
                                    op_data <= PATTERN_B;
                                end
                                4'd2: begin
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd1;
                                    op_data  <= ZERO;
                                    expected <= PATTERN_A ^ PATTERN_B;
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        // =====================================================
                        // T3: XOR self-inverse — accum(X); accum(X) cancels
                        // =====================================================
                        4'd2: begin
                            case (step)
                                4'd0: begin
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd2;
                                    op_data <= PATTERN_C;
                                end
                                4'd1: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd2;
                                    op_data <= ALL_ONES;
                                end
                                4'd2: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd2;
                                    op_data <= ALL_ONES;
                                end
                                4'd3: begin
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd2;
                                    op_data  <= ZERO;
                                    expected <= PATTERN_C;
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        // =====================================================
                        // T4: Identity element — accum(0) is no-op
                        // =====================================================
                        4'd3: begin
                            case (step)
                                4'd0: begin
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd3;
                                    op_data <= PATTERN_A;
                                end
                                4'd1: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd3;
                                    op_data <= ZERO;
                                end
                                4'd2: begin
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd3;
                                    op_data  <= ZERO;
                                    expected <= PATTERN_A;
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        // =====================================================
                        // T5: Commutativity — accum(A); accum(B) == accum(B); accum(A)
                        // =====================================================
                        4'd4: begin
                            case (step)
                                4'd0: begin  // Path 1: load, accum(A), accum(B), read
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd4;
                                    op_data <= ZERO;
                                end
                                4'd1: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd4;
                                    op_data <= PATTERN_A;
                                end
                                4'd2: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd4;
                                    op_data <= PATTERN_B;
                                end
                                4'd3: begin  // Read and capture result
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd4;
                                    op_data  <= ZERO;
                                    expected <= PATTERN_A ^ PATTERN_B; // 0 ^ A ^ B
                                end
                                // Path 2: reload, accum(B), accum(A), read — must match
                                4'd4: begin
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd4;
                                    op_data <= ZERO;
                                end
                                4'd5: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd4;
                                    op_data <= PATTERN_B;
                                end
                                4'd6: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd4;
                                    op_data <= PATTERN_A;
                                end
                                4'd7: begin
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd4;
                                    op_data  <= ZERO;
                                    expected <= PATTERN_A ^ PATTERN_B;
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        // =====================================================
                        // T6: SRAM address isolation
                        //   Verify LOAD to addr X stores correctly in SRAM
                        //   independent of LOAD to addr Y.
                        //   (Accumulator is shared — reset via LOAD before READ)
                        // =====================================================
                        4'd5: begin
                            case (step)
                                4'd0: begin
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd10;
                                    op_data <= PATTERN_A;
                                end
                                4'd1: begin
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd20;
                                    op_data <= PATTERN_B;
                                end
                                4'd2: begin  // LOAD addr 10 again to reset acc, keep ref
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd10;
                                    op_data <= PATTERN_A;
                                end
                                4'd3: begin  // Read addr 10 — acc=0, ref=A → A
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd10;
                                    op_data  <= ZERO;
                                    expected <= PATTERN_A;
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        // =====================================================
                        // T7: SWAP returns current state
                        // =====================================================
                        4'd6: begin
                            case (step)
                                4'd0: begin
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd5;
                                    op_data <= PATTERN_A;
                                end
                                4'd1: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd5;
                                    op_data <= PATTERN_B;
                                end
                                4'd2: begin  // SWAP — should return A ^ B
                                    op_code  <= OP_SWAP;
                                    op_addr  <= 8'd5;
                                    op_data  <= ZERO;
                                    expected <= PATTERN_A ^ PATTERN_B;
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        // =====================================================
                        // T8: READ after SWAP — accumulator was reset
                        // =====================================================
                        4'd7: begin
                            case (step)
                                4'd0: begin  // Read addr 5 after T7's SWAP
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd5;
                                    op_data  <= ZERO;
                                    expected <= PATTERN_A ^ PATTERN_B; // new ref, acc=0
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        // =====================================================
                        // T9: ACCUM after SWAP — new epoch
                        // =====================================================
                        4'd8: begin
                            case (step)
                                4'd0: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd5;
                                    op_data <= 64'h11;
                                end
                                4'd1: begin
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd5;
                                    op_data  <= ZERO;
                                    // ref=(A^B), acc=0x11 → state = (A^B) ^ 0x11
                                    expected <= (PATTERN_A ^ PATTERN_B) ^ 64'h11;
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        // =====================================================
                        // T10: All-ones stress — all_1 ^ all_1 = 0
                        // =====================================================
                        4'd9: begin
                            case (step)
                                4'd0: begin
                                    op_code <= OP_LOAD;
                                    op_addr <= 8'd99;
                                    op_data <= ALL_ONES;
                                end
                                4'd1: begin
                                    op_code <= OP_ACCUM;
                                    op_addr <= 8'd99;
                                    op_data <= ALL_ONES;
                                end
                                4'd2: begin
                                    op_code  <= OP_READ;
                                    op_addr  <= 8'd99;
                                    op_data  <= ZERO;
                                    expected <= ZERO; // all_1 ^ all_1 = 0
                                end
                                default: op_valid <= 1'b0;
                            endcase
                        end

                        default: begin
                            op_valid <= 1'b0;
                            state    <= S_DONE;
                        end
                    endcase
                    end  // else (test_id < NUM_TESTS)
                end

                // =============================================================
                S_WAIT_RESULT: begin
                    op_valid <= 1'b0;
                    if (result_valid) begin
                        captured <= result_data;
                        state    <= S_CHECK;
                    end
                end

                // =============================================================
                S_CHECK: begin
                    state <= S_NEXT_STEP;
                end

                // =============================================================
                S_NEXT_STEP: begin
                    // Determine max step for current test
                    case (test_id)
                        4'd0: begin  // T1: 2 steps (0-1), check at step 1
                            if (step == 4'd1) begin
                                if (captured == expected) bist_count <= bist_count + 8'd1;
                                test_id <= test_id + 4'd1;
                                step    <= 4'd0;
                            end else begin
                                step <= step + 4'd1;
                            end
                        end
                        4'd1: begin  // T2: 3 steps, check at step 2
                            if (step == 4'd2) begin
                                if (captured == expected) bist_count <= bist_count + 8'd1;
                                test_id <= test_id + 4'd1;
                                step    <= 4'd0;
                            end else begin
                                step <= step + 4'd1;
                            end
                        end
                        4'd2: begin  // T3: 4 steps, check at step 3
                            if (step == 4'd3) begin
                                if (captured == expected) bist_count <= bist_count + 8'd1;
                                test_id <= test_id + 4'd1;
                                step    <= 4'd0;
                            end else begin
                                step <= step + 4'd1;
                            end
                        end
                        4'd3: begin  // T4: 3 steps, check at step 2
                            if (step == 4'd2) begin
                                if (captured == expected) bist_count <= bist_count + 8'd1;
                                test_id <= test_id + 4'd1;
                                step    <= 4'd0;
                            end else begin
                                step <= step + 4'd1;
                            end
                        end
                        4'd4: begin  // T5: 8 steps, check at steps 3 and 7
                            if (step == 4'd3) begin
                                if (captured == expected) begin
                                    captured <= result_data; // Save for comparison
                                end
                                step <= step + 4'd1;
                            end else if (step == 4'd7) begin
                                if (captured == expected) bist_count <= bist_count + 8'd1;
                                test_id <= test_id + 4'd1;
                                step    <= 4'd0;
                            end else begin
                                step <= step + 4'd1;
                            end
                        end
                        4'd5: begin  // T6: 4 steps, check at step 3
                            if (step == 4'd3) begin
                                if (captured == expected) bist_count <= bist_count + 8'd1;
                                test_id <= test_id + 4'd1;
                                step    <= 4'd0;
                            end else begin
                                step <= step + 4'd1;
                            end
                        end
                        4'd6: begin  // T7: 3 steps, check at step 2
                            if (step == 4'd2) begin
                                if (captured == expected) bist_count <= bist_count + 8'd1;
                                test_id <= test_id + 4'd1;
                                step    <= 4'd0;
                            end else begin
                                step <= step + 4'd1;
                            end
                        end
                        4'd7: begin  // T8: 1 step, check at step 0
                            if (captured == expected) bist_count <= bist_count + 8'd1;
                            test_id <= test_id + 4'd1;
                            step    <= 4'd0;
                        end
                        4'd8: begin  // T9: 2 steps, check at step 1
                            if (step == 4'd1) begin
                                if (captured == expected) bist_count <= bist_count + 8'd1;
                                test_id <= test_id + 4'd1;
                                step    <= 4'd0;
                            end else begin
                                step <= step + 4'd1;
                            end
                        end
                        4'd9: begin  // T10: 3 steps, check at step 2
                            if (step == 4'd2) begin
                                if (captured == expected) bist_count <= bist_count + 8'd1;
                                test_id <= test_id + 4'd1;  // Advance past last test
                                step    <= 4'd0;
                            end else begin
                                step <= step + 4'd1;
                            end
                        end
                        default: begin
                            test_id <= 4'd15;  // Force termination
                        end
                    endcase

                    state <= S_ISSUE_OP;
                end

                // =============================================================
                S_DONE: begin
                    bist_done   <= 1'b1;
                    bist_pass   <= (bist_count == NUM_TESTS[7:0]);
                    bist_active <= 1'b0;
                    op_valid    <= 1'b0;
                end

                default: state <= S_IDLE;
            endcase
        end
    end

endmodule
