// =============================================================================
// ATOMiK Video Domain Hardware Demonstrator - Top Level
//
// Module:      video_demo_top
// Description: H.264 delta pipeline demo for Tang Nano 9K. Integrates
//              atomik_core_v2 (256-bit) with a simulated camera frame source
//              and performance instrumentation (perf_counter, throughput
//              monitor, latency timer).
//
// Architecture:
//   ┌──────────────────────────────────────────────────────────────────────┐
//   │                        video_demo_top                               │
//   │                                                                     │
//   │  ┌──────────────────┐     ┌──────────────────────────────────────┐  │
//   │  │ camera_interface │────>│           atomik_core_v2             │  │
//   │  │  (frame deltas)  │     │          DATA_WIDTH=256              │  │
//   │  └──────────────────┘     └──────────────────────────────────────┘  │
//   │                                                                     │
//   │  ┌──────────────┐  ┌──────────────────┐  ┌────────────────┐        │
//   │  │ perf_counter │  │throughput_monitor │  │ latency_timer  │        │
//   │  └──────────────┘  └──────────────────┘  └────────────────┘        │
//   │                                                                     │
//   │  LED[5:0] status outputs                                            │
//   └──────────────────────────────────────────────────────────────────────┘
//
// Target: Gowin GW1NR-LV9QN88C6/I5 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   January 27, 2026
// =============================================================================

