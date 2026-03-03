// =============================================================================
// ATOMiK v3 FSM Controller
//
// Module:      atomik_v3_control
// Description: Multi-cycle FSM for RV64I instruction execution.
//              States: FETCH → DECODE → EXECUTE → MEMORY → WRITEBACK
//              Controls datapath muxing and sub-module enables.
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

/* verilator lint_off UNUSEDSIGNAL */

module atomik_v3_control (
    input  wire        clk,
    input  wire        rst_n,

    // Fetch interface
    output reg         fetch_start,
    input  wire        fetch_done,

    // Decode signals (from decoder)
    input  wire        is_lui,
    input  wire        is_auipc,
    input  wire        is_jal,
    input  wire        is_jalr,
    input  wire        is_branch,
    input  wire        is_load,
    input  wire        is_store,
    input  wire        is_alu_reg,
    input  wire        is_alu_imm,
    input  wire        is_system,
    input  wire        is_fence,
    input  wire        is_custom0,
    input  wire        illegal_instr,

    // System sub-type
    input  wire [2:0]  funct3,
    input  wire [31:0] instr,  // Full instruction for ECALL/EBREAK detect

    // Branch result
    input  wire        branch_taken,

    // LSU interface
    output reg         lsu_start,
    input  wire        lsu_done,
    input  wire        lsu_misaligned, // Address alignment violation

    // Control outputs
    output reg         regfile_wen,    // Register file write enable
    output reg         pc_wen,         // PC write enable
    output reg         csr_wen,        // CSR write enable
    output reg         trap_enter,     // Enter trap
    output reg         trap_return,    // MRET
    output reg         instr_retire,   // Instruction retired (for minstret)

    // PC source select
    output reg  [1:0]  pc_src,
    // 0 = PC + 4
    // 1 = PC + imm (branch/JAL)
    // 2 = ALU result (JALR: (rs1+imm) & ~1)
    // 3 = trap vector (mtvec)

    // Writeback source select
    output reg  [2:0]  wb_src,
    // 0 = ALU result
    // 1 = memory load data
    // 2 = PC + 4 (JAL/JALR link)
    // 3 = CSR read data

    // Misaligned trap flag (for cause code generation)
    output wire        misaligned_trap,

    // Current state (exposed for bus arbitration)
    output wire [2:0]  state_out
);

    // FSM states
    localparam S_FETCH     = 3'd0;
    localparam S_DECODE    = 3'd1;
    localparam S_EXECUTE   = 3'd2;
    localparam S_MEMORY    = 3'd3;
    localparam S_WRITEBACK = 3'd4;

    reg [2:0] state, state_next;
    assign state_out = state;
    assign misaligned_trap = was_misaligned;

    // Latch misaligned status from EXECUTE for use in WRITEBACK
    reg was_misaligned;
    always @(posedge clk) begin
        if (!rst_n)
            was_misaligned <= 1'b0;
        else if (state == S_EXECUTE)
            was_misaligned <= lsu_misaligned;
        else if (state == S_WRITEBACK)
            was_misaligned <= 1'b0;
    end

    // ECALL/EBREAK detection
    wire is_ecall  = is_system && (funct3 == 3'b000) && (instr[31:20] == 12'h000);
    wire is_ebreak = is_system && (funct3 == 3'b000) && (instr[31:20] == 12'h001);
    wire is_mret   = is_system && (funct3 == 3'b000) && (instr[31:20] == 12'h302);
    wire is_csr_op = is_system && (funct3 != 3'b000);

    // =========================================================================
    // FSM state register
    // =========================================================================
    always @(posedge clk) begin
        if (!rst_n)
            state <= S_FETCH;
        else
            state <= state_next;
    end

    // =========================================================================
    // FSM next-state logic
    // =========================================================================
    always @(*) begin
        state_next = state;
        case (state)
            S_FETCH: begin
                if (fetch_done)
                    state_next = S_DECODE;
            end

            S_DECODE: begin
                state_next = S_EXECUTE;
            end

            S_EXECUTE: begin
                if ((is_load || is_store) && !lsu_misaligned)
                    state_next = S_MEMORY;
                else
                    state_next = S_WRITEBACK;  // Also handles misaligned → trap in WB
            end

            S_MEMORY: begin
                if (lsu_done)
                    state_next = S_WRITEBACK;
            end

            S_WRITEBACK: begin
                state_next = S_FETCH;
            end

            default: state_next = S_FETCH;
        endcase
    end

    // =========================================================================
    // Control signal generation
    // =========================================================================
    always @(*) begin
        // Defaults: everything off
        fetch_start  = 1'b0;
        lsu_start    = 1'b0;
        regfile_wen  = 1'b0;
        pc_wen       = 1'b0;
        csr_wen      = 1'b0;
        trap_enter   = 1'b0;
        trap_return  = 1'b0;
        instr_retire = 1'b0;
        pc_src       = 2'd0;
        wb_src       = 3'd0;

        case (state)
            // -----------------------------------------------------------------
            // FETCH: Start instruction fetch
            // -----------------------------------------------------------------
            S_FETCH: begin
                fetch_start = !fetch_done;  // Keep requesting until done
            end

            // -----------------------------------------------------------------
            // DECODE: Register read happens combinationally, nothing to drive
            // -----------------------------------------------------------------
            S_DECODE: begin
                // Nothing — regfile reads are combinational
            end

            // -----------------------------------------------------------------
            // EXECUTE: Start LSU for loads/stores, compute branches
            // -----------------------------------------------------------------
            S_EXECUTE: begin
                if (is_load || is_store)
                    lsu_start = 1'b1;
            end

            // -----------------------------------------------------------------
            // MEMORY: Wait for LSU completion
            // -----------------------------------------------------------------
            S_MEMORY: begin
                // Waiting for lsu_done
            end

            // -----------------------------------------------------------------
            // WRITEBACK: Write results, advance PC, retire instruction
            // -----------------------------------------------------------------
            S_WRITEBACK: begin
                instr_retire = 1'b1;
                pc_wen = 1'b1;

                // Default: PC + 4
                pc_src = 2'd0;

                if (was_misaligned) begin
                    trap_enter = 1'b1;
                    pc_src = 2'd3;  // mtvec
                end

                else if (is_lui || is_auipc || is_alu_reg || is_alu_imm) begin
                    regfile_wen = 1'b1;
                    wb_src = 3'd0;  // ALU result
                end

                else if (is_load) begin
                    regfile_wen = 1'b1;
                    wb_src = 3'd1;  // Memory load data
                end

                else if (is_store) begin
                    // No register write
                end

                else if (is_jal) begin
                    regfile_wen = 1'b1;
                    wb_src = 3'd2;  // PC + 4 (link)
                    pc_src = 2'd1;  // PC + imm
                end

                else if (is_jalr) begin
                    regfile_wen = 1'b1;
                    wb_src = 3'd2;  // PC + 4 (link)
                    pc_src = 2'd2;  // ALU result (rs1 + imm) & ~1
                end

                else if (is_branch) begin
                    if (branch_taken)
                        pc_src = 2'd1;  // PC + imm
                    // else PC + 4 (default)
                end

                else if (is_csr_op) begin
                    regfile_wen = 1'b1;
                    wb_src = 3'd3;  // CSR read data
                    csr_wen = 1'b1;
                end

                else if (is_ecall || is_ebreak) begin
                    trap_enter = 1'b1;
                    pc_src = 2'd3;  // mtvec
                end

                else if (is_mret) begin
                    trap_return = 1'b1;
                    pc_src = 2'd3;  // mepc (handled in top-level PC mux)
                end

                else if (is_custom0) begin
                    // ATOMIK.READ (funct3=010): write reconstructed state to rd
                    // ATOMIK.SWAP (funct3=011): write current_state to rd, then update ref + clear acc
                    if (funct3 == 3'b010 || funct3 == 3'b011) begin
                        regfile_wen = 1'b1;
                        wb_src = 3'd4;  // ATOMiK current_state
                    end
                    // LOAD (000), ACCUM (001): no regfile write, just advance PC
                end

                else if (is_fence) begin
                    // NOP: just advance PC
                end

                else if (illegal_instr) begin
                    trap_enter = 1'b1;
                    pc_src = 2'd3;  // mtvec
                end
            end

            default: ;
        endcase
    end

endmodule
