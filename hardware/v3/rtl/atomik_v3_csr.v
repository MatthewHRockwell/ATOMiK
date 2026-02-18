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

    // CSR addresses
    localparam ADDR_MSTATUS  = 12'h300;
    localparam ADDR_MISA     = 12'h301;
    localparam ADDR_MIE      = 12'h304;
    localparam ADDR_MTVEC    = 12'h305;
    localparam ADDR_MSCRATCH = 12'h340;
    localparam ADDR_MEPC     = 12'h341;
    localparam ADDR_MCAUSE   = 12'h342;
    localparam ADDR_MTVAL    = 12'h343;
    localparam ADDR_MIP      = 12'h344;
    localparam ADDR_MCYCLE   = 12'hB00;
    localparam ADDR_MINSTRET = 12'hB02;
    // RV32 compat aliases — not needed for RV64 but reserved for future
    // localparam ADDR_MCYCLEH  = 12'hB80;
    // localparam ADDR_MINSTRETH = 12'hB82;

    // CSR operation encoding (funct3)
    localparam CSR_RW  = 3'b001;  // CSRRW
    localparam CSR_RS  = 3'b010;  // CSRRS
    localparam CSR_RC  = 3'b011;  // CSRRC
    localparam CSR_RWI = 3'b101;  // CSRRWI
    localparam CSR_RSI = 3'b110;  // CSRRSI
    localparam CSR_RCI = 3'b111;  // CSRRCI

    // CSR registers
    reg [63:0] mstatus;
    reg [63:0] mtvec;
    reg [63:0] mscratch;
    reg [63:0] mepc;
    reg [63:0] mcause;
    reg [63:0] mtval;
    reg [63:0] mie;
    reg [63:0] mip;
    reg [63:0] mcycle;
    reg [63:0] minstret;

    // misa: RV64I, no extensions
    // MXL = 2 (64-bit), extensions bit 8 (I) set
    wire [63:0] misa = {2'b10, 36'b0, 26'b00000000000000000100000000};

    // mhartid: always 0
    wire [63:0] mhartid = 64'b0;

    assign mtvec_out = mtvec;
    assign mepc_out  = mepc;

    // =========================================================================
    // CSR read
    // =========================================================================
    always @(*) begin
        case (csr_addr)
            ADDR_MSTATUS:  csr_rdata = mstatus;
            ADDR_MISA:     csr_rdata = misa;
            ADDR_MIE:      csr_rdata = mie;
            ADDR_MTVEC:    csr_rdata = mtvec;
            ADDR_MSCRATCH: csr_rdata = mscratch;
            ADDR_MEPC:     csr_rdata = mepc;
            ADDR_MCAUSE:   csr_rdata = mcause;
            ADDR_MTVAL:    csr_rdata = mtval;
            ADDR_MIP:      csr_rdata = mip;
            ADDR_MCYCLE:   csr_rdata = mcycle;
            ADDR_MINSTRET: csr_rdata = minstret;
            12'hF14:       csr_rdata = mhartid;  // mhartid
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
            mstatus  <= 64'b0;
            mtvec    <= 64'b0;
            mscratch <= 64'b0;
            mepc     <= 64'b0;
            mcause   <= 64'b0;
            mtval    <= 64'b0;
            mie      <= 64'b0;
            mip      <= 64'b0;
            mcycle   <= 64'b0;
            minstret <= 64'b0;
        end else begin
            // mcycle always increments
            mcycle <= mcycle + 64'd1;

            // minstret increments on retire
            if (instr_retire)
                minstret <= minstret + 64'd1;

            // Trap entry
            if (trap_enter) begin
                mepc   <= trap_pc;
                mcause <= trap_cause;
                // Set MPIE = MIE, MIE = 0
                mstatus[7]  <= mstatus[3];  // MPIE = MIE
                mstatus[3]  <= 1'b0;        // MIE = 0
                mstatus[12:11] <= 2'b11;    // MPP = M-mode
            end

            // Trap return (MRET)
            if (trap_return) begin
                // Restore MIE from MPIE
                mstatus[3] <= mstatus[7];  // MIE = MPIE
                mstatus[7] <= 1'b1;        // MPIE = 1
            end

            // CSR write (lower priority than trap)
            if (csr_wen && !trap_enter) begin
                case (csr_addr)
                    ADDR_MSTATUS:  mstatus  <= csr_new_val;
                    ADDR_MIE:      mie      <= csr_new_val;
                    ADDR_MTVEC:    mtvec    <= csr_new_val;
                    ADDR_MSCRATCH: mscratch <= csr_new_val;
                    ADDR_MEPC:     mepc     <= csr_new_val;
                    ADDR_MCAUSE:   mcause   <= csr_new_val;
                    ADDR_MTVAL:    mtval    <= csr_new_val;
                    ADDR_MIP:      mip      <= csr_new_val;
                    ADDR_MCYCLE:   mcycle   <= csr_new_val;
                    ADDR_MINSTRET: minstret <= csr_new_val;
                    default: ; // Read-only or unimplemented
                endcase
            end
        end
    end

endmodule
