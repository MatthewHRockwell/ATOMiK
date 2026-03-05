// =============================================================================
// ATOMiK Delta-Driven Display Pipeline
//
// Implements change-driven pixel updates:
//   pixel_out = change ? (pixel_ref ^ LUT[index]) : pixel_ref
//
// Sits between svo_overlay output and svo_enc input in the HDMI pipeline.
// Two BSRAM blocks (inferred by synthesis):
//   1. Delta Color LUT: 256 × 24-bit transition delta colors
//   2. Scanline Delta Buffer: 640 × 9-bit {change_flag, lut_index[7:0]}
//
// MMIO at 0xC000_0000 (S3 bus slot):
//   +0x00  DISP_CTRL   [0] enable, [1] vblank (RO), [11:2] row (RO)
//   +0x04  DISP_LUT    Write: {addr[31:24], color[23:0]}
//                       Read: {addr[31:24], lut_data[23:0]} at last addr
//   +0x08  DISP_SCAN   Write: {col[25:16], change[8], index[7:0]}
//   +0x0C  DISP_STATUS [9:0] col (RO), [19:10] row (RO), [20] frame_done (RO)
//
// Pipeline: 2-cycle latency (scanline_buf read → LUT read → XOR)
// =============================================================================

`timescale 1ns/1ps
`include "svo_defines.vh"

