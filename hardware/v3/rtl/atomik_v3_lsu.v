// =============================================================================
// ATOMiK v3 Load/Store Unit
//
// Module:      atomik_v3_lsu
// Description: Load/store unit with 64-to-32-bit bus adapter.
//              Handles LB/LH/LW/LD/LBU/LHU/LWU/SB/SH/SW/SD.
//              64-bit accesses are split into two 32-bit bus transactions.
//
// Bus protocol: PicoRV32-compatible valid/ready handshake.
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

/* verilator lint_off UNUSEDSIGNAL */

module atomik_v3_lsu (
    input  wire        clk,
    input  wire        rst_n,

    // CPU interface
    input  wire        lsu_start,       // Start a load/store
    input  wire        lsu_is_store,    // 1 = store, 0 = load
    input  wire [2:0]  lsu_size,        // funct3: size + sign
    input  wire [63:0] lsu_addr,        // Effective address (rs1 + imm)
    input  wire [63:0] lsu_wdata,       // Store data (rs2 value)
    output reg  [63:0] lsu_rdata,       // Load result (aligned, sign-extended)
    output reg         lsu_done,        // Transaction complete

    // 32-bit bus interface (directly driven, no arbitration here)
    output reg         bus_valid,
    input  wire        bus_ready,
    output reg  [31:0] bus_addr,
    output reg  [31:0] bus_wdata,
    output reg  [3:0]  bus_wstrb,
    input  wire [31:0] bus_rdata
);

    // Size encoding (from funct3)
    localparam SIZE_B  = 3'b000;  // LB / SB
    localparam SIZE_H  = 3'b001;  // LH / SH
    localparam SIZE_W  = 3'b010;  // LW / SW
    localparam SIZE_D  = 3'b011;  // LD / SD
    localparam SIZE_BU = 3'b100;  // LBU
    localparam SIZE_HU = 3'b101;  // LHU
    localparam SIZE_WU = 3'b110;  // LWU

    // FSM states
    localparam S_IDLE  = 2'd0;
    localparam S_XACT1 = 2'd1;  // First (or only) bus transaction
    localparam S_XACT2 = 2'd2;  // Second transaction (64-bit only)
    localparam S_DONE  = 2'd3;

    reg [1:0] state, state_next;

    // Latched request
    reg        req_is_store;
    reg [2:0]  req_size;
    reg [63:0] req_addr;
    reg [63:0] req_wdata;
    reg [31:0] rdata_lo;   // First 32-bit read result (for 64-bit loads)

    // Need two transactions?
    wire needs_two = (lsu_size == SIZE_D);
    reg  req_needs_two;

    // Byte offset within word
    wire [1:0] byte_off = req_addr[1:0];

    // =========================================================================
    // FSM
    // =========================================================================
    always @(posedge clk) begin
        if (!rst_n)
            state <= S_IDLE;
        else
            state <= state_next;
    end

    always @(*) begin
        state_next = state;
        case (state)
            S_IDLE:  if (lsu_start) state_next = S_XACT1;
            S_XACT1: if (bus_valid && bus_ready) state_next = req_needs_two ? S_XACT2 : S_DONE;
            S_XACT2: if (bus_valid && bus_ready) state_next = S_DONE;
            S_DONE:  state_next = S_IDLE;
            default: state_next = S_IDLE;
        endcase
    end

    // =========================================================================
    // Latch request on start
    // =========================================================================
    always @(posedge clk) begin
        if (!rst_n) begin
            req_is_store  <= 1'b0;
            req_size      <= 3'b0;
            req_addr      <= 64'b0;
            req_wdata     <= 64'b0;
            req_needs_two <= 1'b0;
        end else if (lsu_start && state == S_IDLE) begin
            req_is_store  <= lsu_is_store;
            req_size      <= lsu_size;
            req_addr      <= lsu_addr;
            req_wdata     <= lsu_wdata;
            req_needs_two <= needs_two;
        end
    end

    // =========================================================================
    // Latch first word of 64-bit read
    // =========================================================================
    always @(posedge clk) begin
        if (!rst_n)
            rdata_lo <= 32'b0;
        else if (state == S_XACT1 && bus_ready && !req_is_store && req_needs_two)
            rdata_lo <= bus_rdata;
    end

    // =========================================================================
    // Bus signals (REGISTERED to prevent glitches - see KNOWN_ISSUES V3-015)
    // =========================================================================
    always @(posedge clk) begin
        if (!rst_n) begin
            bus_valid <= 1'b0;
            bus_addr  <= 32'b0;
            bus_wdata <= 32'b0;
            bus_wstrb <= 4'b0;
        end else begin
            case (state_next)  // Use NEXT state to avoid one-cycle lag
                S_IDLE: begin
                    bus_valid <= 1'b0;
                    bus_wstrb <= 4'b0;
                end

                S_XACT1: begin
                    bus_valid <= 1'b1;
                    bus_addr  <= lsu_addr[31:0] & 32'hFFFFFFFC;  // Word-aligned

                    if (lsu_is_store) begin
                        case (lsu_size)
                            SIZE_B: begin
                                bus_wdata <= {4{lsu_wdata[7:0]}};
                                bus_wstrb <= 4'b0001 << lsu_addr[1:0];
                            end
                            SIZE_H: begin
                                bus_wdata <= {2{lsu_wdata[15:0]}};
                                bus_wstrb <= 4'b0011 << lsu_addr[1:0];
                            end
                            SIZE_W: begin
                                bus_wdata <= lsu_wdata[31:0];
                                bus_wstrb <= 4'b1111;
                            end
                            SIZE_D: begin
                                bus_wdata <= lsu_wdata[31:0];  // Lower word first
                                bus_wstrb <= 4'b1111;
                            end
                            default: begin
                                bus_wdata <= lsu_wdata[31:0];
                                bus_wstrb <= 4'b1111;
                            end
                        endcase
                    end else begin
                        bus_wstrb <= 4'b0;  // Reads: no write strobes
                    end
                end

                S_XACT2: begin
                    bus_valid <= 1'b1;
                    bus_addr  <= (req_addr[31:0] + 32'd4) & 32'hFFFFFFFC;  // Next word

                    if (req_is_store) begin
                        bus_wdata <= req_wdata[63:32];  // Upper word
                        bus_wstrb <= 4'b1111;
                    end else begin
                        bus_wstrb <= 4'b0;  // Reads: no write strobes
                    end
                end

                S_DONE: begin
                    bus_valid <= 1'b0;
                    bus_wstrb <= 4'b0;
                end

                default: begin
                    bus_valid <= 1'b0;
                    bus_wstrb <= 4'b0;
                end
            endcase
        end
    end

    // =========================================================================
    // Done signal
    // =========================================================================
    always @(posedge clk) begin
        if (!rst_n)
            lsu_done <= 1'b0;
        else
            lsu_done <= (state == S_DONE);
    end

    // =========================================================================
    // Load data alignment and sign extension
    // =========================================================================
    // For single-transaction loads, result is from bus_rdata captured in DONE
    // For double-transaction loads, result is {bus_rdata, rdata_lo}
    reg [31:0] last_rdata;
    always @(posedge clk) begin
        if (!rst_n)
            last_rdata <= 32'b0;
        else if ((state == S_XACT1 || state == S_XACT2) && bus_valid && bus_ready && !req_is_store)
            last_rdata <= bus_rdata;
    end

    // Extract the relevant byte/halfword from the word
    wire [31:0] shifted_rdata = last_rdata >> (byte_off * 8);
    wire [7:0]  byte_val  = shifted_rdata[7:0];
    wire [15:0] half_val  = shifted_rdata[15:0];
    wire [31:0] word_val  = last_rdata;

    // lsu_rdata is always valid after a load completes (last_rdata holds data).
    // The writeback mux only selects this when wb_src == LOAD.
    always @(*) begin
        case (req_size)
            SIZE_B:  lsu_rdata = {{56{byte_val[7]}}, byte_val};
            SIZE_H:  lsu_rdata = {{48{half_val[15]}}, half_val};
            SIZE_W:  lsu_rdata = {{32{word_val[31]}}, word_val};
            SIZE_D:  lsu_rdata = {last_rdata, rdata_lo};
            SIZE_BU: lsu_rdata = {56'b0, byte_val};
            SIZE_HU: lsu_rdata = {48'b0, half_val};
            SIZE_WU: lsu_rdata = {32'b0, word_val};
            default: lsu_rdata = 64'b0;
        endcase
    end

endmodule