`timescale 1ns / 1ps

// Operation code definitions (must match atomik_core_v2)
`define OP_NOP        2'b00
`define OP_LOAD       2'b01
`define OP_ACCUMULATE 2'b10
`define OP_READ       2'b11

module video_demo_top #(
    parameter DATA_WIDTH     = 256,
    parameter ROLLBACK_DEPTH = 512
)(
    // =========================================================================
    // Clock and Reset
    // =========================================================================
    input  wire        clk_27m,       // 27 MHz onboard crystal
    input  wire        btn_rst_n,     // Active-low reset button (S1)

    // =========================================================================
    // LED Status Outputs (Active-Low, Tang Nano 9K has 6 LEDs)
    // =========================================================================
    output wire [5:0]  led,

    // =========================================================================
    // Camera DVP Interface (directly mapped, directly active when connected)
    // =========================================================================
    input  wire        cam_pclk,      // Pixel clock from camera
    input  wire        cam_vsync,     // Vertical sync
    input  wire        cam_href,      // Horizontal reference
    input  wire [7:0]  cam_data       // Pixel data bus
);

    // =========================================================================
    // Reset Synchronization
    // =========================================================================
    reg [15:0] por_cnt;
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
    // Heartbeat Counter (LED blink at ~1.6 Hz)
    // =========================================================================
    reg [23:0] heartbeat_cnt;
    always @(posedge clk_27m or negedge sys_rst_n) begin
        if (!sys_rst_n)
            heartbeat_cnt <= 24'd0;
        else
            heartbeat_cnt <= heartbeat_cnt + 24'd1;
    end

    // =========================================================================
    // Camera / Frame Source Interface
    // =========================================================================
    wire [DATA_WIDTH-1:0] frame_delta;
    wire                  cam_ready;

    camera_interface #(
        .DATA_WIDTH (DATA_WIDTH),
        .FRAME_DIV  (27_000)       // ~1000 fps at 27 MHz (fast demo rate)
    ) u_camera (
        .clk       (clk_27m),
        .rst_n     (sys_rst_n),
        .cam_vsync (cam_vsync),
        .cam_href  (cam_href),
        .cam_data  (cam_data),
        .frame_delta (frame_delta),
        .cam_ready   (cam_ready)
    );

    // =========================================================================
    // Demo State Machine
    // =========================================================================
    // Drives atomik_core_v2 through:
    //   IDLE -> LOAD initial frame -> ACCUMULATE frame deltas -> READ -> repeat
    localparam FSM_IDLE       = 3'd0;
    localparam FSM_LOAD       = 3'd1;
    localparam FSM_LOAD_WAIT  = 3'd2;
    localparam FSM_ACCUM      = 3'd3;
    localparam FSM_ACCUM_WAIT = 3'd4;
    localparam FSM_READ       = 3'd5;
    localparam FSM_READ_WAIT  = 3'd6;

    reg [2:0]              fsm_state;
    reg [1:0]              core_operation;
    reg [DATA_WIDTH-1:0]   core_data_in;
    reg                    loaded;          // Initial frame loaded flag
    reg                    perf_start;      // Pulse to start perf counter
    reg                    perf_stop;       // Pulse to stop perf counter
    reg                    latency_start;   // Latency measurement start
    reg [1:0]              latency_op;      // Operation being measured

    wire [DATA_WIDTH-1:0]  core_data_out;
    wire                   core_data_valid;
    wire                   core_acc_zero;
    wire [DATA_WIDTH-1:0]  core_dbg_init;
    wire [DATA_WIDTH-1:0]  core_dbg_accum;

    always @(posedge clk_27m or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            fsm_state      <= FSM_IDLE;
            core_operation <= `OP_NOP;
            core_data_in   <= {DATA_WIDTH{1'b0}};
            loaded         <= 1'b0;
            perf_start     <= 1'b0;
            perf_stop      <= 1'b0;
            latency_start  <= 1'b0;
            latency_op     <= `OP_NOP;
        end
        else begin
            // Default: clear pulses
            core_operation <= `OP_NOP;
            perf_start     <= 1'b0;
            perf_stop      <= 1'b0;
            latency_start  <= 1'b0;

            case (fsm_state)
                FSM_IDLE: begin
                    if (!loaded) begin
                        // Load a synthetic initial frame state
                        fsm_state <= FSM_LOAD;
                    end
                    else if (cam_ready) begin
                        // New frame delta available
                        fsm_state <= FSM_ACCUM;
                    end
                end

                FSM_LOAD: begin
                    core_operation <= `OP_LOAD;
                    core_data_in   <= {DATA_WIDTH{1'b0}};  // Zero initial state
                    perf_start     <= 1'b1;
                    latency_start  <= 1'b1;
                    latency_op     <= `OP_LOAD;
                    fsm_state      <= FSM_LOAD_WAIT;
                end

                FSM_LOAD_WAIT: begin
                    if (core_data_valid) begin
                        loaded    <= 1'b1;
                        fsm_state <= FSM_IDLE;
                    end
                end

                FSM_ACCUM: begin
                    core_operation <= `OP_ACCUMULATE;
                    core_data_in   <= frame_delta;
                    latency_start  <= 1'b1;
                    latency_op     <= `OP_ACCUMULATE;
                    fsm_state      <= FSM_ACCUM_WAIT;
                end

                FSM_ACCUM_WAIT: begin
                    if (core_data_valid) begin
                        fsm_state <= FSM_READ;
                    end
                end

                FSM_READ: begin
                    core_operation <= `OP_READ;
                    latency_start  <= 1'b1;
                    latency_op     <= `OP_READ;
                    fsm_state      <= FSM_READ_WAIT;
                end

                FSM_READ_WAIT: begin
                    if (core_data_valid) begin
                        perf_stop <= 1'b1;
                        fsm_state <= FSM_IDLE;
                    end
                end

                default: fsm_state <= FSM_IDLE;
            endcase
        end
    end

    // =========================================================================
    // ATOMiK Core v2 (256-bit Delta Pipeline)
    // =========================================================================
    atomik_core_v2 #(
        .DATA_WIDTH(DATA_WIDTH)
    ) u_core (
        .clk                (clk_27m),
        .rst_n              (sys_rst_n),
        .operation          (core_operation),
        .data_in            (core_data_in),
        .data_out           (core_data_out),
        .data_valid         (core_data_valid),
        .accumulator_zero   (core_acc_zero),
        .debug_initial_state(core_dbg_init),
        .debug_accumulator  (core_dbg_accum)
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
        .start   (perf_start),
        .stop    (perf_stop),
        .clear   (1'b0),
        .count   (perf_count),
        .running (perf_running),
        .done    (perf_done)
    );

    // =========================================================================
    // Throughput Monitor
    // =========================================================================
    wire [31:0] tput_ops_count;
    wire [31:0] tput_ops_result;
    wire [31:0] tput_window_count;
    wire        tput_window_done;

    throughput_monitor #(
        .WINDOW_CYCLES(27_000_000),   // 1 second at 27 MHz
        .COUNT_WIDTH  (32)
    ) u_throughput (
        .clk          (clk_27m),
        .rst_n        (sys_rst_n),
        .enable       (loaded),
        .op_valid     (core_data_valid),
        .ops_count    (tput_ops_count),
        .ops_result   (tput_ops_result),
        .window_count (tput_window_count),
        .window_done  (tput_window_done)
    );

    // =========================================================================
    // Latency Timer
    // =========================================================================
    wire [15:0] lat_load;
    wire [15:0] lat_accumulate;
    wire [15:0] lat_reconstruct;
    wire [15:0] lat_rollback;
    wire        lat_measuring;

    latency_timer #(
        .COUNT_WIDTH(16)
    ) u_latency (
        .clk                  (clk_27m),
        .rst_n                (sys_rst_n),
        .operation            (latency_op),
        .op_start             (latency_start),
        .op_done              (core_data_valid),
        .load_latency         (lat_load),
        .accumulate_latency   (lat_accumulate),
        .reconstruct_latency  (lat_reconstruct),
        .rollback_latency     (lat_rollback),
        .measuring            (lat_measuring)
    );

    // =========================================================================
    // LED Status Display (Active-Low on Tang Nano 9K)
    // =========================================================================
    // LED[5] - Heartbeat (blinks to show FPGA is alive)
    // LED[4] - System reset done (steady on when running)
    // LED[3] - Initial frame loaded
    // LED[2] - Accumulator is zero (all deltas cancel)
    // LED[1] - Performance counter running
    // LED[0] - Throughput window complete (blinks each second)

    assign led[5] = ~heartbeat_cnt[23];
    assign led[4] = ~sys_rst_n;
    assign led[3] = ~loaded;
    assign led[2] = ~core_acc_zero;
    assign led[1] = ~perf_running;
    assign led[0] = ~tput_window_done;

endmodule
