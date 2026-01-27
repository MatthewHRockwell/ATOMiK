// =============================================================================
// ATOMiK Finance Domain Hardware Demonstrator - Top Level
//
// Module:      finance_demo_top
// Description: 64-bit price tick delta pipeline on Tang Nano 9K FPGA.
//              Integrates atomik_core_v2 for XOR-based delta processing
//              with a UART tick stream interface for high-speed tick
//              ingestion and a status UART for telemetry output.
//
// Architecture:
//   uart_rx ─► tick_stream_interface ─► atomik_core_v2 ─► status UART ─► uart_tx
//                                            │
//                          ┌─────────────────┼─────────────────┐
//                          │                 │                 │
//                     perf_counter   throughput_monitor   latency_timer
//                          │                 │                 │
//                          └────────► LED status display ◄─────┘
//
// Operation Encoding (matching atomik_core_v2):
//   2'b00 (NOP):        No operation, hold state
//   2'b01 (LOAD):       Load initial_state <- data_in
//   2'b10 (ACCUMULATE): Accumulate delta: accumulator <- accumulator XOR data_in
//   2'b11 (READ):       Output current_state to data_out
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   January 2026
// =============================================================================

`timescale 1ns / 1ps

`define OP_NOP        2'b00
`define OP_LOAD       2'b01
`define OP_ACCUMULATE 2'b10
`define OP_READ       2'b11

module finance_demo_top #(
    parameter DATA_WIDTH     = 64,
    parameter ROLLBACK_DEPTH = 4096,
    parameter BAUD_RATE      = 115200,
    parameter SYS_CLK_HZ     = 27_000_000
)(
    input  wire       clk_27m,       // 27 MHz onboard crystal
    input  wire       btn_rst_n,     // Active-low reset button
    output wire [5:0] led,           // Status LEDs (active-low)
    input  wire       uart_rx,       // UART receive: tick data input
    output wire       uart_tx        // UART transmit: status output
);

    // =========================================================================
    // Reset Synchronization
    // =========================================================================
    reg [15:0] por_cnt = 16'd0;
    always @(posedge clk_27m or negedge btn_rst_n) begin
        if (!btn_rst_n)
            por_cnt <= 16'd0;
        else if (por_cnt != 16'hFFFF)
            por_cnt <= por_cnt + 16'd1;
    end
    wire por_done = (por_cnt == 16'hFFFF);

    reg [2:0] rst_sync;
    always @(posedge clk_27m or negedge btn_rst_n) begin
        if (!btn_rst_n)
            rst_sync <= 3'b000;
        else
            rst_sync <= {rst_sync[1:0], por_done};
    end
    wire sys_rst_n = rst_sync[2];

    // =========================================================================
    // Heartbeat Counter (for LED blink)
    // =========================================================================
    reg [23:0] heartbeat_cnt;
    always @(posedge clk_27m or negedge sys_rst_n) begin
        if (!sys_rst_n)
            heartbeat_cnt <= 24'd0;
        else
            heartbeat_cnt <= heartbeat_cnt + 24'd1;
    end

    // =========================================================================
    // Tick Stream Interface
    // =========================================================================
    wire [DATA_WIDTH-1:0] tick_delta;
    wire                  tick_valid;
    wire [31:0]           tick_count;

    tick_stream_interface #(
        .DATA_WIDTH  (DATA_WIDTH),
        .CLK_FREQ    (SYS_CLK_HZ),
        .BAUD_RATE   (BAUD_RATE)
    ) u_tick_stream (
        .clk         (clk_27m),
        .rst_n       (sys_rst_n),
        .uart_rx     (uart_rx),
        .sim_mode    (1'b0),
        .sim_rate    (16'd1000),
        .price_delta (tick_delta),
        .tick_valid  (tick_valid),
        .tick_count  (tick_count)
    );

    // =========================================================================
    // Core Control State Machine
    // =========================================================================
    localparam [2:0] ST_IDLE      = 3'd0;
    localparam [2:0] ST_LOAD_INIT = 3'd1;
    localparam [2:0] ST_RUN       = 3'd2;
    localparam [2:0] ST_READ      = 3'd3;
    localparam [2:0] ST_READ_WAIT = 3'd4;

    reg  [2:0]              ctrl_state;
    reg  [1:0]              core_operation;
    reg  [DATA_WIDTH-1:0]   core_data_in;
    wire [DATA_WIDTH-1:0]   core_data_out;
    wire                    core_data_valid;
    wire                    core_acc_zero;
    wire [DATA_WIDTH-1:0]   core_dbg_init;
    wire [DATA_WIDTH-1:0]   core_dbg_accum;

    // Initial price state loaded on first tick
    reg first_tick_loaded;

    always @(posedge clk_27m or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            ctrl_state       <= ST_IDLE;
            core_operation   <= `OP_NOP;
            core_data_in     <= {DATA_WIDTH{1'b0}};
            first_tick_loaded <= 1'b0;
        end else begin
            core_operation <= `OP_NOP;

            case (ctrl_state)
                ST_IDLE: begin
                    if (tick_valid && !first_tick_loaded) begin
                        // First tick: use as initial price state
                        core_operation    <= `OP_LOAD;
                        core_data_in      <= tick_delta;
                        first_tick_loaded <= 1'b1;
                        ctrl_state        <= ST_RUN;
                    end else if (tick_valid) begin
                        ctrl_state <= ST_RUN;
                    end
                end

                ST_RUN: begin
                    if (tick_valid) begin
                        core_operation <= `OP_ACCUMULATE;
                        core_data_in   <= tick_delta;
                    end
                end

                ST_READ: begin
                    core_operation <= `OP_READ;
                    ctrl_state     <= ST_READ_WAIT;
                end

                ST_READ_WAIT: begin
                    ctrl_state <= ST_RUN;
                end

                default: ctrl_state <= ST_IDLE;
            endcase
        end
    end

    // =========================================================================
    // ATOMiK Core v2 Instance (64-bit Delta Architecture)
    // =========================================================================
    atomik_core_v2 #(
        .DATA_WIDTH(DATA_WIDTH)
    ) u_core (
        .clk               (clk_27m),
        .rst_n             (sys_rst_n),
        .operation         (core_operation),
        .data_in           (core_data_in),
        .data_out          (core_data_out),
        .data_valid        (core_data_valid),
        .accumulator_zero  (core_acc_zero),
        .debug_initial_state(core_dbg_init),
        .debug_accumulator (core_dbg_accum)
    );

    // =========================================================================
    // Performance Counter
    // =========================================================================
    wire [31:0] perf_count;
    wire        perf_running;
    wire        perf_done;

    perf_counter #(
        .COUNT_WIDTH(32)
    ) u_perf (
        .clk     (clk_27m),
        .rst_n   (sys_rst_n),
        .start   (tick_valid & first_tick_loaded),
        .stop    (core_data_valid),
        .clear   (~sys_rst_n),
        .count   (perf_count),
        .running (perf_running),
        .done    (perf_done)
    );

    // =========================================================================
    // Throughput Monitor (1-second window at 27 MHz)
    // =========================================================================
    wire [31:0] tput_ops_count;
    wire [31:0] tput_ops_result;
    wire [31:0] tput_window_count;
    wire        tput_window_done;

    throughput_monitor #(
        .WINDOW_CYCLES(SYS_CLK_HZ),
        .COUNT_WIDTH  (32)
    ) u_throughput (
        .clk          (clk_27m),
        .rst_n        (sys_rst_n),
        .enable       (first_tick_loaded),
        .op_valid     (core_data_valid),
        .ops_count    (tput_ops_count),
        .ops_result   (tput_ops_result),
        .window_count (tput_window_count),
        .window_done  (tput_window_done)
    );

    // =========================================================================
    // Latency Timer
    // =========================================================================
    wire [15:0] lat_load, lat_accum, lat_read, lat_rollback;
    wire        lat_measuring;

    latency_timer #(
        .COUNT_WIDTH(16)
    ) u_latency (
        .clk                  (clk_27m),
        .rst_n                (sys_rst_n),
        .operation            (core_operation),
        .op_start             (core_operation != `OP_NOP),
        .op_done              (core_data_valid),
        .load_latency         (lat_load),
        .accumulate_latency   (lat_accum),
        .reconstruct_latency  (lat_read),
        .rollback_latency     (lat_rollback),
        .measuring            (lat_measuring)
    );

    // =========================================================================
    // Status UART Transmitter (simplified single-byte status output)
    // =========================================================================
    localparam integer BAUD_DIV = SYS_CLK_HZ / BAUD_RATE;

    reg        tx_out_r;
    reg [15:0] tx_baud_cnt;
    reg [3:0]  tx_bit_cnt;
    reg [7:0]  tx_shift;
    reg        tx_busy;
    reg [7:0]  tx_data_reg;
    reg        tx_start;

    assign uart_tx = tx_out_r;

    always @(posedge clk_27m or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            tx_baud_cnt <= 16'd0;
            tx_bit_cnt  <= 4'd0;
            tx_shift    <= 8'hFF;
            tx_busy     <= 1'b0;
            tx_out_r    <= 1'b1;
        end else begin
            if (!tx_busy && tx_start) begin
                tx_shift    <= tx_data_reg;
                tx_busy     <= 1'b1;
                tx_bit_cnt  <= 4'd0;
                tx_baud_cnt <= 16'd0;
                tx_out_r    <= 1'b0;  // Start bit
            end else if (tx_busy) begin
                if (tx_baud_cnt == BAUD_DIV[15:0] - 1) begin
                    tx_baud_cnt <= 16'd0;
                    tx_bit_cnt  <= tx_bit_cnt + 4'd1;
                    case (tx_bit_cnt)
                        4'd0, 4'd1, 4'd2, 4'd3,
                        4'd4, 4'd5, 4'd6, 4'd7: begin
                            tx_out_r <= tx_shift[0];
                            tx_shift <= {1'b1, tx_shift[7:1]};
                        end
                        4'd8: tx_out_r <= 1'b1;  // Stop bit
                        4'd9: begin
                            tx_busy  <= 1'b0;
                            tx_out_r <= 1'b1;
                        end
                        default: begin
                            tx_busy  <= 1'b0;
                            tx_out_r <= 1'b1;
                        end
                    endcase
                end else begin
                    tx_baud_cnt <= tx_baud_cnt + 16'd1;
                end
            end
        end
    end

    // Periodic status byte output (once per throughput window)
    always @(posedge clk_27m or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            tx_start    <= 1'b0;
            tx_data_reg <= 8'd0;
        end else begin
            tx_start <= 1'b0;
            if (tput_window_done && !tx_busy) begin
                // Send low byte of ops_result as status
                tx_data_reg <= tput_ops_result[7:0];
                tx_start    <= 1'b1;
            end
        end
    end

    // =========================================================================
    // LED Status Display (Active-Low on Tang Nano 9K)
    // =========================================================================
    //   LED[5]: Heartbeat (blink at ~1.6 Hz)
    //   LED[4]: First tick loaded (system active)
    //   LED[3]: Accumulator zero (delta algebra identity)
    //   LED[2]: TX busy (status UART transmitting)
    //   LED[1]: Tick activity (blinks on tick_valid)
    //   LED[0]: Latency measurement active

    reg [19:0] tick_activity;
    always @(posedge clk_27m or negedge sys_rst_n) begin
        if (!sys_rst_n)
            tick_activity <= 20'd0;
        else if (tick_valid)
            tick_activity <= 20'hFFFFF;
        else if (tick_activity != 20'd0)
            tick_activity <= tick_activity - 20'd1;
    end

    assign led[5] = ~heartbeat_cnt[23];
    assign led[4] = ~first_tick_loaded;
    assign led[3] = ~core_acc_zero;
    assign led[2] = ~tx_busy;
    assign led[1] = ~(tick_activity != 20'd0);
    assign led[0] = ~lat_measuring;

endmodule

// =============================================================================
// Module Documentation
// =============================================================================
//
// DEPLOYMENT:
//   Target FPGA: Gowin GW1NR-9 (Tang Nano 9K)
//   Clock:       27 MHz onboard crystal (no PLL required for demo)
//   UART:        115200 baud, 8N1
//                uart_rx = tick data input (pin 18)
//                uart_tx = status output (pin 17)
//
// RESOURCE ESTIMATES (GW1NR-9):
//   atomik_core_v2:      ~192 FFs, ~161 LUTs
//   tick_stream_iface:   ~150 FFs, ~100 LUTs
//   perf_counter:         ~34 FFs,  ~20 LUTs
//   throughput_monitor:  ~128 FFs,  ~80 LUTs
//   latency_timer:        ~70 FFs,  ~40 LUTs
//   UART TX + control:    ~50 FFs,  ~30 LUTs
//   Reset + heartbeat:    ~50 FFs,  ~10 LUTs
//   ------------------------------------------------
//   TOTAL:               ~674 FFs, ~441 LUTs
//                         (10.4% FF, 5.1% LUT utilization)
//
// =============================================================================
