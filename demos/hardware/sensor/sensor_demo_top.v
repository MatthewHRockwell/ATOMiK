// =============================================================================
// ATOMiK Edge Sensor Domain - Hardware Demonstrator Top-Level
//
// Module:      sensor_demo_top
// Description: 64-bit IMU fusion delta pipeline on Tang Nano 9K.
//              Integrates atomik_core_v2 for delta processing with a SPI IMU
//              interface (simulated fallback when no physical sensor attached).
//
// Architecture:
//   clk_27m ──► [Reset Sync] ──► internal_rst_n
//                                    │
//                 ┌──────────────────┴──────────────────────┐
//                 │           sensor_demo_top                │
//                 │                                          │
//                 │  ┌──────────────┐   ┌────────────────┐  │
//                 │  │ spi_imu_intf │──►│ atomik_core_v2 │  │
//                 │  │ (sensor data)│   │  (64-bit delta) │  │
//                 │  └──────────────┘   └───────┬────────┘  │
//                 │                              │           │
//                 │  ┌──────────┐  ┌───────────┐│┌────────┐ │
//                 │  │perf_cntr │  │throughput  │││latency │ │
//                 │  └──────────┘  │ _monitor   │││_timer  │ │
//                 │                └───────────┘│└────────┘ │
//                 │                              ▼           │
//                 │                    [LED / alert_out]     │
//                 └──────────────────────────────────────────┘
//
// Operation Encoding (matches atomik_core_v2):
//   2'b00 (NOP):        No operation, hold state
//   2'b01 (LOAD):       Load initial_state <- data_in
//   2'b10 (ACCUMULATE): Accumulate delta: accumulator <- accumulator XOR data_in
//   2'b11 (READ):       Output current_state to data_out
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   January 27, 2026
// =============================================================================

