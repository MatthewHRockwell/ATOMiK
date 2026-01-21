// =============================================================================
// ATOMiK BIOS: UART Genome Loader (v1.6 - PLL-safe)
// Fixes:
// - PHASE_INC computed from CLK_FREQ and BAUD_RATE (works at 27MHz and 81MHz)
// - Reset added to baud gen + UART receiver regs for deterministic bring-up
// =============================================================================

module uart_genome_loader #(
    parameter integer CLK_FREQ  = 27000000,
    parameter integer BAUD_RATE = 115200
)(
    input  wire        clk,
    input  wire        rst_n,
    input  wire        uart_rx,
    output reg  [31:0] poly_freq_out,
    output reg  [31:0] poly_seed_out,
    output reg  [255:0] dna_storage,
    output reg         core_enable,
    output wire        loader_busy,
    output reg         otp_en
);

    // --- 1. SIGNAL STABILIZATION & SYNC ---
    reg [2:0] rx_sync;
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) rx_sync <= 3'b111;
        else        rx_sync <= {rx_sync[1:0], uart_rx};
    end

    // Majority vote filter to ignore micro-glitches
    wire rx_clean = (rx_sync[2] & rx_sync[1]) |
                    (rx_sync[2] & rx_sync[0]) |
                    (rx_sync[1] & rx_sync[0]);

    // --- 2. PHASE-ACCUMULATOR BAUD GENERATOR (16x oversample) ---
    // Increment = round((BAUD * 16 / CLK) * 2^16)
    localparam integer OVERSAMPLE = 16;
    localparam integer PHASE_INC_I =
        (BAUD_RATE * OVERSAMPLE * 65536 + (CLK_FREQ/2)) / CLK_FREQ;

    localparam [15:0] PHASE_INC = (PHASE_INC_I < 1) ? 16'd1 :
                                  (PHASE_INC_I > 65535) ? 16'hFFFF :
                                  PHASE_INC_I[15:0];

    reg [15:0] phase_acc;
    reg        tick;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            phase_acc <= 16'd0;
            tick      <= 1'b0;
        end else begin
            {tick, phase_acc} <= phase_acc + PHASE_INC;
        end
    end

    // --- 3. ROBUST UART RECEIVER (16x oversample) ---
    reg [3:0] b_cnt;
    reg [3:0] s_cnt;
    reg [7:0] rx_reg;
    reg       rx_done_tick;
    reg [1:0] uart_state;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            b_cnt        <= 4'd0;
            s_cnt        <= 4'd0;
            rx_reg       <= 8'd0;
            rx_done_tick <= 1'b0;
            uart_state   <= 2'd0;
        end else begin
            rx_done_tick <= 1'b0;

            if (tick) begin
                case (uart_state)
                    2'd0: begin // IDLE: wait for start bit
                        if (!rx_clean) begin
                            s_cnt      <= 4'd0;
                            uart_state <= 2'd1;
                        end
                    end

                    2'd1: begin // Validate start bit at center
                        s_cnt <= s_cnt + 4'd1;
                        if (s_cnt == 4'd7) begin
                            if (!rx_clean) begin
                                s_cnt      <= 4'd0;
                                b_cnt      <= 4'd0;
                                uart_state <= 2'd2;
                            end else begin
                                uart_state <= 2'd0;
                            end
                        end
                    end

                    2'd2: begin // Data bits
                        s_cnt <= s_cnt + 4'd1;
                        if (s_cnt == 4'd15) begin
                            s_cnt  <= 4'd0;
                            rx_reg <= {rx_clean, rx_reg[7:1]};
                            if (b_cnt == 4'd7) uart_state <= 2'd3;
                            else               b_cnt <= b_cnt + 4'd1;
                        end
                    end

                    2'd3: begin // Stop bit
                        s_cnt <= s_cnt + 4'd1;
                        if (s_cnt == 4'd15) begin
                            rx_done_tick <= 1'b1;
                            uart_state   <= 2'd0;
                        end
                    end

                    default: uart_state <= 2'd0;
                endcase
            end
        end
    end

    // --- 4. ATOMiK BIOS STATE MACHINE ---
    localparam [2:0] S_IDLE    = 3'd0;
    localparam [2:0] S_MAGIC   = 3'd1;
    localparam [2:0] S_VERSION = 3'd2;
    localparam [2:0] S_FREQ    = 3'd3;
    localparam [2:0] S_POLICY  = 3'd4;
    localparam [2:0] S_DNA     = 3'd5;

    reg [2:0] state;
    reg [5:0] byte_idx;

    reg [21:0] busy_stretch;
    assign loader_busy = (state != S_IDLE) || (busy_stretch != 0);

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state         <= S_IDLE;
            core_enable   <= 1'b0;
            poly_freq_out <= 32'd0;
            poly_seed_out <= 32'h12345678;
            otp_en        <= 1'b0;
            dna_storage   <= 256'd0;
            byte_idx      <= 6'd0;
            busy_stretch  <= 22'd0;
        end else begin
            if (busy_stretch != 0) busy_stretch <= busy_stretch - 22'd1;

            if (rx_done_tick) begin
                case (state)
                    S_IDLE: begin
                        if (rx_reg == 8'h41) begin // 'A'
                            state        <= S_MAGIC;
                            byte_idx     <= 6'd1;
                            core_enable  <= 1'b0;
                            busy_stretch <= 22'h3FFFFF;
                        end
                    end

                    S_MAGIC: begin
                        if (byte_idx == 6'd1 && rx_reg == 8'h54)       byte_idx <= 6'd2; // 'T'
                        else if (byte_idx == 6'd2 && rx_reg == 8'h4F)  byte_idx <= 6'd3; // 'O'
                        else if (byte_idx == 6'd3 && rx_reg == 8'h4D) begin               // 'M'
                            state        <= S_VERSION;
                            byte_idx     <= 6'd0;
                            busy_stretch <= 22'h3FFFFF;
                        end else begin
                            state <= S_IDLE;
                        end
                    end

                    S_VERSION: begin
                        state    <= S_FREQ;
                        byte_idx <= 6'd0;
                    end

                    S_FREQ: begin
                        poly_freq_out <= {poly_freq_out[23:0], rx_reg};
                        if (byte_idx == 6'd3) begin
                            state    <= S_POLICY;
                            byte_idx <= 6'd0;
                        end else begin
                            byte_idx <= byte_idx + 6'd1;
                        end
                    end

                    S_POLICY: begin
                        otp_en   <= rx_reg[0];
                        state    <= S_DNA;
                        byte_idx <= 6'd0;
                    end

                    S_DNA: begin
                        dna_storage <= {dna_storage[247:0], rx_reg};
                        if (byte_idx == 6'd31) begin
                            state        <= S_IDLE;
                            core_enable  <= 1'b1;
                            busy_stretch <= 22'd0;
                        end else begin
                            byte_idx <= byte_idx + 6'd1;
                        end
                    end

                    default: state <= S_IDLE;
                endcase
            end
        end
    end

endmodule
