// =============================================================================
// ATOMiK Finance Demo - Tick Stream Interface
//
// Module:      tick_stream_interface
// Description: High-speed tick data ingestion module. Receives 8-bit UART
//              bytes and assembles them into 64-bit price_delta values.
//              Supports both live UART mode and simulated synthetic tick
//              generation for standalone FPGA testing.
//
// UART Protocol:
//   - 8 consecutive bytes (MSB first) form one 64-bit price delta
//   - tick_valid pulses high for 1 cycle when assembly is complete
//   - tick_count increments on each complete tick
//
// Simulated Mode:
//   - When sim_mode=1, generates synthetic price ticks at sim_rate interval
//   - Uses xorshift-style LFSR for pseudo-random tick generation
//   - Useful for bench testing without a host connection
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// Author: ATOMiK Project
// Date:   January 2026
// =============================================================================

`timescale 1ns / 1ps

module tick_stream_interface #(
    parameter DATA_WIDTH = 64,
    parameter CLK_FREQ   = 27_000_000,
    parameter BAUD_RATE  = 115200
)(
    // Clock and Reset
    input  wire                    clk,
    input  wire                    rst_n,

    // UART Receive Pin
    input  wire                    uart_rx,

    // Simulation Control
    input  wire                    sim_mode,     // 1 = synthetic ticks, 0 = UART
    input  wire [15:0]             sim_rate,     // Ticks per sim_rate clock cycles

    // Tick Output Interface
    output reg  [DATA_WIDTH-1:0]  price_delta,  // Assembled 64-bit price delta
    output reg                     tick_valid,   // Pulsed when tick is ready
    output reg  [31:0]             tick_count    // Running tick sequence number
);

    // =========================================================================
    // UART Receiver (8N1, configurable baud)
    // =========================================================================
    localparam integer BAUD_DIV = CLK_FREQ / BAUD_RATE;
    localparam integer HALF_BIT = BAUD_DIV / 2;

    reg [2:0]  rx_sync;
    wire       rx_pin = rx_sync[2];

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            rx_sync <= 3'b111;
        else
            rx_sync <= {rx_sync[1:0], uart_rx};
    end

    localparam RX_IDLE  = 2'd0;
    localparam RX_START = 2'd1;
    localparam RX_DATA  = 2'd2;
    localparam RX_STOP  = 2'd3;

    reg [15:0] rx_baud_cnt;
    reg [3:0]  rx_bit_cnt;
    reg [7:0]  rx_shift;
    reg [7:0]  rx_data;
    reg        rx_valid;
    reg [1:0]  rx_state;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
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
    // Byte-to-64-bit Assembly (MSB-first, big-endian)
    // =========================================================================
    reg [DATA_WIDTH-1:0] assemble_buf;
    reg [2:0]            byte_idx;
    reg                  uart_tick_valid;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            assemble_buf    <= {DATA_WIDTH{1'b0}};
            byte_idx        <= 3'd0;
            uart_tick_valid <= 1'b0;
        end else begin
            uart_tick_valid <= 1'b0;

            if (rx_valid && !sim_mode) begin
                assemble_buf <= {assemble_buf[DATA_WIDTH-9:0], rx_data};
                if (byte_idx == 3'd7) begin
                    byte_idx        <= 3'd0;
                    uart_tick_valid <= 1'b1;
                end else begin
                    byte_idx <= byte_idx + 3'd1;
                end
            end
        end
    end

    // =========================================================================
    // Simulated Tick Generator (xorshift-style LFSR)
    // =========================================================================
    reg [DATA_WIDTH-1:0] lfsr;
    reg [15:0]           sim_counter;
    reg                  sim_tick_valid;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            lfsr           <= 64'hACE1_CAFE_BABE_0001;
            sim_counter    <= 16'd0;
            sim_tick_valid <= 1'b0;
        end else begin
            sim_tick_valid <= 1'b0;

            if (sim_mode) begin
                if (sim_counter >= sim_rate) begin
                    sim_counter    <= 16'd0;
                    sim_tick_valid <= 1'b1;
                    // xorshift64 step
                    lfsr <= lfsr ^ (lfsr << 13);
                end else begin
                    sim_counter <= sim_counter + 16'd1;
                    // Complete the xorshift in pipeline stages
                    // (simplified: update after output)
                end

                // Additional xorshift feedback on tick boundary
                if (sim_tick_valid) begin
                    lfsr <= lfsr ^ (lfsr >> 7) ^ (lfsr << 17);
                end
            end
        end
    end

    // =========================================================================
    // Output Multiplexer and Tick Counter
    // =========================================================================
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            price_delta <= {DATA_WIDTH{1'b0}};
            tick_valid  <= 1'b0;
            tick_count  <= 32'd0;
        end else begin
            tick_valid <= 1'b0;

            if (sim_mode && sim_tick_valid) begin
                price_delta <= lfsr;
                tick_valid  <= 1'b1;
                tick_count  <= tick_count + 32'd1;
            end else if (!sim_mode && uart_tick_valid) begin
                price_delta <= {assemble_buf[DATA_WIDTH-9:0], rx_data};
                tick_valid  <= 1'b1;
                tick_count  <= tick_count + 32'd1;
            end
        end
    end

endmodule

// =============================================================================
// Module Documentation
// =============================================================================
//
// USAGE:
//   tick_stream_interface #(
//       .DATA_WIDTH(64),
//       .CLK_FREQ(27_000_000),
//       .BAUD_RATE(115200)
//   ) u_tick (
//       .clk         (clk_27m),
//       .rst_n       (sys_rst_n),
//       .uart_rx     (uart_rx_pin),
//       .sim_mode    (1'b0),
//       .sim_rate    (16'd1000),
//       .price_delta (tick_data),
//       .tick_valid  (tick_strobe),
//       .tick_count  (tick_seq)
//   );
//
// TIMING (UART mode):
//   Each 64-bit tick requires 8 UART bytes at 115200 baud:
//     8 bytes * 10 bits/byte * (1/115200) = ~694 us per tick
//     Throughput: ~1440 ticks/second via UART
//
// TIMING (Simulated mode):
//   At sim_rate=1000 and 27 MHz clock:
//     1 tick per 1000 cycles = 27,000 ticks/second
//
// RESOURCE ESTIMATES (GW1NR-9):
//   UART receiver:     ~40 FFs, ~30 LUTs
//   Byte assembly:     ~75 FFs, ~30 LUTs
//   Sim generator:     ~85 FFs, ~40 LUTs
//   Output mux:        ~100 FFs, ~20 LUTs
//   -----------------------------------------------
//   TOTAL:             ~300 FFs, ~120 LUTs
//
// =============================================================================
