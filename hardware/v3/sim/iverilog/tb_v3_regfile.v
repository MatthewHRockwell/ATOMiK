// =============================================================================
// ATOMiK v3 Register File Testbench (iverilog)
//
// Tests: x0 hardwire, read-after-write, all 32 registers, reset.
//
// BSRAM timing: reads are registered (1-cycle latency).
//   - Write: committed on posedge when rd_wen=1
//   - Read: address must be stable before posedge; data available after posedge
//   - After write, need 1 extra cycle before read sees new data
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

    // Helper: write a register (sets up at negedge, commits on posedge)
    task write_reg;
        input [4:0]  addr;
        input [63:0] data;
        begin
            @(negedge clk);
            rd_addr = addr;
            rd_data = data;
            rd_wen  = 1;
            @(posedge clk);  // Write committed here
            #1;
            rd_wen = 0;
        end
    endtask

    // Helper: read rs1 port (present addr at negedge, latch on posedge)
    task read_rs1;
        input [4:0] addr;
        begin
            @(negedge clk);
            rs1_addr = addr;
            @(posedge clk);  // Read registered here
            #1;              // Small delay for output to settle
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
        read_rs1(5'd0);
        check(rs1_data, 64'h0, "x0 reads zero");

        // --- Write to x0 should be ignored ---
        write_reg(5'd0, 64'hDEADBEEFCAFEBABE);
        read_rs1(5'd0);
        check(rs1_data, 64'h0, "x0 write ignored");

        // --- Write and read x1 ---
        write_reg(5'd1, 64'h1111_2222_3333_4444);
        read_rs1(5'd1);
        check(rs1_data, 64'h1111_2222_3333_4444, "x1 write/read");

        // --- Dual-port read: x1 on port1, x2 on port2 ---
        write_reg(5'd2, 64'hAAAA_BBBB_CCCC_DDDD);
        // Present both addresses at negedge, latch on posedge
        @(negedge clk);
        rs1_addr = 5'd1;
        rs2_addr = 5'd2;
        @(posedge clk);
        #1;
        check(rs1_data, 64'h1111_2222_3333_4444, "dual read: x1");
        check(rs2_data, 64'hAAAA_BBBB_CCCC_DDDD, "dual read: x2");

        // --- Write all 31 registers, then read back ---
        for (i = 1; i < 32; i = i + 1) begin
            write_reg(i[4:0], {32'h0, i[31:0]} * 64'h0101_0101_0101_0101);
        end

        for (i = 1; i < 32; i = i + 1) begin
            read_rs1(i[4:0]);
            check(rs1_data, {32'h0, i[31:0]} * 64'h0101_0101_0101_0101,
                  "all regs readback");
        end

        // --- Reset: x0 still hardwired to zero ---
        // Note: BSRAM does not support reset. x1-x31 retain values
        // through reset (rst_n is unused). Only x0 is guaranteed zero
        // via the output mux hardwire.
        rst_n = 0;
        @(posedge clk);
        @(posedge clk);
        rst_n = 1;
        @(posedge clk);
        read_rs1(5'd0);
        check(rs1_data, 64'h0, "x0 zero after reset");
        // x1 retains old value (BSRAM has no reset)
        read_rs1(5'd1);
        check(rs1_data, 64'h0101_0101_0101_0101, "x1 retained after reset");

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
