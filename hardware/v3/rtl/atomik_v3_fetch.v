// =============================================================================
// ATOMiK v3 Fetch Unit
//
// Module:      atomik_v3_fetch
// Description: Program counter and instruction fetch.
//              Drives bus to fetch 32-bit instruction at PC.
//              PC update controlled by top-level FSM.
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

/* verilator lint_off WIDTHEXPAND */

module atomik_v3_fetch (
    input  wire        clk,
    input  wire        rst_n,

    // PC control
    input  wire [63:0] pc_next,       // Next PC value
    input  wire        pc_wen,        // PC write enable
    output reg  [63:0] pc,            // Current PC

    // Instruction fetch
    input  wire        fetch_start,   // Start instruction fetch
    output reg  [31:0] instr,         // Fetched instruction
    output reg         fetch_done,    // Fetch complete

    // Bus interface (directly driven during fetch)
    output reg         fetch_bus_valid,
    input  wire        bus_ready,
    output wire [31:0] fetch_bus_addr,
    input  wire [31:0] bus_rdata
);

    parameter RESET_PC = 64'h0000_0000_0010_0000;  // Default reset vector

    // Fetch FSM
    localparam F_IDLE = 1'b0;
    localparam F_WAIT = 1'b1;

    reg fstate, fstate_next;

    assign fetch_bus_addr = pc[31:0];

    always @(posedge clk) begin
        if (!rst_n) begin
            pc <= RESET_PC;
            fstate <= F_IDLE;
        end else begin
            fstate <= fstate_next;
            if (pc_wen)
                pc <= pc_next;
        end
    end

    always @(*) begin
        fstate_next = fstate;
        case (fstate)
            F_IDLE: if (fetch_start) fstate_next = F_WAIT;
            F_WAIT: if (bus_ready)   fstate_next = F_IDLE;
            default: fstate_next = F_IDLE;
        endcase
    end

    // Instruction latch
    always @(posedge clk) begin
        if (!rst_n) begin
            instr <= 32'h00000013;  // NOP (ADDI x0, x0, 0)
        end else if (fstate == F_WAIT && bus_ready) begin
            instr <= bus_rdata;
        end
    end

    // Fetch done pulse
    always @(posedge clk) begin
        if (!rst_n)
            fetch_done <= 1'b0;
        else
            fetch_done <= (fstate == F_WAIT && bus_ready);
    end

    // Bus valid
    always @(*) begin
        fetch_bus_valid = (fstate == F_WAIT);
    end

endmodule
