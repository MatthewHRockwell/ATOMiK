/**
 * atomik_edge_sensor_imu_fusion
 * 
 * Delta-state module for IMUFusion
 * Generated from ATOMiK schema
 * 
 * Operations:
 *   LOAD: Set initial state
 *   ACCUMULATE: XOR delta into accumulator
 *   READ: Reconstruct current state
 *   STATUS: Check if accumulator is zero
 */

module atomik_edge_sensor_imu_fusion #(
    parameter DATA_WIDTH = 64
) (
    // Clock and reset
    input wire clk,
    input wire rst_n,

    // Control signals
    input wire load_en,
    input wire accumulate_en,
    input wire read_en,
    input wire rollback_en,

    // Data signals
    input wire [DATA_WIDTH-1:0] data_in,
    output reg [DATA_WIDTH-1:0] data_out,

    // Status signals
    output wire accumulator_zero
);

    // Internal state
    reg [DATA_WIDTH-1:0] initial_state;
    reg [DATA_WIDTH-1:0] accumulator;

    // Rollback history
    reg [DATA_WIDTH-1:0] history [0:1023];
    reg [$clog2(1024):0] history_count;
    reg [$clog2(1024)-1:0] history_head;

    // Status: accumulator is zero
    assign accumulator_zero = (accumulator == {DATA_WIDTH{1'b0}});

    // LOAD operation: Set initial state
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            initial_state <= {DATA_WIDTH{1'b0}};
        end else if (load_en) begin
            initial_state <= data_in;
        end
    end

    // ACCUMULATE operation: XOR delta into accumulator
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            accumulator <= {DATA_WIDTH{1'b0}};
            history_count <= 0;
            history_head <= 0;
        end else if (load_en) begin
            // Reset accumulator on load
            accumulator <= {DATA_WIDTH{1'b0}};
            history_count <= 0;
            history_head <= 0;
        end else if (accumulate_en) begin
            // XOR delta into accumulator
            accumulator <= accumulator ^ data_in;
            // Save to history
            history[history_head] <= data_in;
            history_head <= (history_head + 1) % 1024;
            if (history_count < 1024)
                history_count <= history_count + 1;
        end else if (rollback_en && history_count > 0) begin
            // Rollback: XOR out the last delta
            history_head <= (history_head - 1 + 1024) % 1024;
            accumulator <= accumulator ^ history[(history_head - 1 + 1024) % 1024];
            history_count <= history_count - 1;
        end
    end

    // READ operation: Reconstruct current state
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            data_out <= {DATA_WIDTH{1'b0}};
        end else if (read_en) begin
            // current_state = initial_state XOR accumulator
            data_out <= initial_state ^ accumulator;
        end
    end

endmodule
