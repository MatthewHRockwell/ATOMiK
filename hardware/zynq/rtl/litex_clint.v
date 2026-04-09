// Minimal RISC-V CLINT (Core Local Interruptor) for single-core VexRiscv
//
// Provides mtime, mtimecmp, and msip for Linux timer interrupts.
// Wishbone slave interface, compatible with LiteX bus.
//
// Register map (byte offsets from base):
//   0x0000  msip       [0]    — software interrupt pending
//   0x4000  mtimecmp_lo [31:0]
//   0x4004  mtimecmp_hi [31:0]
//   0xBFF8  mtime_lo    [31:0]
//   0xBFFC  mtime_hi    [31:0]
//
// This matches the SiFive CLINT layout used by OpenSBI/Linux.

module litex_clint #(
    parameter SYSTEM_CLOCK_FREQUENCY = 100_000_000
) (
    input  wire        clk,
    input  wire        rst,

    // Wishbone slave
    input  wire [13:0] adr,
    input  wire [31:0] dat_w,
    output reg  [31:0] dat_r,
    input  wire        we,
    input  wire        cyc,
    input  wire        stb,
    output reg         ack,

    // Interrupt outputs to CPU
    output wire        timer_interrupt,
    output wire        software_interrupt
);

    reg [63:0] mtime;
    reg [63:0] mtimecmp;
    reg msip;

    assign timer_interrupt    = (mtime >= mtimecmp);
    assign software_interrupt = msip;

    wire wb_access = cyc & stb & ~ack;

    // Single always block — no multi-driver
    always @(posedge clk) begin
        if (rst) begin
            mtime    <= 64'd0;
            mtimecmp <= 64'hFFFFFFFF_FFFFFFFF;
            msip     <= 1'b0;
            ack      <= 1'b0;
            dat_r    <= 32'd0;
        end else begin
            // mtime increments every cycle by default
            mtime <= mtime + 64'd1;

            ack <= 1'b0;

            if (wb_access) begin
                ack <= 1'b1;

                case (adr)
                    14'h0000: begin // msip
                        dat_r <= {31'd0, msip};
                        if (we) msip <= dat_w[0];
                    end

                    14'h1000: begin // mtimecmp_lo
                        dat_r <= mtimecmp[31:0];
                        if (we) mtimecmp[31:0] <= dat_w;
                    end

                    14'h1001: begin // mtimecmp_hi
                        dat_r <= mtimecmp[63:32];
                        if (we) mtimecmp[63:32] <= dat_w;
                    end

                    14'h2FFE: begin // mtime_lo
                        dat_r <= mtime[31:0];
                        if (we) mtime[31:0] <= dat_w;
                    end

                    14'h2FFF: begin // mtime_hi
                        dat_r <= mtime[63:32];
                        if (we) mtime[63:32] <= dat_w;
                    end

                    default: dat_r <= 32'd0;
                endcase
            end
        end
    end

endmodule
