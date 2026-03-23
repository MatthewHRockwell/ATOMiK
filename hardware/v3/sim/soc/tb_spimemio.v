// Quick testbench for spimemio_puya XIP read path
// Simulates a simple SPI flash that responds to 0x03 read command

`timescale 1ns / 1ps

module tb_spimemio;
    reg clk, resetn;
    reg valid;
    wire ready;
    reg [23:0] addr;
    wire [31:0] rdata;

    wire flash_csb, flash_clk;
    wire flash_io0_oe, flash_io1_oe;
    wire flash_io0_do, flash_io1_do;
    wire flash_io0_di, flash_io1_di;

    reg [3:0] cfgreg_we;
    reg [31:0] cfgreg_di;
    wire [31:0] cfgreg_do;

    // Simple SPI flash model
    reg [7:0] flash_mem [0:255];
    reg [2:0] flash_state;  // 0=idle, 1=cmd, 2=addr2, 3=addr1, 4=addr0, 5=data
    reg [7:0] flash_shiftin;
    reg [7:0] flash_shiftout;
    reg [3:0] flash_bitcnt;
    reg [23:0] flash_addr;
    reg flash_miso_r;

    assign flash_io1_di = flash_miso_r;  // MISO
    assign flash_io0_di = flash_io0_do;  // Loopback MOSI (flash ignores during read data)

    // SPI flash model: shift in on rising edge, shift out on falling edge
    always @(posedge flash_clk or posedge flash_csb) begin
        if (flash_csb) begin
            flash_state <= 0;
            flash_bitcnt <= 0;
            flash_miso_r <= 1'b1;  // MISO idle high
        end else begin
            // Rising edge: sample MOSI
            flash_shiftin <= {flash_shiftin[6:0], flash_io0_do};
            flash_bitcnt <= flash_bitcnt + 1;
            if (flash_bitcnt == 7) begin
                case (flash_state)
                    0: begin  // Command byte received
                        if ({flash_shiftin[6:0], flash_io0_do} == 8'hFF) begin
                            flash_state <= 0;  // MBR - ignore
                        end else if ({flash_shiftin[6:0], flash_io0_do} == 8'hAB) begin
                            flash_state <= 0;  // RPD - wake up
                        end else if ({flash_shiftin[6:0], flash_io0_do} == 8'h03) begin
                            flash_state <= 2;  // READ command
                        end
                    end
                    2: begin  // Address byte 2 (MSB)
                        flash_addr[23:16] <= {flash_shiftin[6:0], flash_io0_do};
                        flash_state <= 3;
                    end
                    3: begin  // Address byte 1
                        flash_addr[15:8] <= {flash_shiftin[6:0], flash_io0_do};
                        flash_state <= 4;
                    end
                    4: begin  // Address byte 0 (LSB)
                        flash_addr[7:0] <= {flash_shiftin[6:0], flash_io0_do};
                        flash_state <= 5;
                        // Preload first data byte for output
                    end
                    5: begin  // Data byte sent, advance address
                        flash_addr <= flash_addr + 1;
                    end
                endcase
            end
        end
    end

    // Output data on falling edge of flash_clk
    always @(negedge flash_clk or posedge flash_csb) begin
        if (flash_csb) begin
            flash_shiftout <= 8'hFF;
        end else begin
            if (flash_state == 5 || (flash_state == 4 && flash_bitcnt == 7)) begin
                if (flash_bitcnt == 0) begin
                    // Load new byte at start of byte
                    flash_shiftout <= flash_mem[flash_addr[7:0]];
                end else begin
                    flash_shiftout <= {flash_shiftout[6:0], 1'b0};
                end
                flash_miso_r <= flash_shiftout[7];
            end else begin
                flash_miso_r <= 1'b1;  // Idle high during command/address
            end
        end
    end

    // DUT
    spimemio_puya dut (
        .clk(clk),
        .resetn(resetn),
        .valid(valid),
        .ready(ready),
        .addr(addr),
        .rdata(rdata),
        .flash_csb(flash_csb),
        .flash_clk(flash_clk),
        .flash_io0_oe(flash_io0_oe),
        .flash_io1_oe(flash_io1_oe),
        .flash_io0_do(flash_io0_do),
        .flash_io1_do(flash_io1_do),
        .flash_io0_di(flash_io0_di),
        .flash_io1_di(flash_io1_di),
        .cfgreg_we(cfgreg_we),
        .cfgreg_di(cfgreg_di),
        .cfgreg_do(cfgreg_do)
    );

    // Clock: 25.2 MHz (period ~39.7 ns)
    always #20 clk = ~clk;

    integer i;
    initial begin
        $dumpfile("tb_spimemio.vcd");
        $dumpvars(0, tb_spimemio);

        // Initialize flash memory with known pattern
        for (i = 0; i < 256; i = i + 1)
            flash_mem[i] = i[7:0];

        // Override first 4 bytes to match test firmware
        flash_mem[0] = 8'h9B;
        flash_mem[1] = 8'h02;
        flash_mem[2] = 8'h30;
        flash_mem[3] = 8'h08;

        clk = 0;
        resetn = 0;
        valid = 0;
        addr = 0;
        cfgreg_we = 0;
        cfgreg_di = 0;

        // Reset
        #200;
        resetn = 1;

        // Wait for init sequence to complete (MBR + RPD + pudly + read cmd)
        // ~500 cycles at 40ns period = 20,000 ns
        #30000;

        $display("=== Init complete, cfgreg_do = %08x ===", cfgreg_do);
        $display("    config_en=%b, flash_csb=%b, flash_clk=%b",
                 cfgreg_do[31], cfgreg_do[5], cfgreg_do[4]);

        // Request read at address 0x000000
        $display("\n=== Requesting XIP read at 0x000000 ===");
        valid = 1;
        addr = 24'h000000;

        // Wait for ready
        @(posedge ready);
        $display("=== XIP read complete! rdata = %08x ===", rdata);
        $display("    Expected: 0830029B (little-endian of 9B 02 30 08)");

        valid = 0;
        #200;

        // Try second read at address 0x000004
        $display("\n=== Requesting XIP read at 0x000004 ===");
        valid = 1;
        addr = 24'h000004;
        @(posedge ready);
        $display("=== XIP read complete! rdata = %08x ===", rdata);

        valid = 0;
        #200;

        $display("\n=== TEST PASSED ===");
        $finish;
    end

    // Timeout
    initial begin
        #500000;
        $display("\n=== TIMEOUT - XIP read hung! ===");
        $display("    state=%d, cfgreg_do=%08x", dut.state, cfgreg_do);
        $display("    valid=%b, ready=%b, rd_valid=%b, rd_addr=%06x",
                 valid, ready, dut.rd_valid, dut.rd_addr);
        $display("    din_valid=%b, din_ready=%b, din_data=%02x, din_tag=%d",
                 dut.din_valid, dut.din_ready, dut.din_data, dut.din_tag);
        $display("    xfer: count=%d, csb=%b, clk=%b, fetch=%b",
                 dut.xfer.count, flash_csb, flash_clk, dut.xfer.fetch);
        $finish;
    end

endmodule
