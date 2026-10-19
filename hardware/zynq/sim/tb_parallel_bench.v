// =============================================================================
// tb_parallel_bench.v — testbench for atomik_parallel_bench
//
// Verifies:
//   1. Result is IDENTICAL across active_banks (1,2,3,4,8) for same count/seed
//      (the honesty invariant: more banks must not change the answer).
//   2. CYCLES == ceil(count / active_banks)  (real throughput scaling).
//   3. Hardware result == independent software-style reference XOR.
//
// Run: iverilog -g2012 -o /tmp/tb_pb tb_parallel_bench.v \
//        ../rtl/atomik_parallel_bench.v \
//        ../../rtl/atomik_parallel_acc.v ../../rtl/atomik_delta_acc.v
//      vvp /tmp/tb_pb
// =============================================================================
`timescale 1ns/1ps

module tb_parallel_bench;
    reg         clk = 0, rst = 1;
    reg  [4:0]  adr;
    reg  [31:0] dat_w;
    wire [31:0] dat_r;
    reg         we, cyc, stb;
    wire        ack;

    integer errors = 0;

    atomik_parallel_bench #(.N_BANKS(8), .DELTA_WIDTH(64)) dut (
        .clk(clk), .rst(rst),
        .adr(adr), .dat_w(dat_w), .dat_r(dat_r),
        .we(we), .cyc(cyc), .stb(stb), .ack(ack)
    );

    always #5 clk = ~clk;   // 100 MHz

    // ── Wishbone helpers ─────────────────────────────────────────────
    task wb_write(input [4:0] a, input [31:0] d);
        begin
            @(posedge clk); adr=a; dat_w=d; we=1; cyc=1; stb=1;
            wait(ack); @(posedge clk); we=0; cyc=0; stb=0; adr=0; dat_w=0;
        end
    endtask

    task wb_read(input [4:0] a, output [31:0] d);
        begin
            @(posedge clk); adr=a; we=0; cyc=1; stb=1;
            wait(ack); d=dat_r; @(posedge clk); cyc=0; stb=0; adr=0;
        end
    endtask

    // ── Reference: same stateless mix the engine uses ────────────────
    function [63:0] ref_dmix(input [63:0] x_in);
        reg [63:0] x;
        begin
            x = x_in;
            x = x ^ (x << 13);
            x = x ^ (x >> 7);
            x = x ^ (x << 17);
            ref_dmix = x;
        end
    endfunction

    function [63:0] ref_xor(input [63:0] seed, input [31:0] count);
        reg [63:0] acc; reg [63:0] i64;
        begin
            acc = 64'd0;
            for (i64 = 0; i64 < {32'd0, count}; i64 = i64 + 1)
                acc = acc ^ ref_dmix(seed ^ i64);
            ref_xor = acc;
        end
    endfunction

    // ── Run one benchmark, return result + cycles ────────────────────
    task run_bench(input [31:0] count, input [31:0] active, input [63:0] seed,
                   output [63:0] result, output [31:0] cycles);
        reg [31:0] st, lo, hi;
        begin
            wb_write(5'h01, count);
            wb_write(5'h02, active);
            wb_write(5'h03, seed[31:0]);
            wb_write(5'h04, seed[63:32]);
            wb_write(5'h00, 32'h1);          // start
            // poll until done (CTRL bit1); bit0 is busy
            st = 0;
            while (st[1] !== 1'b1) wb_read(5'h00, st);
            wb_read(5'h05, cycles);
            wb_read(5'h06, lo);
            wb_read(5'h07, hi);
            result = {hi, lo};
        end
    endtask

    // ceil(count/active)
    function [31:0] ceil_div(input [31:0] a, input [31:0] b);
        ceil_div = (a + b - 1) / b;
    endfunction

    reg [63:0] res, base_res, ref_res;
    reg [31:0] cyc_hw, exp_cyc;
    integer ti;
    reg [31:0] counts [0:3];
    reg [31:0] actives [0:4];
    reg [63:0] seed;

    // poll uses STATUS bit1=done; fix mask
    initial begin
        adr=0; dat_w=0; we=0; cyc=0; stb=0;
        repeat (4) @(posedge clk);
        rst = 0;
        repeat (2) @(posedge clk);

        counts[0]=1; counts[1]=8; counts[2]=1000; counts[3]=4097;
        actives[0]=1; actives[1]=2; actives[2]=3; actives[3]=4; actives[4]=8;
        seed = 64'hDEADBEEF_0BADF00D;

        for (ti = 0; ti < 4; ti = ti + 1) begin : count_loop
            integer aj;
            base_res = 64'hX;
            ref_res  = ref_xor(seed, counts[ti]);
            for (aj = 0; aj < 5; aj = aj + 1) begin
                run_bench(counts[ti], actives[aj], seed, res, cyc_hw);
                exp_cyc = ceil_div(counts[ti], (actives[aj] > 8) ? 8 : actives[aj]);
                if (aj == 0) base_res = res;

                // invariance across bank counts
                if (res !== base_res) begin
                    $display("FAIL invariance: count=%0d active=%0d res=%h base=%h",
                             counts[ti], actives[aj], res, base_res);
                    errors = errors + 1;
                end
                // reference match
                if (res !== ref_res) begin
                    $display("FAIL ref: count=%0d active=%0d res=%h ref=%h",
                             counts[ti], actives[aj], res, ref_res);
                    errors = errors + 1;
                end
                // cycle scaling
                if (cyc_hw !== exp_cyc) begin
                    $display("FAIL cycles: count=%0d active=%0d hw=%0d exp=%0d",
                             counts[ti], actives[aj], cyc_hw, exp_cyc);
                    errors = errors + 1;
                end
                $display("ok count=%5d active=%0d cycles=%5d (exp %5d) result=%h",
                         counts[ti], actives[aj], cyc_hw, exp_cyc, res);
            end
        end

        $display("");
        if (errors == 0) $display("ALL PASS");
        else             $display("%0d FAILURES", errors);
        $finish;
    end

    // safety timeout
    initial begin
        #2000000;
        $display("TIMEOUT");
        $finish;
    end
endmodule
