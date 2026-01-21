module uart_genome_loader #(
    parameter CLK_FREQ = 27000000,
    parameter BAUD_RATE = 115200
)(
    input wire clk,
    input wire rst_n,
    input wire uart_rx,
    output reg [31:0] poly_freq_out,
    output reg [31:0] poly_seed_out,
    output reg [255:0] dna_storage,
    output reg core_enable,
    output reg otp_en,          // NEW: One-Time Pad Enable Signal
    output reg loader_busy
);

    // --- UART RECEIVER LOGIC (Standard 8N1) ---
    localparam CLK_DIV = CLK_FREQ / BAUD_RATE;
    reg [15:0] clk_cnt;
    reg [2:0] bit_idx;
    reg [7:0] rx_shift_reg;
    reg rx_busy;
    reg rx_done_tick;
    reg rx_sync, rx_d1;

    always @(posedge clk) begin rx_d1 <= uart_rx; rx_sync <= rx_d1; end

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            clk_cnt <= 0; bit_idx <= 0; rx_busy <= 0; rx_done_tick <= 0;
        end else begin
            rx_done_tick <= 0;
            if (!rx_busy) begin
                if (rx_sync == 0) begin rx_busy <= 1; clk_cnt <= CLK_DIV / 2; bit_idx <= 0; end
            end else begin
                if (clk_cnt >= CLK_DIV) begin
                    clk_cnt <= 0;
                    if (bit_idx < 8) begin rx_shift_reg[bit_idx] <= rx_sync; bit_idx <= bit_idx + 1; end
                    else begin rx_busy <= 0; rx_done_tick <= 1; end
                end else clk_cnt <= clk_cnt + 1;
            end
        end
    end

    // --- BIOS STATE MACHINE (Updated for v1.1 Header) ---
    localparam S_IDLE     = 0;
    localparam S_MAGIC    = 1;
    localparam S_VERSION  = 2;
    localparam S_FREQ     = 3;
    localparam S_POLICY   = 4; // NEW STATE
    localparam S_DNA      = 5;
    localparam S_DONE     = 6;
    
    reg [3:0] state;
    reg [7:0] byte_idx;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S_IDLE; core_enable <= 0; loader_busy <= 0;
            byte_idx <= 0; poly_freq_out <= 0; poly_seed_out <= 32'hDEADBEEF;
            otp_en <= 0;
        end else if (rx_done_tick) begin
            case (state)
                S_IDLE: if (rx_shift_reg == 8'h41) begin // 'A'
                            state <= S_MAGIC; byte_idx <= 1; loader_busy <= 1; core_enable <= 0;
                        end
                S_MAGIC: if (byte_idx==1 && rx_shift_reg==8'h54) byte_idx<=2;      // 'T'
                         else if (byte_idx==2 && rx_shift_reg==8'h4F) byte_idx<=3; // 'O'
                         else if (byte_idx==3 && rx_shift_reg==8'h4D) state<=S_VERSION; // 'M'
                         else begin state<=S_IDLE; loader_busy<=0; end
                S_VERSION: begin state <= S_FREQ; byte_idx <= 0; end
                S_FREQ: begin
                    poly_freq_out[byte_idx*8 +: 8] <= rx_shift_reg;
                    if (byte_idx == 3) begin state <= S_POLICY; end 
                    else byte_idx <= byte_idx + 1;
                end
                S_POLICY: begin // NEW: Read Policy Byte
                    otp_en <= rx_shift_reg[0]; // Bit 0 is Burn-on-Read
                    state <= S_DNA; byte_idx <= 0;
                end
                S_DNA: begin
                    dna_storage[byte_idx*8 +: 8] <= rx_shift_reg;
                    if (byte_idx == 31) state <= S_DONE;
                    else byte_idx <= byte_idx + 1;
                end
            endcase
        end else if (state == S_DONE) begin
            core_enable <= 1; loader_busy <= 0; state <= S_IDLE;
        end
    end
endmodule