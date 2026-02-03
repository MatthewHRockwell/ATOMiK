// =============================================================================
// ATOMiK Performance Counter
//
// Cycle-accurate counter for measuring operation latency and pipeline
// throughput. Counts clock cycles between start and stop events.
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// =============================================================================

`timescale 1ns / 1ps

module perf_counter #(
    parameter COUNT_WIDTH = 32
)(
    input  wire                     clk,
    input  wire                     rst_n,
    input  wire                     start,       // Pulse to start counting
    input  wire                     stop,        // Pulse to stop counting
    input  wire                     clear,       // Clear counter
    output reg  [COUNT_WIDTH-1:0]   count,       // Current cycle count
    output reg                      running,     // Counter is running
    output reg                      done         // Measurement complete (pulse)
);

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            count   <= {COUNT_WIDTH{1'b0}};
            running <= 1'b0;
            done    <= 1'b0;
        end
        else begin
            done <= 1'b0;

            if (clear) begin
                count   <= {COUNT_WIDTH{1'b0}};
                running <= 1'b0;
            end
            else if (start && !running) begin
                count   <= {COUNT_WIDTH{1'b0}};
                running <= 1'b1;
            end
            else if (stop && running) begin
                running <= 1'b0;
                done    <= 1'b1;
            end
            else if (running) begin
                count <= count + 1'b1;
            end
        end
    end

endmodule
