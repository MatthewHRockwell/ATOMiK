// =============================================================================
// ATOMiK v3 CPU Top-Level
//
// Module:      atomik_v3_cpu
// Description: RV64I multi-cycle CPU top-level.
//              Wires: fetch, decode, regfile, ALU, branch, LSU, CSR, control.
//              32-bit external bus (PicoRV32-compatible valid/ready protocol).
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

/* verilator lint_off UNUSEDSIGNAL */
/* verilator lint_off UNUSEDPARAM */

module atomik_v3_cpu #(
    parameter RESET_PC = 64'h0000_0000_0010_0000
)(
    input  wire        clk,
    input  wire        rst_n,

    // 32-bit bus interface (PicoRV32-compatible)
    output wire        mem_valid,
    input  wire        mem_ready,
    output wire [31:0] mem_addr,
    output wire [31:0] mem_wdata,
    output wire [3:0]  mem_wstrb,
    input  wire [31:0] mem_rdata
);

    // =========================================================================
    // Internal wires
    // =========================================================================

    // Fetch
    wire [63:0] pc;
    wire [63:0] pc_next;
    wire        pc_wen;
    wire        fetch_start;
    wire        fetch_done;
    wire [31:0] instr;
    wire        fetch_bus_valid;
    wire [31:0] fetch_bus_addr;

    // Decode
    wire [4:0]  dec_rd, dec_rs1, dec_rs2;
    wire [63:0] dec_imm;
    wire [4:0]  dec_alu_op;
    wire        dec_alu_src_b_imm;
    wire        dec_is_word_op;
    wire        dec_is_lui, dec_is_auipc, dec_is_jal, dec_is_jalr;
    wire        dec_is_branch, dec_is_load, dec_is_store;
    wire        dec_is_alu_reg, dec_is_alu_imm;
    wire        dec_is_system, dec_is_fence, dec_is_custom0;
    wire [2:0]  dec_mem_size, dec_funct3;
    wire [6:0]  dec_funct7; // Connected to decoder; unused at top-level (used in Phase 2+ ATOMiK decode)
    wire        dec_illegal;

    // Register file
    wire [63:0] rs1_data, rs2_data;
    wire [63:0] rd_data;
    wire        regfile_wen;

    // ALU
    wire [63:0] alu_result;
    wire [63:0] alu_operand_a, alu_operand_b;

    // Branch
    wire        branch_taken;

    // LSU
    wire        lsu_start, lsu_done;
    wire        lsu_misaligned;
    wire [63:0] lsu_rdata;
    wire        lsu_bus_valid;
    wire [31:0] lsu_bus_addr;
    wire [31:0] lsu_bus_wdata;
    wire [3:0]  lsu_bus_wstrb;

    // CSR
    wire [63:0] csr_rdata;
    wire        csr_wen;
    wire        trap_enter, trap_return;
    wire [63:0] mtvec_out, mepc_out;
    wire        instr_retire;

    // ATOMiK
    wire [63:0] atomik_current_state;
    wire        atomik_acc_zero;
    reg  [63:0] atomik_result_r;     // Registered result for WRITEBACK

    // Control
    wire [1:0]  pc_src;
    wire [2:0]  wb_src;
    wire [2:0]  state_out;
    wire        misaligned_trap;

    // FSM state constants (must match atomik_v3_control.v)
    localparam S_DECODE    = 3'd1;
    localparam S_EXECUTE   = 3'd2;
    localparam S_MEMORY    = 3'd3;

    // =========================================================================
    // Registered decode outputs — breaks critical path from instr→decode→control
    // Latched during DECODE state (1 cycle), valid from EXECUTE onward.
    // No extra cycle added — DECODE already takes 1 cycle.
    // =========================================================================
    reg [63:0] dec_imm_r;
    reg [4:0]  dec_alu_op_r;
    reg        dec_alu_src_b_imm_r;
    reg        dec_is_word_op_r;
    reg        dec_is_lui_r, dec_is_auipc_r, dec_is_jal_r, dec_is_jalr_r;
    reg        dec_is_branch_r, dec_is_load_r, dec_is_store_r;
    reg        dec_is_alu_reg_r, dec_is_alu_imm_r;
    reg        dec_is_system_r, dec_is_fence_r, dec_is_custom0_r;
    reg [2:0]  dec_mem_size_r, dec_funct3_r;
    reg        dec_illegal_r;

    always @(posedge clk) begin
        if (!rst_n) begin
            dec_imm_r           <= 64'b0;
            dec_alu_op_r        <= 5'b0;
            dec_alu_src_b_imm_r <= 1'b0;
            dec_is_word_op_r    <= 1'b0;
            dec_is_lui_r        <= 1'b0;
            dec_is_auipc_r      <= 1'b0;
            dec_is_jal_r        <= 1'b0;
            dec_is_jalr_r       <= 1'b0;
            dec_is_branch_r     <= 1'b0;
            dec_is_load_r       <= 1'b0;
            dec_is_store_r      <= 1'b0;
            dec_is_alu_reg_r    <= 1'b0;
            dec_is_alu_imm_r    <= 1'b0;
            dec_is_system_r     <= 1'b0;
            dec_is_fence_r      <= 1'b0;
            dec_is_custom0_r    <= 1'b0;
            dec_mem_size_r      <= 3'b0;
            dec_funct3_r        <= 3'b0;
            dec_illegal_r       <= 1'b0;
        end else if (state_out == S_DECODE) begin
            dec_imm_r           <= dec_imm;
            dec_alu_op_r        <= dec_alu_op;
            dec_alu_src_b_imm_r <= dec_alu_src_b_imm;
            dec_is_word_op_r    <= dec_is_word_op;
            dec_is_lui_r        <= dec_is_lui;
            dec_is_auipc_r      <= dec_is_auipc;
            dec_is_jal_r        <= dec_is_jal;
            dec_is_jalr_r       <= dec_is_jalr;
            dec_is_branch_r     <= dec_is_branch;
            dec_is_load_r       <= dec_is_load;
            dec_is_store_r      <= dec_is_store;
            dec_is_alu_reg_r    <= dec_is_alu_reg;
            dec_is_alu_imm_r    <= dec_is_alu_imm;
            dec_is_system_r     <= dec_is_system;
            dec_is_fence_r      <= dec_is_fence;
            dec_is_custom0_r    <= dec_is_custom0;
            dec_mem_size_r      <= dec_mem_size;
            dec_funct3_r        <= dec_funct3;
            dec_illegal_r       <= dec_illegal;
        end
    end

    // =========================================================================
    // Bus arbitration: fetch vs LSU
    // FSM guarantees mutual exclusion — fetch only active in FETCH state,
    // LSU only active in MEMORY state
    // =========================================================================
    wire lsu_has_bus = (state_out == S_MEMORY) || (state_out == S_EXECUTE && (dec_is_load_r || dec_is_store_r));

    // synthesis translate_off
    // Bus mutual exclusion assertion: fetch and LSU must never drive bus simultaneously
    always @(posedge clk) begin
        if (rst_n && fetch_bus_valid && lsu_bus_valid) begin
            $display("BUS ASSERTION FAIL: fetch_bus_valid and lsu_bus_valid both high at time %0t", $time);
            $finish(1);
        end
    end
    // synthesis translate_on

    // Gate mem_ready based on bus ownership (FIX: prevent crosstalk)
    wire fetch_bus_ready = mem_ready && !lsu_has_bus;
    wire lsu_bus_ready   = mem_ready && lsu_has_bus;

    assign mem_valid = lsu_has_bus ? lsu_bus_valid : fetch_bus_valid;
    assign mem_addr  = lsu_has_bus ? lsu_bus_addr  : fetch_bus_addr;
    assign mem_wdata = lsu_bus_wdata;
    assign mem_wstrb = lsu_has_bus ? lsu_bus_wstrb : 4'b0;

    // =========================================================================
    // Fetch Unit
    // =========================================================================
    atomik_v3_fetch #(
        .RESET_PC(RESET_PC)
    ) u_fetch (
        .clk            (clk),
        .rst_n          (rst_n),
        .pc_next        (pc_next),
        .pc_wen         (pc_wen),
        .pc             (pc),
        .fetch_start    (fetch_start),
        .instr          (instr),
        .fetch_done     (fetch_done),
        .fetch_bus_valid(fetch_bus_valid),
        .bus_ready      (fetch_bus_ready),  // FIX: use gated ready signal
        .fetch_bus_addr (fetch_bus_addr),
        .bus_rdata      (mem_rdata)
    );

    // =========================================================================
    // Decoder
    // =========================================================================
    atomik_v3_decode u_decode (
        .instr          (instr),
        .rd             (dec_rd),
        .rs1            (dec_rs1),
        .rs2            (dec_rs2),
        .imm            (dec_imm),
        .alu_op         (dec_alu_op),
        .alu_src_b_imm  (dec_alu_src_b_imm),
        .is_word_op     (dec_is_word_op),
        .is_lui         (dec_is_lui),
        .is_auipc       (dec_is_auipc),
        .is_jal         (dec_is_jal),
        .is_jalr        (dec_is_jalr),
        .is_branch      (dec_is_branch),
        .is_load        (dec_is_load),
        .is_store       (dec_is_store),
        .is_alu_reg     (dec_is_alu_reg),
        .is_alu_imm     (dec_is_alu_imm),
        .is_system      (dec_is_system),
        .is_fence       (dec_is_fence),
        .is_custom0     (dec_is_custom0),
        .mem_size       (dec_mem_size),
        .funct3         (dec_funct3),
        .funct7         (dec_funct7),
        .illegal_instr  (dec_illegal)
    );

    // =========================================================================
    // Register File
    // =========================================================================
    atomik_v3_regfile u_regfile (
        .clk      (clk),
        .rst_n    (rst_n),
        .rs1_addr (dec_rs1),
        .rs1_data (rs1_data),
        .rs2_addr (dec_rs2),
        .rs2_data (rs2_data),
        .rd_addr  (dec_rd),
        .rd_data  (rd_data),
        .rd_wen   (regfile_wen)
    );

    // =========================================================================
    // ALU operand muxing
    // =========================================================================
    // Operand A: PC for AUIPC/branch/JAL (target = PC + imm), else RS1
    assign alu_operand_a = (dec_is_auipc_r || dec_is_branch_r || dec_is_jal_r) ? pc : rs1_data;

    // Operand B: immediate if flagged, else RS2
    assign alu_operand_b = dec_alu_src_b_imm_r ? dec_imm_r : rs2_data;

    // =========================================================================
    // ALU
    // =========================================================================
    atomik_v3_alu u_alu (
        .operand_a  (alu_operand_a),
        .operand_b  (alu_operand_b),
        .alu_op     (dec_alu_op_r),
        .is_word_op (dec_is_word_op_r),
        .result     (alu_result)
    );

    // =========================================================================
    // Branch Comparator
    // =========================================================================
    atomik_v3_branch u_branch (
        .rs1_val      (rs1_data),
        .rs2_val      (rs2_data),
        .funct3       (dec_funct3_r),
        .branch_taken (branch_taken)
    );

    // =========================================================================
    // Load/Store Unit
    // =========================================================================
    atomik_v3_lsu u_lsu (
        .clk          (clk),
        .rst_n        (rst_n),
        .lsu_start    (lsu_start),
        .lsu_is_store (dec_is_store_r),
        .lsu_size     (dec_mem_size_r),
        .lsu_addr     (alu_result),     // Effective addr = rs1 + imm (from ALU)
        .lsu_wdata    (rs2_data),
        .lsu_rdata    (lsu_rdata),
        .lsu_done     (lsu_done),
        .lsu_misaligned (lsu_misaligned),
        .bus_valid    (lsu_bus_valid),
        .bus_ready    (lsu_bus_ready),  // FIX: use gated ready signal
        .bus_addr     (lsu_bus_addr),
        .bus_wdata    (lsu_bus_wdata),
        .bus_wstrb    (lsu_bus_wstrb),
        .bus_rdata    (mem_rdata)
    );

    // =========================================================================
    // CSR Unit
    // =========================================================================
    // CSR write data: for CSRRWI/CSRRSI/CSRRCI, use zero-extended zimm (rs1 field)
    wire [63:0] csr_wdata_mux = (dec_funct3_r[2]) ?
        {59'b0, dec_rs1} : rs1_data;

    atomik_v3_csr u_csr (
        .clk          (clk),
        .rst_n        (rst_n),
        .csr_addr     (instr[31:20]),   // CSR address from immediate field
        .csr_wdata    (csr_wdata_mux),
        .csr_op       (dec_funct3_r),
        .csr_wen      (csr_wen),
        .csr_rdata    (csr_rdata),
        .trap_enter   (trap_enter),
        .trap_pc      (pc),
        .trap_cause   (misaligned_trap ?
                       (dec_is_store_r ? 64'd6 : 64'd4) :  // Store/load address misaligned
                       dec_is_system_r && dec_funct3_r == 3'b000 && instr[20] ?
                       64'd3 :  // EBREAK = cause 3
                       dec_illegal_r ? 64'd2 :  // Illegal instruction = cause 2
                       64'd11),  // ECALL from M-mode = cause 11
        .trap_return  (trap_return),
        .mtvec_out    (mtvec_out),
        .mepc_out     (mepc_out),
        .instr_retire (instr_retire)
    );

    // =========================================================================
    // ATOMiK Datapath (accumulator, state table, reconstructor)
    // =========================================================================
    // Custom-0 sub-type decode
    wire atomik_load  = dec_is_custom0_r && (dec_funct3_r == 3'b000);
    wire atomik_accum = dec_is_custom0_r && (dec_funct3_r == 3'b001);
    wire atomik_swap  = dec_is_custom0_r && (dec_funct3_r == 3'b011);

    // Enable signals: fire once during EXECUTE state
    wire atomik_load_en  = (state_out == S_EXECUTE) && atomik_load;
    wire atomik_accum_en = (state_out == S_EXECUTE) && atomik_accum;
    wire atomik_swap_en  = (state_out == S_EXECUTE) && atomik_swap;

    atomik_v3_atomik u_atomik (
        .clk           (clk),
        .rst_n         (rst_n),
        .load_en       (atomik_load_en),
        .accum_en      (atomik_accum_en),
        .swap_en       (atomik_swap_en),
        .addr_in       (rs1_data[7:0]),    // LOAD/SWAP address
        .delta_in      (rs1_data),          // ACCUM delta
        .init_data     (rs2_data),          // LOAD initial state (from rs2)
        .current_state (atomik_current_state),
        .acc_zero      (atomik_acc_zero)
    );

    // =========================================================================
    // ATOMiK result register
    // Captures current_state at each clock edge so WRITEBACK (1 cycle after
    // EXECUTE) reads the pre-EXECUTE value. Fixes Gowin BSRAM write-through:
    // during SWAP, the state_table read port returns the NEW value on
    // simultaneous read/write, corrupting the combinational current_state.
    // =========================================================================
    always @(posedge clk) begin
        if (!rst_n)
            atomik_result_r <= 64'b0;
        else
            atomik_result_r <= atomik_current_state;
    end

    // =========================================================================
    // FSM Controller
    // =========================================================================
    atomik_v3_control u_control (
        .clk           (clk),
        .rst_n         (rst_n),
        .fetch_start   (fetch_start),
        .fetch_done    (fetch_done),
        .is_lui        (dec_is_lui_r),
        .is_auipc      (dec_is_auipc_r),
        .is_jal        (dec_is_jal_r),
        .is_jalr       (dec_is_jalr_r),
        .is_branch     (dec_is_branch_r),
        .is_load       (dec_is_load_r),
        .is_store      (dec_is_store_r),
        .is_alu_reg    (dec_is_alu_reg_r),
        .is_alu_imm    (dec_is_alu_imm_r),
        .is_system     (dec_is_system_r),
        .is_fence      (dec_is_fence_r),
        .is_custom0    (dec_is_custom0_r),
        .illegal_instr (dec_illegal_r),
        .funct3        (dec_funct3_r),
        .instr         (instr),
        .branch_taken  (branch_taken),
        .lsu_start     (lsu_start),
        .lsu_done      (lsu_done),
        .lsu_misaligned(lsu_misaligned),
        .regfile_wen   (regfile_wen),
        .pc_wen        (pc_wen),
        .csr_wen       (csr_wen),
        .trap_enter    (trap_enter),
        .trap_return   (trap_return),
        .instr_retire  (instr_retire),
        .pc_src        (pc_src),
        .wb_src        (wb_src),
        .misaligned_trap(misaligned_trap),
        .state_out     (state_out)
    );

    // =========================================================================
    // PC next mux
    // =========================================================================
    wire [63:0] pc_plus_4   = pc + 64'd4;
    // Branch/JAL target computed by ALU (pc + imm); JALR target is (rs1 + imm) & ~1
    wire [63:0] jalr_target = {alu_result[63:1], 1'b0};

    reg [63:0] pc_next_mux;
    always @(*) begin
        case (pc_src)
            2'd0: pc_next_mux = pc_plus_4;
            2'd1: pc_next_mux = alu_result;     // Branch/JAL: PC + imm (from ALU)
            2'd2: pc_next_mux = jalr_target;
            2'd3: pc_next_mux = trap_return ? mepc_out : mtvec_out;
            default: pc_next_mux = pc_plus_4;
        endcase
    end
    assign pc_next = pc_next_mux;

    // =========================================================================
    // Writeback mux
    // =========================================================================
    reg [63:0] wb_data;
    always @(*) begin
        case (wb_src)
            3'd0: wb_data = alu_result;
            3'd1: wb_data = lsu_rdata;
            3'd2: wb_data = pc_plus_4;
            3'd3: wb_data = csr_rdata;
            3'd4: wb_data = atomik_result_r;  // ATOMiK result (registered from EXECUTE)
            default: wb_data = 64'b0;
        endcase
    end
    assign rd_data = wb_data;

endmodule
