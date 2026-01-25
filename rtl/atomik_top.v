// =============================================================================
// ATOMiK Top-Level (Tang Nano 9K) - Phase 3 Delta Architecture
// 
// This version implements the verified delta-state computation model:
//   current_state = initial_state ⊕ delta_accumulator
//
// UART Protocol:
//   - Send 'L' + 8 bytes: LOAD initial state
//   - Send 'A' + 8 bytes: ACCUMULATE delta  
//   - Send 'R': READ current state (returns 8 bytes)
//   - Send 'S': STATUS (returns accumulator_zero flag)
//
// Version: 4.1 (Fixed TX state machine handshake)
// Date: January 25, 2026
// =============================================================================

module atomik_top #(
    parameter integer SYS_CLK_HZ = 94_500_000,
    parameter integer BAUD_RATE  = 115200,
    parameter integer USE_PLL    = 1
)(
    input  wire       sys_clk,      // 27MHz onboard oscillator
    input  wire       sys_rst_n,    // Active-low reset button
    input  wire       uart_rx,      // UART receive
    output wire       uart_tx,      // UART transmit
    output wire [5:0] led           // Status LEDs (active-low)
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
    // UART Receiver (115200 baud, 8N1)
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
                        // UART is LSB first: shift right, new bit enters at MSB
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
    // UART Transmitter (115200 baud, 8N1)
    // =========================================================================
    // Frame: [START=0][D0][D1][D2][D3][D4][D5][D6][D7][STOP=1]
    // Total 10 bits, LSB first for data
    
    reg [15:0] tx_baud_cnt;
    reg [3:0]  tx_bit_cnt;   // 0-9 for 10 bits
    reg [7:0]  tx_shift;     // Data shift register
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
            tx_out      <= 1'b1;  // Idle high
        end else begin
            if (!tx_busy && tx_start) begin
                // Start new transmission
                tx_shift    <= tx_data;
                tx_busy     <= 1'b1;
                tx_bit_cnt  <= 4'd0;
                tx_baud_cnt <= 16'd0;
                tx_out      <= 1'b0;  // Start bit
            end else if (tx_busy) begin
                if (tx_baud_cnt == BAUD_DIV[15:0] - 1) begin
                    tx_baud_cnt <= 16'd0;
                    tx_bit_cnt  <= tx_bit_cnt + 4'd1;
                    
                    case (tx_bit_cnt)
                        4'd0, 4'd1, 4'd2, 4'd3, 4'd4, 4'd5, 4'd6, 4'd7: begin
                            // Output data bit (LSB first)
                            tx_out   <= tx_shift[0];
                            tx_shift <= {1'b1, tx_shift[7:1]};
                        end
                        4'd8: begin
                            // Stop bit
                            tx_out <= 1'b1;
                        end
                        4'd9: begin
                            // Done
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
    // ATOMiK Core v2 (Delta Architecture)
    // =========================================================================
    reg  [1:0]  core_operation;
    reg  [63:0] core_data_in;
    wire [63:0] core_data_out;
    wire        core_data_valid;
    wire        core_accumulator_zero;
    wire [63:0] core_debug_initial;
    wire [63:0] core_debug_accum;

    atomik_core_v2 #(
        .DATA_WIDTH(64)
    ) u_core_v2 (
        .clk              (clk_int),
        .rst_n            (internal_rst_n),
        .operation        (core_operation),
        .data_in          (core_data_in),
        .data_out         (core_data_out),
        .data_valid       (core_data_valid),
        .accumulator_zero (core_accumulator_zero),
        .debug_initial_state(core_debug_initial),
        .debug_accumulator(core_debug_accum)
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
    localparam CMD_DEBUG     = 4'd6;
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
            cmd_state      <= CMD_IDLE;
            byte_cnt       <= 4'd0;
            cmd_buffer     <= 64'd0;
            tx_buffer      <= 64'd0;
            tx_byte_cnt    <= 4'd0;
            tx_bytes_total <= 4'd0;
            tx_start       <= 1'b0;
            tx_data        <= 8'd0;
            core_operation <= 2'b00;
            core_data_in   <= 64'd0;
            cmd_active     <= 1'b0;
        end else begin
            tx_start       <= 1'b0;
            core_operation <= 2'b00;
            
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
                                cmd_state      <= CMD_READ;
                                core_operation <= 2'b11;
                                cmd_active     <= 1'b1;
                            end
                            8'h53: begin  // 'S' - Status
                                cmd_state  <= CMD_STATUS;
                                cmd_active <= 1'b1;
                            end
                            8'h44: begin  // 'D' - Debug
                                cmd_state  <= CMD_DEBUG;
                                cmd_active <= 1'b1;
                            end
                            default: ;
                        endcase
                    end
                end
                
                CMD_LOAD: begin
                    if (rx_valid) begin
                        // Shift in bytes MSB-first (big-endian)
                        cmd_buffer <= {cmd_buffer[55:0], rx_data};
                        if (byte_cnt == 4'd7) begin
                            // All 8 bytes received, execute LOAD
                            core_data_in   <= {cmd_buffer[55:0], rx_data};
                            core_operation <= 2'b01;
                            cmd_state      <= CMD_IDLE;
                            cmd_active     <= 1'b0;
                        end else begin
                            byte_cnt <= byte_cnt + 4'd1;
                        end
                    end
                end
                
                CMD_ACCUM: begin
                    if (rx_valid) begin
                        // Shift in bytes MSB-first (big-endian)
                        cmd_buffer <= {cmd_buffer[55:0], rx_data};
                        if (byte_cnt == 4'd7) begin
                            // All 8 bytes received, execute ACCUMULATE
                            core_data_in   <= {cmd_buffer[55:0], rx_data};
                            core_operation <= 2'b10;
                            cmd_state      <= CMD_IDLE;
                            cmd_active     <= 1'b0;
                        end else begin
                            byte_cnt <= byte_cnt + 4'd1;
                        end
                    end
                end
                
                CMD_READ: begin
                    // READ operation issued, wait for core to update output
                    cmd_state <= CMD_READ_WAIT;
                end
                
                CMD_READ_WAIT: begin
                    // Core has now registered the output, capture it
                    tx_buffer      <= core_data_out;
                    tx_byte_cnt    <= 4'd0;
                    tx_bytes_total <= 4'd8;
                    cmd_state      <= CMD_TX_START;
                end
                
                CMD_STATUS: begin
                    // Put status byte in MSB position (sent first)
                    tx_buffer      <= {core_accumulator_zero, 63'd0};
                    tx_byte_cnt    <= 4'd0;
                    tx_bytes_total <= 4'd1;
                    cmd_state      <= CMD_TX_START;
                end
                
                CMD_DEBUG: begin
                    tx_buffer      <= core_debug_initial;
                    tx_byte_cnt    <= 4'd0;
                    tx_bytes_total <= 4'd8;
                    cmd_state      <= CMD_TX_START;
                end
                
                // TX State machine: START -> WAIT -> DONE -> (loop or IDLE)
                CMD_TX_START: begin
                    if (tx_byte_cnt < tx_bytes_total) begin
                        // Load next byte and start transmission
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
                    // Wait for TX to become busy (should happen next cycle)
                    if (tx_busy) begin
                        cmd_state <= CMD_TX_DONE;
                    end
                end
                
                CMD_TX_DONE: begin
                    // Wait for TX to finish
                    if (!tx_busy) begin
                        // More bytes to send?
                        if (tx_byte_cnt < tx_bytes_total) begin
                            cmd_state <= CMD_TX_START;
                        end else begin
                            cmd_state <= CMD_IDLE;
                        end
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
    assign led[2] = ~core_accumulator_zero;
    assign led[1] = ~tx_busy;
    assign led[0] = ~(rx_activity != 20'd0);

endmodule
