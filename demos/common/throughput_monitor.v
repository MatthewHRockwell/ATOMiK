// =============================================================================
// ATOMiK Throughput Monitor
//
// Measures operations per second by counting completed operations within
// a configurable measurement window. Reports ops/window which can be
// scaled to ops/second based on clock frequency.
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// =============================================================================

`timescale 1ns / 1ps

module throughput_monitor #(
    parameter WINDOW_CYCLES = 27_000_000,   // 1 second at 27 MHz
    parameter COUNT_WIDTH   = 32
)(
    input  wire                     clk,
    input  wire                     rst_n,
    input  wire                     enable,      // Enable monitoring
    input  wire                     op_valid,    // Pulse on each completed operation
    output reg  [COUNT_WIDTH-1:0]   ops_count,   // Operations in current window
    output reg  [COUNT_WIDTH-1:0]   ops_result,  // Operations in last complete window
    output reg  [COUNT_WIDTH-1:0]   window_count, // Window cycle counter
    output reg                      window_done  // Window complete pulse
);

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            ops_count    <= {COUNT_WIDTH{1'b0}};
            ops_result   <= {COUNT_WIDTH{1'b0}};
            window_count <= {COUNT_WIDTH{1'b0}};
            window_done  <= 1'b0;
        end
        else begin
            window_done <= 1'b0;

            if (!enable) begin
                ops_count    <= {COUNT_WIDTH{1'b0}};
                window_count <= {COUNT_WIDTH{1'b0}};
            end
            else begin
                // Count operations
                if (op_valid) begin
                    ops_count <= ops_count + 1'b1;
                end

                // Window management
                if (window_count >= WINDOW_CYCLES - 1) begin
                    // Window complete: latch result and reset
                    ops_result   <= ops_count + (op_valid ? 1'b1 : 1'b0);
                    ops_count    <= {COUNT_WIDTH{1'b0}};
                    window_count <= {COUNT_WIDTH{1'b0}};
                    window_done  <= 1'b1;
                end
                else begin
                    window_count <= window_count + 1'b1;
                end
            end
        end
    end

endmodule
