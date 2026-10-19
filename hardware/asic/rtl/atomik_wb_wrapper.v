// =============================================================================
// ATOMiK Wishbone Slave Wrapper
//
// Module:      atomik_wb_wrapper
// Description: Wishbone B4 slave interface for the ATOMiK delta-state core.
//              Maps ATOMiK operations to memory-mapped registers accessible
//              from the Caravel management SoC (PicoRV32).
//
// Register Map (offsets from base address 0x30000000):
//   0x00  LOAD_ADDR    [W]  Address [7:0] for LOAD operation
//   0x04  LOAD_DATA_LO [W]  init_state bits [31:0]
//   0x08  LOAD_DATA_HI [W]  init_state bits [63:32] — triggers LOAD
//   0x0C  ACCUM_LO     [W]  delta bits [31:0]
//   0x10  ACCUM_HI     [W]  delta bits [63:32] — triggers ACCUM
//   0x14  STATE_LO     [R]  current_state [31:0] (latches 64-bit snapshot)
//   0x18  STATE_HI     [R]  current_state [63:32] (returns cached)
//   0x1C  STATUS       [R]  {version[7:0], 15'h0, acc_zero}
//   0x20  SWAP_ADDR    [W]  addr [7:0] — triggers SWAP
//   0x24  CONFIG       [R/W] Bit 0 = core_enable
//
// Protocol: 32-bit Wishbone ↔ 64-bit ATOMiK via LO/HI register split.
//           HI write triggers the operation (same pattern as AXI wrapper).
//
// Target:  chipIgnite Caravel SoC (SKY130 130nm)
// Author:  ATOMiK Project
// Date:    March 2026
// =============================================================================

`timescale 1ns / 1ps
`default_nettype none

module atomik_wb_wrapper (
    // Wishbone Slave Interface
    input  wire        wb_clk_i,
    input  wire        wb_rst_i,       // Active HIGH (Caravel convention)
    input  wire        wbs_stb_i,
    input  wire        wbs_cyc_i,
    input  wire        wbs_we_i,
    input  wire [3:0]  wbs_sel_i,
    input  wire [31:0] wbs_dat_i,
    input  wire [31:0] wbs_adr_i,
    output reg         wbs_ack_o,
    output reg  [31:0] wbs_dat_o,

    // ATOMiK Core Interface
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
    // Parameters
    // =========================================================================
    localparam VERSION = 8'h10;         // v1.0 ASIC

    // Register offsets (byte addresses, 4-byte aligned)
    localparam ADDR_LOAD_ADDR    = 8'h00;
    localparam ADDR_LOAD_DATA_LO = 8'h04;
    localparam ADDR_LOAD_DATA_HI = 8'h08;
    localparam ADDR_ACCUM_LO     = 8'h0C;
    localparam ADDR_ACCUM_HI     = 8'h10;
    localparam ADDR_STATE_LO     = 8'h14;
    localparam ADDR_STATE_HI     = 8'h18;
    localparam ADDR_STATUS       = 8'h1C;
    localparam ADDR_SWAP_ADDR    = 8'h20;
    localparam ADDR_CONFIG       = 8'h24;

    // =========================================================================
    // Internal Registers
    // =========================================================================
    reg [7:0]  load_addr_reg;
    reg [31:0] load_data_lo;
    reg [31:0] accum_lo;
    reg [63:0] state_snapshot;          // Latched on STATE_LO read
    reg        core_enable;

    // =========================================================================
    // Wishbone Transaction Detection
    // =========================================================================
    wire valid = wbs_cyc_i && wbs_stb_i;
    wire write = valid && wbs_we_i;
    wire read  = valid && !wbs_we_i;
    wire [7:0] reg_addr = wbs_adr_i[7:0];  // Low 8 bits for register decode

    // =========================================================================
    // Operation Triggers (active for one cycle)
    // =========================================================================
    // Operations complete in one cycle — ack is immediate for non-trigger writes,
    // one-cycle delayed for trigger writes (to let the core execute).

    reg op_pending;
    reg op_done;

    always @(posedge wb_clk_i) begin
        if (wb_rst_i) begin
            core_load_en  <= 1'b0;
            core_accum_en <= 1'b0;
            core_swap_en  <= 1'b0;
            core_addr     <= 8'b0;
            core_delta    <= 64'b0;
            core_init_data <= 64'b0;
            load_addr_reg <= 8'b0;
            load_data_lo  <= 32'b0;
            accum_lo      <= 32'b0;
            state_snapshot <= 64'b0;
            core_enable   <= 1'b1;
            op_pending    <= 1'b0;
            op_done       <= 1'b0;
            wbs_ack_o     <= 1'b0;
            wbs_dat_o     <= 32'b0;
        end else begin
            // Default: clear one-cycle pulses
            core_load_en  <= 1'b0;
            core_accum_en <= 1'b0;
            core_swap_en  <= 1'b0;
            wbs_ack_o     <= 1'b0;

            // Operation completion (delayed ack for trigger writes)
            if (op_pending) begin
                op_pending <= 1'b0;
                wbs_ack_o  <= 1'b1;
            end

            // Write transactions
            if (write && !op_pending) begin
                case (reg_addr)
                    ADDR_LOAD_ADDR: begin
                        load_addr_reg <= wbs_dat_i[7:0];
                        wbs_ack_o <= 1'b1;     // Immediate ack
                    end
                    ADDR_LOAD_DATA_LO: begin
                        load_data_lo <= wbs_dat_i;
                        wbs_ack_o <= 1'b1;
                    end
                    ADDR_LOAD_DATA_HI: begin
                        // Trigger LOAD operation
                        core_init_data <= {wbs_dat_i, load_data_lo};
                        core_addr      <= load_addr_reg;
                        core_load_en   <= 1'b1;
                        op_pending     <= 1'b1; // Ack next cycle
                    end
                    ADDR_ACCUM_LO: begin
                        accum_lo <= wbs_dat_i;
                        wbs_ack_o <= 1'b1;
                    end
                    ADDR_ACCUM_HI: begin
                        // Trigger ACCUM operation
                        core_delta   <= {wbs_dat_i, accum_lo};
                        core_accum_en <= 1'b1;
                        op_pending    <= 1'b1;
                    end
                    ADDR_SWAP_ADDR: begin
                        // Trigger SWAP operation
                        core_addr    <= wbs_dat_i[7:0];
                        core_swap_en <= 1'b1;
                        op_pending   <= 1'b1;
                    end
                    ADDR_CONFIG: begin
                        core_enable <= wbs_dat_i[0];
                        wbs_ack_o <= 1'b1;
                    end
                    default: begin
                        wbs_ack_o <= 1'b1;     // Ack unknown addresses
                    end
                endcase
            end

            // Read transactions
            if (read && !op_pending) begin
                wbs_ack_o <= 1'b1;
                case (reg_addr)
                    ADDR_STATE_LO: begin
                        // Latch full 64-bit snapshot on LO read
                        state_snapshot <= core_current_state;
                        wbs_dat_o <= core_current_state[31:0];
                    end
                    ADDR_STATE_HI: begin
                        // Return cached upper half
                        wbs_dat_o <= state_snapshot[63:32];
                    end
                    ADDR_STATUS: begin
                        wbs_dat_o <= {VERSION, 23'b0, core_acc_zero};
                    end
                    ADDR_CONFIG: begin
                        wbs_dat_o <= {31'b0, core_enable};
                    end
                    default: begin
                        wbs_dat_o <= 32'b0;
                    end
                endcase
            end
        end
    end

endmodule

`default_nettype wire