module atomik_delta_display #( `SVO_DEFAULT_PARAMS ) (
    input wire clk,
    input wire resetn,

    // AXI-stream input (from svo_overlay)
    input  wire                          in_axis_tvalid,
    output wire                          in_axis_tready,
    input  wire [SVO_BITS_PER_PIXEL-1:0] in_axis_tdata,
    input  wire [0:0]                    in_axis_tuser,

    // AXI-stream output (to svo_enc)
    output wire                          out_axis_tvalid,
    input  wire                          out_axis_tready,
    output wire [SVO_BITS_PER_PIXEL-1:0] out_axis_tdata,
    output wire [0:0]                    out_axis_tuser,

    // MMIO bus (from S3 slot)
    input  wire        mmio_valid,
    output reg         mmio_ready,
    input  wire [31:0] mmio_addr,
    input  wire [31:0] mmio_wdata,
    input  wire [3:0]  mmio_wstrb,
    output reg  [31:0] mmio_rdata
);
    `SVO_DECLS

    // =========================================================================
    // Control registers
    // =========================================================================
    reg delta_enable;   // Bit 0 of DISP_CTRL
    reg [7:0] lut_addr_reg;  // Last-written LUT address (for readback)

    // =========================================================================
    // Pixel position tracking
    // =========================================================================
    reg [9:0] pixel_col;    // 0-639
    reg [9:0] pixel_row;    // 0-479
    reg       frame_done;   // Set when last active pixel completes
    reg [15:0] frame_count; // Increments on each SOF (wraps at 65535)

    wire pixel_advance = in_axis_tvalid & in_axis_tready;

    always @(posedge clk) begin
        if (!resetn) begin
            pixel_col   <= 10'd0;
            pixel_row   <= 10'd0;
            frame_done  <= 1'b0;
            frame_count <= 16'd0;
        end else if (pixel_advance) begin
            if (in_axis_tuser[0]) begin
                // Start of frame
                pixel_col   <= 10'd0;
                pixel_row   <= 10'd0;
                frame_done  <= 1'b0;
                frame_count <= frame_count + 16'd1;
            end else if (pixel_col == SVO_HOR_PIXELS - 1) begin
                pixel_col <= 10'd0;
                if (pixel_row == SVO_VER_PIXELS - 1) begin
                    frame_done <= 1'b1;
                end else begin
                    pixel_row <= pixel_row + 10'd1;
                end
            end else begin
                pixel_col <= pixel_col + 10'd1;
            end
        end
    end

    // =========================================================================
    // Delta Color LUT — 256 × 24-bit (inferred as BSRAM)
    // Port A: CPU write/read via MMIO
    // Port B: Display pipeline read
    // =========================================================================
    reg [23:0] lut_mem [0:255];

    // Port B: Display read (addressed by pipeline stage 1)
    reg [7:0]  lut_rd_addr_b;
    reg [23:0] lut_rd_data_b;

    always @(posedge clk) begin
        lut_rd_data_b <= lut_mem[lut_rd_addr_b];
    end

    // Port A: CPU read (for readback)
    reg [23:0] lut_rd_data_a;

    always @(posedge clk) begin
        lut_rd_data_a <= lut_mem[lut_addr_reg];
    end

    // =========================================================================
    // Scanline Delta Buffer — 1024 × 9-bit (inferred as BSRAM)
    // {change_flag[8], lut_index[7:0]}
    // Applied to ALL scanlines (same pattern every row)
    // Port A: CPU write via MMIO
    // Port B: Display pipeline read
    // =========================================================================
    reg [8:0] scan_mem [0:1023];

    // Port B: Display read (addressed by pixel_col)
    reg [8:0] scan_rd_data_b;

    always @(posedge clk) begin
        scan_rd_data_b <= scan_mem[pixel_col];
    end

    // =========================================================================
    // 2-Stage Pipeline with backpressure (stall-capable)
    //
    // Pipeline advances only when output is consumed (tready) or output is
    // empty (!valid_d2). This prevents pixel loss when svo_enc deasserts
    // tready during blanking periods.
    //
    // Cycle N:   scan_mem.rd_addr = pixel_col_N (combinational from counter)
    // Cycle N+1: scan_rd_data_b = {change_N, index_N}
    //            lut_rd_addr_b = index_N
    //            pixel_d1 = pixel_N, valid_d1, tuser_d1
    // Cycle N+2: lut_rd_data_b = LUT[index_N]
    //            pixel_out = change_d1 ? (pixel_d2 ^ lut_delta) : pixel_d2
    // =========================================================================

    // Pipeline advance signal: move data forward when output can accept
    wire pipe_advance = out_axis_tready || !valid_d2;

    // Pipeline stage 1 registers
    reg [SVO_BITS_PER_PIXEL-1:0] pixel_d1;
    reg        valid_d1;
    reg [0:0]  tuser_d1;
    reg [7:0]  scan_index_d1;
    reg        scan_change_d1;

    // Pipeline stage 2 registers
    reg [SVO_BITS_PER_PIXEL-1:0] pixel_d2;
    reg        valid_d2;
    reg [0:0]  tuser_d2;
    reg        change_d1;

    always @(posedge clk) begin
        if (!resetn) begin
            valid_d1      <= 1'b0;
            valid_d2      <= 1'b0;
            pixel_d1      <= {SVO_BITS_PER_PIXEL{1'b0}};
            pixel_d2      <= {SVO_BITS_PER_PIXEL{1'b0}};
            tuser_d1      <= 1'b0;
            tuser_d2      <= 1'b0;
            change_d1     <= 1'b0;
            scan_index_d1 <= 8'b0;
            scan_change_d1<= 1'b0;
        end else if (pipe_advance) begin
            // Stage 1: Capture pixel data and scanline buffer result
            pixel_d1      <= in_axis_tdata;
            valid_d1      <= in_axis_tvalid & in_axis_tready;
            tuser_d1      <= in_axis_tuser;
            scan_index_d1 <= scan_rd_data_b[7:0];
            scan_change_d1<= scan_rd_data_b[8];

            // Stage 2: Capture for XOR application
            pixel_d2      <= pixel_d1;
            valid_d2      <= valid_d1;
            tuser_d2      <= tuser_d1;
            change_d1     <= scan_change_d1;
            lut_rd_addr_b <= scan_index_d1;  // LUT address from stage 1
        end
    end

    // Output: conditional XOR
    wire [SVO_BITS_PER_PIXEL-1:0] lut_delta = lut_rd_data_b;
    wire apply_delta = change_d1 & delta_enable;

    assign out_axis_tdata  = apply_delta ? (pixel_d2 ^ lut_delta) : pixel_d2;
    assign out_axis_tvalid = valid_d2;
    assign out_axis_tuser  = tuser_d2;

    // Accept input only when pipeline can advance
    assign in_axis_tready = pipe_advance;

    // =========================================================================
    // MMIO Register Interface
    // =========================================================================
    wire mmio_is_write = mmio_valid & |mmio_wstrb;
    wire [1:0] mmio_reg_sel = mmio_addr[3:2];

    always @(posedge clk) begin
        if (!resetn) begin
            mmio_ready   <= 1'b0;
            mmio_rdata   <= 32'h0;
            delta_enable <= 1'b0;
            lut_addr_reg <= 8'h0;
        end else begin
            mmio_ready <= 1'b0;

            if (mmio_valid && !mmio_ready) begin
                mmio_ready <= 1'b1;

                case (mmio_reg_sel)
                    2'b00: begin  // DISP_CTRL
                        if (mmio_is_write)
                            delta_enable <= mmio_wdata[0];
                        mmio_rdata <= {20'b0, pixel_row, 1'b0, delta_enable};
                    end

                    2'b01: begin  // DISP_LUT
                        if (mmio_is_write) begin
                            lut_addr_reg <= mmio_wdata[31:24];
                            lut_mem[mmio_wdata[31:24]] <= mmio_wdata[23:0];
                        end
                        mmio_rdata <= {lut_addr_reg, lut_rd_data_a};
                    end

                    2'b10: begin  // DISP_SCAN
                        if (mmio_is_write) begin
                            scan_mem[mmio_wdata[25:16]] <= mmio_wdata[8:0];
                        end
                        mmio_rdata <= 32'h0;
                    end

                    2'b11: begin  // DISP_STATUS
                        mmio_rdata <= {frame_count, 5'b0, frame_done, pixel_row};
                    end
                endcase
            end
        end
    end

    // =========================================================================
    // Initialize memories to zero (all pixels unchanged on startup)
    // =========================================================================
    integer i;
    initial begin
        for (i = 0; i < 256; i = i + 1)
            lut_mem[i] = 24'h0;
        for (i = 0; i < 1024; i = i + 1)
            scan_mem[i] = 9'h0;
    end

endmodule
