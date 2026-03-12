// =============================================================================
// ATOMiK CDC Bridge — Toggle-Handshake Clock Domain Crossing
//
// Module:      atomik_cdc_bridge
// Description: Bridges AXI clock domain (100 MHz) to ATOMiK core clock domain
//              (200+ MHz) using a toggle-handshake protocol with 2FF
//              synchronizers.
//
//              The AXI-side wrapper issues operation requests (load, accum,
//              swap) with address and data. The bridge transfers these to the
//              ATOMiK core clock domain, executes the operation, and returns
//              the result (current_state, acc_zero) to the AXI side.
//
// Protocol:
//   1. AXI side latches op/addr/data, toggles req_toggle
//   2. Core side syncs req_toggle via 2FF, detects edge
//   3. Core side drives ATOMiK core for one cycle, waits for BSRAM pipeline
//   4. Core side latches response (state, acc_zero), toggles ack_toggle
//   5. AXI side syncs ack_toggle via 2FF, detects edge, captures response
//
// Latency: ~6-8 AXI clock cycles per operation (2FF sync each way + execution)
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_cdc_bridge (
    // AXI-side clock domain (100 MHz)
    input  wire        clk_axi,
    input  wire        rst_axi_n,

    // AXI-side request interface
    input  wire        req_valid,     // Pulse: new operation request
    output reg         req_ready,     // High when bridge can accept
    input  wire        req_load_en,   // Operation: LOAD
    input  wire        req_accum_en,  // Operation: ACCUM
    input  wire        req_swap_en,   // Operation: SWAP
    input  wire [7:0]  req_addr,      // Address for LOAD/SWAP
    input  wire [63:0] req_data,      // init_data for LOAD, delta for ACCUM

    // AXI-side response interface
    output reg  [63:0] resp_state,    // Current state after operation
    output reg         resp_acc_zero, // Accumulator zero flag
    output reg         resp_valid,    // Pulse: response available

    // Core-side clock domain (200+ MHz)
    input  wire        clk_core,
    input  wire        rst_core_n,

    // Core-side ATOMiK interface
    output reg         core_load_en,
    output reg         core_accum_en,
    output reg         core_swap_en,
    output reg  [7:0]  core_addr,
    output reg  [63:0] core_delta,
    output reg  [63:0] core_init_data,
    input  wire [63:0] core_current_state,
    input  wire        core_acc_zero
);

    // =========================================================================
    // AXI-Side Logic (clk_axi domain)
    // =========================================================================

    // Latched request data — held stable before toggle crosses
    reg        lat_load_en;
    reg        lat_accum_en;
    reg        lat_swap_en;
    reg [7:0]  lat_addr;
    reg [63:0] lat_data;

    // Toggle handshake signals
    reg        req_toggle;
    reg        ack_sync_0, ack_sync_1, ack_sync_2;
    wire       ack_edge;

    // Response data from core side (latched, stable when ack toggles)
    reg [63:0] core_resp_state;
    reg        core_resp_acc_zero;

    // AXI-side FSM
    localparam AXI_IDLE = 2'd0;
    localparam AXI_WAIT = 2'd1;
    localparam AXI_DONE = 2'd2;
    reg [1:0]  axi_state;

    // 2FF synchronizer: ack_toggle → AXI domain
    always @(posedge clk_axi or negedge rst_axi_n) begin
        if (!rst_axi_n) begin
            ack_sync_0 <= 1'b0;
            ack_sync_1 <= 1'b0;
            ack_sync_2 <= 1'b0;
        end else begin
            ack_sync_0 <= ack_toggle;
            ack_sync_1 <= ack_sync_0;
            ack_sync_2 <= ack_sync_1;
        end
    end

    assign ack_edge = ack_sync_1 ^ ack_sync_2;

    // AXI-side FSM
    always @(posedge clk_axi or negedge rst_axi_n) begin
        if (!rst_axi_n) begin
            axi_state     <= AXI_IDLE;
            req_toggle    <= 1'b0;
            req_ready     <= 1'b1;
            resp_valid    <= 1'b0;
            resp_state    <= 64'b0;
            resp_acc_zero <= 1'b0;
            lat_load_en   <= 1'b0;
            lat_accum_en  <= 1'b0;
            lat_swap_en   <= 1'b0;
            lat_addr      <= 8'b0;
            lat_data      <= 64'b0;
        end else begin
            resp_valid <= 1'b0;  // Default: one-cycle pulse

            case (axi_state)
                AXI_IDLE: begin
                    if (req_valid && req_ready) begin
                        // Latch request data (must be stable before toggle)
                        lat_load_en  <= req_load_en;
                        lat_accum_en <= req_accum_en;
                        lat_swap_en  <= req_swap_en;
                        lat_addr     <= req_addr;
                        lat_data     <= req_data;
                        // Toggle to signal core side
                        req_toggle   <= ~req_toggle;
                        req_ready    <= 1'b0;
                        axi_state    <= AXI_WAIT;
                    end
                end

                AXI_WAIT: begin
                    if (ack_edge) begin
                        // Core side completed — capture response
                        resp_state    <= core_resp_state;
                        resp_acc_zero <= core_resp_acc_zero;
                        resp_valid    <= 1'b1;
                        axi_state     <= AXI_DONE;
                    end
                end

                AXI_DONE: begin
                    // One cycle gap then re-arm
                    req_ready <= 1'b1;
                    axi_state <= AXI_IDLE;
                end

                default: axi_state <= AXI_IDLE;
            endcase
        end
    end

    // =========================================================================
    // Core-Side Logic (clk_core domain)
    // =========================================================================

    // 2FF synchronizer: req_toggle → core domain
    reg        req_sync_0, req_sync_1, req_sync_2;
    wire       req_edge;

    // Acknowledge toggle
    reg        ack_toggle;

    // Core-side FSM
    // Pipeline depth: 9 core cycles after firing to handle Zynq-optimized SWAP
    // with 4-stage pipeline + BRAM output register (READ_LATENCY=2).
    //
    // SWAP timeline (atomik_core_zynq 4-stage pipeline):
    //   EXEC:  CDC fires swap_en via NBA (core sees it next cycle)
    //   WAIT1: Core sees swap_en, active_addr updates, swap_s1 <= 1
    //   WAIT2: swap_s1: BRAM array read in progress, swap_s2 <= 1
    //   WAIT3: swap_s2: BRAM output register captures data, swap_s3 <= 1
    //   WAIT4: swap_s3: mem_rd_data valid, swap_data computed, swap_s4 <= 1
    //   WAIT5: swap_s4: BRAM write-back, accumulator clears
    //   WAIT6: BRAM array re-reads updated state_table
    //   WAIT7: BRAM output register captures post-swap data
    //   SETTLE: mem_rd_data valid → latch core_current_state
    //
    // LOAD/ACCUM complete earlier but use the same wait depth (deterministic).
    localparam CORE_IDLE   = 4'd0;
    localparam CORE_EXEC   = 4'd1;
    localparam CORE_WAIT1  = 4'd2;  // Core sees enable pulse
    localparam CORE_WAIT2  = 4'd3;  // BRAM array reading
    localparam CORE_WAIT3  = 4'd4;  // BRAM output register
    localparam CORE_WAIT4  = 4'd5;  // SWAP data computed
    localparam CORE_WAIT5  = 4'd6;  // SWAP write-back + acc clear
    localparam CORE_WAIT6  = 4'd7;  // BRAM array re-read
    localparam CORE_WAIT7  = 4'd8;  // BRAM output register (post-swap)
    localparam CORE_SETTLE = 4'd9;  // mem_rd_data valid → latch
    localparam CORE_ACK    = 4'd10;
    reg [3:0]  core_state;

    always @(posedge clk_core or negedge rst_core_n) begin
        if (!rst_core_n) begin
            req_sync_0 <= 1'b0;
            req_sync_1 <= 1'b0;
            req_sync_2 <= 1'b0;
        end else begin
            req_sync_0 <= req_toggle;
            req_sync_1 <= req_sync_0;
            req_sync_2 <= req_sync_1;
        end
    end

    assign req_edge = req_sync_1 ^ req_sync_2;

    // Core-side FSM: detect request, fire operation, wait for BSRAM, latch, ack
    always @(posedge clk_core or negedge rst_core_n) begin
        if (!rst_core_n) begin
            core_state        <= CORE_IDLE;
            ack_toggle        <= 1'b0;
            core_load_en      <= 1'b0;
            core_accum_en     <= 1'b0;
            core_swap_en      <= 1'b0;
            core_addr         <= 8'b0;
            core_delta        <= 64'b0;
            core_init_data    <= 64'b0;
            core_resp_state   <= 64'b0;
            core_resp_acc_zero <= 1'b0;
            // Note: core_state is 4 bits
        end else begin
            // Default: clear one-cycle enable pulses
            core_load_en  <= 1'b0;
            core_accum_en <= 1'b0;
            core_swap_en  <= 1'b0;

            case (core_state)
                CORE_IDLE: begin
                    if (req_edge) begin
                        // New request — fire the operation
                        core_load_en   <= lat_load_en;
                        core_accum_en  <= lat_accum_en;
                        core_swap_en   <= lat_swap_en;
                        core_addr      <= lat_addr;
                        core_delta     <= lat_data;
                        core_init_data <= lat_data;
                        core_state     <= CORE_EXEC;
                    end
                end

                CORE_EXEC: begin
                    core_state <= CORE_WAIT1;
                end

                CORE_WAIT1: begin
                    core_state <= CORE_WAIT2;
                end

                CORE_WAIT2: begin
                    core_state <= CORE_WAIT3;
                end

                CORE_WAIT3: begin
                    core_state <= CORE_WAIT4;
                end

                CORE_WAIT4: begin
                    core_state <= CORE_WAIT5;
                end

                CORE_WAIT5: begin
                    core_state <= CORE_WAIT6;
                end

                CORE_WAIT6: begin
                    core_state <= CORE_WAIT7;
                end

                CORE_WAIT7: begin
                    core_state <= CORE_SETTLE;
                end

                CORE_SETTLE: begin
                    // mem_rd_data valid — latch response
                    core_resp_state    <= core_current_state;
                    core_resp_acc_zero <= core_acc_zero;
                    // Toggle ack to signal AXI side
                    ack_toggle <= ~ack_toggle;
                    core_state <= CORE_ACK;
                end

                CORE_ACK: begin
                    // One cycle gap before accepting next request
                    core_state <= CORE_IDLE;
                end

                default: core_state <= CORE_IDLE;
            endcase
        end
    end

endmodule
