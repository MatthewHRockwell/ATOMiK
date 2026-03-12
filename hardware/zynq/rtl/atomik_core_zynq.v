// =============================================================================
// ATOMiK Core — Zynq-Optimized Variant
//
// Module:      atomik_core_zynq
// Description: Zynq-optimized ATOMiK delta-state core. Functionally identical
//              to atomik_v3_atomik but restructured for higher Fmax on 7-series:
//
//              Optimizations vs. atomik_v3_atomik:
//                1. BRAM via explicit XPM instantiation — eliminates the
//                   combinational dist RAM read path + fanout-120 routing
//                2. SWAP write-back pipelined in 3 stages to account for BRAM
//                   registered read latency. Breaks read-XOR-write chain.
//
//              SWAP pipeline (4-stage, BRAM output-registered):
//                Stage 1 (swap_s1): Address registered, BRAM array reading
//                Stage 2 (swap_s2): BRAM output register valid
//                Stage 3 (swap_s3): compute swap_data = ref ^ acc
//                Stage 4 (swap_s4): Write BRAM, clear accumulator
//
//              Same port interface as atomik_v3_atomik — drop-in replacement.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_core_zynq (
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
    // Accumulator (XOR with clear)
    // =========================================================================
    reg [63:0] accumulator;

    // 4-stage SWAP pipeline (BRAM output register adds 1 cycle)
    //   swap_s1: BRAM array read in progress
    //   swap_s2: BRAM output register captures array output
    //   swap_s3: mem_rd_data valid → compute swap_data = ref ^ acc
    //   swap_s4: write BRAM, clear accumulator
    reg swap_s1;
    reg swap_s2;
    reg swap_s3;
    reg swap_s4;

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

    always @(posedge clk) begin
        if (!rst_n)
            accumulator <= 64'b0;
        else if (load_en)
            accumulator <= 64'b0;
        else if (swap_s4)
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
    // BRAM State Table (256 x 64-bit)
    // =========================================================================
    // Write port: muxed between LOAD (immediate) and SWAP (stage 3)
    wire        mem_wr_en   = load_en | swap_s4;
    wire [7:0]  mem_wr_addr = load_en ? addr_in : swap_addr;
    wire [63:0] mem_wr_data = load_en ? init_data : swap_data;

    // Read port output
    wire [63:0] mem_rd_data;

`ifdef SIMULATION
    // =====================================================================
    // Simulation: behavioral BRAM model (iverilog-compatible)
    // Matches READ_LATENCY_B=2: array read + output register = 2 cycles
    // =====================================================================
    reg [63:0] state_table [0:255];
    reg [63:0] bram_array_out;    // Stage 1: array read (internal to BRAM)
    reg [63:0] bram_reg_out;      // Stage 2: output register (DOB_REG)

    always @(posedge clk) begin
        if (mem_wr_en)
            state_table[mem_wr_addr] <= mem_wr_data;
    end

    always @(posedge clk) begin
        bram_array_out <= state_table[active_addr];  // BRAM array read
        bram_reg_out   <= bram_array_out;            // BRAM output register
    end

    assign mem_rd_data = bram_reg_out;

    // Simulation-only initialization
    integer i;
    initial begin
        for (i = 0; i < 256; i = i + 1)
            state_table[i] = 64'b0;
        bram_array_out = 64'b0;
        bram_reg_out   = 64'b0;
    end

`else
    // =====================================================================
    // Synthesis: XPM Simple Dual-Port BRAM (guaranteed RAMB36E1/RAMB18E1)
    // =====================================================================
    // Port A = write, Port B = read (independent addresses)
    // READ_LATENCY_B=2: array read + output register (DOB_REG=1)
    // Output register breaks the 2.1ns BRAM Tco path, enabling 400+ MHz
    xpm_memory_sdpram #(
        .ADDR_WIDTH_A           (8),
        .ADDR_WIDTH_B           (8),
        .AUTO_SLEEP_TIME        (0),
        .BYTE_WRITE_WIDTH_A     (64),       // No byte enables, full-width write
        .CASCADE_HEIGHT         (0),
        .CLOCKING_MODE          ("common_clock"),
        .ECC_BIT_RANGE          ("7:0"),
        .ECC_MODE               ("no_ecc"),
        .ECC_TYPE               ("none"),
        .IGNORE_INIT_SYNTH      (0),
        .MEMORY_INIT_FILE       ("none"),
        .MEMORY_INIT_PARAM      ("0"),
        .MEMORY_OPTIMIZATION    ("true"),
        .MEMORY_PRIMITIVE       ("block"),   // Force Block RAM
        .MEMORY_SIZE            (16384),     // 256 x 64 = 16384 bits
        .MESSAGE_CONTROL        (0),
        .RAM_DECOMP             ("auto"),
        .READ_DATA_WIDTH_B      (64),
        .READ_LATENCY_B         (2),        // 2 cycle read latency (array + output register)
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
        // Write port (A)
        .clka           (clk),
        .ena            (mem_wr_en),
        .wea            (1'b1),
        .addra          (mem_wr_addr),
        .dina           (mem_wr_data),

        // Read port (B)
        .clkb           (clk),               // Common clock
        .enb            (1'b1),              // Always reading
        .rstb           (~rst_n),
        .addrb          (active_addr),
        .doutb          (mem_rd_data),
        .regceb         (1'b1),

        // Unused
        .injectdbiterra (1'b0),
        .injectsbiterra (1'b0),
        .dbiterrb       (),
        .sbiterrb       (),
        .sleep          (1'b0)
    );
`endif

    // =========================================================================
    // SWAP write-back pipeline (4-stage, BRAM output-registered)
    // =========================================================================
    // Timeline (core clock cycles after swap_en pulse):
    //   Cycle 0 (swap_en):  active_addr updates via NBA
    //   Cycle 1 (swap_s1):  BRAM array read in progress
    //   Cycle 2 (swap_s2):  BRAM output register captures data
    //   Cycle 3 (swap_s3):  mem_rd_data valid → compute swap_data = ref ^ acc
    //   Cycle 4 (swap_s4):  Write BRAM[addr] = swap_data, clear accumulator

    reg [63:0] swap_data;
    reg [7:0]  swap_addr;

    always @(posedge clk) begin
        if (swap_s3) begin
            swap_data <= mem_rd_data ^ accumulator;
            swap_addr <= active_addr;
        end
    end

    // =========================================================================
    // State Reconstructor: current_state = initial_state XOR accumulator
    // =========================================================================
    assign current_state = mem_rd_data ^ accumulator;

    // =========================================================================
    // Simulation-only register initialization
    // =========================================================================
    // synthesis translate_off
    initial begin
        accumulator = 64'b0;
        active_addr = 8'b0;
        swap_s1 = 1'b0;
        swap_s2 = 1'b0;
        swap_s3 = 1'b0;
        swap_s4 = 1'b0;
        swap_data = 64'b0;
        swap_addr = 8'b0;
    end
    // synthesis translate_on

endmodule
