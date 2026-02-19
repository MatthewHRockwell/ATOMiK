// =============================================================================
// ATOMiK v3 Datapath
//
// Module:      atomik_v3_atomik
// Description: ATOMiK delta-state core for RV64I custom-0 integration.
//              Contains: XOR accumulator, BSRAM state table, state reconstructor.
//              All operations are direct-wired (no bus, no CDC).
//
//              Instructions:
//                ATOMIK.LOAD  (funct3=000): Write initial_state to table, set addr, clear acc
//                ATOMIK.ACCUM (funct3=001): acc ^= delta_in
//                ATOMIK.READ  (funct3=010): output = initial_state ^ accumulator
//                ATOMIK.SWAP  (funct3=011): update ref = current_state, clear acc, return old state
//
// BSRAM Mapping:
//   State table: 256x64-bit via 2 SDPB blocks (256x32-bit low + 256x32-bit high)
//
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_v3_atomik (
    input  wire        clk,
    input  wire        rst_n,

    // Control (active for one cycle in EXECUTE state)
    input  wire        load_en,     // ATOMIK.LOAD: clear acc, write state table, set addr
    input  wire        accum_en,    // ATOMIK.ACCUM: acc ^= delta_in
    input  wire        swap_en,     // ATOMIK.SWAP: set addr only

    // Data inputs
    input  wire [7:0]  addr_in,     // Address for LOAD/SWAP (rs1[7:0])
    input  wire [63:0] delta_in,    // Delta for ACCUM (rs1_data)
    input  wire [63:0] init_data,   // Initial state data for LOAD (rs2_data)

    // Outputs
    output wire [63:0] current_state,  // initial_state ^ accumulator (state reconstructor)
    output wire        acc_zero        // Accumulator is all zeros
);

    // =========================================================================
    // Accumulator (XOR with clear)
    // =========================================================================
    (* syn_preserve = 1 *) reg [63:0] accumulator;

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
            accumulator <= 64'b0;        // Clear on LOAD
        else if (swap_pending)
            accumulator <= 64'b0;        // Clear one cycle after SWAP (after writeback reads)
        else if (accum_en)
            accumulator <= accumulator ^ delta_in;  // XOR accumulate
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
    // BSRAM State Table (256 x 64-bit via dual 256x32 SDP)
    // =========================================================================
    (* syn_ramstyle = "block_ram" *) reg [63:0] state_table [0:255];
    reg [63:0] state_read_reg;

    // Write port: ATOMIK.LOAD writes initial_state, ATOMIK.SWAP updates reference
    always @(posedge clk) begin
        if (load_en)
            state_table[addr_in] <= init_data;
        else if (swap_en)
            state_table[active_addr] <= state_read_reg ^ accumulator;  // New ref = current_state
    end

    // Read port: registered output, always reads active_addr
    always @(posedge clk) begin
        state_read_reg <= state_table[active_addr];
    end

    // =========================================================================
    // State Reconstructor: current_state = initial_state XOR accumulator
    // =========================================================================
    (* syn_keep = 1 *) wire [63:0] reconstructed;
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
        for (i = 0; i < 256; i = i + 1)
            state_table[i] = 64'b0;
    end
    // synthesis translate_on

endmodule
