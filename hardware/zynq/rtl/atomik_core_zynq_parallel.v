// =============================================================================
// ATOMiK Zynq Parallel Core — N-Bank XOR Accumulators + XPM BRAM
//
// Module:      atomik_core_zynq_parallel
// Description: Parameterized N-bank parallel accumulator for Zynq 7-series.
//              Combines the N-bank architecture (round-robin distribution,
//              XOR merge tree) with the Zynq-optimized BRAM pipeline
//              (XPM SDPRAM, READ_LATENCY_B=2, 4-stage SWAP).
//
//              Same port interface as atomik_core_zynq — drop-in replacement.
//              When N_BANKS=1, functionally identical to atomik_core_zynq.
//
// Architecture:
//   - N parallel accumulators fed via round-robin (one per ACCUM cycle)
//   - Flat XOR reduction (no tree — XOR is bitwise, no carry propagation)
//     Vivado packs up to 6 inputs per LUT6 for optimal depth.
//   - Shared XPM BRAM state table (256 x 64-bit, RAMB36E1)
//   - 4-stage SWAP pipeline (BRAM output register, READ_LATENCY_B=2)
//   - State reconstruction: current_state = BRAM[addr] ^ merged_acc
//
// Mathematical Foundation (108 Lean4 theorems):
//   XOR commutativity + associativity guarantee that delta distribution
//   across banks yields identical results regardless of order.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_core_zynq_parallel #(
    parameter N_BANKS    = 1,
    parameter BANK_SEL_W = (N_BANKS > 1) ? $clog2(N_BANKS) : 1
)(
    input  wire        clk,
    input  wire        rst_n,

    // Control (active for one cycle)
    input  wire        load_en,
    input  wire        accum_en,
    input  wire        swap_en,

    // Data inputs
    input  wire [7:0]  addr_in,
    input  wire [63:0] delta_in,
    input  wire [63:0] init_data,

    // Outputs
    output wire [63:0] current_state,
    output wire        acc_zero
);

    // =========================================================================
    // Round-Robin Bank Selector
    // =========================================================================
    reg [BANK_SEL_W-1:0] bank_sel;

    always @(posedge clk) begin
        if (!rst_n)
            bank_sel <= {BANK_SEL_W{1'b0}};
        else if (load_en)
            bank_sel <= {BANK_SEL_W{1'b0}};
        else if (accum_en) begin
            if (bank_sel == N_BANKS[BANK_SEL_W-1:0] - 1'b1)
                bank_sel <= {BANK_SEL_W{1'b0}};
            else
                bank_sel <= bank_sel + 1'b1;
        end
    end

    // =========================================================================
    // Per-Bank Accumulate Enable (round-robin only — no parallel port on Zynq)
    // =========================================================================
    wire [N_BANKS-1:0] bank_accum_en;

    genvar gi;
    generate
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : bank_en_gen
            assign bank_accum_en[gi] = accum_en && (bank_sel == gi[BANK_SEL_W-1:0]);
        end
    endgenerate

    // =========================================================================
    // N Parallel Accumulator Banks
    // =========================================================================
    reg [63:0] bank_acc [0:N_BANKS-1];

    // 4-stage SWAP pipeline (same as single-bank atomik_core_zynq)
    reg swap_s1, swap_s2, swap_s3, swap_s4;

    always @(posedge clk) begin
        if (!rst_n) begin
            swap_s1 <= 1'b0;
            swap_s2 <= 1'b0;
            swap_s3 <= 1'b0;
            swap_s4 <= 1'b0;
        end else begin
            swap_s1 <= swap_en;
            swap_s2 <= swap_s1;
            swap_s3 <= swap_s2;
            swap_s4 <= swap_s3;
        end
    end

    generate
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : banks
            always @(posedge clk) begin
                if (!rst_n)
                    bank_acc[gi] <= 64'b0;
                else if (load_en)
                    bank_acc[gi] <= 64'b0;
                else if (swap_s4)
                    bank_acc[gi] <= 64'b0;
                else if (bank_accum_en[gi])
                    bank_acc[gi] <= bank_acc[gi] ^ delta_in;
            end
        end
    endgenerate

    // =========================================================================
    // XOR Reduction (Balanced Binary Tree, no DONT_TOUCH)
    // =========================================================================
    // XOR is purely bitwise — no carry propagation, each bit independent.
    // A balanced binary tree provides optimal combinational depth (log2(N)
    // levels) while letting Vivado pack adjacent XOR levels into LUT6s.
    //
    // No DONT_TOUCH/syn_keep — unlike Gowin, Xilinx does not infer ALU
    // carry chains for XOR, so the synthesizer can freely optimize.
    //
    // Depth: log2(N) XOR levels, Vivado may further reduce via LUT packing.

    wire [63:0] merged_acc;

    generate
        if (N_BANKS == 1) begin : single_bank
            assign merged_acc = bank_acc[0];
        end else begin : multi_bank
            wire [63:0] merge_node [0:2*N_BANKS-2];

            for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : merge_leaf
                assign merge_node[gi] = bank_acc[gi];
            end

            for (gi = 0; gi < N_BANKS - 1; gi = gi + 1) begin : merge_inner
                assign merge_node[N_BANKS + gi] = merge_node[2*gi] ^ merge_node[2*gi + 1];
            end

            assign merged_acc = merge_node[2*N_BANKS - 2];
        end
    endgenerate

    assign acc_zero = ~(|merged_acc);

    // =========================================================================
    // Active Address Register
    // =========================================================================
    reg [7:0] active_addr;

    always @(posedge clk) begin
        if (!rst_n)
            active_addr <= 8'b0;
        else if (load_en || swap_en)
            active_addr <= addr_in;
    end

    // =========================================================================
    // BRAM State Table (256 x 64-bit)
    // =========================================================================
    wire        mem_wr_en   = load_en | swap_s4;
    wire [7:0]  mem_wr_addr = load_en ? addr_in : swap_addr;
    wire [63:0] mem_wr_data = load_en ? init_data : swap_data;

    wire [63:0] mem_rd_data;

`ifdef SIMULATION
    // Behavioral BRAM model (iverilog-compatible, matches READ_LATENCY_B=2)
    reg [63:0] state_table [0:255];
    reg [63:0] bram_array_out;
    reg [63:0] bram_reg_out;

    always @(posedge clk) begin
        if (mem_wr_en)
            state_table[mem_wr_addr] <= mem_wr_data;
    end

    always @(posedge clk) begin
        bram_array_out <= state_table[active_addr];
        bram_reg_out   <= bram_array_out;
    end

    assign mem_rd_data = bram_reg_out;

    integer i;
    initial begin
        for (i = 0; i < 256; i = i + 1)
            state_table[i] = 64'b0;
        bram_array_out = 64'b0;
        bram_reg_out   = 64'b0;
    end

`else
    // XPM Simple Dual-Port BRAM (guaranteed RAMB36E1)
    xpm_memory_sdpram #(
        .ADDR_WIDTH_A           (8),
        .ADDR_WIDTH_B           (8),
        .AUTO_SLEEP_TIME        (0),
        .BYTE_WRITE_WIDTH_A     (64),
        .CASCADE_HEIGHT         (0),
        .CLOCKING_MODE          ("common_clock"),
        .ECC_BIT_RANGE          ("7:0"),
        .ECC_MODE               ("no_ecc"),
        .ECC_TYPE               ("none"),
        .IGNORE_INIT_SYNTH      (0),
        .MEMORY_INIT_FILE       ("none"),
        .MEMORY_INIT_PARAM      ("0"),
        .MEMORY_OPTIMIZATION    ("true"),
        .MEMORY_PRIMITIVE       ("block"),
        .MEMORY_SIZE            (16384),     // 256 x 64
        .MESSAGE_CONTROL        (0),
        .RAM_DECOMP             ("auto"),
        .READ_DATA_WIDTH_B      (64),
        .READ_LATENCY_B         (2),
        .READ_RESET_VALUE_B     ("0"),
        .RST_MODE_A             ("SYNC"),
        .RST_MODE_B             ("SYNC"),
        .SIM_ASSERT_CHK         (0),
        .USE_EMBEDDED_CONSTRAINT(0),
        .USE_MEM_INIT           (0),
        .USE_MEM_INIT_MMI       (0),
        .WAKEUP_TIME            ("disable_sleep"),
        .WRITE_DATA_WIDTH_A     (64),
        .WRITE_MODE_B           ("read_first"),
        .WRITE_PROTECT          (1)
    ) u_state_bram (
        .clka   (clk),
        .ena    (mem_wr_en),
        .wea    (1'b1),
        .addra  (mem_wr_addr),
        .dina   (mem_wr_data),
        .clkb   (clk),
        .enb    (1'b1),
        .rstb   (~rst_n),
        .addrb  (active_addr),
        .doutb  (mem_rd_data),
        .regceb (1'b1),
        .injectdbiterra (1'b0),
        .injectsbiterra (1'b0),
        .dbiterrb       (),
        .sbiterrb       (),
        .sleep          (1'b0)
    );
`endif

    // =========================================================================
    // SWAP Write-Back Pipeline (4-stage, BRAM output-registered)
    // =========================================================================
    reg [63:0] swap_data;
    reg [7:0]  swap_addr;

    always @(posedge clk) begin
        if (swap_s3) begin
            swap_data <= mem_rd_data ^ merged_acc;
            swap_addr <= active_addr;
        end
    end

    // =========================================================================
    // State Reconstructor: current_state = BRAM[addr] ^ merged_acc
    // =========================================================================
    assign current_state = mem_rd_data ^ merged_acc;

    // =========================================================================
    // Simulation-only register initialization
    // =========================================================================
    // synthesis translate_off
    integer k;
    initial begin
        bank_sel    = {BANK_SEL_W{1'b0}};
        active_addr = 8'b0;
        swap_s1     = 1'b0;
        swap_s2     = 1'b0;
        swap_s3     = 1'b0;
        swap_s4     = 1'b0;
        swap_data   = 64'b0;
        swap_addr   = 8'b0;
        for (k = 0; k < N_BANKS; k = k + 1)
            bank_acc[k] = 64'b0;
    end
    // synthesis translate_on

endmodule
