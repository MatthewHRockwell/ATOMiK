// =============================================================================
// ATOMiK v3 SoC Top-Level
//
// Port of v2 picotiny.v:
//   - Replace PicoRV32 with atomik_v3_cpu (RV64I + ATOMiK custom instructions)
//   - Remove ATOMiK bus wrapper, CDC bridge, second PLL (ATOMiK is direct-wired)
//   - Tie off S3 (0xC0000000) — ATOMiK MMIO slot freed
//   - Same peripheral hierarchy, same HDMI, same memory map
//
// Clock: PLL1 27 MHz → 108 MHz (CLKOUT), CLKDIV ÷5 → 21.6 MHz (clk_cpu)
// No PLL2 needed: ATOMiK runs in CPU clock domain via custom instructions
// V3-020 fix: Lowered from 126/25.2 MHz to 108/21.6 MHz for timing closure
//
// Memory Map:
//   0x00000000  SPI Flash XIP (8 MB)
//   0x40000000  Data SRAM (8 KB, 4 BSRAM)
//   0x80000000  Boot ROM (8 KB, 4 BSRAM)
//   0x81000000  SPI Flash Config
//   0x82000000  GPIO (7-bit)
//   0x83000000  UART (115200)
//   0xC0000000  Display Pipeline MMIO (delta color LUT, scanline buffer)
//
// BSRAM budget: 4 regfile + 2 ATOMiK + 4 BROM + 4 SRAM = 14/26 (54%)
// =============================================================================

