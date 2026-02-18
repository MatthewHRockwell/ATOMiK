// =============================================================================
// ATOMiK v3 Register File Testbench (iverilog)
//
// Tests: x0 hardwire, read-after-write, all 32 registers, reset.
// =============================================================================

`timescale 1ns / 1ps

module tb_v3_regfile;

    reg         clk, rst_n;
    reg  [4:0]  rs1_addr, rs2_addr, rd_addr;
    reg  [63:0] rd_data;
    reg         rd_wen;
    wire [63:0] rs1_data, rs2_data;

    atomik_v3_regfile uut (
        .clk      (clk),
        .rst_n    (rst_n),
        .rs1_addr (rs1_addr),
        .rs1_data (rs1_data),
        .rs2_addr (rs2_addr),
        .rs2_data (rs2_data),
        .rd_addr  (rd_addr),
        .rd_data  (rd_data),
        .rd_wen   (rd_wen)
    );

    localparam CLK_PERIOD = 10.0;

    always #(CLK_PERIOD/2) clk = ~clk;

    integer test_count = 0;
    integer pass_count = 0;
    integer fail_count = 0;
    integer i;

    task check;
        input [63:0] got;
        input [63:0] expected;
        input [255:0] name;
        begin
            test_count = test_count + 1;
            if (got === expected) begin
                pass_count = pass_count + 1;
                $display("PASS [%0s]", name);
            end else begin
                fail_count = fail_count + 1;
                $display("FAIL [%0s]: got %016x, expected %016x", name, got, expected);
            end
        end
    endtask

    initial begin
        $display("==============================================");
        $display("ATOMiK v3 Register File Testbench");
        $display("==============================================");
        $display("");

        clk = 0;
        rst_n = 0;
        rs1_addr = 0;
        rs2_addr = 0;
        rd_addr = 0;
        rd_data = 0;
        rd_wen = 0;

        // Reset
        @(posedge clk);
        @(posedge clk);
        rst_n = 1;
        @(posedge clk);

        // --- x0 always reads zero ---
        rs1_addr = 5'd0;
        @(negedge clk);
        check(rs1_data, 64'h0, "x0 reads zero");

        // --- Write to x0 should be ignored ---
        rd_addr = 5'd0;
        rd_data = 64'hDEADBEEFCAFEBABE;
        rd_wen = 1;
        @(posedge clk);
        rd_wen = 0;
        @(negedge clk);
        rs1_addr = 5'd0;
        @(negedge clk);
        check(rs1_data, 64'h0, "x0 write ignored");

        // --- Write and read x1 ---
        @(negedge clk);
        rd_addr = 5'd1;
        rd_data = 64'h1111_2222_3333_4444;
        rd_wen = 1;
        @(posedge clk);
        #1;
        rd_wen = 0;
        rs1_addr = 5'd1;
        #1;
        check(rs1_data, 64'h1111_2222_3333_4444, "x1 write/read");

        // --- Dual-port read: x1 on port1, x2 on port2 ---
        @(negedge clk);
        rd_addr = 5'd2;
        rd_data = 64'hAAAA_BBBB_CCCC_DDDD;
        rd_wen = 1;
        @(posedge clk);
        #1;
        rd_wen = 0;
        rs1_addr = 5'd1;
        rs2_addr = 5'd2;
        #1;
        check(rs1_data, 64'h1111_2222_3333_4444, "dual read: x1");
        check(rs2_data, 64'hAAAA_BBBB_CCCC_DDDD, "dual read: x2");

        // --- Write all 31 registers, then read back ---
        for (i = 1; i < 32; i = i + 1) begin
            @(negedge clk);
            rd_addr = i[4:0];
            rd_data = {32'h0, i[31:0]} * 64'h0101_0101_0101_0101;
            rd_wen = 1;
            @(posedge clk);
            #1;
        end
        rd_wen = 0;

        for (i = 1; i < 32; i = i + 1) begin
            rs1_addr = i[4:0];
            #1;
            check(rs1_data, {32'h0, i[31:0]} * 64'h0101_0101_0101_0101,
                  "all regs readback");
        end

        // --- Reset clears all ---
        rst_n = 0;
        @(posedge clk);
        @(posedge clk);
        rst_n = 1;
        @(posedge clk);
        rs1_addr = 5'd1;
        @(negedge clk);
        check(rs1_data, 64'h0, "reset clears x1");
        rs1_addr = 5'd31;
        @(negedge clk);
        check(rs1_data, 64'h0, "reset clears x31");

        // =================================================================
        // Summary
        // =================================================================
        $display("");
        $display("==============================================");
        $display("Test Summary: %0d/%0d passed", pass_count, test_count);
        $display("==============================================");

        if (fail_count == 0)
            $display("*** ALL TESTS PASSED ***");
        else
            $display("*** %0d TESTS FAILED ***", fail_count);

        $finish;
    end

endmodule
