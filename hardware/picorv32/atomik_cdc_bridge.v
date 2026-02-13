// =============================================================================
// ATOMiK CDC Bridge — Toggle-Handshake Clock Domain Crossing
//
// Module:      atomik_cdc_bridge
// Description: Bridges PicoRV32 bus domain (25.2 MHz) to ATOMiK core domain
//              (81 MHz) using a toggle-handshake protocol. Only 1 bit crosses
//              in each direction. Data is latched and stable before toggle.
//
// Protocol:
//   1. Bus side latches addr/wdata/wstrb, toggles req_toggle
//   2. Core side syncs req_toggle via 2FF, detects edge
//   3. Core side executes operation on atomik_parallel_acc
//   4. Core side latches result, toggles ack_toggle
//   5. Bus side syncs ack_toggle via 2FF, detects edge, drives mem_ready
//
// Latency: ~3 bus cycles per MMIO access
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   February 12, 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_cdc_bridge #(
    parameter DATA_WIDTH  = 32,
    parameter N_BANKS     = 4
)(
    // Bus-side clock domain (25.2 MHz)
    input  wire                    clk_bus,
    input  wire                    rst_bus_n,

    // PicoRV32 Bus Interface (bus domain)
    input  wire                    mem_valid,
    input  wire [31:0]             mem_addr,
    input  wire [31:0]             mem_wdata,
    input  wire [3:0]              mem_wstrb,
    output reg                     mem_ready,
    output reg  [31:0]             mem_rdata,

    // Core-side clock domain (81 MHz)
    input  wire                    clk_core,
    input  wire                    rst_core_n,

    // ATOMiK parallel_acc interface (core domain)
    output reg  [DATA_WIDTH-1:0]   core_initial_state_in,
    output reg                     core_load_initial,
    output reg  [DATA_WIDTH-1:0]   core_delta_in,
    output reg                     core_delta_valid,
    output reg                     core_soft_reset,
    input  wire [DATA_WIDTH-1:0]   core_current_state,
    input  wire [DATA_WIDTH-1:0]   core_merged_accumulator,
    input  wire                    core_accumulator_zero,
    input  wire [DATA_WIDTH-1:0]   core_initial_state_out
);

    // =========================================================================
    // Bus-Side Logic (clk_bus domain)
    // =========================================================================

    // Latched request data — stable before toggle
    reg [2:0]  req_reg_sel;
    reg [31:0] req_wdata;
    reg        req_is_write;

    // Toggle handshake signals
    reg        req_toggle;
    reg        ack_sync_0, ack_sync_1, ack_sync_2;
    wire       ack_edge;

    // Bus FSM
    localparam BUS_IDLE    = 2'd0;
    localparam BUS_WAIT    = 2'd1;
    localparam BUS_DONE    = 2'd2;
    reg [1:0]  bus_state;

    // Response data from core side (latched, stable when ack toggles)
    reg [31:0] resp_rdata;

    // 2FF synchronizer for ack_toggle into bus domain
    always @(posedge clk_bus or negedge rst_bus_n) begin
        if (!rst_bus_n) begin
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

    // Bus-side FSM
    always @(posedge clk_bus or negedge rst_bus_n) begin
        if (!rst_bus_n) begin
            bus_state   <= BUS_IDLE;
            req_toggle  <= 1'b0;
            req_reg_sel <= 3'b0;
            req_wdata   <= 32'b0;
            req_is_write <= 1'b0;
            mem_ready   <= 1'b0;
            mem_rdata   <= 32'b0;
        end else begin
            mem_ready <= 1'b0;

            case (bus_state)
                BUS_IDLE: begin
                    if (mem_valid && !mem_ready) begin
                        // Latch request data
                        req_reg_sel  <= mem_addr[4:2];
                        req_wdata    <= mem_wdata;
                        req_is_write <= |mem_wstrb;
                        // Toggle to signal core side
                        req_toggle   <= ~req_toggle;
                        bus_state    <= BUS_WAIT;
                    end
                end

                BUS_WAIT: begin
                    if (ack_edge) begin
                        // Core side has completed — capture response
                        mem_rdata <= resp_rdata;
                        mem_ready <= 1'b1;
                        bus_state <= BUS_DONE;
                    end
                end

                BUS_DONE: begin
                    // One cycle with mem_ready high, then back to idle
                    bus_state <= BUS_IDLE;
                end

                default: bus_state <= BUS_IDLE;
            endcase
        end
    end

    // =========================================================================
    // Core-Side Logic (clk_core domain)
    // =========================================================================

    // 2FF synchronizer for req_toggle into core domain
    reg        req_sync_0, req_sync_1, req_sync_2;
    wire       req_edge;

    // Acknowledge toggle
    reg        ack_toggle;

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

    // Core-side operation execution
    always @(posedge clk_core or negedge rst_core_n) begin
        if (!rst_core_n) begin
            ack_toggle             <= 1'b0;
            core_load_initial      <= 1'b0;
            core_delta_valid       <= 1'b0;
            core_soft_reset        <= 1'b0;
            core_initial_state_in  <= {DATA_WIDTH{1'b0}};
            core_delta_in          <= {DATA_WIDTH{1'b0}};
            resp_rdata             <= 32'b0;
        end else begin
            // Default: clear one-cycle pulses
            core_load_initial <= 1'b0;
            core_delta_valid  <= 1'b0;
            core_soft_reset   <= 1'b0;

            if (req_edge) begin
                // New request from bus side — decode and execute
                if (req_is_write) begin
                    case (req_reg_sel)
                        3'b000: begin  // 0x00: LOAD
                            core_initial_state_in <= req_wdata[DATA_WIDTH-1:0];
                            core_load_initial     <= 1'b1;
                        end
                        3'b001: begin  // 0x04: ACCUM
                            core_delta_in    <= req_wdata[DATA_WIDTH-1:0];
                            core_delta_valid <= 1'b1;
                        end
                        3'b100: begin  // 0x10: CONFIG (soft reset)
                            core_soft_reset <= req_wdata[0];
                        end
                        default: ;  // Ignore writes to read-only registers
                    endcase
                    resp_rdata <= 32'b0;
                end else begin
                    case (req_reg_sel)
                        3'b000: begin  // 0x00: LOAD (read returns initial state)
                            resp_rdata <= core_initial_state_out;
                        end
                        3'b001: begin  // 0x04: ACCUM (read returns accumulator)
                            resp_rdata <= core_merged_accumulator;
                        end
                        3'b010: begin  // 0x08: STATE
                            resp_rdata <= core_current_state;
                        end
                        3'b011: begin  // 0x0C: STATUS
                            resp_rdata <= {31'b0, core_accumulator_zero};
                        end
                        3'b100: begin  // 0x10: CONFIG (read returns 0)
                            resp_rdata <= 32'b0;
                        end
                        3'b101: begin  // 0x14: INIT (debug)
                            resp_rdata <= core_initial_state_out;
                        end
                        3'b110: begin  // 0x18: DELTA (debug)
                            resp_rdata <= core_merged_accumulator;
                        end
                        default: begin
                            resp_rdata <= 32'b0;
                        end
                    endcase
                end

                // Toggle ack to signal completion back to bus side
                ack_toggle <= ~ack_toggle;
            end
        end
    end

endmodule
