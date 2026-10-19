// =============================================================================
// ATOMiK CFU Adapter — Wishbone MMIO Wrapper
//
// Wraps atomik_cfu_adapter.v with a Wishbone slave interface for MMIO access
// from Linux userspace (/dev/mem or UIO). This allows testing the adapter
// logic on the proven SMP SoC without requiring CfuPlugin integration.
//
// Register map (32-bit word-addressed from base):
//   0x00  CMD       — Write: funct3 in dat_w[2:0] triggers command
//                     (NOT the top bits — a stale comment here cost a day)
//   0x04  RS1       — Write: rs1 (input 0)
//   0x08  RS2       — Write: rs2 (input 1)
//   0x0C  RD        — Read: rd (output from last command)
//   0x10  STATUS    — Read: {30'b0, rsp_ok, busy}
//
// Usage from software:
//   1. Write RS1 and RS2 (order doesn't matter)
//   2. Write CMD with funct3 — this triggers the operation
//   3. Poll STATUS until busy=0
//   4. Read RD for the result
//
// For 64-bit ATOMiK ops on RV32:
//   LOAD:     write RS1=addr, RS2=init_lo, CMD=0  → write RS2=init_hi, CMD=4
//   ACCUM:    write RS1=delta_lo, CMD=1            → write RS1=delta_hi, CMD=5
//   READ:     CMD=2 → read RD (lo)                 → CMD=6 → read RD (hi)
//   SWAP:     write RS1=addr, CMD=3
//   STATUS:   CMD=7 → read RD
//
// =============================================================================

module atomik_cfu_wishbone (
    input  wire        clk,
    input  wire        rst,

    // Wishbone slave
    input  wire [2:0]  adr,       // Word address (8 registers)
    input  wire [31:0] dat_w,
    output reg  [31:0] dat_r,
    input  wire        we,
    input  wire        cyc,
    input  wire        stb,
    output reg         ack
);

    // Registers
    reg [2:0]  reg_funct3;
    reg [31:0] reg_rs1;
    reg [31:0] reg_rs2;
    reg [31:0] reg_rd;
    reg        reg_busy;

    // CFU adapter wires
    reg         cmd_valid;
    wire        cmd_ready;
    wire        rsp_valid;
    reg         rsp_ready;
    wire [31:0] rsp_outputs_0;
    wire        rsp_response_ok;

    // Instantiate the ATOMiK CFU adapter
    atomik_cfu_adapter #(
        .N_BANKS    (1),
        .ADDR_WIDTH (8),
        .DATA_WIDTH (64),
        .VERSION    (2)
    ) u_adapter (
        .clk              (clk),
        .rst_n            (~rst),
        .cmd_valid        (cmd_valid),
        .cmd_ready        (cmd_ready),
        .cmd_function_id  (reg_funct3),
        .cmd_inputs_0     (reg_rs1),
        .cmd_inputs_1     (reg_rs2),
        .rsp_valid        (rsp_valid),
        .rsp_ready        (rsp_ready),
        .rsp_outputs_0    (rsp_outputs_0),
        .rsp_response_ok  (rsp_response_ok)
    );

    // FSM states
    localparam S_IDLE    = 2'd0;
    localparam S_CMD     = 2'd1;
    localparam S_WAIT    = 2'd2;

    reg [1:0] state;

    // Wishbone transaction detect (active for one cycle per bus access)
    wire wb_access = cyc & stb & ~ack;

    // Single always block — no multi-driver hazard
    always @(posedge clk) begin
        if (rst) begin
            state      <= S_IDLE;
            cmd_valid  <= 1'b0;
            rsp_ready  <= 1'b0;
            reg_rd     <= 32'd0;
            reg_busy   <= 1'b0;
            reg_funct3 <= 3'd0;
            reg_rs1    <= 32'd0;
            reg_rs2    <= 32'd0;
            ack        <= 1'b0;
            dat_r      <= 32'd0;
        end else begin
            // Default: deassert ack every cycle
            ack <= 1'b0;

            // ── FSM: drive adapter handshake ─────────────────────────
            case (state)
                S_IDLE: begin
                    cmd_valid <= 1'b0;
                    rsp_ready <= 1'b0;
                end

                S_CMD: begin
                    cmd_valid <= 1'b1;
                    if (cmd_valid && cmd_ready) begin
                        cmd_valid <= 1'b0;
                        rsp_ready <= 1'b1;
                        state     <= S_WAIT;
                    end
                end

                S_WAIT: begin
                    if (rsp_valid && rsp_ready) begin
                        reg_rd    <= rsp_outputs_0;
                        rsp_ready <= 1'b0;
                        reg_busy  <= 1'b0;
                        state     <= S_IDLE;
                    end
                end

                default: state <= S_IDLE;
            endcase

            // ── Wishbone register access ─────────────────────────────
            if (wb_access) begin
                ack <= 1'b1;

                case (adr)
                    3'h0: begin  // CMD — triggers operation
                        if (we && state == S_IDLE) begin
                            reg_funct3 <= dat_w[2:0];
                            reg_busy   <= 1'b1;
                            state      <= S_CMD;
                        end
                        dat_r <= {29'd0, reg_funct3};
                    end

                    3'h1: begin  // RS1
                        if (we) reg_rs1 <= dat_w;
                        dat_r <= reg_rs1;
                    end

                    3'h2: begin  // RS2
                        if (we) reg_rs2 <= dat_w;
                        dat_r <= reg_rs2;
                    end

                    3'h3: begin  // RD (read-only)
                        dat_r <= reg_rd;
                    end

                    3'h4: begin  // STATUS
                        dat_r <= {30'd0, rsp_response_ok, reg_busy};
                    end

                    default: dat_r <= 32'd0;
                endcase
            end
        end
    end

endmodule
