// =============================================================================
// ATOMiK v3 Datapath — Zynq Port (Distributed RAM)
//
// Module:      atomik_v3_atomik_zynq
// Description: ATOMiK delta-state core for RV64I custom-0 integration on Zynq.
//              Drop-in replacement for atomik_v3_atomik (Gowin BSRAM).
//              Uses distributed RAM for 1-cycle read timing (no CDC needed).
//
//              Instructions:
//                ATOMIK.LOAD  (funct3=000): Write initial_state to table, set addr, clear acc
//                ATOMIK.ACCUM (funct3=001): acc ^= delta_in
//                ATOMIK.READ  (funct3=010): output = initial_state ^ accumulator
//                ATOMIK.SWAP  (funct3=011): update ref = current_state, clear acc
//
// Distributed RAM Mapping:
//   State table: 256x64-bit = 256 LUT6 (via Xilinx RAM64M/RAM128X1D)
//
// Timing:
//   Reads are registered (same as Gowin version). Address in cycle N,
//   state_read_reg valid at cycle N+1. Distributed RAM read is combinational
//   (no write-through artifact), so SWAP timing is identical to v3.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_v3_atomik_zynq (
    input  wire        clk,
    input  wire        rst_n,

    // Control (active for one cycle in EXECUTE state)
    input  wire        load_en,     // ATOMIK.LOAD: clear acc, write state table, set addr
    input  wire        accum_en,    // ATOMIK.ACCUM: acc ^= delta_in
    input  wire        swap_en,     // ATOMIK.SWAP: set addr only (clear delayed 1 cycle)

    // Data inputs
    input  wire [7:0]  addr_in,     // Address for LOAD/SWAP (rs1[7:0])
    input  wire [63:0] delta_in,    // Delta for ACCUM (rs1_data)
    input  wire [63:0] init_data,   // Initial state data for LOAD (rs2_data)

    // Outputs
    output wire [63:0] current_state,  // initial_state ^ accumulator
    output wire        acc_zero        // Accumulator is all zeros
);

    // =========================================================================
    // Accumulator (XOR with clear)
    // =========================================================================
    reg [63:0] accumulator;

    // Delayed accumulator clear for SWAP: the writeback mux reads
    // current_state one cycle after EXECUTE, so the accumulator must
    // hold its value through WRITEBACK before clearing.
    reg swap_pending;

    always @(posedge clk) begin
        if (!rst_n)
            swap_pending <= 1'b0;
        else
            swap_pending <= swap_en;
    end

    always @(posedge clk) begin
        if (!rst_n)
            accumulator <= 64'b0;
        else if (load_en)
            accumulator <= 64'b0;
        else if (swap_pending)
            accumulator <= 64'b0;
        else if (accum_en)
            accumulator <= accumulator ^ delta_in;
    end

    assign acc_zero = ~(|accumulator);

    // =========================================================================
    // Active address register
    // =========================================================================
    reg [7:0] active_addr;

    always @(posedge clk) begin
        if (!rst_n)
            active_addr <= 8'b0;
        else if (load_en || swap_en)
            active_addr <= addr_in;
    end

    // =========================================================================
    // Distributed RAM State Table (256 x 64-bit)
    //
    // Distributed RAM provides combinational reads (no write-through artifact).
    // Output registered for timing consistency with v3 BSRAM version.
    // =========================================================================
    (* ram_style = "distributed" *) reg [63:0] state_table [0:255];
    reg [63:0] state_read_reg;

    // Write port: ATOMIK.LOAD writes initial_state, ATOMIK.SWAP updates reference
    always @(posedge clk) begin
        if (load_en)
            state_table[addr_in] <= init_data;
        else if (swap_en)
            state_table[active_addr] <= state_read_reg ^ accumulator;
    end

    // Read port: registered output, always reads active_addr
    always @(posedge clk) begin
        state_read_reg <= state_table[active_addr];
    end

    // =========================================================================
    // State Reconstructor: current_state = initial_state XOR accumulator
    // =========================================================================
    wire [63:0] reconstructed;
    assign reconstructed = state_read_reg ^ accumulator;
    assign current_state = reconstructed;

    // =========================================================================
    // Simulation-only initialization
    // =========================================================================
    // synthesis translate_off
    integer i;
    initial begin
        accumulator = 64'b0;
        active_addr = 8'b0;
        state_read_reg = 64'b0;
        swap_pending = 1'b0;
        for (i = 0; i < 256; i = i + 1)
            state_table[i] = 64'b0;
    end
    // synthesis translate_on

endmodule
