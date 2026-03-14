// =============================================================================
// ATOMiK Mailbox — ARM ↔ RV64I Communication
//
// Module:      atomik_mailbox
// Description: Dual-ported mailbox registers for ARM Cortex-A9 to RV64I
//              co-processor communication. ARM accesses via AXI4-Lite,
//              RV64I accesses via 32-bit valid/ready bus (memory-mapped).
//
// Register Map (both ports see same offsets):
//   0x00  CTRL     - Control register
//                     [0]    = RV64I reset (1=run, 0=hold in reset)
//                     [1]    = RV64I halt request
//                     [31:2] = reserved
//   0x04  STATUS   - Status register (read-only from ARM)
//                     [0]    = RV64I running
//                     [1]    = RV64I halted
//                     [2]    = CMD pending (RV64I has written CMD)
//                     [3]    = RESP ready (RV64I has written RESP)
//                     [31:4] = reserved
//   0x08  CMD      - Command register (ARM writes, RV64I reads)
//   0x0C  ARG0     - Argument 0 (ARM writes, RV64I reads)
//   0x10  ARG1     - Argument 1 (ARM writes, RV64I reads)
//   0x14  ARG2     - Argument 2 (ARM writes, RV64I reads)
//   0x18  RESP0    - Response 0 (RV64I writes, ARM reads)
//   0x1C  RESP1    - Response 1 (RV64I writes, ARM reads)
//   0x20  DEBUG_PC - RV64I program counter (read-only, updated by CPU)
//   0x24  FW_ADDR  - Firmware base address in DDR3 (ARM writes)
//   0x28  FW_SIZE  - Firmware size in bytes (ARM writes)
//   0x2C  DOORBELL - Doorbell register
//                     Write bit[0] from ARM → IRQ to RV64I
//                     Write bit[1] from RV64I → IRQ to ARM
//                     Read clears respective bit
//
// Interrupts:
//   irq_to_arm   - Pulses when RV64I writes DOORBELL[1] or RESP0
//   irq_to_rv64i - Pulses when ARM writes DOORBELL[0] or CMD
//
// Author: ATOMiK Project
// Date:   March 2026
// =============================================================================

