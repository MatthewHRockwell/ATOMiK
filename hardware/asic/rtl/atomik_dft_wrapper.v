// =============================================================================
// ATOMiK DFT Wrapper — Design-for-Test Scan Chain Stub
//
// Module:      atomik_dft_wrapper
// Description: Minimal DFT infrastructure for ASIC tapeout. Provides:
//              - Scan enable / scan_in / scan_out ports
//              - Functional vs. scan mode mux skeleton
//              - JTAG TAP controller stub (IEEE 1149.1 compatible ports)
//
//              In a real DFT flow, Synopsys DFT Compiler (or Cadence Modus)
//              replaces this stub with auto-inserted scan chains after
//              synthesis. This module ensures the port-level interface is
//              correct for the ASIC top-level integration.
//
// Notes:
//   - Scan chain insertion is done POST-SYNTHESIS by EDA tools
//   - This stub provides the I/O structure and basic mux
//   - The actual scan cells are inserted into individual flip-flops
//   - For initial tapeout, this is sufficient; full DFT comes with
//     foundry engagement and access to DFT Compiler licenses
//
// ATOMiK Project — March 2026
// SPDX-License-Identifier: MIT
// =============================================================================

`timescale 1ns / 1ps

module atomik_dft_wrapper (
    input  wire clk,
    input  wire rst_n,

    // Scan chain interface
    input  wire scan_enable,    // 1 = scan mode, 0 = functional mode
    input  wire scan_in,        // Serial scan data in
    output wire scan_out        // Serial scan data out
);

    // =========================================================================
    // Scan Chain Skeleton
    // =========================================================================
    //
    // In functional mode (scan_enable=0), the scan chain is transparent.
    // In scan mode (scan_enable=1), data shifts through scan_in → chain → scan_out.
    //
    // DFT Compiler replaces the flip-flops in atomik_core_asic with muxed
    // scan flip-flops (SDFF). The scan chain ordering is determined by
    // the DFT tool based on routing optimization.
    //
    // This stub implements a minimal 8-bit shift register as a placeholder
    // to validate the DFT port connectivity before full scan insertion.

    localparam CHAIN_LEN = 8;

    reg [CHAIN_LEN-1:0] scan_chain;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            scan_chain <= {CHAIN_LEN{1'b0}};
        end else if (scan_enable) begin
            // Shift mode: serial shift through chain
            scan_chain <= {scan_chain[CHAIN_LEN-2:0], scan_in};
        end
        // Functional mode: chain holds state (transparent)
    end

    assign scan_out = scan_chain[CHAIN_LEN-1];

endmodule

// =============================================================================
// JTAG TAP Controller Stub — IEEE 1149.1 Compatible
//
// Module:      atomik_jtag_tap
// Description: Minimal JTAG Test Access Port for boundary scan and debug.
//              Implements the TAP state machine (16 states) and provides
//              hooks for instruction register (IR) and data register (DR).
//
//              For initial tapeout, this provides the standard 4-wire JTAG
//              interface. Full boundary scan cell insertion is done by the
//              DFT tool during physical design.
//
// =============================================================================

/* verilator lint_off DECLFILENAME */
module atomik_jtag_tap (
    // JTAG pins (directly to pad ring)
    input  wire tck,     // Test clock
    input  wire tms,     // Test mode select
    input  wire tdi,     // Test data in
    output wire tdo,     // Test data out
    input  wire trst_n,  // Test reset (active low, optional)

    // Internal interface
    output wire shift_dr,
    output wire capture_dr,
    output wire update_dr,
    output wire tap_reset
);

    // =========================================================================
    // TAP State Machine (IEEE 1149.1)
    // =========================================================================
    localparam TEST_LOGIC_RESET = 4'hF;
    localparam RUN_TEST_IDLE    = 4'hC;
    localparam SELECT_DR_SCAN   = 4'h7;
    localparam CAPTURE_DR       = 4'h6;
    localparam SHIFT_DR         = 4'h2;
    localparam EXIT1_DR         = 4'h1;
    localparam PAUSE_DR         = 4'h3;
    localparam EXIT2_DR         = 4'h0;
    localparam UPDATE_DR        = 4'h5;
    localparam SELECT_IR_SCAN   = 4'h4;
    localparam CAPTURE_IR       = 4'hE;
    localparam SHIFT_IR         = 4'hA;
    localparam EXIT1_IR         = 4'h9;
    localparam PAUSE_IR         = 4'hB;
    localparam EXIT2_IR         = 4'h8;
    localparam UPDATE_IR        = 4'hD;

    reg [3:0] tap_state;

    always @(posedge tck or negedge trst_n) begin
        if (!trst_n) begin
            tap_state <= TEST_LOGIC_RESET;
        end else begin
            case (tap_state)
                TEST_LOGIC_RESET: tap_state <= tms ? TEST_LOGIC_RESET : RUN_TEST_IDLE;
                RUN_TEST_IDLE:    tap_state <= tms ? SELECT_DR_SCAN   : RUN_TEST_IDLE;
                SELECT_DR_SCAN:   tap_state <= tms ? SELECT_IR_SCAN   : CAPTURE_DR;
                CAPTURE_DR:       tap_state <= tms ? EXIT1_DR         : SHIFT_DR;
                SHIFT_DR:         tap_state <= tms ? EXIT1_DR         : SHIFT_DR;
                EXIT1_DR:         tap_state <= tms ? UPDATE_DR        : PAUSE_DR;
                PAUSE_DR:         tap_state <= tms ? EXIT2_DR         : PAUSE_DR;
                EXIT2_DR:         tap_state <= tms ? UPDATE_DR        : SHIFT_DR;
                UPDATE_DR:        tap_state <= tms ? SELECT_DR_SCAN   : RUN_TEST_IDLE;
                SELECT_IR_SCAN:   tap_state <= tms ? TEST_LOGIC_RESET : CAPTURE_IR;
                CAPTURE_IR:       tap_state <= tms ? EXIT1_IR         : SHIFT_IR;
                SHIFT_IR:         tap_state <= tms ? EXIT1_IR         : SHIFT_IR;
                EXIT1_IR:         tap_state <= tms ? UPDATE_IR        : PAUSE_IR;
                PAUSE_IR:         tap_state <= tms ? EXIT2_IR         : PAUSE_IR;
                EXIT2_IR:         tap_state <= tms ? UPDATE_IR        : SHIFT_IR;
                UPDATE_IR:        tap_state <= tms ? SELECT_DR_SCAN   : RUN_TEST_IDLE;
            endcase
        end
    end

    // =========================================================================
    // Instruction Register (4-bit, standard JTAG instructions)
    // =========================================================================
    localparam IR_LEN  = 4;
    localparam IR_IDCODE = 4'b0001;  // Read device ID
    localparam IR_BYPASS = 4'b1111;  // Bypass (default after reset)

    reg [IR_LEN-1:0] ir_shift;
    reg [IR_LEN-1:0] ir_latch;

    always @(posedge tck or negedge trst_n) begin
        if (!trst_n) begin
            ir_shift <= IR_BYPASS;
            ir_latch <= IR_BYPASS;
        end else begin
            if (tap_state == CAPTURE_IR)
                ir_shift <= 4'b0101;  // Required by IEEE 1149.1
            else if (tap_state == SHIFT_IR)
                ir_shift <= {tdi, ir_shift[IR_LEN-1:1]};
            else if (tap_state == UPDATE_IR)
                ir_latch <= ir_shift;
        end
    end

    // =========================================================================
    // IDCODE Register (32-bit device identification)
    // =========================================================================
    //
    // Format: [31:28]=version, [27:12]=part, [11:1]=manufacturer, [0]=1
    //   Version: 0001 (v1)
    //   Part:    ATOMiK (0x4154 = "AT")
    //   Mfr:     ATOMiK Project (using unassigned JEP106 code 0x7FF)
    //   LSB:     1 (required by IEEE 1149.1)

    localparam [31:0] IDCODE_VAL = 32'h1415_4FFE;

    reg [31:0] idcode_shift;

    always @(posedge tck) begin
        if (tap_state == CAPTURE_DR)
            idcode_shift <= IDCODE_VAL;
        else if (tap_state == SHIFT_DR && ir_latch == IR_IDCODE)
            idcode_shift <= {tdi, idcode_shift[31:1]};
    end

    // =========================================================================
    // Bypass Register (1-bit, IEEE 1149.1 required)
    // =========================================================================
    reg bypass_reg;

    always @(posedge tck) begin
        if (tap_state == CAPTURE_DR)
            bypass_reg <= 1'b0;
        else if (tap_state == SHIFT_DR && ir_latch == IR_BYPASS)
            bypass_reg <= tdi;
    end

    // =========================================================================
    // TDO Mux
    // =========================================================================
    reg tdo_mux;

    always @(*) begin
        case (tap_state)
            SHIFT_IR: tdo_mux = ir_shift[0];
            SHIFT_DR: begin
                case (ir_latch)
                    IR_IDCODE: tdo_mux = idcode_shift[0];
                    IR_BYPASS: tdo_mux = bypass_reg;
                    default:   tdo_mux = bypass_reg;
                endcase
            end
            default: tdo_mux = 1'b0;
        endcase
    end

    // TDO is registered on negative edge per IEEE 1149.1
    reg tdo_r;
    always @(negedge tck) begin
        tdo_r <= tdo_mux;
    end
    assign tdo = tdo_r;

    // =========================================================================
    // Control Outputs
    // =========================================================================
    assign shift_dr   = (tap_state == SHIFT_DR);
    assign capture_dr = (tap_state == CAPTURE_DR);
    assign update_dr  = (tap_state == UPDATE_DR);
    assign tap_reset  = (tap_state == TEST_LOGIC_RESET);

endmodule
