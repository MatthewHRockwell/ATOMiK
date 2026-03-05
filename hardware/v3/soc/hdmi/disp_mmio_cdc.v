// =============================================================================
// Display MMIO CDC Bridge — Toggle-Handshake Clock Domain Crossing
//
// Bridges CPU bus domain (21.6 MHz) to HDMI pixel domain (25.2 MHz) for
// display pipeline MMIO registers. Adapted from v2 atomik_cdc_bridge.
//
// Protocol:
//   1. CPU side latches addr/wdata/wstrb, toggles req_toggle
//   2. Pixel side syncs req_toggle via 2FF, detects edge
//   3. Pixel side drives atomik_delta_display MMIO, waits for ready
//   4. Pixel side latches response, toggles ack_toggle
//   5. CPU side syncs ack_toggle via 2FF, detects edge, drives mem_ready
//
// Latency: ~5-6 CPU cycles per MMIO access
// =============================================================================

`timescale 1ns / 1ps

module disp_mmio_cdc (
    // CPU-side clock domain (21.6 MHz)
    input  wire        clk_cpu,
    input  wire        rst_cpu_n,

    // CPU bus interface (CPU domain)
    input  wire        cpu_valid,
    output reg         cpu_ready,
    input  wire [31:0] cpu_addr,
    input  wire [31:0] cpu_wdata,
    input  wire [3:0]  cpu_wstrb,
    output reg  [31:0] cpu_rdata,

    // Pixel-side clock domain (25.2 MHz)
    input  wire        clk_pixel,
    input  wire        rst_pixel_n,

    // Display MMIO interface (pixel domain)
    output reg         disp_valid,
    input  wire        disp_ready,
    output reg  [31:0] disp_addr,
    output reg  [31:0] disp_wdata,
    output reg  [3:0]  disp_wstrb,
    input  wire [31:0] disp_rdata
);

    // =========================================================================
    // CPU-Side Logic (clk_cpu domain)
    // =========================================================================

    // Latched request data — stable before toggle
    reg [31:0] req_addr;
    reg [31:0] req_wdata_r;
    reg [3:0]  req_wstrb_r;

    // Toggle handshake signals
    reg        req_toggle;
    reg        ack_sync_0, ack_sync_1, ack_sync_2;
    wire       ack_edge;

    // Bus FSM
    localparam BUS_IDLE = 2'd0;
    localparam BUS_WAIT = 2'd1;
    localparam BUS_DONE = 2'd2;
    reg [1:0]  bus_state;

    // Response data from pixel side (latched, stable when ack toggles)
    reg [31:0] resp_rdata;

    // 2FF synchronizer for ack_toggle into CPU domain
    always @(posedge clk_cpu or negedge rst_cpu_n) begin
        if (!rst_cpu_n) begin
            ack_sync_0 <= 1'b0;
            ack_sync_1 <= 1'b0;
            ack_sync_2 <= 1'b0;
        end else begin
            ack_sync_0 <= ack_toggle;
            ack_sync_1 <= ack_sync_0;
            ack_sync_2 <= ack_sync_1;
        end
    end

    assign ack_edge = ack_sync_1 ^ ack_sync_2;

    // CPU-side FSM
    always @(posedge clk_cpu or negedge rst_cpu_n) begin
        if (!rst_cpu_n) begin
            bus_state   <= BUS_IDLE;
            req_toggle  <= 1'b0;
            req_addr    <= 32'b0;
            req_wdata_r <= 32'b0;
            req_wstrb_r <= 4'b0;
            cpu_ready   <= 1'b0;
            cpu_rdata   <= 32'b0;
        end else begin
            cpu_ready <= 1'b0;

            case (bus_state)
                BUS_IDLE: begin
                    if (cpu_valid && !cpu_ready) begin
                        // Latch request data (stable before toggle)
                        req_addr    <= cpu_addr;
                        req_wdata_r <= cpu_wdata;
                        req_wstrb_r <= cpu_wstrb;
                        // Toggle to signal pixel side
                        req_toggle  <= ~req_toggle;
                        bus_state   <= BUS_WAIT;
                    end
                end

                BUS_WAIT: begin
                    if (ack_edge) begin
                        // Pixel side completed — capture response
                        cpu_rdata <= resp_rdata;
                        cpu_ready <= 1'b1;
                        bus_state <= BUS_DONE;
                    end
                end

                BUS_DONE: begin
                    bus_state <= BUS_IDLE;
                end

                default: bus_state <= BUS_IDLE;
            endcase
        end
    end

    // =========================================================================
    // Pixel-Side Logic (clk_pixel domain)
    // =========================================================================

    // 2FF synchronizer for req_toggle into pixel domain
    reg        req_sync_0, req_sync_1, req_sync_2;
    wire       req_edge;

    // Acknowledge toggle
    reg        ack_toggle;

    // Pixel-side FSM
    localparam PX_IDLE = 2'd0;
    localparam PX_EXEC = 2'd1;
    localparam PX_ACK  = 2'd2;
    reg [1:0]  px_state;

    always @(posedge clk_pixel or negedge rst_pixel_n) begin
        if (!rst_pixel_n) begin
            req_sync_0 <= 1'b0;
            req_sync_1 <= 1'b0;
            req_sync_2 <= 1'b0;
        end else begin
            req_sync_0 <= req_toggle;
            req_sync_1 <= req_sync_0;
            req_sync_2 <= req_sync_1;
        end
    end

    assign req_edge = req_sync_1 ^ req_sync_2;

    // Pixel-side FSM: receive request, drive display MMIO, return response
    always @(posedge clk_pixel or negedge rst_pixel_n) begin
        if (!rst_pixel_n) begin
            px_state    <= PX_IDLE;
            ack_toggle  <= 1'b0;
            disp_valid  <= 1'b0;
            disp_addr   <= 32'b0;
            disp_wdata  <= 32'b0;
            disp_wstrb  <= 4'b0;
            resp_rdata  <= 32'b0;
        end else begin
            case (px_state)
                PX_IDLE: begin
                    disp_valid <= 1'b0;
                    if (req_edge) begin
                        // New request — drive display MMIO
                        disp_valid <= 1'b1;
                        disp_addr  <= req_addr;
                        disp_wdata <= req_wdata_r;
                        disp_wstrb <= req_wstrb_r;
                        px_state   <= PX_EXEC;
                    end
                end

                PX_EXEC: begin
                    if (disp_ready) begin
                        // Display MMIO completed — capture response
                        resp_rdata <= disp_rdata;
                        disp_valid <= 1'b0;
                        // Toggle ack to signal CPU side
                        ack_toggle <= ~ack_toggle;
                        px_state   <= PX_ACK;
                    end
                end

                PX_ACK: begin
                    // One cycle gap before accepting next request
                    px_state <= PX_IDLE;
                end

                default: px_state <= PX_IDLE;
            endcase
        end
    end

endmodule
