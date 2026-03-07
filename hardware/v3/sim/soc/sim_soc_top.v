// =============================================================================
// ATOMiK v3 SoC — Simulation-Only Top
//
// Simulation-friendly wrapper that replaces Gowin IP (PLL, CLKDIV, HDMI LVDS)
// with behavioral models. Loads firmware via $readmemh.
//
// Memory map matches atomik_v3_soc.v:
//   0x00000000  Flash (behavioral, loaded from file)
//   0x40000000  Data SRAM (8 KB, behavioral)
//   0x80000000  Boot ROM (8 KB, behavioral, loaded from file)
//   0x81000000  SPI Flash Config (stub — immediate ready)
//   0x82000000  GPIO (behavioral)
//   0x83000000  UART (behavioral — tx_byte output for C++ monitor)
// =============================================================================

`timescale 1ns/1ps

/* verilator lint_off UNUSEDSIGNAL */
/* verilator lint_off UNUSEDPARAM */

module sim_soc_top (
    input  wire        clk,
    input  wire        rst_n,

    // UART pins
    input  wire        ser_rx,
    output wire        ser_tx,

    // Link UART pins (inter-board)
    input  wire        link_rx,
    output wire        link_tx,

    // GPIO (directly exposed for monitoring)
    output wire [6:0]  gpio_out,

    // Debug: link UART internal state
    output wire        dbg_link_recv_valid,
    output wire [7:0]  dbg_link_recv_data,
    output wire [31:0] dbg_link_cfg_div
);

    // =========================================================================
    // CPU bus signals
    // =========================================================================
    wire        mem_valid;
    wire        mem_ready;
    wire [31:0] mem_addr;
    wire [31:0] mem_wdata;
    wire [3:0]  mem_wstrb;
    wire [31:0] mem_rdata;

    // =========================================================================
    // CPU
    // =========================================================================
    atomik_v3_cpu #(
        .RESET_PC(64'h0000_0000_8000_0000)
    ) u_cpu (
        .clk       (clk),
        .rst_n     (rst_n),
        .mem_valid  (mem_valid),
        .mem_ready  (mem_ready),
        .mem_addr   (mem_addr),
        .mem_wdata  (mem_wdata),
        .mem_wstrb  (mem_wstrb),
        .mem_rdata  (mem_rdata)
    );

    // =========================================================================
    // Address decode (top level)
    // =========================================================================
    wire [1:0] addr_top = mem_addr[31:30];

    wire flash_sel  = (addr_top == 2'b00);  // 0x00000000
    wire sram_sel   = (addr_top == 2'b01);  // 0x40000000
    wire periph_sel = (addr_top == 2'b10);  // 0x80000000
    wire unused_sel = (addr_top == 2'b11);  // 0xC0000000

    // Peripheral sub-decode (addr[27:24])
    wire [3:0] periph_sub = mem_addr[27:24];
    wire brom_sel     = periph_sel && (periph_sub == 4'h0);
    wire spicfg_sel   = periph_sel && (periph_sub == 4'h1);
    wire gpio_sel     = periph_sel && (periph_sub == 4'h2);
    wire uart_sel     = periph_sel && (periph_sub == 4'h3);

    // =========================================================================
    // Flash memory (behavioral, 64 KB simulated)
    // =========================================================================
    localparam FLASH_WORDS = 16384;  // 64 KB
    reg [31:0] flash_mem [0:FLASH_WORDS-1];
    reg [31:0] flash_rdata;
    reg flash_ready;

    initial begin
        integer i;
        for (i = 0; i < FLASH_WORDS; i = i + 1)
            flash_mem[i] = 32'h0;
        $readmemh("fw-flash.hex32", flash_mem);
    end

    always @(posedge clk) begin
        if (!rst_n) begin
            flash_ready <= 1'b0;
        end else begin
            flash_ready <= 1'b0;
            if (mem_valid && flash_sel && !flash_ready) begin
                flash_rdata <= flash_mem[mem_addr[15:2]];
                flash_ready <= 1'b1;
            end
        end
    end

    // =========================================================================
    // Data SRAM (8 KB behavioral)
    // =========================================================================
    localparam SRAM_WORDS = 2048;  // 8 KB
    reg [31:0] sram_mem [0:SRAM_WORDS-1];
    reg [31:0] sram_rdata;
    reg sram_ready;

    initial begin
        integer i;
        for (i = 0; i < SRAM_WORDS; i = i + 1)
            sram_mem[i] = 32'h0;
    end

    always @(posedge clk) begin
        if (!rst_n) begin
            sram_ready <= 1'b0;
        end else begin
            sram_ready <= 1'b0;
            if (mem_valid && sram_sel && !sram_ready) begin
                sram_rdata <= sram_mem[mem_addr[12:2]];
                if (mem_wstrb[0]) sram_mem[mem_addr[12:2]][ 7: 0] <= mem_wdata[ 7: 0];
                if (mem_wstrb[1]) sram_mem[mem_addr[12:2]][15: 8] <= mem_wdata[15: 8];
                if (mem_wstrb[2]) sram_mem[mem_addr[12:2]][23:16] <= mem_wdata[23:16];
                if (mem_wstrb[3]) sram_mem[mem_addr[12:2]][31:24] <= mem_wdata[31:24];
                sram_ready <= 1'b1;
            end
        end
    end

    // =========================================================================
    // Boot ROM (8 KB behavioral)
    // =========================================================================
    localparam BROM_WORDS = 2048;  // 8 KB
    reg [31:0] brom_mem [0:BROM_WORDS-1];
    reg [31:0] brom_rdata;
    reg brom_ready;

    initial begin
        integer i;
        for (i = 0; i < BROM_WORDS; i = i + 1)
            brom_mem[i] = 32'h0;
        $readmemh("fw-brom.hex32", brom_mem);
    end

    always @(posedge clk) begin
        if (!rst_n) begin
            brom_ready <= 1'b0;
        end else begin
            brom_ready <= 1'b0;
            if (mem_valid && brom_sel && !brom_ready) begin
                brom_rdata <= brom_mem[mem_addr[12:2]];
                // BROM is writable (ISP flasher writes to itself)
                if (mem_wstrb[0]) brom_mem[mem_addr[12:2]][ 7: 0] <= mem_wdata[ 7: 0];
                if (mem_wstrb[1]) brom_mem[mem_addr[12:2]][15: 8] <= mem_wdata[15: 8];
                if (mem_wstrb[2]) brom_mem[mem_addr[12:2]][23:16] <= mem_wdata[23:16];
                if (mem_wstrb[3]) brom_mem[mem_addr[12:2]][31:24] <= mem_wdata[31:24];
                brom_ready <= 1'b1;
            end
        end
    end

    // =========================================================================
    // SPI Flash Config (stub — immediate ready, zero data)
    // =========================================================================
    wire spicfg_ready = mem_valid && spicfg_sel;
    wire [31:0] spicfg_rdata = 32'h0;

    // =========================================================================
    // GPIO (behavioral)
    // =========================================================================
    reg [31:0] gpio_out_r;
    reg [31:0] gpio_oe_r;
    reg [31:0] gpio_rdata;
    reg gpio_ready;

    assign gpio_out = gpio_out_r[6:0];

    always @(posedge clk) begin
        if (!rst_n) begin
            gpio_out_r <= 32'b0;
            gpio_oe_r  <= 32'b0;
            gpio_ready <= 1'b0;
            gpio_rdata <= 32'b0;
        end else begin
            gpio_ready <= 1'b0;
            if (mem_valid && gpio_sel && !gpio_ready) begin
                gpio_ready <= 1'b1;
                case (mem_addr[3:2])
                    2'b00: begin
                        gpio_rdata <= gpio_out_r;
                        if (mem_wstrb[0]) gpio_out_r[ 7: 0] <= mem_wdata[ 7: 0];
                        if (mem_wstrb[1]) gpio_out_r[15: 8] <= mem_wdata[15: 8];
                        if (mem_wstrb[2]) gpio_out_r[23:16] <= mem_wdata[23:16];
                        if (mem_wstrb[3]) gpio_out_r[31:24] <= mem_wdata[31:24];
                    end
                    2'b01: gpio_rdata <= 32'h0;  // IN — no external inputs in sim
                    2'b10: begin
                        gpio_rdata <= gpio_oe_r;
                        if (mem_wstrb[0]) gpio_oe_r[ 7: 0] <= mem_wdata[ 7: 0];
                        if (mem_wstrb[1]) gpio_oe_r[15: 8] <= mem_wdata[15: 8];
                        if (mem_wstrb[2]) gpio_oe_r[23:16] <= mem_wdata[23:16];
                        if (mem_wstrb[3]) gpio_oe_r[31:24] <= mem_wdata[31:24];
                    end
                    default: gpio_rdata <= 32'hDEADBEEF;
                endcase
            end
        end
    end

    // =========================================================================
    // UART (behavioral — uses simpleuart protocol)
    // =========================================================================
    // Instantiate the real simpleuart + PicoMem_UART for bit-accurate behavior
    wire uart_ready_w;
    wire [31:0] uart_rdata_w;

    PicoMem_UART u_uart (
        .resetn     (rst_n),
        .clk        (clk),
        .mem_s_valid(mem_valid && uart_sel),
        .mem_s_ready(uart_ready_w),
        .mem_s_addr (mem_addr),
        .mem_s_wdata(mem_wdata),
        .mem_s_wstrb(mem_wstrb),
        .mem_s_rdata(uart_rdata_w),
        .ser_rx     (ser_rx),
        .ser_tx     (ser_tx)
    );

    // =========================================================================
    // S3 region (0xC0000000) — sub-mux: display MMIO stub + link UART
    //   addr[8]=0 → Display MMIO (stub: immediate ready, zero data)
    //   addr[8]=1 → Link UART (real simpleuart)
    // =========================================================================
    wire s3_sel = unused_sel;
    wire s3_link_sel = s3_sel && mem_addr[8];
    wire s3_disp_sel = s3_sel && !mem_addr[8];

    // Display MMIO stub
    wire s3_disp_ready = mem_valid && s3_disp_sel;

    // Link UART
    wire s3_link_ready;
    wire [31:0] s3_link_rdata;

    PicoMem_UART u_link_uart (
        .resetn     (rst_n),
        .clk        (clk),
        .mem_s_valid(mem_valid && s3_link_sel),
        .mem_s_ready(s3_link_ready),
        .mem_s_addr (mem_addr),
        .mem_s_wdata(mem_wdata),
        .mem_s_wstrb(mem_wstrb),
        .mem_s_rdata(s3_link_rdata),
        .ser_rx     (link_rx),
        .ser_tx     (link_tx)
    );

    wire s3_ready = s3_link_sel ? s3_link_ready : s3_disp_ready;
    wire [31:0] s3_rdata = s3_link_sel ? s3_link_rdata : 32'h0;

    // Debug: expose link UART internal state
    assign dbg_link_recv_valid = u_link_uart.u_uart.recv_buf_valid;
    assign dbg_link_recv_data  = u_link_uart.u_uart.recv_buf_data;
    assign dbg_link_cfg_div    = u_link_uart.u_uart.cfg_divider;

    // =========================================================================
    // Bus mux
    // =========================================================================
    assign mem_ready = flash_sel   ? flash_ready :
                       sram_sel    ? sram_ready :
                       brom_sel    ? brom_ready :
                       spicfg_sel  ? spicfg_ready :
                       gpio_sel    ? gpio_ready :
                       uart_sel    ? uart_ready_w :
                       s3_sel      ? s3_ready :
                       1'b0;

    assign mem_rdata = flash_sel   ? flash_rdata :
                       sram_sel    ? sram_rdata :
                       brom_sel    ? brom_rdata :
                       spicfg_sel  ? spicfg_rdata :
                       gpio_sel    ? gpio_rdata :
                       uart_sel    ? uart_rdata_w :
                       s3_sel      ? s3_rdata :
                       32'h0;

endmodule