`timescale 1ns / 1ps

module atomik_mailbox #(
    parameter ADDR_WIDTH = 6,
    parameter DATA_WIDTH = 32
) (
    // =========================================================================
    // AXI4-Lite Slave Interface (ARM side, 100 MHz)
    // =========================================================================
    input  wire                    s_axi_aclk,
    input  wire                    s_axi_aresetn,

    input  wire [ADDR_WIDTH-1:0]   s_axi_awaddr,
    input  wire                    s_axi_awvalid,
    output reg                     s_axi_awready,

    input  wire [DATA_WIDTH-1:0]   s_axi_wdata,
    input  wire [DATA_WIDTH/8-1:0] s_axi_wstrb,
    input  wire                    s_axi_wvalid,
    output reg                     s_axi_wready,

    output reg  [1:0]              s_axi_bresp,
    output reg                     s_axi_bvalid,
    input  wire                    s_axi_bready,

    input  wire [ADDR_WIDTH-1:0]   s_axi_araddr,
    input  wire                    s_axi_arvalid,
    output reg                     s_axi_arready,

    output reg  [DATA_WIDTH-1:0]   s_axi_rdata,
    output reg  [1:0]              s_axi_rresp,
    output reg                     s_axi_rvalid,
    input  wire                    s_axi_rready,

    // =========================================================================
    // RV64I Bus Interface (co-processor side, same clock domain as CPU)
    // =========================================================================
    input  wire                    rv_clk,
    input  wire                    rv_rst_n,

    input  wire                    rv_valid,
    output reg                     rv_ready,
    input  wire [31:0]             rv_addr,
    input  wire [31:0]             rv_wdata,
    input  wire [3:0]              rv_wstrb,
    output reg  [31:0]             rv_rdata,

    // =========================================================================
    // Control outputs
    // =========================================================================
    output wire                    rv64i_reset_n,    // Active-low reset to RV64I CPU
    output wire                    irq_to_arm,       // Interrupt to PS (active high pulse)

    // Debug input from CPU
    input  wire [31:0]             debug_pc          // Current RV64I PC (directly connected)
);

    // =========================================================================
    // Register addresses (byte offsets, word-aligned)
    // =========================================================================
    localparam REG_CTRL     = 6'h00;
    localparam REG_STATUS   = 6'h04;
    localparam REG_CMD      = 6'h08;
    localparam REG_ARG0     = 6'h0C;
    localparam REG_ARG1     = 6'h10;
    localparam REG_ARG2     = 6'h14;
    localparam REG_RESP0    = 6'h18;
    localparam REG_RESP1    = 6'h1C;
    localparam REG_DEBUG_PC = 6'h20;
    localparam REG_FW_ADDR  = 6'h24;
    localparam REG_FW_SIZE  = 6'h28;
    localparam REG_DOORBELL = 6'h2C;

    // =========================================================================
    // Mailbox registers (dual-ported, AXI clock domain is master)
    // =========================================================================
    reg [31:0] reg_ctrl;
    reg [31:0] reg_cmd;
    reg [31:0] reg_arg0;
    reg [31:0] reg_arg1;
    reg [31:0] reg_arg2;
    reg [31:0] reg_resp0;
    reg [31:0] reg_resp1;
    reg [31:0] reg_fw_addr;
    reg [31:0] reg_fw_size;

    // Status flags
    reg        cmd_pending;     // ARM wrote CMD, RV64I hasn't read it
    reg        resp_ready;      // RV64I wrote RESP, ARM hasn't read it

    // Doorbell bits
    reg        doorbell_to_rv;  // ARM→RV64I doorbell
    reg        doorbell_to_arm; // RV64I→ARM doorbell

    // IRQ generation
    reg        irq_to_arm_r;
    assign irq_to_arm = irq_to_arm_r;

    // Control outputs
    assign rv64i_reset_n = reg_ctrl[0];  // Bit 0 = run (1=run, 0=reset)

    // Status word
    wire [31:0] status_word = {
        28'b0,
        resp_ready,
        cmd_pending,
        reg_ctrl[1],    // halt request echo
        reg_ctrl[0]     // running
    };

    // =========================================================================
    // AXI4-Lite Write FSM
    // =========================================================================
    reg [ADDR_WIDTH-1:0] axi_awaddr_r;
    reg [DATA_WIDTH-1:0] axi_wdata_r;
    reg                  aw_received;
    reg                  w_received;

    // Effective address and data: use registered value if channel arrived
    // in a previous cycle, else use current input (simultaneous arrival)
    wire [5:0]  eff_wr_addr = aw_received ? axi_awaddr_r[5:0] : s_axi_awaddr[5:0];
    wire [31:0] eff_wr_data = w_received  ? axi_wdata_r       : s_axi_wdata;

    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            s_axi_awready <= 1'b1;
            s_axi_wready  <= 1'b1;
            s_axi_bvalid  <= 1'b0;
            s_axi_bresp   <= 2'b00;
            aw_received   <= 1'b0;
            w_received    <= 1'b0;
            axi_awaddr_r  <= {ADDR_WIDTH{1'b0}};
            axi_wdata_r   <= {DATA_WIDTH{1'b0}};

            reg_ctrl    <= 32'b0;  // RV64I held in reset
            reg_cmd     <= 32'b0;
            reg_arg0    <= 32'b0;
            reg_arg1    <= 32'b0;
            reg_arg2    <= 32'b0;
            reg_fw_addr <= 32'h10000000;  // Default firmware address
            reg_fw_size <= 32'b0;

            cmd_pending     <= 1'b0;
            doorbell_to_rv  <= 1'b0;
            doorbell_to_arm <= 1'b0;
            irq_to_arm_r    <= 1'b0;
        end else begin
            // Default: clear IRQ pulse
            irq_to_arm_r <= 1'b0;

            // AW handshake
            if (s_axi_awvalid && s_axi_awready) begin
                axi_awaddr_r <= s_axi_awaddr;
                aw_received  <= 1'b1;
                s_axi_awready <= 1'b0;
            end

            // W handshake
            if (s_axi_wvalid && s_axi_wready) begin
                axi_wdata_r <= s_axi_wdata;
                w_received  <= 1'b1;
                s_axi_wready <= 1'b0;
            end

            // Both received — perform write
            if ((aw_received || (s_axi_awvalid && s_axi_awready)) &&
                (w_received  || (s_axi_wvalid  && s_axi_wready))) begin

                case (eff_wr_addr)
                    REG_CTRL:     reg_ctrl    <= eff_wr_data;
                    REG_CMD: begin
                        reg_cmd     <= eff_wr_data;
                        cmd_pending <= 1'b1;
                    end
                    REG_ARG0:     reg_arg0    <= eff_wr_data;
                    REG_ARG1:     reg_arg1    <= eff_wr_data;
                    REG_ARG2:     reg_arg2    <= eff_wr_data;
                    REG_FW_ADDR:  reg_fw_addr <= eff_wr_data;
                    REG_FW_SIZE:  reg_fw_size <= eff_wr_data;
                    REG_DOORBELL: begin
                        if (eff_wr_data[0])
                            doorbell_to_rv <= 1'b1;
                    end
                    default: ;
                endcase

                s_axi_bvalid  <= 1'b1;
                s_axi_bresp   <= 2'b00;
                aw_received   <= 1'b0;
                w_received    <= 1'b0;
            end

            // B handshake
            if (s_axi_bvalid && s_axi_bready) begin
                s_axi_bvalid  <= 1'b0;
                s_axi_awready <= 1'b1;
                s_axi_wready  <= 1'b1;
            end

            // Clear resp_ready when ARM reads RESP0
            // (handled in read path below)
        end
    end

    // =========================================================================
    // AXI4-Lite Read FSM
    // =========================================================================
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            s_axi_arready <= 1'b1;
            s_axi_rvalid  <= 1'b0;
            s_axi_rdata   <= 32'b0;
            s_axi_rresp   <= 2'b00;
            resp_ready    <= 1'b0;
        end else begin
            if (s_axi_arvalid && s_axi_arready) begin
                s_axi_arready <= 1'b0;
                s_axi_rvalid  <= 1'b1;
                s_axi_rresp   <= 2'b00;

                case (s_axi_araddr[5:0])
                    REG_CTRL:     s_axi_rdata <= reg_ctrl;
                    REG_STATUS:   s_axi_rdata <= status_word;
                    REG_CMD:      s_axi_rdata <= reg_cmd;
                    REG_ARG0:     s_axi_rdata <= reg_arg0;
                    REG_ARG1:     s_axi_rdata <= reg_arg1;
                    REG_ARG2:     s_axi_rdata <= reg_arg2;
                    REG_RESP0: begin
                        s_axi_rdata <= reg_resp0;
                        resp_ready  <= 1'b0;  // Clear on read
                    end
                    REG_RESP1:    s_axi_rdata <= reg_resp1;
                    REG_DEBUG_PC: s_axi_rdata <= debug_pc;
                    REG_FW_ADDR:  s_axi_rdata <= reg_fw_addr;
                    REG_FW_SIZE:  s_axi_rdata <= reg_fw_size;
                    REG_DOORBELL: s_axi_rdata <= {30'b0, doorbell_to_arm, doorbell_to_rv};
                    default:      s_axi_rdata <= 32'hDEAD_BEEF;
                endcase
            end

            if (s_axi_rvalid && s_axi_rready) begin
                s_axi_rvalid  <= 1'b0;
                s_axi_arready <= 1'b1;
            end
        end
    end

    // =========================================================================
    // RV64I Bus Interface (co-processor side)
    //
    // Simple 1-cycle read/write. The RV64I bus is valid/ready protocol.
    // Mailbox registers are in the RV64I memory map at a fixed base address.
    // The coprocessor top routes address decoding.
    // =========================================================================

    // Synchronize cmd_pending and doorbell_to_rv into RV64I clock domain
    reg [1:0] cmd_pending_sync;
    reg [1:0] doorbell_to_rv_sync;

    always @(posedge rv_clk) begin
        if (!rv_rst_n) begin
            cmd_pending_sync    <= 2'b0;
            doorbell_to_rv_sync <= 2'b0;
        end else begin
            cmd_pending_sync    <= {cmd_pending_sync[0], cmd_pending};
            doorbell_to_rv_sync <= {doorbell_to_rv_sync[0], doorbell_to_rv};
        end
    end

    always @(posedge rv_clk) begin
        if (!rv_rst_n) begin
            rv_ready  <= 1'b0;
            rv_rdata  <= 32'b0;
            reg_resp0 <= 32'b0;
            reg_resp1 <= 32'b0;
        end else begin
            rv_ready <= 1'b0;

            if (rv_valid && !rv_ready) begin
                rv_ready <= 1'b1;

                if (|rv_wstrb) begin
                    // Write
                    case (rv_addr[5:0])
                        REG_RESP0: begin
                            reg_resp0  <= rv_wdata;
                            resp_ready <= 1'b1;
                            irq_to_arm_r <= 1'b1;
                        end
                        REG_RESP1:    reg_resp1 <= rv_wdata;
                        REG_DEBUG_PC: ;  // Read-only from RV64I (PC updated via debug_pc input)
                        REG_DOORBELL: begin
                            if (rv_wdata[1]) begin
                                doorbell_to_arm <= 1'b1;
                                irq_to_arm_r    <= 1'b1;
                            end
                        end
                        default: ;
                    endcase
                end else begin
                    // Read
                    case (rv_addr[5:0])
                        REG_CTRL:     rv_rdata <= reg_ctrl;
                        REG_STATUS:   rv_rdata <= status_word;
                        REG_CMD: begin
                            rv_rdata    <= reg_cmd;
                            cmd_pending <= 1'b0;  // Clear on read
                        end
                        REG_ARG0:     rv_rdata <= reg_arg0;
                        REG_ARG1:     rv_rdata <= reg_arg1;
                        REG_ARG2:     rv_rdata <= reg_arg2;
                        REG_RESP0:    rv_rdata <= reg_resp0;
                        REG_RESP1:    rv_rdata <= reg_resp1;
                        REG_DEBUG_PC: rv_rdata <= debug_pc;
                        REG_FW_ADDR:  rv_rdata <= reg_fw_addr;
                        REG_FW_SIZE:  rv_rdata <= reg_fw_size;
                        REG_DOORBELL: begin
                            rv_rdata       <= {30'b0, doorbell_to_arm, doorbell_to_rv_sync[1]};
                            doorbell_to_rv <= 1'b0;  // Clear on read
                        end
                        default:      rv_rdata <= 32'hDEAD_BEEF;
                    endcase
                end
            end
        end
    end

endmodule
