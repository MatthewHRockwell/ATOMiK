/*
 * atomik_cfu_adapter.v — ATOMiK CFU (Custom Function Unit) Adapter
 *
 * Bridges the VexRiscv CFU bus to the ATOMiK delta-state core.
 * Implements the four ATOMiK operations via RISC-V custom-0 instructions:
 *
 *   funct3  Operation  Inputs              Output
 *   000     LOAD       rs1[7:0]=addr,      rd = current_state[31:0]
 *                      rs2=init_data
 *   001     ACCUM      rs1=delta            rd = current_state[31:0]
 *   010     READ       (none)               rd = current_state[31:0]
 *   011     SWAP       rs1[7:0]=addr        rd = current_state[31:0]
 *   100     LOAD_HI    rs2=init_data_hi     rd = current_state[63:32]
 *   101     ACCUM_HI   rs1=delta_hi         rd = current_state[63:32]
 *   110     READ_HI    (none)               rd = current_state[63:32]
 *   111     STATUS     (none)               rd = {ver, banks, acc_zero}
 *
 * On RV32, 64-bit operations use LO/HI pairs:
 *   LOAD:  execute LOAD (stores rs2 as init_lo), then LOAD_HI (stores rs2 as init_hi, triggers)
 *   ACCUM: execute ACCUM (stores rs1 as delta_lo), then ACCUM_HI (stores rs1 as delta_hi, triggers)
 *   READ:  execute READ (returns state_lo), then READ_HI (returns state_hi)
 *
 * ATOMiK Project — April 2026
 */

module atomik_cfu_adapter #(
    parameter N_BANKS     = 1,
    parameter ADDR_WIDTH  = 8,
    parameter DATA_WIDTH  = 64,
    parameter VERSION     = 2
) (
    input  wire        clk,
    input  wire        rst_n,

    /* CFU command bus (from VexRiscv) */
    input  wire        cmd_valid,
    output reg         cmd_ready,
    input  wire [2:0]  cmd_function_id,   /* funct3 */
    input  wire [31:0] cmd_inputs_0,      /* rs1 */
    input  wire [31:0] cmd_inputs_1,      /* rs2 */

    /* CFU response bus (to VexRiscv) */
    output reg         rsp_valid,
    input  wire        rsp_ready,
    output reg  [31:0] rsp_outputs_0,     /* rd */
    output wire        rsp_response_ok
);

    assign rsp_response_ok = 1'b1;  /* Always OK — no error conditions */

    /* ── ATOMiK state ──────────────────────────────────────────────── */

    /* State table: 256 entries x 64 bits */
    reg [DATA_WIDTH-1:0] state_table [0:(1<<ADDR_WIDTH)-1];

    /* Accumulator (shared across all addresses) */
    reg [DATA_WIDTH-1:0] accumulator;

    /* Current address (set by LOAD/SWAP) */
    reg [ADDR_WIDTH-1:0] current_addr;

    /* Latched LO halves for 64-bit operations */
    reg [31:0] init_lo;
    reg [31:0] delta_lo;

    /* Derived signals */
    wire [DATA_WIDTH-1:0] initial_state = state_table[current_addr];
    wire [DATA_WIDTH-1:0] current_state = initial_state ^ accumulator;
    wire                  acc_zero      = (accumulator == {DATA_WIDTH{1'b0}});

    /* Status word */
    wire [31:0] status_word = {8'd0, VERSION[7:0], N_BANKS[7:0], 7'd0, acc_zero};

    /* ── Function ID decode ────────────────────────────────────────── */

    localparam F_LOAD     = 3'b000;
    localparam F_ACCUM    = 3'b001;
    localparam F_READ     = 3'b010;
    localparam F_SWAP     = 3'b011;
    localparam F_LOAD_HI  = 3'b100;
    localparam F_ACCUM_HI = 3'b101;
    localparam F_READ_HI  = 3'b110;
    localparam F_STATUS   = 3'b111;

    /* ── Command processing ────────────────────────────────────────── */

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            accumulator  <= {DATA_WIDTH{1'b0}};
            current_addr <= {ADDR_WIDTH{1'b0}};
            init_lo      <= 32'b0;
            delta_lo     <= 32'b0;
            cmd_ready    <= 1'b0;
            rsp_valid    <= 1'b0;
            rsp_outputs_0 <= 32'b0;
        end else begin
            /* Default: deassert handshake signals */
            cmd_ready <= 1'b0;
            if (rsp_valid && rsp_ready)
                rsp_valid <= 1'b0;

            /* Process command when valid and response channel is free */
            if (cmd_valid && !cmd_ready && (!rsp_valid || rsp_ready)) begin
                cmd_ready <= 1'b1;
                rsp_valid <= 1'b1;

                case (cmd_function_id)
                    F_LOAD: begin
                        /* Latch address and init_lo; wait for LOAD_HI to trigger */
                        current_addr <= cmd_inputs_0[ADDR_WIDTH-1:0];
                        init_lo      <= cmd_inputs_1;
                        rsp_outputs_0 <= current_state[31:0];
                    end

                    F_LOAD_HI: begin
                        /* Trigger LOAD: write {init_hi, init_lo} to state table, clear acc */
                        state_table[current_addr] <= {cmd_inputs_1, init_lo};
                        accumulator <= {DATA_WIDTH{1'b0}};
                        rsp_outputs_0 <= current_state[63:32];
                    end

                    F_ACCUM: begin
                        /* Latch delta_lo; wait for ACCUM_HI to trigger */
                        delta_lo <= cmd_inputs_0;
                        rsp_outputs_0 <= current_state[31:0];
                    end

                    F_ACCUM_HI: begin
                        /* Trigger ACCUM: XOR {delta_hi, delta_lo} into accumulator */
                        accumulator <= accumulator ^ {cmd_inputs_0, delta_lo};
                        rsp_outputs_0 <= current_state[63:32];
                    end

                    F_READ: begin
                        rsp_outputs_0 <= current_state[31:0];
                    end

                    F_READ_HI: begin
                        rsp_outputs_0 <= current_state[63:32];
                    end

                    F_SWAP: begin
                        /* SWAP: commit current_state at OLD addr, switch to NEW addr */
                        state_table[current_addr] <= current_state;
                        current_addr <= cmd_inputs_0[ADDR_WIDTH-1:0];
                        accumulator <= {DATA_WIDTH{1'b0}};
                        rsp_outputs_0 <= current_state[31:0];
                    end

                    F_STATUS: begin
                        rsp_outputs_0 <= status_word;
                    end
                endcase
            end
        end
    end

    /* ── Initialize state table to zero ────────────────────────────── */
    integer i;
    initial begin
        for (i = 0; i < (1 << ADDR_WIDTH); i = i + 1)
            state_table[i] = {DATA_WIDTH{1'b0}};
    end

endmodule
