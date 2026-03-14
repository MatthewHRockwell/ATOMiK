// =============================================================================
// Generic Dual-Port SRAM — ASIC-Portable Memory Wrapper
//
// Module:      atomik_sram_dp
// Description: Behavioral simple dual-port SRAM (1 write port, 1 read port).
//              Used for ATOMiK state table (256x64) and CPU register file (32x64).
//
//              Port A: Write-only (synchronous)
//              Port B: Read-only (synchronous, configurable latency)
//
// Parameters:
//   ADDR_WIDTH    - Address width
//   DATA_WIDTH    - Data width in bits
//   DEPTH         - Memory depth
//   READ_LATENCY  - 1 = array read only, 2 = array + output register
//
// ATOMiK Project — March 2026
// SPDX-License-Identifier: MIT
// =============================================================================

`timescale 1ns / 1ps

module atomik_sram_dp #(
    parameter ADDR_WIDTH   = 8,
    parameter DATA_WIDTH   = 64,
    parameter DEPTH        = (1 << ADDR_WIDTH),
    parameter READ_LATENCY = 1
) (
    input  wire                    clk,

    // Port A: Write
    input  wire                    we,       // Write enable
    input  wire [ADDR_WIDTH-1:0]   waddr,
    input  wire [DATA_WIDTH-1:0]   wdata,

    // Port B: Read
    input  wire                    re,       // Read enable
    input  wire [ADDR_WIDTH-1:0]   raddr,
    output wire [DATA_WIDTH-1:0]   rdata
);

    // =========================================================================
    // Memory Array
    // =========================================================================

    (* ram_style = "block" *)
    reg [DATA_WIDTH-1:0] mem [0:DEPTH-1];

    // =========================================================================
    // Write Port (synchronous)
    // =========================================================================
    always @(posedge clk) begin
        if (we) begin
            mem[waddr] <= wdata;
        end
    end

    // =========================================================================
    // Read Port (synchronous, configurable latency)
    // =========================================================================

    reg [DATA_WIDTH-1:0] rdata_pipe0;

    always @(posedge clk) begin
        if (re) begin
            rdata_pipe0 <= mem[raddr];
        end
    end

    generate
        if (READ_LATENCY == 2) begin : gen_output_reg
            // Two-cycle read: array register + output register
            // Matches Xilinx BRAM with DOB_REG=1 and Zynq 4-stage SWAP pipeline
            reg [DATA_WIDTH-1:0] rdata_pipe1;
            always @(posedge clk) begin
                if (re) begin
                    rdata_pipe1 <= rdata_pipe0;
                end
            end
            assign rdata = rdata_pipe1;
        end else begin : gen_single_reg
            // Single-cycle read: array register only
            assign rdata = rdata_pipe0;
        end
    endgenerate

    // =========================================================================
    // Simulation initialization
    // =========================================================================
`ifdef SIMULATION
    integer i;
    initial begin
        for (i = 0; i < DEPTH; i = i + 1)
            mem[i] = {DATA_WIDTH{1'b0}};
    end
`endif

endmodule
