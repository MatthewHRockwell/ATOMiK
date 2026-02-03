// =============================================================================
// ATOMiK SPI IMU Interface - Sensor Demo Peripheral
//
// Module:      spi_imu_interface
// Description: SPI master for MPU-6050/ICM-20948 IMU communication with
//              automatic fallback to synthetic sensor data generation when
//              no physical IMU is detected.
//
// Features:
//   - SPI Mode 0 (CPOL=0, CPHA=0) master
//   - Configurable SPI clock divider
//   - Auto-detection: polls WHO_AM_I register on startup
//   - Simulated 64-bit motion deltas (accel[31:0] | gyro[31:0])
//   - data_ready pulse with 64-bit sensor_data output
//   - Synthetic acceleration and gyroscope data patterns
//
// Simulated Data Format (64-bit):
//   [63:48] accel_x delta (16-bit signed)
//   [47:32] accel_y delta (16-bit signed)
//   [31:16] gyro_x  delta (16-bit signed)
//   [15: 0] gyro_y  delta (16-bit signed)
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   January 27, 2026
// =============================================================================

`timescale 1ns / 1ps

module spi_imu_interface #(
    parameter DATA_WIDTH    = 64,
    parameter SPI_CLK_DIV   = 27,       // 27 MHz / 27 = 1 MHz SPI clock
    parameter SAMPLE_DIV    = 270_000,  // 27 MHz / 270000 = 100 Hz sample rate
    parameter DETECT_CYCLES = 1_000_000 // Cycles to wait for IMU detection
)(
    // =========================================================================
    // Clock and Reset
    // =========================================================================
    input  wire                    clk,
    input  wire                    rst_n,

    // =========================================================================
    // SPI Physical Interface
    // =========================================================================
    output reg                     spi_sclk,
    output reg                     spi_mosi,
    input  wire                    spi_miso,
    output reg                     spi_cs_n,

    // =========================================================================
    // Sensor Data Output
    // =========================================================================
    output reg                     data_ready,
    output reg  [DATA_WIDTH-1:0]  sensor_data
);

    // =========================================================================
    // IMU Register Addresses (MPU-6050 compatible)
    // =========================================================================
    localparam REG_WHO_AM_I    = 8'h75;
    localparam WHO_AM_I_MPU    = 8'h68;  // MPU-6050 device ID
    localparam WHO_AM_I_ICM    = 8'hEA;  // ICM-20948 device ID
    localparam REG_ACCEL_XOUT  = 8'h3B;

    // =========================================================================
    // SPI Master State Machine
    // =========================================================================
    localparam SPI_IDLE     = 4'd0;
    localparam SPI_CS_LOW   = 4'd1;
    localparam SPI_TX_BIT   = 4'd2;
    localparam SPI_CLK_HIGH = 4'd3;
    localparam SPI_CLK_LOW  = 4'd4;
    localparam SPI_CS_HIGH  = 4'd5;
    localparam SPI_DONE     = 4'd6;

    reg [3:0]  spi_state;
    reg [15:0] spi_clk_cnt;
    reg [4:0]  spi_bit_cnt;
    reg [7:0]  spi_tx_data;
    reg [7:0]  spi_rx_data;
    reg        spi_busy;
    reg        spi_done_pulse;
    reg        spi_start;
    reg [7:0]  spi_tx_byte;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            spi_state     <= SPI_IDLE;
            spi_sclk      <= 1'b0;
            spi_mosi      <= 1'b0;
            spi_cs_n      <= 1'b1;
            spi_clk_cnt   <= 16'd0;
            spi_bit_cnt   <= 5'd0;
            spi_tx_data   <= 8'd0;
            spi_rx_data   <= 8'd0;
            spi_busy      <= 1'b0;
            spi_done_pulse<= 1'b0;
        end
        else begin
            spi_done_pulse <= 1'b0;

            case (spi_state)
                SPI_IDLE: begin
                    spi_cs_n <= 1'b1;
                    spi_sclk <= 1'b0;
                    spi_busy <= 1'b0;
                    if (spi_start) begin
                        spi_tx_data <= spi_tx_byte;
                        spi_busy    <= 1'b1;
                        spi_state   <= SPI_CS_LOW;
                        spi_clk_cnt <= 16'd0;
                    end
                end

                SPI_CS_LOW: begin
                    spi_cs_n    <= 1'b0;
                    spi_bit_cnt <= 5'd0;
                    spi_state   <= SPI_TX_BIT;
                end

                SPI_TX_BIT: begin
                    // Drive MOSI with MSB
                    spi_mosi <= spi_tx_data[7];
                    spi_state <= SPI_CLK_HIGH;
                    spi_clk_cnt <= 16'd0;
                end

                SPI_CLK_HIGH: begin
                    if (spi_clk_cnt >= SPI_CLK_DIV[15:0] - 1) begin
                        spi_sclk    <= 1'b1;
                        // Sample MISO on rising edge (Mode 0)
                        spi_rx_data <= {spi_rx_data[6:0], spi_miso};
                        spi_clk_cnt <= 16'd0;
                        spi_state   <= SPI_CLK_LOW;
                    end
                    else begin
                        spi_clk_cnt <= spi_clk_cnt + 16'd1;
                    end
                end

                SPI_CLK_LOW: begin
                    if (spi_clk_cnt >= SPI_CLK_DIV[15:0] - 1) begin
                        spi_sclk    <= 1'b0;
                        spi_tx_data <= {spi_tx_data[6:0], 1'b0};
                        spi_clk_cnt <= 16'd0;

                        if (spi_bit_cnt == 5'd7) begin
                            spi_state <= SPI_CS_HIGH;
                        end
                        else begin
                            spi_bit_cnt <= spi_bit_cnt + 5'd1;
                            spi_state   <= SPI_TX_BIT;
                        end
                    end
                    else begin
                        spi_clk_cnt <= spi_clk_cnt + 16'd1;
                    end
                end

                SPI_CS_HIGH: begin
                    spi_cs_n       <= 1'b1;
                    spi_done_pulse <= 1'b1;
                    spi_state      <= SPI_DONE;
                end

                SPI_DONE: begin
                    spi_busy  <= 1'b0;
                    spi_state <= SPI_IDLE;
                end

                default: begin
                    spi_state <= SPI_IDLE;
                end
            endcase
        end
    end

    // =========================================================================
    // IMU Detection and Mode Selection
    // =========================================================================
    localparam MODE_DETECT   = 2'd0;
    localparam MODE_PHYSICAL = 2'd1;
    localparam MODE_SIMULATE = 2'd2;

    reg [1:0]  imu_mode;
    reg [19:0] detect_cnt;
    reg        detect_sent;
    reg        imu_detected;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            imu_mode     <= MODE_DETECT;
            detect_cnt   <= 20'd0;
            detect_sent  <= 1'b0;
            imu_detected <= 1'b0;
            spi_start    <= 1'b0;
            spi_tx_byte  <= 8'd0;
        end
        else begin
            spi_start <= 1'b0;

            case (imu_mode)
                MODE_DETECT: begin
                    if (!detect_sent && !spi_busy) begin
                        // Send read command: register address with R bit set
                        spi_tx_byte <= REG_WHO_AM_I | 8'h80;
                        spi_start   <= 1'b1;
                        detect_sent <= 1'b1;
                    end
                    else if (detect_sent && spi_done_pulse) begin
                        if (spi_rx_data == WHO_AM_I_MPU ||
                            spi_rx_data == WHO_AM_I_ICM) begin
                            imu_detected <= 1'b1;
                            imu_mode     <= MODE_PHYSICAL;
                        end
                        else begin
                            detect_cnt <= detect_cnt + 20'd1;
                        end
                        detect_sent <= 1'b0;
                    end

                    // Timeout: switch to simulated mode
                    if (detect_cnt >= DETECT_CYCLES[19:0]) begin
                        imu_mode <= MODE_SIMULATE;
                    end
                end

                MODE_PHYSICAL: begin
                    // Physical IMU read path (placeholder for full burst read)
                    // In production, burst-read 12 bytes from ACCEL_XOUT
                    imu_mode <= MODE_SIMULATE;
                end

                MODE_SIMULATE: begin
                    // Handled by synthetic data generator below
                end

                default: begin
                    imu_mode <= MODE_DETECT;
                end
            endcase
        end
    end

    // =========================================================================
    // Synthetic Sensor Data Generator
    // =========================================================================
    reg [31:0] sample_cnt;
    reg [15:0] sim_accel_x;
    reg [15:0] sim_accel_y;
    reg [15:0] sim_gyro_x;
    reg [15:0] sim_gyro_y;
    reg [15:0] phase_cnt;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            sample_cnt  <= 32'd0;
            sim_accel_x <= 16'd0;
            sim_accel_y <= 16'd0;
            sim_gyro_x  <= 16'd0;
            sim_gyro_y  <= 16'd0;
            phase_cnt   <= 16'd0;
            data_ready  <= 1'b0;
            sensor_data <= {DATA_WIDTH{1'b0}};
        end
        else begin
            data_ready <= 1'b0;

            if (imu_mode == MODE_SIMULATE) begin
                if (sample_cnt >= SAMPLE_DIV[31:0] - 1) begin
                    sample_cnt <= 32'd0;
                    phase_cnt  <= phase_cnt + 16'd1;

                    // Generate synthetic motion delta patterns
                    // Sinusoidal-ish acceleration via triangle wave
                    sim_accel_x <= {phase_cnt[7], phase_cnt[7] ? ~phase_cnt[6:0] : phase_cnt[6:0], 8'h00};
                    sim_accel_y <= {~phase_cnt[9], phase_cnt[9] ? ~phase_cnt[8:2] : phase_cnt[8:2], 8'h00};

                    // Gyroscope: slower rotation pattern
                    sim_gyro_x <= {phase_cnt[11], phase_cnt[11] ? ~phase_cnt[10:4] : phase_cnt[10:4], 8'h00};
                    sim_gyro_y <= {~phase_cnt[13], phase_cnt[13] ? ~phase_cnt[12:6] : phase_cnt[12:6], 8'h00};

                    // Pack into 64-bit sensor delta
                    sensor_data <= {sim_accel_x, sim_accel_y, sim_gyro_x, sim_gyro_y};
                    data_ready  <= 1'b1;
                end
                else begin
                    sample_cnt <= sample_cnt + 32'd1;
                end
            end
        end
    end

endmodule
