// =============================================================================
// ATOMiK Bus Wrapper for PicoRV32
//
// Module:      atomik_bus_wrapper
// Description: Wraps atomik_core_v2 with PicoRV32's valid/ready bus protocol.
//              Maps memory-mapped registers to ATOMiK operations.
//
// Register Map (active bits of address[4:2]):
//   0x00  LOAD   (W)  - Load initial state, clears accumulator
//   0x04  ACCUM  (W)  - Accumulate delta via XOR
//   0x08  STATE  (R)  - Read reconstructed state (initial XOR accumulator)
//   0x0C  STATUS (R)  - Bit 0: accumulator_zero
//   0x10  CONFIG (W)  - Bit 0: soft reset
//   0x14  INIT   (R)  - Read initial state (debug)
//   0x18  DELTA  (R)  - Read delta accumulator (debug)
//
// Bus Protocol:
//   - Transaction: master asserts valid, slave responds with ready
//   - Write: wstrb != 0, data on wdata
//   - Read:  wstrb == 0, result on rdata
//   - All operations complete in 1 cycle (ready asserted next cycle)
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   February 12, 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_bus_wrapper (
    input  wire        clk,
    input  wire        resetn,         // Active-low reset (PicoRV32 convention)

    // PicoRV32 Bus Interface
    input  wire        mem_valid,
    input  wire [31:0] mem_addr,
    input  wire [31:0] mem_wdata,
    input  wire [3:0]  mem_wstrb,
    output reg         mem_ready,
    output reg  [31:0] mem_rdata
);

    // =========================================================================
    // Internal Signals
    // =========================================================================

    // ATOMiK core connections
    reg  [1:0]  core_operation;
    reg  [31:0] core_data_in;
    wire [31:0] core_data_out;
    wire        core_data_valid;
    wire        core_accumulator_zero;
    wire [31:0] core_debug_initial;
    wire [31:0] core_debug_accumulator;

    // Soft reset
    reg         soft_reset;
    wire        rst_n = resetn & ~soft_reset;

    // Register select from address bits [4:2]
    wire [2:0]  reg_sel = mem_addr[4:2];

    // Write transaction detection
    wire        is_write = |mem_wstrb;

    // =========================================================================
    // ATOMiK Core Instance (32-bit for PicoRV32 compatibility)
    // =========================================================================

    atomik_core_v2 #(
        .DATA_WIDTH(32)
    ) u_atomik_core (
        .clk               (clk),
        .rst_n              (rst_n),
        .operation          (core_operation),
        .data_in            (core_data_in),
        .data_out           (core_data_out),
        .data_valid         (core_data_valid),
        .accumulator_zero   (core_accumulator_zero),
        .debug_initial_state(core_debug_initial),
        .debug_accumulator  (core_debug_accumulator)
    );

    // =========================================================================
    // Bus Transaction Handler
    // =========================================================================

    always @(posedge clk) begin
        if (!resetn) begin
            mem_ready      <= 1'b0;
            mem_rdata      <= 32'b0;
            core_operation <= 2'b00;  // NOP
            core_data_in   <= 32'b0;
            soft_reset     <= 1'b0;
        end else begin
            // Default: NOP and clear ready
            mem_ready      <= 1'b0;
            core_operation <= 2'b00;  // NOP
            soft_reset     <= 1'b0;

            if (mem_valid && !mem_ready) begin
                mem_ready <= 1'b1;

                if (is_write) begin
                    // --- Write Operations ---
                    case (reg_sel)
                        3'b000: begin  // 0x00: LOAD
                            core_operation <= 2'b01;  // OP_LOAD
                            core_data_in   <= mem_wdata;
                        end
                        3'b001: begin  // 0x04: ACCUM
                            core_operation <= 2'b10;  // OP_ACCUMULATE
                            core_data_in   <= mem_wdata;
                        end
                        3'b100: begin  // 0x10: CONFIG
                            soft_reset <= mem_wdata[0];
                        end
                        default: begin
                            // Ignore writes to read-only registers
                        end
                    endcase
                end else begin
                    // --- Read Operations ---
                    case (reg_sel)
                        3'b000: begin  // 0x00: LOAD (read returns initial state)
                            mem_rdata <= core_debug_initial;
                        end
                        3'b001: begin  // 0x04: ACCUM (read returns accumulator)
                            mem_rdata <= core_debug_accumulator;
                        end
                        3'b010: begin  // 0x08: STATE
                            core_operation <= 2'b11;  // OP_READ
                            mem_rdata <= core_debug_initial ^ core_debug_accumulator;
                        end
                        3'b011: begin  // 0x0C: STATUS
                            mem_rdata <= {31'b0, core_accumulator_zero};
                        end
                        3'b100: begin  // 0x10: CONFIG (read returns 0)
                            mem_rdata <= 32'b0;
                        end
                        3'b101: begin  // 0x14: INIT (debug)
                            mem_rdata <= core_debug_initial;
                        end
                        3'b110: begin  // 0x18: DELTA (debug)
                            mem_rdata <= core_debug_accumulator;
                        end
                        default: begin
                            mem_rdata <= 32'b0;
                        end
                    endcase
                end
            end
        end
    end

endmodule
