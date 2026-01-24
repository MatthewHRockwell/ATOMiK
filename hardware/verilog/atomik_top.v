// ATOMiK Top-Level Module
// Integrates all submodules for Tang Nano 9K deployment
// Target: Tang Nano 9K (Gowin GW1NR-9)

`timescale 1ns / 1ps

module atomik_top (
    input  wire        clk_27mhz,     // 27 MHz onboard oscillator
    input  wire        rst_n,         // Active-low reset (directly from button)
    
    // Camera interface (directly from FPGA pins)
    input  wire [7:0]  cam_data,      // 8-bit parallel pixel data
    input  wire        cam_pclk,      // Pixel clock
    input  wire        cam_vsync,     // Vertical sync
    input  wire        cam_hsync,     // Horizontal sync
    
    // UART output
    output wire        uart_tx,       // Serial data output
    
    // Debug LEDs
    output reg  [5:0]  led            // Onboard LEDs
);

    // Internal signals
    wire        sys_clk = clk_27mhz;
    wire        sys_rst_n;
    
    // Reset synchronizer
    reg [2:0] rst_sync;
    always @(posedge sys_clk or negedge rst_n) begin
        if (!rst_n)
            rst_sync <= 3'b0;
        else
            rst_sync <= {rst_sync[1:0], 1'b1};
    end
    assign sys_rst_n = rst_sync[2];
    
    // Pixel processing signals
    reg  [7:0]  pixel_reg;
    reg         pixel_valid;
    wire [7:0]  tile_mean;
    wire        mean_valid;
    wire        binary_out;
    wire        binary_valid;
    
    // Pattern encoding signals
    wire [3:0]  pattern_bits;
    wire        pattern_ready;
    reg         frame_sync;
    
    // Delta computation signals
    reg  [63:0] prev_voxel;
    reg  [63:0] curr_voxel;
    reg         voxels_valid;
    wire [63:0] delta_word;
    wire        event_valid;
    wire        delta_ready;
    
    // Motif classification signals
    wire [3:0]  motif_id;
    wire        motif_valid;
    
    // UART transmission signals
    reg  [63:0] tx_data;
    reg         tx_start;
    wire        tx_busy;
    wire        tx_done;
    
    // Frame and tile counters
    reg [9:0]  pixel_x;
    reg [9:0]  pixel_y;
    reg [3:0]  frame_count;
    reg [5:0]  voxel_bit_idx;
    reg [63:0] voxel_accumulator;
    
    // Instantiate tile mean calculator
    tile_mean_calculator #(
        .TILE_SIZE(16)
    ) mean_calc (
        .clk(sys_clk),
        .rst_n(sys_rst_n),
        .pixel_value(pixel_reg),
        .pixel_valid(pixel_valid),
        .tile_start(pixel_x[1:0] == 2'b00 && pixel_y[1:0] == 2'b00),
        .tile_mean(tile_mean),
        .mean_valid(mean_valid)
    );
    
    // Instantiate binarizer
    binarizer binarizer_inst (
        .clk(sys_clk),
        .rst_n(sys_rst_n),
        .pixel_value(pixel_reg),
        .tile_mean(tile_mean),
        .pixel_valid(pixel_valid && mean_valid),
        .binary_out(binary_out),
        .output_valid(binary_valid)
    );
    
    // Instantiate pattern accumulator
    pattern_accumulator pattern_acc (
        .clk(sys_clk),
        .rst_n(sys_rst_n),
        .binary_in(binary_out),
        .input_valid(binary_valid),
        .frame_sync(frame_sync),
        .pattern_bits(pattern_bits),
        .pattern_ready(pattern_ready)
    );
    
    // Instantiate delta core
    delta_core delta_inst (
        .clk(sys_clk),
        .rst_n(sys_rst_n),
        .enable(1'b1),
        .prev_word(prev_voxel),
        .curr_word(curr_voxel),
        .words_valid(voxels_valid),
        .delta_word(delta_word),
        .event_valid(event_valid),
        .output_ready(delta_ready)
    );
    
    // Instantiate motif classifier
    motif_classifier classifier_inst (
        .clk(sys_clk),
        .rst_n(sys_rst_n),
        .delta(delta_word),
        .delta_valid(delta_ready),
        .motif_id(motif_id),
        .motif_valid(motif_valid)
    );
    
    // Instantiate 64-bit UART transmitter
    uart_tx_64 #(
        .CLK_FREQ(27_000_000),
        .BAUD_RATE(115200)
    ) uart_inst (
        .clk(sys_clk),
        .rst_n(sys_rst_n),
        .data_in(tx_data),
        .tx_start(tx_start),
        .tx_line(uart_tx),
        .tx_busy(tx_busy),
        .tx_done(tx_done)
    );
    
    // Pixel capture from camera
    always @(posedge cam_pclk or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            pixel_reg <= 8'b0;
            pixel_valid <= 1'b0;
            pixel_x <= 10'b0;
            pixel_y <= 10'b0;
        end else begin
            if (cam_vsync) begin
                pixel_x <= 10'b0;
                pixel_y <= 10'b0;
                pixel_valid <= 1'b0;
            end else if (cam_hsync) begin
                pixel_x <= 10'b0;
                pixel_y <= pixel_y + 1;
                pixel_valid <= 1'b0;
            end else begin
                pixel_reg <= cam_data;
                pixel_valid <= 1'b1;
                pixel_x <= pixel_x + 1;
            end
        end
    end
    
    // Frame synchronization
    reg vsync_prev;
    always @(posedge sys_clk or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            vsync_prev <= 1'b0;
            frame_sync <= 1'b0;
            frame_count <= 4'b0;
        end else begin
            vsync_prev <= cam_vsync;
            
            // Detect rising edge of vsync (new frame)
            if (cam_vsync && !vsync_prev) begin
                frame_sync <= 1'b1;
                frame_count <= frame_count + 1;
            end else begin
                frame_sync <= 1'b0;
            end
        end
    end
    
    // Voxel accumulation (simplified - collect 64 bits from binary stream)
    always @(posedge sys_clk or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            voxel_bit_idx <= 6'b0;
            voxel_accumulator <= 64'b0;
            prev_voxel <= 64'b0;
            curr_voxel <= 64'b0;
            voxels_valid <= 1'b0;
        end else begin
            voxels_valid <= 1'b0;
            
            if (binary_valid) begin
                voxel_accumulator[voxel_bit_idx] <= binary_out;
                
                if (voxel_bit_idx == 6'd63) begin
                    // Complete voxel accumulated
                    prev_voxel <= curr_voxel;
                    curr_voxel <= voxel_accumulator;
                    voxels_valid <= 1'b1;
                    voxel_bit_idx <= 6'b0;
                end else begin
                    voxel_bit_idx <= voxel_bit_idx + 1;
                end
            end
        end
    end
    
    // UART transmission control
    reg [1:0] tx_state;
    localparam TX_IDLE = 2'b00;
    localparam TX_SEND = 2'b01;
    localparam TX_WAIT = 2'b10;
    
    always @(posedge sys_clk or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            tx_state <= TX_IDLE;
            tx_data <= 64'b0;
            tx_start <= 1'b0;
        end else begin
            case (tx_state)
                TX_IDLE: begin
                    tx_start <= 1'b0;
                    if (motif_valid && event_valid) begin
                        // Pack motif and delta for transmission
                        tx_data <= {motif_id, 4'b0, delta_word[55:0]};
                        tx_state <= TX_SEND;
                    end
                end
                
                TX_SEND: begin
                    if (!tx_busy) begin
                        tx_start <= 1'b1;
                        tx_state <= TX_WAIT;
                    end
                end
                
                TX_WAIT: begin
                    tx_start <= 1'b0;
                    if (tx_done) begin
                        tx_state <= TX_IDLE;
                    end
                end
                
                default: tx_state <= TX_IDLE;
            endcase
        end
    end
    
    // LED status display
    always @(posedge sys_clk or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            led <= 6'b111111;  // All off (active low)
        end else begin
            led[0] <= ~event_valid;           // Event detected
            led[1] <= ~tx_busy;               // UART active
            led[3:2] <= ~frame_count[1:0];    // Frame counter
            led[5:4] <= ~motif_id[1:0];       // Motif type
        end
    end

endmodule
