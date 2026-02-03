// =============================================================================
// ATOMiK Latency Timer
//
// Measures per-operation latency in clock cycles. Records the cycle count
// for each of the 4 ATOMiK operations (LOAD, ACCUMULATE, RECONSTRUCT,
// ROLLBACK) independently.
//
// Target: Gowin GW1NR-9 (Tang Nano 9K)
// =============================================================================

`timescale 1ns / 1ps

module latency_timer #(
    parameter COUNT_WIDTH = 16
)(
    input  wire                     clk,
    input  wire                     rst_n,
    input  wire [1:0]               operation,    // ATOMiK operation code
    input  wire                     op_start,     // Operation start pulse
    input  wire                     op_done,      // Operation complete pulse
    output reg  [COUNT_WIDTH-1:0]   load_latency,       // LOAD cycles
    output reg  [COUNT_WIDTH-1:0]   accumulate_latency,  // ACCUMULATE cycles
    output reg  [COUNT_WIDTH-1:0]   reconstruct_latency, // RECONSTRUCT/READ cycles
    output reg  [COUNT_WIDTH-1:0]   rollback_latency,    // ROLLBACK cycles
    output reg                      measuring     // Currently measuring
);

    // Operation codes (matching atomik_core_v2)
    localparam OP_NOP        = 2'b00;
    localparam OP_LOAD       = 2'b01;
    localparam OP_ACCUMULATE = 2'b10;
    localparam OP_READ       = 2'b11;

    reg [COUNT_WIDTH-1:0] cycle_count;
    reg [1:0]             current_op;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            load_latency        <= {COUNT_WIDTH{1'b0}};
            accumulate_latency  <= {COUNT_WIDTH{1'b0}};
            reconstruct_latency <= {COUNT_WIDTH{1'b0}};
            rollback_latency    <= {COUNT_WIDTH{1'b0}};
            cycle_count         <= {COUNT_WIDTH{1'b0}};
            current_op          <= OP_NOP;
            measuring           <= 1'b0;
        end
        else begin
            if (op_start && !measuring) begin
                cycle_count <= {COUNT_WIDTH{1'b0}};
                current_op  <= operation;
                measuring   <= 1'b1;
            end
            else if (op_done && measuring) begin
                measuring <= 1'b0;

                // Record latency for the completed operation type
                case (current_op)
                    OP_LOAD:       load_latency        <= cycle_count + 1'b1;
                    OP_ACCUMULATE: accumulate_latency   <= cycle_count + 1'b1;
                    OP_READ:       reconstruct_latency  <= cycle_count + 1'b1;
                    default:       rollback_latency     <= cycle_count + 1'b1;
                endcase
            end
            else if (measuring) begin
                cycle_count <= cycle_count + 1'b1;
            end
        end
    end

endmodule
