// =============================================================================
// ATOMiK AXI4-Lite Slave Wrapper
//
// Module:      atomik_axi4lite_wrapper
// Description: AXI4-Lite slave interface for ATOMiK on Zynq.
//              Provides memory-mapped register access for Linux userspace
//              via UIO on Zynq PS.
//
//              Connects to ATOMiK core through a CDC bridge when running
//              in dual-clock mode (AXI @ 100 MHz, core @ 200+ MHz).
//
//              Register Map (32-bit AXI, 64-bit ATOMiK datapath):
//                0x00  LOAD_ADDR     (W)   Address for next LOAD [7:0]
//                0x04  LOAD_DATA_LO  (W)   Initial state bits [31:0]
//                0x08  LOAD_DATA_HI  (W)   Initial state bits [63:32], triggers LOAD
//                0x0C  ACCUM_LO      (W)   Delta bits [31:0]
//                0x10  ACCUM_HI      (W)   Delta bits [63:32], triggers ACCUM
//                0x14  STATE_LO      (R)   Current state bits [31:0], latches 64-bit snapshot
//                0x18  STATE_HI      (R)   Current state bits [63:32] (from latch)
//                0x1C  STATUS        (R)   {8'h0, version[7:0], n_banks[7:0], 7'h0, acc_zero}
//                0x20  SWAP_ADDR     (W)   Address + triggers SWAP [7:0]
//                0x24  CONFIG        (R/W) Bit 0 = enable (resets to 1)
//
//              32-bit AXI to 64-bit datapath: HI write triggers the operation.
//              Software must write LO first, then HI.
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_axi4lite_wrapper #(
    parameter ADDR_WIDTH = 6,    // 64 bytes of register space (10 registers)
    parameter DATA_WIDTH = 32
) (
    // AXI4-Lite Slave Interface
    input  wire                    s_axi_aclk,
    input  wire                    s_axi_aresetn,

    // Write address channel
    input  wire [ADDR_WIDTH-1:0]   s_axi_awaddr,
    input  wire                    s_axi_awvalid,
    output reg                     s_axi_awready,

    // Write data channel
    input  wire [DATA_WIDTH-1:0]   s_axi_wdata,
    input  wire [DATA_WIDTH/8-1:0] s_axi_wstrb,
    input  wire                    s_axi_wvalid,
    output reg                     s_axi_wready,

    // Write response channel
    output reg  [1:0]              s_axi_bresp,
    output reg                     s_axi_bvalid,
    input  wire                    s_axi_bready,

    // Read address channel
    input  wire [ADDR_WIDTH-1:0]   s_axi_araddr,
    input  wire                    s_axi_arvalid,
    output reg                     s_axi_arready,

    // Read data channel
    output reg  [DATA_WIDTH-1:0]   s_axi_rdata,
    output reg  [1:0]              s_axi_rresp,
    output reg                     s_axi_rvalid,
    input  wire                    s_axi_rready,

    // CDC bridge interface (active when dual-clock)
    output reg         cdc_req_valid,
    input  wire        cdc_req_ready,
    output reg         cdc_req_load_en,
    output reg         cdc_req_accum_en,
    output reg         cdc_req_swap_en,
    output reg  [7:0]  cdc_req_addr,
    output reg  [63:0] cdc_req_data,

    input  wire [63:0] cdc_resp_state,
    input  wire        cdc_resp_acc_zero,
    input  wire        cdc_resp_valid,

    // Core enable (directly drives core reset)
    output wire        core_enable
);

    // =========================================================================
    // Register address offsets (word-aligned)
    // =========================================================================
    localparam REG_LOAD_ADDR    = 6'h00;
    localparam REG_LOAD_DATA_LO = 6'h04;
    localparam REG_LOAD_DATA_HI = 6'h08;
    localparam REG_ACCUM_LO     = 6'h0C;
    localparam REG_ACCUM_HI     = 6'h10;
    localparam REG_STATE_LO     = 6'h14;
    localparam REG_STATE_HI     = 6'h18;
    localparam REG_STATUS       = 6'h1C;
    localparam REG_SWAP_ADDR    = 6'h20;
    localparam REG_CONFIG       = 6'h24;

    // =========================================================================
    // Configuration constants
    // =========================================================================
    localparam [7:0] VERSION  = 8'h02;  // v2: CDC-capable
    localparam [7:0] N_BANKS  = 8'h01;

    // =========================================================================
    // Soft registers (buffered halves, config)
    // =========================================================================
    reg  [7:0]  reg_load_addr;
    reg  [31:0] reg_load_data_lo;
    reg  [31:0] reg_accum_lo;
    reg         reg_enable;          // CONFIG bit 0
    reg  [63:0] state_snapshot;      // Latched on STATE_LO read
    reg         acc_zero_snapshot;   // Latched with state

    assign core_enable = reg_enable;

    // =========================================================================
    // Cached response from CDC bridge
    // =========================================================================
    reg  [63:0] cached_state;
    reg         cached_acc_zero;

    // Capture CDC responses as they arrive
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            cached_state    <= 64'b0;
            cached_acc_zero <= 1'b1;
        end else if (cdc_resp_valid) begin
            cached_state    <= cdc_resp_state;
            cached_acc_zero <= cdc_resp_acc_zero;
        end
    end

    // =========================================================================
    // Operation sequencer — serializes AXI register writes into CDC requests
    // =========================================================================
    // When a trigger register is written (LOAD_DATA_HI, ACCUM_HI, SWAP_ADDR),
    // we issue a CDC request and stall the AXI write response until the CDC
    // bridge acknowledges completion. This ensures the response read after a
    // write reflects the operation result.

    reg        op_pending;     // Operation in flight through CDC

    // =========================================================================
    // Write channel: independent AW/W capture
    // =========================================================================
    reg        aw_captured;
    reg        w_captured;
    reg  [ADDR_WIDTH-1:0] aw_addr_reg;
    reg  [DATA_WIDTH-1:0] w_data_reg;

    wire write_fire = aw_captured & w_captured & !op_pending;

    // Trigger registers: BRESP deferred until CDC completes
    wire is_trigger_write = (aw_addr_reg == REG_LOAD_DATA_HI) ||
                            (aw_addr_reg == REG_ACCUM_HI) ||
                            (aw_addr_reg == REG_SWAP_ADDR);

    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            aw_captured  <= 1'b0;
            w_captured   <= 1'b0;
            aw_addr_reg  <= {ADDR_WIDTH{1'b0}};
            w_data_reg   <= {DATA_WIDTH{1'b0}};
            s_axi_awready <= 1'b1;
            s_axi_wready  <= 1'b1;
            s_axi_bvalid  <= 1'b0;
            s_axi_bresp   <= 2'b00;
        end else begin
            if (!aw_captured)
                s_axi_awready <= 1'b1;
            if (!w_captured)
                s_axi_wready <= 1'b1;

            // Capture AW
            if (s_axi_awvalid && s_axi_awready) begin
                aw_addr_reg   <= s_axi_awaddr;
                aw_captured   <= 1'b1;
                s_axi_awready <= 1'b0;
            end

            // Capture W
            if (s_axi_wvalid && s_axi_wready) begin
                w_data_reg  <= s_axi_wdata;
                w_captured  <= 1'b1;
                s_axi_wready <= 1'b0;
            end

            // Both captured + no pending op → process write
            if (write_fire) begin
                aw_captured <= 1'b0;
                w_captured  <= 1'b0;
                // Non-trigger registers: immediate BRESP
                // Trigger registers: defer BRESP until CDC completes
                if (!is_trigger_write) begin
                    s_axi_bvalid <= 1'b1;
                    s_axi_bresp  <= 2'b00;
                end
            end

            // BRESP handshake complete
            if (s_axi_bvalid && s_axi_bready) begin
                s_axi_bvalid <= 1'b0;
            end

            // Issue deferred BRESP when CDC operation completes
            if (op_pending && cdc_resp_valid) begin
                s_axi_bvalid <= 1'b1;
                s_axi_bresp  <= 2'b00;
            end
        end
    end

    // =========================================================================
    // Write register decode + CDC request generation
    // =========================================================================
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            reg_load_addr    <= 8'h00;
            reg_load_data_lo <= 32'h0;
            reg_accum_lo     <= 32'h0;
            reg_enable       <= 1'b1;
            cdc_req_valid    <= 1'b0;
            cdc_req_load_en  <= 1'b0;
            cdc_req_accum_en <= 1'b0;
            cdc_req_swap_en  <= 1'b0;
            cdc_req_addr     <= 8'h00;
            cdc_req_data     <= 64'h0;
            op_pending       <= 1'b0;
        end else begin
            cdc_req_valid <= 1'b0;

            // Clear pending when CDC responds
            if (op_pending && cdc_resp_valid)
                op_pending <= 1'b0;

            if (write_fire) begin
                case (aw_addr_reg)
                    REG_LOAD_ADDR: begin
                        reg_load_addr <= w_data_reg[7:0];
                    end

                    REG_LOAD_DATA_LO: begin
                        reg_load_data_lo <= w_data_reg;
                    end

                    REG_LOAD_DATA_HI: begin
                        // HI write triggers LOAD via CDC
                        cdc_req_valid    <= 1'b1;
                        cdc_req_load_en  <= 1'b1;
                        cdc_req_accum_en <= 1'b0;
                        cdc_req_swap_en  <= 1'b0;
                        cdc_req_addr     <= reg_load_addr;
                        cdc_req_data     <= {w_data_reg, reg_load_data_lo};
                        op_pending       <= 1'b1;
                    end

                    REG_ACCUM_LO: begin
                        reg_accum_lo <= w_data_reg;
                    end

                    REG_ACCUM_HI: begin
                        // HI write triggers ACCUM via CDC
                        cdc_req_valid    <= 1'b1;
                        cdc_req_load_en  <= 1'b0;
                        cdc_req_accum_en <= 1'b1;
                        cdc_req_swap_en  <= 1'b0;
                        cdc_req_addr     <= 8'h00;
                        cdc_req_data     <= {w_data_reg, reg_accum_lo};
                        op_pending       <= 1'b1;
                    end

                    REG_SWAP_ADDR: begin
                        // Write triggers SWAP via CDC
                        cdc_req_valid    <= 1'b1;
                        cdc_req_load_en  <= 1'b0;
                        cdc_req_accum_en <= 1'b0;
                        cdc_req_swap_en  <= 1'b1;
                        cdc_req_addr     <= w_data_reg[7:0];
                        cdc_req_data     <= 64'h0;
                        op_pending       <= 1'b1;
                    end

                    REG_CONFIG: begin
                        reg_enable <= w_data_reg[0];
                    end

                    default: ;
                endcase
            end
        end
    end

    // =========================================================================
    // Read channel
    // =========================================================================
    reg ar_captured;
    reg [ADDR_WIDTH-1:0] ar_addr_reg;

    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            ar_captured   <= 1'b0;
            ar_addr_reg   <= {ADDR_WIDTH{1'b0}};
            s_axi_arready <= 1'b1;
            s_axi_rvalid  <= 1'b0;
            s_axi_rdata   <= {DATA_WIDTH{1'b0}};
            s_axi_rresp   <= 2'b00;
            state_snapshot <= 64'h0;
            acc_zero_snapshot <= 1'b1;
        end else begin
            if (!ar_captured)
                s_axi_arready <= 1'b1;

            // Capture AR
            if (s_axi_arvalid && s_axi_arready) begin
                ar_addr_reg   <= s_axi_araddr;
                ar_captured   <= 1'b1;
                s_axi_arready <= 1'b0;
            end

            // Process read
            if (ar_captured && !s_axi_rvalid) begin
                ar_captured  <= 1'b0;
                s_axi_rvalid <= 1'b1;
                s_axi_rresp  <= 2'b00;

                case (ar_addr_reg)
                    REG_STATE_LO: begin
                        // Latch full 64-bit snapshot on LO read
                        state_snapshot    <= cached_state;
                        acc_zero_snapshot <= cached_acc_zero;
                        s_axi_rdata       <= cached_state[31:0];
                    end

                    REG_STATE_HI: begin
                        s_axi_rdata <= state_snapshot[63:32];
                    end

                    REG_STATUS: begin
                        s_axi_rdata <= {8'h00, VERSION, N_BANKS, 7'h00, acc_zero_snapshot};
                    end

                    REG_CONFIG: begin
                        s_axi_rdata <= {31'h0, reg_enable};
                    end

                    default: begin
                        s_axi_rdata <= 32'h0;
                    end
                endcase
            end

            // RDATA handshake complete
            if (s_axi_rvalid && s_axi_rready) begin
                s_axi_rvalid <= 1'b0;
            end
        end
    end

endmodule
