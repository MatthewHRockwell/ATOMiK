// =============================================================================
// ATOMiK Top-Level (Tang Nano 9K) - Phase 6 Parallel Accumulator Banks
//
// Integrates atomik_parallel_acc with N_BANKS parallel XOR accumulators
// into the same UART protocol as atomik_top.v for hardware validation.
//
// UART Protocol (identical to Phase 3):
//   - Send 'L' + 8 bytes: LOAD initial state
//   - Send 'A' + 8 bytes: ACCUMULATE delta (round-robin across banks)
//   - Send 'R': READ current state (returns 8 bytes)
//   - Send 'S': STATUS (returns accumulator_zero flag)
//
// Key: N_BANKS parallel accumulators share workload via round-robin.
// XOR merge tree reconstructs final state combinationally.
//
// Version: 6.0 (Phase 6 parallel banks)
// Date: January 27, 2026
// =============================================================================

module atomik_top_parallel #(
    parameter integer SYS_CLK_HZ = 94_500_000,
    parameter integer BAUD_RATE  = 115200,
    parameter integer USE_PLL    = 1,
    parameter integer N_BANKS    = 4
)(
    input  wire       sys_clk,
    input  wire       sys_rst_n,
    input  wire       uart_rx,
    output wire       uart_tx,
    output wire [5:0] led
);

    // =========================================================================
    // Clock Generation (PLL)
    // =========================================================================
    wire clk_int;
    wire pll_lock;

    generate
        if (USE_PLL != 0) begin : gen_pll
            atomik_pll_94p5m u_pll (
                .clkin  (sys_clk),
                .reset  (~sys_rst_n),
                .clkout (clk_int),
                .lock   (pll_lock)
            );
        end else begin : gen_nopll
            assign clk_int  = sys_clk;
            assign pll_lock = 1'b1;
        end
    endgenerate

    // =========================================================================
    // Reset Synchronization
    // =========================================================================
    reg [15:0] por_cnt = 16'd0;
    always @(posedge sys_clk or negedge sys_rst_n) begin
        if (!sys_rst_n)
            por_cnt <= 16'd0;
        else if (por_cnt != 16'hFFFF)
            por_cnt <= por_cnt + 16'd1;
    end
    wire por_done = (por_cnt == 16'hFFFF);

    reg [2:0] rst_sync;
    always @(posedge clk_int or negedge sys_rst_n) begin
        if (!sys_rst_n)
            rst_sync <= 3'b000;
        else
            rst_sync <= {rst_sync[1:0], (por_done & pll_lock)};
    end
    wire internal_rst_n = rst_sync[2];

    // =========================================================================
    // Heartbeat Counter
    // =========================================================================
    reg [23:0] heartbeat_cnt;
    always @(posedge clk_int or negedge internal_rst_n) begin
        if (!internal_rst_n)
            heartbeat_cnt <= 24'd0;
        else
            heartbeat_cnt <= heartbeat_cnt + 24'd1;
    end

    // =========================================================================
    // UART Receiver (8N1)
    // =========================================================================
    localparam integer BAUD_DIV = SYS_CLK_HZ / BAUD_RATE;
    localparam integer HALF_BIT = BAUD_DIV / 2;

    reg [15:0] rx_baud_cnt;
    reg [3:0]  rx_bit_cnt;
    reg [7:0]  rx_shift;
    reg [7:0]  rx_data;
    reg        rx_valid;
    reg [1:0]  rx_state;
    reg [2:0]  rx_sync;

    wire rx_pin = rx_sync[2];

    always @(posedge clk_int or negedge internal_rst_n) begin
        if (!internal_rst_n)
            rx_sync <= 3'b111;
        else
            rx_sync <= {rx_sync[1:0], uart_rx};
    end

    localparam RX_IDLE  = 2'd0;
    localparam RX_START = 2'd1;
    localparam RX_DATA  = 2'd2;
    localparam RX_STOP  = 2'd3;

    always @(posedge clk_int or negedge internal_rst_n) begin
        if (!internal_rst_n) begin
            rx_state    <= RX_IDLE;
            rx_baud_cnt <= 16'd0;
            rx_bit_cnt  <= 4'd0;
            rx_shift    <= 8'd0;
            rx_data     <= 8'd0;
            rx_valid    <= 1'b0;
        end else begin
            rx_valid <= 1'b0;
            case (rx_state)
                RX_IDLE: begin
                    if (!rx_pin) begin
                        rx_state    <= RX_START;
                        rx_baud_cnt <= 16'd0;
                    end
                end
                RX_START: begin
                    if (rx_baud_cnt == HALF_BIT[15:0]) begin
                        if (!rx_pin) begin
                            rx_state    <= RX_DATA;
                            rx_baud_cnt <= 16'd0;
                            rx_bit_cnt  <= 4'd0;
                        end else begin
                            rx_state <= RX_IDLE;
                        end
                    end else begin
                        rx_baud_cnt <= rx_baud_cnt + 16'd1;
                    end
                end
                RX_DATA: begin
                    if (rx_baud_cnt == BAUD_DIV[15:0] - 1) begin
                        rx_baud_cnt <= 16'd0;
                        rx_shift    <= {rx_pin, rx_shift[7:1]};
                        if (rx_bit_cnt == 4'd7)
                            rx_state <= RX_STOP;
                        else
                            rx_bit_cnt <= rx_bit_cnt + 4'd1;
                    end else begin
                        rx_baud_cnt <= rx_baud_cnt + 16'd1;
                    end
                end
                RX_STOP: begin
                    if (rx_baud_cnt == BAUD_DIV[15:0] - 1) begin
                        rx_data  <= rx_shift;
                        rx_valid <= 1'b1;
                        rx_state <= RX_IDLE;
                    end else begin
                        rx_baud_cnt <= rx_baud_cnt + 16'd1;
                    end
                end
            endcase
        end
    end

    // =========================================================================
    // UART Transmitter (8N1)
    // =========================================================================
    reg [15:0] tx_baud_cnt;
    reg [3:0]  tx_bit_cnt;
    reg [7:0]  tx_shift;
    reg        tx_busy;
    reg        tx_out;
    reg        tx_start;
    reg [7:0]  tx_data;

    assign uart_tx = tx_out;

    always @(posedge clk_int or negedge internal_rst_n) begin
        if (!internal_rst_n) begin
            tx_baud_cnt <= 16'd0;
            tx_bit_cnt  <= 4'd0;
            tx_shift    <= 8'hFF;
            tx_busy     <= 1'b0;
            tx_out      <= 1'b1;
        end else begin
            if (!tx_busy && tx_start) begin
                tx_shift    <= tx_data;
                tx_busy     <= 1'b1;
                tx_bit_cnt  <= 4'd0;
                tx_baud_cnt <= 16'd0;
                tx_out      <= 1'b0;
            end else if (tx_busy) begin
                if (tx_baud_cnt == BAUD_DIV[15:0] - 1) begin
                    tx_baud_cnt <= 16'd0;
                    tx_bit_cnt  <= tx_bit_cnt + 4'd1;
                    case (tx_bit_cnt)
                        4'd0, 4'd1, 4'd2, 4'd3, 4'd4, 4'd5, 4'd6, 4'd7: begin
                            tx_out   <= tx_shift[0];
                            tx_shift <= {1'b1, tx_shift[7:1]};
                        end
                        4'd8: tx_out <= 1'b1;
                        4'd9: begin
                            tx_busy <= 1'b0;
                            tx_out  <= 1'b1;
                        end
                        default: begin
                            tx_busy <= 1'b0;
                            tx_out  <= 1'b1;
                        end
                    endcase
                end else begin
                    tx_baud_cnt <= tx_baud_cnt + 16'd1;
                end
            end
        end
    end

    // =========================================================================
    // Parallel Accumulator Core (N banks, XOR merge tree)
    // =========================================================================
    reg         par_delta_valid;
    reg  [63:0] par_delta_in;
    reg         par_load_initial;
    reg  [63:0] par_initial_state_in;

    wire [63:0] par_current_state;
    wire [63:0] par_merged_acc;
    wire        par_acc_zero;

    atomik_parallel_acc #(
        .DELTA_WIDTH (64),
        .N_BANKS     (N_BANKS)
    ) u_parallel_acc (
        .clk                  (clk_int),
        .rst_n                (internal_rst_n),
        .delta_in             (par_delta_in),
        .delta_valid          (par_delta_valid),
        .delta_parallel_in    ({N_BANKS*64{1'b0}}),
        .delta_parallel_valid ({N_BANKS{1'b0}}),
        .parallel_mode        (1'b0),   // Round-robin for UART
        .initial_state_in     (par_initial_state_in),
        .load_initial         (par_load_initial),
        .current_state        (par_current_state),
        .merged_accumulator   (par_merged_acc),
        .accumulator_zero     (par_acc_zero),
        .current_bank         (),
        .bank_active          ()
    );

    // =========================================================================
    // Command Protocol State Machine
    // =========================================================================
    localparam CMD_IDLE      = 4'd0;
    localparam CMD_LOAD      = 4'd1;
    localparam CMD_ACCUM     = 4'd2;
    localparam CMD_READ      = 4'd3;
    localparam CMD_READ_WAIT = 4'd4;
    localparam CMD_STATUS    = 4'd5;
    localparam CMD_TX_START  = 4'd7;
    localparam CMD_TX_WAIT   = 4'd8;
    localparam CMD_TX_DONE   = 4'd9;

    reg [3:0]  cmd_state;
    reg [3:0]  byte_cnt;
    reg [63:0] cmd_buffer;
    reg [63:0] tx_buffer;
    reg [3:0]  tx_byte_cnt;
    reg [3:0]  tx_bytes_total;
    reg        cmd_active;

    always @(posedge clk_int or negedge internal_rst_n) begin
        if (!internal_rst_n) begin
            cmd_state            <= CMD_IDLE;
            byte_cnt             <= 4'd0;
            cmd_buffer           <= 64'd0;
            tx_buffer            <= 64'd0;
            tx_byte_cnt          <= 4'd0;
            tx_bytes_total       <= 4'd0;
            tx_start             <= 1'b0;
            tx_data              <= 8'd0;
            cmd_active           <= 1'b0;
            par_delta_valid      <= 1'b0;
            par_delta_in         <= 64'd0;
            par_load_initial     <= 1'b0;
            par_initial_state_in <= 64'd0;
        end else begin
            tx_start         <= 1'b0;
            par_delta_valid  <= 1'b0;
            par_load_initial <= 1'b0;

            case (cmd_state)
                CMD_IDLE: begin
                    cmd_active <= 1'b0;
                    if (rx_valid) begin
                        case (rx_data)
                            8'h4C: begin  // 'L' - Load
                                cmd_state  <= CMD_LOAD;
                                byte_cnt   <= 4'd0;
                                cmd_buffer <= 64'd0;
                                cmd_active <= 1'b1;
                            end
                            8'h41: begin  // 'A' - Accumulate
                                cmd_state  <= CMD_ACCUM;
                                byte_cnt   <= 4'd0;
                                cmd_buffer <= 64'd0;
                                cmd_active <= 1'b1;
                            end
                            8'h52: begin  // 'R' - Read
                                cmd_state  <= CMD_READ;
                                cmd_active <= 1'b1;
                            end
                            8'h53: begin  // 'S' - Status
                                cmd_state  <= CMD_STATUS;
                                cmd_active <= 1'b1;
                            end
                            default: ;
                        endcase
                    end
                end

                CMD_LOAD: begin
                    if (rx_valid) begin
                        cmd_buffer <= {cmd_buffer[55:0], rx_data};
                        if (byte_cnt == 4'd7) begin
                            par_initial_state_in <= {cmd_buffer[55:0], rx_data};
                            par_load_initial     <= 1'b1;
                            cmd_state            <= CMD_IDLE;
                            cmd_active           <= 1'b0;
                        end else begin
                            byte_cnt <= byte_cnt + 4'd1;
                        end
                    end
                end

                CMD_ACCUM: begin
                    if (rx_valid) begin
                        cmd_buffer <= {cmd_buffer[55:0], rx_data};
                        if (byte_cnt == 4'd7) begin
                            par_delta_in    <= {cmd_buffer[55:0], rx_data};
                            par_delta_valid <= 1'b1;
                            cmd_state       <= CMD_IDLE;
                            cmd_active      <= 1'b0;
                        end else begin
                            byte_cnt <= byte_cnt + 4'd1;
                        end
                    end
                end

                CMD_READ: begin
                    cmd_state <= CMD_READ_WAIT;
                end

                CMD_READ_WAIT: begin
                    tx_buffer      <= par_current_state;
                    tx_byte_cnt    <= 4'd0;
                    tx_bytes_total <= 4'd8;
                    cmd_state      <= CMD_TX_START;
                end

                CMD_STATUS: begin
                    tx_buffer      <= {par_acc_zero, 63'd0};
                    tx_byte_cnt    <= 4'd0;
                    tx_bytes_total <= 4'd1;
                    cmd_state      <= CMD_TX_START;
                end

                CMD_TX_START: begin
                    if (tx_byte_cnt < tx_bytes_total) begin
                        tx_data     <= tx_buffer[63:56];
                        tx_buffer   <= {tx_buffer[55:0], 8'd0};
                        tx_start    <= 1'b1;
                        tx_byte_cnt <= tx_byte_cnt + 4'd1;
                        cmd_state   <= CMD_TX_WAIT;
                    end else begin
                        cmd_state <= CMD_IDLE;
                    end
                end

                CMD_TX_WAIT: begin
                    if (tx_busy)
                        cmd_state <= CMD_TX_DONE;
                end

                CMD_TX_DONE: begin
                    if (!tx_busy) begin
                        if (tx_byte_cnt < tx_bytes_total)
                            cmd_state <= CMD_TX_START;
                        else
                            cmd_state <= CMD_IDLE;
                    end
                end

                default: cmd_state <= CMD_IDLE;
            endcase
        end
    end

    // =========================================================================
    // LED Status Display (Active-Low)
    // =========================================================================
    reg [19:0] rx_activity;
    always @(posedge clk_int or negedge internal_rst_n) begin
        if (!internal_rst_n)
            rx_activity <= 20'd0;
        else if (rx_valid)
            rx_activity <= 20'hFFFFF;
        else if (rx_activity != 20'd0)
            rx_activity <= rx_activity - 20'd1;
    end

    assign led[5] = ~pll_lock;
    assign led[4] = ~heartbeat_cnt[23];
    assign led[3] = ~cmd_active;
    assign led[2] = ~par_acc_zero;
    assign led[1] = ~tx_busy;
    assign led[0] = ~(rx_activity != 20'd0);

endmodule
