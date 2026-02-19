// =============================================================================
// ATOMiK v3 CSR Unit
//
// Module:      atomik_v3_csr
// Description: Minimal CSR implementation for RV64I compliance.
//              Supports: misa, mhartid, mcycle, minstret, mtvec, mepc, mcause,
//                        mscratch, mstatus, mie, mip
//              CSRRW/CSRRS/CSRRC/CSRRWI/CSRRSI/CSRRCI
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

/* verilator lint_off UNUSEDSIGNAL */

module atomik_v3_csr (
    input  wire        clk,
    input  wire        rst_n,

    // CSR access
    input  wire [11:0] csr_addr,      // CSR address
    input  wire [63:0] csr_wdata,     // Write data (rs1 or zimm)
    input  wire [2:0]  csr_op,        // funct3: operation type
    input  wire        csr_wen,       // CSR write enable
    output reg  [63:0] csr_rdata,     // Read data

    // Trap interface
    input  wire        trap_enter,    // Enter trap (ECALL/EBREAK/illegal)
    input  wire [63:0] trap_pc,       // PC of trapping instruction
    input  wire [63:0] trap_cause,    // Cause code
    input  wire        trap_return,   // MRET
    output wire [63:0] mtvec_out,     // Trap vector address
    output wire [63:0] mepc_out,      // Return address from trap

    // Counters
    input  wire        instr_retire   // Increment minstret
);

    // CSR addresses (only writable/non-zero CSRs need address constants)
    localparam ADDR_MSTATUS  = 12'h300;
    localparam ADDR_MISA     = 12'h301;
    localparam ADDR_MTVEC    = 12'h305;
    localparam ADDR_MSCRATCH = 12'h340;
    localparam ADDR_MEPC     = 12'h341;
    localparam ADDR_MCAUSE   = 12'h342;

    // CSR operation encoding (funct3)
    localparam CSR_RW  = 3'b001;  // CSRRW
    localparam CSR_RS  = 3'b010;  // CSRRS
    localparam CSR_RC  = 3'b011;  // CSRRC
    localparam CSR_RWI = 3'b101;  // CSRRWI
    localparam CSR_RSI = 3'b110;  // CSRRSI
    localparam CSR_RCI = 3'b111;  // CSRRCI

    // CSR registers
    // mstatus: only MIE (bit 3), MPIE (bit 7), MPP (bits 12:11) are writable
    reg        mstatus_mie;
    reg        mstatus_mpie;
    reg  [1:0] mstatus_mpp;
    wire [63:0] mstatus_read = {51'b0, mstatus_mpp, 3'b0, mstatus_mpie, 3'b0, mstatus_mie, 3'b0};

    reg [63:0] mtvec;
    reg [63:0] mscratch;
    reg [63:0] mepc;
    reg [63:0] mcause;
    // mtval: hardwired to zero (spec allows this)
    // mie/mip: hardwired to zero (no interrupt sources)
    // mcycle/minstret: read-only zero for now (saves ~300 LUT from LUT-based adders)
    // Can be re-enabled with writable counters when SoC integration needs them

    // misa: RV64I, no extensions
    // MXL = 2 (64-bit), extensions bit 8 (I) set
    wire [63:0] misa = {2'b10, 36'b0, 26'b00000000000000000100000000};

    assign mtvec_out = mtvec;
    assign mepc_out  = mepc;

    // =========================================================================
    // CSR read
    // =========================================================================
    always @(*) begin
        case (csr_addr)
            ADDR_MSTATUS:  csr_rdata = mstatus_read;
            ADDR_MISA:     csr_rdata = misa;
            ADDR_MTVEC:    csr_rdata = mtvec;
            ADDR_MSCRATCH: csr_rdata = mscratch;
            ADDR_MEPC:     csr_rdata = mepc;
            ADDR_MCAUSE:   csr_rdata = mcause;
            // mie, mip, mtval, mcycle, minstret, mhartid: all read as zero
            default:       csr_rdata = 64'b0;
        endcase
    end

    // =========================================================================
    // CSR write value computation
    // =========================================================================
    reg [63:0] csr_new_val;
    always @(*) begin
        case (csr_op)
            CSR_RW, CSR_RWI: csr_new_val = csr_wdata;
            CSR_RS, CSR_RSI: csr_new_val = csr_rdata | csr_wdata;
            CSR_RC, CSR_RCI: csr_new_val = csr_rdata & ~csr_wdata;
            default:         csr_new_val = csr_rdata;
        endcase
    end

    // =========================================================================
    // CSR write + counters + traps
    // =========================================================================
    always @(posedge clk) begin
        if (!rst_n) begin
            mstatus_mie  <= 1'b0;
            mstatus_mpie <= 1'b0;
            mstatus_mpp  <= 2'b0;
            mtvec    <= 64'b0;
            mscratch <= 64'b0;
            mepc     <= 64'b0;
            mcause   <= 64'b0;
        end else begin
            // Trap entry
            if (trap_enter) begin
                mepc   <= trap_pc;
                mcause <= trap_cause;
                mstatus_mpie <= mstatus_mie;  // MPIE = MIE
                mstatus_mie  <= 1'b0;         // MIE = 0
                mstatus_mpp  <= 2'b11;        // MPP = M-mode
            end

            // Trap return (MRET)
            if (trap_return) begin
                mstatus_mie  <= mstatus_mpie; // MIE = MPIE
                mstatus_mpie <= 1'b1;         // MPIE = 1
            end

            // CSR write (lower priority than trap)
            if (csr_wen && !trap_enter) begin
                case (csr_addr)
                    ADDR_MSTATUS: begin
                        mstatus_mie  <= csr_new_val[3];
                        mstatus_mpie <= csr_new_val[7];
                        mstatus_mpp  <= csr_new_val[12:11];
                    end
                    // mie, mip, mtval, mcycle, minstret: writes ignored
                    ADDR_MTVEC:    mtvec    <= csr_new_val;
                    ADDR_MSCRATCH: mscratch <= csr_new_val;
                    ADDR_MEPC:     mepc     <= csr_new_val;
                    ADDR_MCAUSE:   mcause   <= csr_new_val;
                    default: ; // Read-only or unimplemented
                endcase
            end
        end
    end

endmodule