`timescale 1ns / 1ps

module sensor_demo_top #(
    parameter DATA_WIDTH      = 64,
    parameter ROLLBACK_DEPTH  = 1024,
    parameter ALERT_THRESHOLD = 32'h0000FFFF
)(
    // =========================================================================
    // Clock and Reset
    // =========================================================================
    input  wire        clk_27m,       // 27 MHz onboard crystal
    input  wire        btn_rst_n,     // Active-low reset button (S1)

    // =========================================================================
    // LED Status (Active-Low, 6 LEDs)
    // =========================================================================
    output wire [5:0]  led,

    // =========================================================================
    // SPI IMU Interface
    // =========================================================================
    output wire        spi_sclk,
    output wire        spi_mosi,
    input  wire        spi_miso,
    output wire        spi_cs_n,

    // =========================================================================
    // Alert Output
    // =========================================================================
    output wire        alert_out
);

    // =========================================================================
    // Operation Code Definitions
    // =========================================================================
    localparam OP_NOP        = 2'b00;
    localparam OP_LOAD       = 2'b01;
    localparam OP_ACCUMULATE = 2'b10;
    localparam OP_READ       = 2'b11;

    // =========================================================================
    // Reset Synchronization (Power-On Reset + Button Debounce)
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
    wire internal_rst_n = rst_sync[2];

    // =========================================================================
    // Heartbeat Counter (Blink indicator)
    // =========================================================================
    reg [23:0] heartbeat_cnt;
    always @(posedge clk_27m or negedge internal_rst_n) begin
        if (!internal_rst_n)
            heartbeat_cnt <= 24'd0;
        else
            heartbeat_cnt <= heartbeat_cnt + 24'd1;
    end

    // =========================================================================
    // SPI IMU Interface (Simulated fallback)
    // =========================================================================
    wire                    imu_data_ready;
    wire [DATA_WIDTH-1:0]   imu_sensor_data;

    spi_imu_interface #(
        .DATA_WIDTH(DATA_WIDTH)
    ) u_spi_imu (
        .clk         (clk_27m),
        .rst_n       (internal_rst_n),
        .spi_sclk    (spi_sclk),
        .spi_mosi    (spi_mosi),
        .spi_miso    (spi_miso),
        .spi_cs_n    (spi_cs_n),
        .data_ready  (imu_data_ready),
        .sensor_data (imu_sensor_data)
    );

    // =========================================================================
    // ATOMiK Core v2 (64-bit Delta Pipeline)
    // =========================================================================
    reg  [1:0]              core_operation;
    reg  [DATA_WIDTH-1:0]   core_data_in;
    wire [DATA_WIDTH-1:0]   core_data_out;
    wire                    core_data_valid;
    wire                    core_accumulator_zero;
    wire [DATA_WIDTH-1:0]   core_debug_initial;
    wire [DATA_WIDTH-1:0]   core_debug_accum;

    atomik_core_v2 #(
        .DATA_WIDTH(DATA_WIDTH)
    ) u_core (
        .clk               (clk_27m),
        .rst_n             (internal_rst_n),
        .operation         (core_operation),
        .data_in           (core_data_in),
        .data_out          (core_data_out),
        .data_valid        (core_data_valid),
        .accumulator_zero  (core_accumulator_zero),
        .debug_initial_state(core_debug_initial),
        .debug_accumulator (core_debug_accum)
    );

    // =========================================================================
    // Sensor Pipeline State Machine
    // =========================================================================
    localparam S_INIT      = 3'd0;
    localparam S_LOAD      = 3'd1;
    localparam S_WAIT_DATA = 3'd2;
    localparam S_ACCUMULATE= 3'd3;
    localparam S_READ      = 3'd4;
    localparam S_IDLE      = 3'd5;

    reg [2:0]  pipe_state;
    reg        initial_loaded;
    reg [31:0] delta_count;

    always @(posedge clk_27m or negedge internal_rst_n) begin
        if (!internal_rst_n) begin
            pipe_state     <= S_INIT;
            core_operation <= OP_NOP;
            core_data_in   <= {DATA_WIDTH{1'b0}};
            initial_loaded <= 1'b0;
            delta_count    <= 32'd0;
        end
        else begin
            core_operation <= OP_NOP;

            case (pipe_state)
                S_INIT: begin
                    // Load an initial sensor baseline state
                    core_operation <= OP_LOAD;
                    core_data_in   <= 64'hACCE_1E20_6900_BA5E;
                    initial_loaded <= 1'b1;
                    pipe_state     <= S_LOAD;
                end

                S_LOAD: begin
                    // Wait one cycle for LOAD to complete
                    pipe_state <= S_WAIT_DATA;
                end

                S_WAIT_DATA: begin
                    // Wait for sensor data from IMU interface
                    if (imu_data_ready) begin
                        core_operation <= OP_ACCUMULATE;
                        core_data_in   <= imu_sensor_data;
                        delta_count    <= delta_count + 32'd1;
                        pipe_state     <= S_ACCUMULATE;
                    end
                end

                S_ACCUMULATE: begin
                    // Accumulation done in 1 cycle, issue READ
                    core_operation <= OP_READ;
                    pipe_state     <= S_READ;
                end

                S_READ: begin
                    // READ captured, return to wait for next sample
                    pipe_state <= S_WAIT_DATA;
                end

                default: begin
                    pipe_state <= S_INIT;
                end
            endcase
        end
    end

    // =========================================================================
    // Alert Threshold Detection
    // =========================================================================
    reg alert_flag;
    always @(posedge clk_27m or negedge internal_rst_n) begin
        if (!internal_rst_n)
            alert_flag <= 1'b0;
        else if (core_data_valid && (core_data_out[31:0] > ALERT_THRESHOLD))
            alert_flag <= 1'b1;
        else if (core_accumulator_zero)
            alert_flag <= 1'b0;
    end
    assign alert_out = alert_flag;

    // =========================================================================
    // Performance Counter
    // =========================================================================
    wire        perf_start = (core_operation == OP_LOAD) ||
                             (core_operation == OP_ACCUMULATE);
    wire        perf_stop  = core_data_valid;
    wire [31:0] perf_count;
    wire        perf_running;
    wire        perf_done;

    perf_counter #(
        .COUNT_WIDTH(32)
    ) u_perf_counter (
        .clk     (clk_27m),
        .rst_n   (internal_rst_n),
        .start   (perf_start),
        .stop    (perf_stop),
        .clear   (1'b0),
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
        .WINDOW_CYCLES(27_000_000),
        .COUNT_WIDTH  (32)
    ) u_throughput (
        .clk          (clk_27m),
        .rst_n        (internal_rst_n),
        .enable       (initial_loaded),
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
        .rst_n                (internal_rst_n),
        .operation            (core_operation),
        .op_start             (core_operation != OP_NOP),
        .op_done              (core_data_valid),
        .load_latency         (lat_load),
        .accumulate_latency   (lat_accumulate),
        .reconstruct_latency  (lat_reconstruct),
        .rollback_latency     (lat_rollback),
        .measuring            (lat_measuring)
    );

    // =========================================================================
    // LED Status Display (Active-Low for Tang Nano 9K)
    // =========================================================================
    // LED[5]: Heartbeat (blinks to show clock is running)
    // LED[4]: Initial state loaded
    // LED[3]: IMU data ready activity
    // LED[2]: Core accumulator is zero
    // LED[1]: Alert flag active
    // LED[0]: Latency timer measuring

    reg [19:0] imu_activity;
    always @(posedge clk_27m or negedge internal_rst_n) begin
        if (!internal_rst_n)
            imu_activity <= 20'd0;
        else if (imu_data_ready)
            imu_activity <= 20'hFFFFF;
        else if (imu_activity != 20'd0)
            imu_activity <= imu_activity - 20'd1;
    end

    assign led[5] = ~heartbeat_cnt[23];
    assign led[4] = ~initial_loaded;
    assign led[3] = ~(imu_activity != 20'd0);
    assign led[2] = ~core_accumulator_zero;
    assign led[1] = ~alert_flag;
    assign led[0] = ~lat_measuring;

endmodule
