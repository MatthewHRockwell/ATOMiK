// =============================================================================
// ATOMiK Wishbone Wrapper Testbench
//
// Validates the Wishbone slave interface + ATOMiK core integration
// for Caravel/chipIgnite tape-out.
// =============================================================================

`timescale 1ns / 1ps

`define MPRJ_IO_PADS 38

module tb_wb_wrapper;

    reg         clk;
    reg         rst;
    reg         stb;
    reg         cyc;
    reg         we;
    reg  [3:0]  sel;
    reg  [31:0] dat_i;
    reg  [31:0] adr;
    wire        ack;
    wire [31:0] dat_o;

    // Unused Caravel signals
    wire [127:0] la_data_out;
    wire [`MPRJ_IO_PADS-1:0] io_out;
    wire [`MPRJ_IO_PADS-1:0] io_oeb;
    wire [2:0] user_irq;

    // Instantiate the full user_project_wrapper
    user_project_wrapper #(.BITS(32)) uut (
        .wb_clk_i   (clk),
        .wb_rst_i   (rst),
        .wbs_stb_i  (stb),
        .wbs_cyc_i  (cyc),
        .wbs_we_i   (we),
        .wbs_sel_i  (sel),
        .wbs_dat_i  (dat_i),
        .wbs_adr_i  (adr),
        .wbs_ack_o  (ack),
        .wbs_dat_o  (dat_o),

        .la_data_in  (128'b0),
        .la_data_out (la_data_out),
        .la_oenb     (128'hFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF),  // User drives all LA

        .io_in       ({`MPRJ_IO_PADS{1'b0}}),
        .io_out      (io_out),
        .io_oeb      (io_oeb),

        .user_clock2 (1'b0),
        .user_irq    (user_irq)
    );

    // Clock: 10 MHz (100ns period, typical Caravel)
    initial clk = 0;
    always #50 clk = ~clk;

    // Test infrastructure
    integer pass_count = 0;
    integer fail_count = 0;
    integer test_num   = 0;

    task wb_write(input [31:0] addr_val, input [31:0] data_val);
        begin
            @(posedge clk); #1;
            adr   = addr_val;
            dat_i = data_val;
            stb   = 1;
            cyc   = 1;
            we    = 1;
            sel   = 4'hF;
            @(posedge clk); #1;
            while (!ack) begin @(posedge clk); #1; end
            stb = 0;
            cyc = 0;
            we  = 0;
            @(posedge clk); #1;
        end
    endtask

    task wb_read(input [31:0] addr_val, output [31:0] data_val);
        begin
            @(posedge clk); #1;
            adr   = addr_val;
            stb   = 1;
            cyc   = 1;
            we    = 0;
            sel   = 4'hF;
            @(posedge clk); #1;
            while (!ack) begin @(posedge clk); #1; end
            data_val = dat_o;
            stb = 0;
            cyc = 0;
            @(posedge clk); #1;
        end
    endtask

    task check(input [31:0] expected, input [31:0] actual, input [255:0] name);
        begin
            test_num = test_num + 1;
            if (expected === actual) begin
                pass_count = pass_count + 1;
                $display("  [PASS] Test %0d: %0s (0x%08x)", test_num, name, actual);
            end else begin
                fail_count = fail_count + 1;
                $display("  [FAIL] Test %0d: %0s expected=0x%08x got=0x%08x",
                         test_num, name, expected, actual);
            end
        end
    endtask

    reg [31:0] rd;

    // Base address for user project in Caravel
    localparam BASE = 32'h30000000;

    initial begin
        // Initialize
        rst = 1; stb = 0; cyc = 0; we = 0; sel = 0; dat_i = 0; adr = 0;
        repeat (10) @(posedge clk);
        rst = 0;
        repeat (5) @(posedge clk);

        $display("");
        $display("============================================");
        $display(" ATOMiK Wishbone Wrapper Tests (Caravel)");
        $display("============================================");
        $display("");

        // ----- Test 1: Read STATUS register -----
        $display("--- STATUS Register ---");
        wb_read(BASE + 8'h1C, rd);
        check(32'h10000001, rd, "STATUS: version=0x10, acc_zero=1");

        // ----- Test 2: Read CONFIG register -----
        $display("--- CONFIG Register ---");
        wb_read(BASE + 8'h24, rd);
        check(32'h00000001, rd, "CONFIG: core_enable=1 (default)");

        // ----- Test 3: LOAD operation (64-bit via LO/HI) -----
        $display("--- LOAD Operation ---");
        wb_write(BASE + 8'h00, 32'h00000042);       // LOAD_ADDR = 0x42
        wb_write(BASE + 8'h04, 32'hDEADBEEF);       // LOAD_DATA_LO
        wb_write(BASE + 8'h08, 32'h12345678);       // LOAD_DATA_HI → triggers LOAD
        repeat (3) @(posedge clk);                   // Let core settle

        // Read back state
        wb_read(BASE + 8'h14, rd);                   // STATE_LO (latches snapshot)
        check(32'hDEADBEEF, rd, "STATE_LO after LOAD");
        wb_read(BASE + 8'h18, rd);                   // STATE_HI
        check(32'h12345678, rd, "STATE_HI after LOAD");

        // ----- Test 4: ACCUM operation -----
        $display("--- ACCUM Operation ---");
        wb_write(BASE + 8'h0C, 32'h000000FF);       // ACCUM_LO
        wb_write(BASE + 8'h10, 32'h00000000);       // ACCUM_HI → triggers ACCUM
        repeat (3) @(posedge clk);

        // State should now be init_data XOR delta
        // 0x12345678_DEADBEEF XOR 0x00000000_000000FF = 0x12345678_DEADBE10
        wb_read(BASE + 8'h14, rd);
        check(32'hDEADBE10, rd, "STATE_LO after ACCUM (BEEF^FF=BE10)");
        wb_read(BASE + 8'h18, rd);
        check(32'h12345678, rd, "STATE_HI after ACCUM (unchanged)");

        // acc_zero should be 0 now (accumulator has 0xFF)
        wb_read(BASE + 8'h1C, rd);
        check(32'h10000000, rd, "STATUS: acc_zero=0 after ACCUM");

        // ----- Test 5: Self-inverse (ACCUM same delta → cancels) -----
        $display("--- Self-Inverse ---");
        wb_write(BASE + 8'h0C, 32'h000000FF);
        wb_write(BASE + 8'h10, 32'h00000000);       // ACCUM same delta again
        repeat (3) @(posedge clk);

        wb_read(BASE + 8'h14, rd);
        check(32'hDEADBEEF, rd, "STATE_LO after self-inverse (restored)");
        wb_read(BASE + 8'h1C, rd);
        check(32'h10000001, rd, "STATUS: acc_zero=1 (self-inverse verified)");

        // ----- Test 6: SWAP operation -----
        $display("--- SWAP Operation ---");
        // First accumulate something
        wb_write(BASE + 8'h0C, 32'hCAFECAFE);
        wb_write(BASE + 8'h10, 32'h00000000);
        repeat (3) @(posedge clk);

        // SWAP at address 0x42
        wb_write(BASE + 8'h20, 32'h00000042);
        repeat (5) @(posedge clk);                   // Extra cycles for swap_pending

        // After SWAP: state table updated, accumulator cleared
        wb_read(BASE + 8'h1C, rd);
        check(32'h10000001, rd, "STATUS: acc_zero=1 after SWAP");

        // Read state — should be the swapped value
        wb_read(BASE + 8'h14, rd);
        // init was 0x12345678_DEADBEEF, acc was 0x00000000_CAFECAFE
        // new ref = 0x12345678_DEADBEEF ^ 0x00000000_CAFECAFE = 0x12345678_14537411
        // After swap, acc=0, so state = new_ref ^ 0 = new_ref
        check(32'h14537411, rd, "STATE_LO after SWAP");

        // ----- Test 7: LA output shows current state -----
        $display("--- Logic Analyzer ---");
        // la_data_out[63:0] should be current_state
        if (la_data_out[64] === 1'b1) begin
            test_num = test_num + 1;
            pass_count = pass_count + 1;
            $display("  [PASS] Test %0d: LA acc_zero=1", test_num);
        end else begin
            test_num = test_num + 1;
            fail_count = fail_count + 1;
            $display("  [FAIL] Test %0d: LA acc_zero expected 1, got %b", test_num, la_data_out[64]);
        end

        // ----- Test 8: GPIO output -----
        $display("--- GPIO ---");
        // io_out[5] = acc_zero
        if (io_out[5] === 1'b1) begin
            test_num = test_num + 1;
            pass_count = pass_count + 1;
            $display("  [PASS] Test %0d: GPIO[5] acc_zero=1", test_num);
        end else begin
            test_num = test_num + 1;
            fail_count = fail_count + 1;
            $display("  [FAIL] Test %0d: GPIO[5] expected 1, got %b", test_num, io_out[5]);
        end

        // ----- Test 9: Multiple addresses -----
        $display("--- Multi-Address ---");
        // LOAD at address 0x00
        wb_write(BASE + 8'h00, 32'h00000000);
        wb_write(BASE + 8'h04, 32'hAAAAAAAA);
        wb_write(BASE + 8'h08, 32'h55555555);
        repeat (3) @(posedge clk);

        wb_read(BASE + 8'h14, rd);
        check(32'hAAAAAAAA, rd, "Addr 0x00: STATE_LO correct");

        // LOAD at address 0x01
        wb_write(BASE + 8'h00, 32'h00000001);
        wb_write(BASE + 8'h04, 32'hBBBBBBBB);
        wb_write(BASE + 8'h08, 32'h66666666);
        repeat (3) @(posedge clk);

        wb_read(BASE + 8'h14, rd);
        check(32'hBBBBBBBB, rd, "Addr 0x01: STATE_LO correct");

        // ----- Summary -----
        $display("");
        $display("============================================");
        $display(" Results: %0d/%0d passed", pass_count, pass_count + fail_count);
        $display("============================================");

        if (fail_count == 0)
            $display(" *** ALL TESTS PASSED ***");
        else
            $display(" *** %0d TESTS FAILED ***", fail_count);

        $display("");
        $finish;
    end

endmodule