`timescale 1ns/1ps

module atomik_v3_soc (
    input clk,
    input resetn,  // External reset pin (S14 on Tang Nano 9K)

    output       tmds_clk_n,
    output       tmds_clk_p,
    output [2:0] tmds_d_n,
    output [2:0] tmds_d_p,

    output  flash_clk,
    output  flash_csb,
    inout   flash_mosi,
    inout   flash_miso,

    input  ser_rx,
    output ser_tx,
    inout [4:0] gpio,  // Reduced from [6:0] to free pins 15,16 for debug

    // Debug outputs
    output wire debug_reset_n,    // LED: HIGH when CPU out of reset
    output wire debug_clk_toggle  // LED: Toggles with divided clock
);

// =========================================================================
// Clock generation: PLL + CLKDIV
// PLL: 27 MHz → 108 MHz (clk_p5 for HDMI 5x serializer)
// CLKDIV: 108 MHz ÷ 5 → 21.6 MHz (clk_p for CPU + HDMI pixel clock)
// V3-020: Reduced from 126/25.2 MHz — Fmax was 24.745 MHz (40 violations)
// =========================================================================
wire clk_p;       // 21.6 MHz CPU + pixel clock
wire clk_p5;      // 108 MHz HDMI 5x serializer clock
wire sys_resetn;
wire pll_lock;    // PLL lock indicator

Gowin_rPLL u_pll (
    .clkin  (clk),
    .clkout (clk_p5),
    .lock   (pll_lock)
);

Gowin_CLKDIV u_div_5 (
    .clkout (clk_p),
    .hclkin (clk_p5),
    .resetn (pll_lock)
);

// Power-on reset generator (hold reset until PLL locks + 16 cycles)
// Reset synchronization - gated by PLL lock
Reset_Sync u_Reset_Sync (
    .clk       (clk_p),
    .ext_reset (resetn),   // External reset button
    .pll_lock  (pll_lock), // Hold reset until PLL stable
    .resetn    (sys_resetn)
);

// =========================================================================
// CPU bus signals (32-bit valid/ready, same as PicoRV32)
// =========================================================================
wire        mem_valid;
wire        mem_ready;
wire [31:0] mem_addr;
wire [31:0] mem_wdata;
wire [3:0]  mem_wstrb;
wire [31:0] mem_rdata;

// =========================================================================
// ATOMiK v3 CPU (RV64I + ATOMiK custom instructions, 32-bit bus)
// Production boot flow: Boot ROM (0x80000000) → Flash (0x00000000)
// =========================================================================
atomik_v3_cpu #(
    .RESET_PC(64'h0000_0000_8000_0000)
) u_cpu (
    .clk       (clk_p),
    .rst_n     (sys_resetn),
    .mem_valid  (mem_valid),
    .mem_ready  (mem_ready),
    .mem_addr   (mem_addr),
    .mem_wdata  (mem_wdata),
    .mem_wstrb  (mem_wstrb),
    .mem_rdata  (mem_rdata)
);

// =========================================================================
// Bus hierarchy — same as v2
// =========================================================================

// S0 → SPI Flash XIP
wire spimemxip_valid, spimemxip_ready;
wire [31:0] spimemxip_addr, spimemxip_wdata, spimemxip_rdata;
wire [3:0]  spimemxip_wstrb;

// S1 → Data SRAM
wire sram_valid, sram_ready;
wire [31:0] sram_addr, sram_wdata, sram_rdata;
wire [3:0]  sram_wstrb;

// S2 → Peripheral mux
wire picop_valid, picop_ready;
wire [31:0] picop_addr, picop_wdata, picop_rdata;
wire [3:0]  picop_wstrb;

// S3 → Display Pipeline MMIO
wire wbp_valid, wbp_ready;
wire [31:0] wbp_addr, wbp_wdata, wbp_rdata;
wire [3:0]  wbp_wstrb;

// Peripheral sub-slots
wire spimemcfg_valid, spimemcfg_ready;
wire [31:0] spimemcfg_addr, spimemcfg_wdata, spimemcfg_rdata;
wire [3:0]  spimemcfg_wstrb;

wire brom_valid, brom_ready;
wire [31:0] brom_addr, brom_wdata, brom_rdata;
wire [3:0]  brom_wstrb;

wire gpio_valid, gpio_ready;
wire [31:0] gpio_addr, gpio_wdata, gpio_rdata;
wire [3:0]  gpio_wstrb;

wire uart_valid, uart_ready;
wire [31:0] uart_addr, uart_wdata, uart_rdata;
wire [3:0]  uart_wstrb;

// =========================================================================
// Top-level address decoder (S0-S3)
// S0 0x0000_0000 → SPI Flash XIP
// S1 0x4000_0000 → SRAM
// S2 0x8000_0000 → PicoPeriph
// S3 0xC000_0000 → Display MMIO
// =========================================================================
PicoMem_Mux_1_4 u_bus_mux (
    .picom_valid (mem_valid),
    .picom_ready (mem_ready),
    .picom_addr  (mem_addr),
    .picom_wdata (mem_wdata),
    .picom_wstrb (mem_wstrb),
    .picom_rdata (mem_rdata),

    .picos0_valid(spimemxip_valid),
    .picos0_ready(spimemxip_ready),
    .picos0_addr (spimemxip_addr),
    .picos0_wdata(spimemxip_wdata),
    .picos0_wstrb(spimemxip_wstrb),
    .picos0_rdata(spimemxip_rdata),

    .picos1_valid(sram_valid),
    .picos1_ready(sram_ready),
    .picos1_addr (sram_addr),
    .picos1_wdata(sram_wdata),
    .picos1_wstrb(sram_wstrb),
    .picos1_rdata(sram_rdata),

    .picos2_valid(picop_valid),
    .picos2_ready(picop_ready),
    .picos2_addr (picop_addr),
    .picos2_wdata(picop_wdata),
    .picos2_wstrb(picop_wstrb),
    .picos2_rdata(picop_rdata),

    .picos3_valid(wbp_valid),
    .picos3_ready(wbp_ready),
    .picos3_addr (wbp_addr),
    .picos3_wdata(wbp_wdata),
    .picos3_wstrb(wbp_wstrb),
    .picos3_rdata(wbp_rdata)
);

// S3 → Display pipeline MMIO (routed through svo_hdmi_top to atomik_delta_display)

// =========================================================================
// Debug outputs - UART WRITE DETAIL TEST
// =========================================================================
// Check which UART registers are being written
reg uart_clkdiv_written;  // Offset 4 (addr[2] = 1)
reg uart_data_written;     // Offset 0 (addr[2] = 0)

always @(posedge clk_p or negedge sys_resetn) begin
    if (!sys_resetn) begin
        uart_clkdiv_written <= 1'b0;
        uart_data_written <= 1'b0;
    end else if (uart_valid && uart_ready && |uart_wstrb) begin
        if (uart_addr[2])
            uart_clkdiv_written <= 1'b1;  // CLKDIV write
        else
            uart_data_written <= 1'b1;    // DATA write
    end
end

// Pin 15: ON if BOTH CLKDIV and DATA written, OFF if only one or neither
assign debug_reset_n = uart_clkdiv_written && uart_data_written;
assign debug_clk_toggle = 1'b0;  // Pin 16: ignore (broken)

// =========================================================================
// Peripheral address decoder (within S2: 0x8000_0000)
// S0 0x8000_0000 → Boot ROM
// S1 0x8100_0000 → SPI Flash Config
// S2 0x8200_0000 → GPIO
// S3 0x8300_0000 → UART
// =========================================================================
PicoMem_Mux_1_4 #(
    .PICOS0_ADDR_BASE(32'h8000_0000),
    .PICOS0_ADDR_MASK(32'h0F00_0000),
    .PICOS1_ADDR_BASE(32'h8100_0000),
    .PICOS1_ADDR_MASK(32'h0F00_0000),
    .PICOS2_ADDR_BASE(32'h8200_0000),
    .PICOS2_ADDR_MASK(32'h0F00_0000),
    .PICOS3_ADDR_BASE(32'h8300_0000),
    .PICOS3_ADDR_MASK(32'h0F00_0000)
) u_periph_mux (
    .picom_valid (picop_valid),
    .picom_ready (picop_ready),
    .picom_addr  (picop_addr),
    .picom_wdata (picop_wdata),
    .picom_wstrb (picop_wstrb),
    .picom_rdata (picop_rdata),

    .picos0_valid(brom_valid),
    .picos0_ready(brom_ready),
    .picos0_addr (brom_addr),
    .picos0_wdata(brom_wdata),
    .picos0_wstrb(brom_wstrb),
    .picos0_rdata(brom_rdata),

    .picos1_valid(spimemcfg_valid),
    .picos1_ready(spimemcfg_ready),
    .picos1_addr (spimemcfg_addr),
    .picos1_wdata(spimemcfg_wdata),
    .picos1_wstrb(spimemcfg_wstrb),
    .picos1_rdata(spimemcfg_rdata),

    .picos2_valid(gpio_valid),
    .picos2_ready(gpio_ready),
    .picos2_addr (gpio_addr),
    .picos2_wdata(gpio_wdata),
    .picos2_wstrb(gpio_wstrb),
    .picos2_rdata(gpio_rdata),

    .picos3_valid(uart_valid),
    .picos3_ready(uart_ready),
    .picos3_addr (uart_addr),
    .picos3_wdata(uart_wdata),
    .picos3_wstrb(uart_wstrb),
    .picos3_rdata(uart_rdata)
);

// =========================================================================
// Data SRAM (8 KB)
// =========================================================================
PicoMem_SRAM_8KB u_sram (
    .resetn     (sys_resetn),
    .clk        (clk_p),
    .mem_s_valid(sram_valid),
    .mem_s_ready(sram_ready),
    .mem_s_addr (sram_addr),
    .mem_s_wdata(sram_wdata),
    .mem_s_wstrb(sram_wstrb),
    .mem_s_rdata(sram_rdata)
);

// =========================================================================
// Boot ROM (8 KB) — ISP flasher, initialized at synthesis
// =========================================================================
PicoMem_BOOT_SRAM_8KB u_boot_rom (
    .resetn     (sys_resetn),
    .clk        (clk_p),
    .mem_s_valid(brom_valid),
    .mem_s_ready(brom_ready),
    .mem_s_addr (brom_addr),
    .mem_s_wdata(brom_wdata),
    .mem_s_wstrb(brom_wstrb),
    .mem_s_rdata(brom_rdata)
);

// =========================================================================
// SPI Flash (XIP + Config)
// =========================================================================
PicoMem_SPI_Flash u_spi_flash (
    .clk    (clk_p),
    .resetn (sys_resetn),

    .flash_csb  (flash_csb),
    .flash_clk  (flash_clk),
    .flash_mosi (flash_mosi),
    .flash_miso (flash_miso),

    .flash_mem_valid(spimemxip_valid),
    .flash_mem_ready(spimemxip_ready),
    .flash_mem_addr (spimemxip_addr),
    .flash_mem_wdata(spimemxip_wdata),
    .flash_mem_wstrb(spimemxip_wstrb),
    .flash_mem_rdata(spimemxip_rdata),

    .flash_cfg_valid(spimemcfg_valid),
    .flash_cfg_ready(spimemcfg_ready),
    .flash_cfg_addr (spimemcfg_addr),
    .flash_cfg_wdata(spimemcfg_wdata),
    .flash_cfg_wstrb(spimemcfg_wstrb),
    .flash_cfg_rdata(spimemcfg_rdata)
);

// =========================================================================
// GPIO (7-bit)
// =========================================================================
PicoMem_GPIO u_gpio (
    .resetn     (sys_resetn),
    .io         (gpio),
    .clk        (clk_p),
    .busin_valid(gpio_valid),
    .busin_ready(gpio_ready),
    .busin_addr (gpio_addr),
    .busin_wdata(gpio_wdata),
    .busin_wstrb(gpio_wstrb),
    .busin_rdata(gpio_rdata)
);

// =========================================================================
// UART (115200 baud)
// =========================================================================
PicoMem_UART u_uart (
    .resetn     (sys_resetn),
    .clk        (clk_p),
    .mem_s_valid(uart_valid),
    .mem_s_ready(uart_ready),
    .mem_s_addr (uart_addr),
    .mem_s_wdata(uart_wdata),
    .mem_s_wstrb(uart_wstrb),
    .mem_s_rdata(uart_rdata),
    .ser_rx     (ser_rx),
    .ser_tx     (ser_tx)
);

// =========================================================================
// HDMI output
// =========================================================================
// CDC: UART write → HDMI terminal (same clock domain, but toggle-based
// protocol preserved from v2 for compatibility with svo_term)
wire svo_term_valid_cpu;
assign svo_term_valid_cpu = (uart_valid && uart_ready) & (~uart_addr[2]) & uart_wstrb[0];

reg [7:0] term_data_hold;
reg       term_valid_toggle;
always @(posedge clk_p or negedge sys_resetn) begin
    if (!sys_resetn) begin
        term_data_hold    <= 8'b0;
        term_valid_toggle <= 1'b0;
    end else if (svo_term_valid_cpu) begin
        term_data_hold    <= uart_wdata[7:0];
        term_valid_toggle <= ~term_valid_toggle;
    end
end

// 2FF synchronizer (CPU and pixel clock are the same, but keep the
// toggle-edge protocol for svo_term compatibility)
reg term_toggle_s1, term_toggle_s2, term_toggle_s3;
always @(posedge clk_p or negedge sys_resetn) begin
    if (!sys_resetn) begin
        term_toggle_s1 <= 1'b0;
        term_toggle_s2 <= 1'b0;
        term_toggle_s3 <= 1'b0;
    end else begin
        term_toggle_s1 <= term_valid_toggle;
        term_toggle_s2 <= term_toggle_s1;
        term_toggle_s3 <= term_toggle_s2;
    end
end

wire svo_term_valid = term_toggle_s2 ^ term_toggle_s3;

svo_hdmi_top u_hdmi (
    .clk          (clk_p),
    .resetn       (sys_resetn),

    .clk_pixel    (clk_p),
    .clk_5x_pixel (clk_p5),
    .locked       (pll_lock),

    .term_in_tvalid (svo_term_valid),
    .term_out_tready(),
    .term_in_tdata  (term_data_hold),

    // Display pipeline MMIO (S3 bus)
    .disp_mmio_valid (wbp_valid),
    .disp_mmio_ready (wbp_ready),
    .disp_mmio_addr  (wbp_addr),
    .disp_mmio_wdata (wbp_wdata),
    .disp_mmio_wstrb (wbp_wstrb),
    .disp_mmio_rdata (wbp_rdata),

    .tmds_clk_n   (tmds_clk_n),
    .tmds_clk_p   (tmds_clk_p),
    .tmds_d_n     (tmds_d_n),
    .tmds_d_p     (tmds_d_p)
);

endmodule
