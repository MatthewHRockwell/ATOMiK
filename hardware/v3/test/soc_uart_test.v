// SoC UART integration test - use SoC peripheral structure but simple state machine instead of CPU
// This isolates whether the issue is in PicoMem_UART wrapper or in CPU bus logic
module soc_uart_test (
    input clk,
    output uart_tx,
    output [5:0] gpio_debug  // GPIO pins for debugging
);

// Clock divider (same as SoC)
wire clk_p;
Gowin_CLKDIV u_div_2 (
    .clkout(clk_p),
    .hclkin(clk),
    .resetn(1'b1)
);

// Reset
reg [7:0] reset_cnt = 0;
wire resetn = &reset_cnt;
always @(posedge clk_p) begin
    if (!resetn)
        reset_cnt <= reset_cnt + 1;
end

// State machine to drive UART through PicoMem_UART wrapper
reg [3:0] state = 0;
reg [31:0] counter = 0;

reg        mem_s_valid = 0;
wire       mem_s_ready;
reg [31:0] mem_s_addr = 0;
reg [31:0] mem_s_wdata = 0;
reg [3:0]  mem_s_wstrb = 0;
wire [31:0] mem_s_rdata;

// Instantiate PicoMem_UART (same as SoC)
PicoMem_UART u_uart (
    .resetn(resetn),
    .clk(clk_p),
    .mem_s_valid(mem_s_valid),
    .mem_s_ready(mem_s_ready),
    .mem_s_addr(mem_s_addr),
    .mem_s_wdata(mem_s_wdata),
    .mem_s_wstrb(mem_s_wstrb),
    .mem_s_rdata(mem_s_rdata),
    .ser_rx(1'b1),
    .ser_tx(uart_tx)
);

// Debug outputs to GPIO pins
//assign gpio_debug[0] = resetn;           // Reset status
assign gpio_debug[0] = mem_s_valid;      // Bus valid
assign gpio_debug[1] = mem_s_ready;      // Bus ready
assign gpio_debug[2] = state[0];         // State LSB
assign gpio_debug[3] = state[1];         // State bit 1
assign gpio_debug[4] = state[2];         // State bit 2
assign gpio_debug[5] = uart_tx;          // Mirror UART TX

always @(posedge clk_p) begin
    if (!resetn) begin
        state <= 0;
        counter <= 0;
        mem_s_valid <= 0;
        mem_s_wstrb <= 0;
    end else begin
        counter <= counter + 1;

        case (state)
            0: begin
                // Wait for stable reset
                if (counter > 1000) begin
                    state <= 1;
                    counter <= 0;
                end
            end

            1: begin
                // Write CLKDIV (address bit 2 = 1)
                // 13.5 MHz / 115200 - 2 = 115
                mem_s_valid <= 1;
                mem_s_addr <= 32'h00000004;  // Bit 2 set = CLKDIV
                mem_s_wdata <= 115;
                mem_s_wstrb <= 4'b1111;

                if (mem_s_ready) begin
                    mem_s_valid <= 0;
                    state <= 2;
                    counter <= 0;
                end
            end

            2: begin
                // Wait for 15 dummy bits to complete
                // 15 bits * 116 cycles/bit = 1740 cycles
                if (counter > 2000) begin
                    state <= 3;
                    counter <= 0;
                end
            end

            3: begin
                // Send 'U' (0x55) - address bit 2 = 0 for DATA
                mem_s_valid <= 1;
                mem_s_addr <= 32'h00000000;  // Bit 2 clear = DATA
                mem_s_wdata <= 8'h55;
                mem_s_wstrb <= 4'b0001;  // Byte write (wstrb[0] = 1)

                if (mem_s_ready) begin
                    mem_s_valid <= 0;
                    state <= 4;
                    counter <= 0;
                end
            end

            4: begin
                // Wait a bit before next character
                if (counter > 2000) begin
                    state <= 3;  // Loop back to send another
                    counter <= 0;
                end
            end
        endcase
    end
end

endmodule
