// =============================================================================
// ATOMiK v3 Instruction Decoder
//
// Module:      atomik_v3_decode
// Description: Full RV64I instruction decoder. Extracts all fields and
//              immediate formats from a 32-bit instruction word.
//              Decodes custom-0 (ATOMiK) opcodes.
//              Pure combinational.
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_v3_decode (
    input  wire [31:0] instr,       // 32-bit instruction word

    // Register addresses
    output wire [4:0]  rd,
    output wire [4:0]  rs1,
    output wire [4:0]  rs2,

    // Immediate (sign-extended to 64 bits)
    output reg  [63:0] imm,

    // ALU control
    output reg  [4:0]  alu_op,
    output reg         alu_src_b_imm,  // 1 = operand_b is immediate
    output reg         is_word_op,     // 1 = W-variant (ADDW, etc.)

    // Opcode type flags
    output reg         is_lui,
    output reg         is_auipc,
    output reg         is_jal,
    output reg         is_jalr,
    output reg         is_branch,
    output reg         is_load,
    output reg         is_store,
    output reg         is_alu_reg,     // R-type ALU (ADD, SUB, etc.)
    output reg         is_alu_imm,     // I-type ALU (ADDI, etc.)
    output reg         is_system,      // ECALL/EBREAK/CSR
    output reg         is_fence,
    output reg         is_custom0,     // ATOMiK custom-0 opcode

    // Memory access size (for load/store)
    output wire [2:0]  mem_size,       // funct3 for load/store

    // Branch/CSR funct3
    output wire [2:0]  funct3,
    output wire [6:0]  funct7,

    // Validity
    output reg         illegal_instr
);

    // =========================================================================
    // Field extraction
    // =========================================================================
    wire [6:0] opcode = instr[6:0];
    assign funct3 = instr[14:12];
    assign funct7 = instr[31:25];
    assign rd     = instr[11:7];
    assign rs1    = instr[19:15];
    assign rs2    = instr[24:20];
    assign mem_size = instr[14:12];

    // =========================================================================
    // Opcode definitions (RV64I)
    // =========================================================================
    localparam OP_LUI     = 7'b0110111;
    localparam OP_AUIPC   = 7'b0010111;
    localparam OP_JAL     = 7'b1101111;
    localparam OP_JALR    = 7'b1100111;
    localparam OP_BRANCH  = 7'b1100011;
    localparam OP_LOAD    = 7'b0000011;
    localparam OP_STORE   = 7'b0100011;
    localparam OP_ALU_IMM = 7'b0010011;
    localparam OP_ALU_REG = 7'b0110011;
    localparam OP_ALU_IMM_W = 7'b0011011;  // ADDIW, SLLIW, SRLIW, SRAIW
    localparam OP_ALU_REG_W = 7'b0111011;  // ADDW, SUBW, SLLW, SRLW, SRAW
    localparam OP_SYSTEM  = 7'b1110011;
    localparam OP_FENCE   = 7'b0001111;
    localparam OP_CUSTOM0 = 7'b0001011;

    // ALU operation encoding (matches atomik_v3_alu.v)
    localparam ALU_ADD  = 5'd0;
    localparam ALU_SUB  = 5'd1;
    localparam ALU_SLL  = 5'd2;
    localparam ALU_SLT  = 5'd3;
    localparam ALU_SLTU = 5'd4;
    localparam ALU_XOR  = 5'd5;
    localparam ALU_SRL  = 5'd6;
    localparam ALU_SRA  = 5'd7;
    localparam ALU_OR   = 5'd8;
    localparam ALU_AND  = 5'd9;
    localparam ALU_PASS_B = 5'd10;

    // =========================================================================
    // Immediate generation (all formats, sign-extended to 64 bits)
    // =========================================================================
    wire [63:0] imm_i = {{52{instr[31]}}, instr[31:20]};
    wire [63:0] imm_s = {{52{instr[31]}}, instr[31:25], instr[11:7]};
    wire [63:0] imm_b = {{51{instr[31]}}, instr[31], instr[7], instr[30:25], instr[11:8], 1'b0};
    wire [63:0] imm_u = {{32{instr[31]}}, instr[31:12], 12'b0};
    wire [63:0] imm_j = {{43{instr[31]}}, instr[31], instr[19:12], instr[20], instr[30:21], 1'b0};

    // Shift immediate for RV64I: 6-bit shamt for 64-bit, 5-bit for W-variants
    // Both use I-type immediate — the shamt is in imm[5:0]

    // =========================================================================
    // Decode logic
    // =========================================================================
    always @(*) begin
        // Defaults
        alu_op        = ALU_ADD;
        alu_src_b_imm = 1'b0;
        is_word_op    = 1'b0;
        is_lui        = 1'b0;
        is_auipc      = 1'b0;
        is_jal        = 1'b0;
        is_jalr       = 1'b0;
        is_branch     = 1'b0;
        is_load       = 1'b0;
        is_store      = 1'b0;
        is_alu_reg    = 1'b0;
        is_alu_imm    = 1'b0;
        is_system     = 1'b0;
        is_fence      = 1'b0;
        is_custom0    = 1'b0;
        illegal_instr = 1'b0;
        imm           = 64'b0;

        case (opcode)
            // -----------------------------------------------------------------
            // LUI: rd = imm_u
            // -----------------------------------------------------------------
            OP_LUI: begin
                is_lui        = 1'b1;
                imm           = imm_u;
                alu_op        = ALU_PASS_B;
                alu_src_b_imm = 1'b1;
            end

            // -----------------------------------------------------------------
            // AUIPC: rd = PC + imm_u
            // -----------------------------------------------------------------
            OP_AUIPC: begin
                is_auipc      = 1'b1;
                imm           = imm_u;
                alu_op        = ALU_ADD;
                alu_src_b_imm = 1'b1;
            end

            // -----------------------------------------------------------------
            // JAL: rd = PC+4, PC = PC + imm_j
            // -----------------------------------------------------------------
            OP_JAL: begin
                is_jal = 1'b1;
                imm    = imm_j;
            end

            // -----------------------------------------------------------------
            // JALR: rd = PC+4, PC = (rs1 + imm_i) & ~1
            // -----------------------------------------------------------------
            OP_JALR: begin
                is_jalr       = 1'b1;
                imm           = imm_i;
                alu_op        = ALU_ADD;
                alu_src_b_imm = 1'b1;
            end

            // -----------------------------------------------------------------
            // Branch: BEQ, BNE, BLT, BGE, BLTU, BGEU
            // -----------------------------------------------------------------
            OP_BRANCH: begin
                is_branch = 1'b1;
                imm       = imm_b;
            end

            // -----------------------------------------------------------------
            // Load: LB, LH, LW, LD, LBU, LHU, LWU
            // -----------------------------------------------------------------
            OP_LOAD: begin
                is_load       = 1'b1;
                imm           = imm_i;
                alu_op        = ALU_ADD;
                alu_src_b_imm = 1'b1;
            end

            // -----------------------------------------------------------------
            // Store: SB, SH, SW, SD
            // -----------------------------------------------------------------
            OP_STORE: begin
                is_store      = 1'b1;
                imm           = imm_s;
                alu_op        = ALU_ADD;
                alu_src_b_imm = 1'b1;
            end

            // -----------------------------------------------------------------
            // ALU immediate (64-bit): ADDI, SLTI, SLTIU, XORI, ORI, ANDI,
            //                          SLLI, SRLI, SRAI
            // -----------------------------------------------------------------
            OP_ALU_IMM: begin
                is_alu_imm    = 1'b1;
                imm           = imm_i;
                alu_src_b_imm = 1'b1;
                case (funct3)
                    3'b000: alu_op = ALU_ADD;   // ADDI
                    3'b010: alu_op = ALU_SLT;   // SLTI
                    3'b011: alu_op = ALU_SLTU;  // SLTIU
                    3'b100: alu_op = ALU_XOR;   // XORI
                    3'b110: alu_op = ALU_OR;    // ORI
                    3'b111: alu_op = ALU_AND;   // ANDI
                    3'b001: alu_op = ALU_SLL;   // SLLI (shamt in imm[5:0])
                    3'b101: begin
                        if (instr[30])
                            alu_op = ALU_SRA;   // SRAI
                        else
                            alu_op = ALU_SRL;   // SRLI
                    end
                    default: illegal_instr = 1'b1;
                endcase
            end

            // -----------------------------------------------------------------
            // ALU register (64-bit): ADD, SUB, SLL, SLT, SLTU, XOR, SRL,
            //                         SRA, OR, AND
            // -----------------------------------------------------------------
            OP_ALU_REG: begin
                is_alu_reg = 1'b1;
                case (funct3)
                    3'b000: alu_op = funct7[5] ? ALU_SUB : ALU_ADD;
                    3'b001: alu_op = ALU_SLL;
                    3'b010: alu_op = ALU_SLT;
                    3'b011: alu_op = ALU_SLTU;
                    3'b100: alu_op = ALU_XOR;
                    3'b101: alu_op = funct7[5] ? ALU_SRA : ALU_SRL;
                    3'b110: alu_op = ALU_OR;
                    3'b111: alu_op = ALU_AND;
                    default: illegal_instr = 1'b1;
                endcase
            end

            // -----------------------------------------------------------------
            // ALU immediate W (32-bit): ADDIW, SLLIW, SRLIW, SRAIW
            // -----------------------------------------------------------------
            OP_ALU_IMM_W: begin
                is_alu_imm    = 1'b1;
                is_word_op    = 1'b1;
                imm           = imm_i;
                alu_src_b_imm = 1'b1;
                case (funct3)
                    3'b000: alu_op = ALU_ADD;   // ADDIW
                    3'b001: alu_op = ALU_SLL;   // SLLIW
                    3'b101: begin
                        if (instr[30])
                            alu_op = ALU_SRA;   // SRAIW
                        else
                            alu_op = ALU_SRL;   // SRLIW
                    end
                    default: illegal_instr = 1'b1;
                endcase
            end

            // -----------------------------------------------------------------
            // ALU register W (32-bit): ADDW, SUBW, SLLW, SRLW, SRAW
            // -----------------------------------------------------------------
            OP_ALU_REG_W: begin
                is_alu_reg = 1'b1;
                is_word_op = 1'b1;
                case (funct3)
                    3'b000: alu_op = funct7[5] ? ALU_SUB : ALU_ADD;
                    3'b001: alu_op = ALU_SLL;
                    3'b101: alu_op = funct7[5] ? ALU_SRA : ALU_SRL;
                    default: illegal_instr = 1'b1;
                endcase
            end

            // -----------------------------------------------------------------
            // System: ECALL, EBREAK, CSR operations
            // -----------------------------------------------------------------
            OP_SYSTEM: begin
                is_system = 1'b1;
                imm       = imm_i;  // CSR address in imm[11:0]
            end

            // -----------------------------------------------------------------
            // FENCE: treated as NOP
            // -----------------------------------------------------------------
            OP_FENCE: begin
                is_fence = 1'b1;
            end

            // -----------------------------------------------------------------
            // Custom-0: ATOMiK instructions (decoded but not executed in Phase 1)
            // -----------------------------------------------------------------
            OP_CUSTOM0: begin
                is_custom0 = 1'b1;
            end

            default: begin
                illegal_instr = 1'b1;
            end
        endcase
    end

endmodule
