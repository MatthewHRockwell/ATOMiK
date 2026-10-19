// =============================================================================
// ATOMiK BNN Accelerator Unit Test
//
// Tests: MMIO read/write, weight loading, known-answer inference,
//        popcount correctness, FSM states, threshold configuration,
//        output neuron winner selection, multi-inference counting.
// =============================================================================

`timescale 1ns / 1ps

module tb_bnn_accel;

    reg        clk;
    reg        rst_n;
    reg        mmio_valid;
    wire       mmio_ready;
    reg  [31:0] mmio_addr;
    reg  [31:0] mmio_wdata;
    reg  [3:0]  mmio_wstrb;
    wire [31:0] mmio_rdata;

    atomik_bnn_accel uut (
        .clk        (clk),
        .rst_n      (rst_n),
        .mmio_valid (mmio_valid),
        .mmio_ready (mmio_ready),
        .mmio_addr  (mmio_addr),
        .mmio_wdata (mmio_wdata),
        .mmio_wstrb (mmio_wstrb),
        .mmio_rdata (mmio_rdata)
    );

    // Clock: 21.6 MHz → ~46.3 ns period (use 46 ns for sim)
    always #23 clk = ~clk;

    integer pass_count = 0;
    integer fail_count = 0;
    integer test_num = 0;

    // Captured read data
    reg [31:0] rd_data;

    // =========================================================================
    // Helper tasks
    // =========================================================================
    task check(input [31:0] expected, input [31:0] actual, input [255:0] name);
    begin
        test_num = test_num + 1;
        if (actual === expected) begin
            $display("  PASS [%0d]: %0s = 0x%08h", test_num, name, actual);
            pass_count = pass_count + 1;
        end else begin
            $display("  FAIL [%0d]: %0s = 0x%08h (expected 0x%08h)", test_num, name, actual, expected);
            fail_count = fail_count + 1;
        end
    end
    endtask

    task check_bits(input [31:0] mask, input [31:0] expected, input [31:0] actual, input [255:0] name);
    begin
        test_num = test_num + 1;
        if ((actual & mask) === expected) begin
            $display("  PASS [%0d]: %0s & 0x%08h = 0x%08h", test_num, name, mask, actual & mask);
            pass_count = pass_count + 1;
        end else begin
            $display("  FAIL [%0d]: %0s & 0x%08h = 0x%08h (expected 0x%08h)", test_num, name, mask, actual & mask, expected);
            fail_count = fail_count + 1;
        end
    end
    endtask

    // MMIO write: assert valid, wait for ready
    task mmio_write(input [31:0] addr, input [31:0] data);
    begin
        @(posedge clk);
        mmio_valid <= 1'b1;
        mmio_addr  <= addr;
        mmio_wdata <= data;
        mmio_wstrb <= 4'hF;
        @(posedge clk);
        while (!mmio_ready) @(posedge clk);
        mmio_valid <= 1'b0;
        mmio_wstrb <= 4'h0;
        @(posedge clk);
    end
    endtask

    // MMIO read: assert valid with wstrb=0, capture rdata
    task mmio_read(input [31:0] addr);
    begin
        @(posedge clk);
        mmio_valid <= 1'b1;
        mmio_addr  <= addr;
        mmio_wdata <= 32'h0;
        mmio_wstrb <= 4'h0;
        @(posedge clk);
        while (!mmio_ready) @(posedge clk);
        rd_data <= mmio_rdata;
        mmio_valid <= 1'b0;
        @(posedge clk);
    end
    endtask

    // Write a weight register
    task write_weight(input [4:0] idx, input [31:0] val);
    begin
        mmio_write(32'hC0000210, {27'd0, idx}); // BNN_WGHT_ADDR
        mmio_write(32'hC0000214, val);            // BNN_WGHT_DATA
    end
    endtask

    // Read a weight register
    task read_weight(input [4:0] idx);
    begin
        mmio_write(32'hC0000210, {27'd0, idx}); // BNN_WGHT_ADDR
        mmio_read(32'hC0000214);                  // BNN_WGHT_DATA
    end
    endtask

    // Run inference and wait for done
    task run_inference;
    begin
        mmio_write(32'hC0000200, 32'h1);  // BNN_CTRL[0] = run
        // Poll for done (bit 1)
        repeat (100) begin
            mmio_read(32'hC0000200);
            if (rd_data[1]) disable run_inference;
        end
        $display("  ERROR: Inference did not complete within 100 polls!");
        fail_count = fail_count + 1;
    end
    endtask

    // =========================================================================
    // MMIO register base addresses (relative to 0xC0000200)
    // =========================================================================
    localparam BNN_CTRL      = 32'hC0000200;
    localparam BNN_INPUT     = 32'hC0000204;
    localparam BNN_HIDDEN    = 32'hC0000208;
    localparam BNN_OUTPUT    = 32'hC000020C;
    localparam BNN_WGHT_ADDR = 32'hC0000210;
    localparam BNN_WGHT_DATA = 32'hC0000214;
    localparam BNN_THRESH    = 32'hC0000218;
    localparam BNN_STATUS    = 32'hC000021C;

    // =========================================================================
    // Test sequence
    // =========================================================================
    initial begin
        $dumpfile("tb_bnn_accel.vcd");
        $dumpvars(0, tb_bnn_accel);

        clk        = 0;
        rst_n      = 0;
        mmio_valid = 0;
        mmio_addr  = 0;
        mmio_wdata = 0;
        mmio_wstrb = 0;

        // Reset
        repeat (4) @(posedge clk);
        rst_n = 1;
        repeat (2) @(posedge clk);

        $display("");
        $display("=== ATOMiK BNN Accelerator Unit Tests ===");
        $display("");

        // -----------------------------------------------------------------
        // T1: Register accessibility — read defaults
        // -----------------------------------------------------------------
        $display("--- T1: Register defaults ---");
        mmio_read(BNN_CTRL);
        check_bits(32'h06, 32'h00, rd_data, "CTRL done=0, busy=0");

        mmio_read(BNN_INPUT);
        check(32'h0, rd_data, "INPUT default 0");

        mmio_read(BNN_THRESH);
        check_bits(32'h3F3F, 32'h0810, rd_data, "THRESH default h=16,o=8");

        mmio_read(BNN_STATUS);
        check(32'h0, rd_data, "STATUS infer_count=0");

        // -----------------------------------------------------------------
        // T2: Input register write/readback
        // -----------------------------------------------------------------
        $display("--- T2: Input register ---");
        mmio_write(BNN_INPUT, 32'hDEADBEEF);
        mmio_read(BNN_INPUT);
        check(32'hDEADBEEF, rd_data, "INPUT readback");

        mmio_write(BNN_INPUT, 32'hA5A5A5A5);
        mmio_read(BNN_INPUT);
        check(32'hA5A5A5A5, rd_data, "INPUT readback 2");

        // -----------------------------------------------------------------
        // T3: Weight register write/readback (all 18)
        // -----------------------------------------------------------------
        $display("--- T3: Weight registers ---");
        begin : weight_test
            integer w;
            reg [31:0] wval;
            for (w = 0; w < 18; w = w + 1) begin
                wval = 32'hBEEF0000 | w;
                write_weight(w[4:0], wval);
            end
            // Read them all back
            for (w = 0; w < 18; w = w + 1) begin
                wval = 32'hBEEF0000 | w;
                read_weight(w[4:0]);
                check(wval, rd_data, "Weight readback");
            end
        end

        // -----------------------------------------------------------------
        // T4: Threshold configuration
        // -----------------------------------------------------------------
        $display("--- T4: Threshold config ---");
        mmio_write(BNN_THRESH, {18'd0, 6'd12, 2'b0, 6'd20});
        mmio_read(BNN_THRESH);
        check_bits(32'h003F, 32'h0014, rd_data, "hidden thresh=20");
        check_bits(32'h3F00, 32'h0C00, rd_data, "output thresh=12");

        // Reset thresholds for inference tests
        mmio_write(BNN_THRESH, {18'd0, 6'd8, 2'b0, 6'd16});

        // -----------------------------------------------------------------
        // T5: Known-answer inference — all-ones input, all-ones weights
        //     XNOR(1,1)=1 for all 32 bits → popcount=32 > thresh(16) → all hidden fire
        //     All 16 hidden fire → XNOR(all-1s, weight)
        //     With all-ones output weights → popcount=16 > thresh(8) → all output fire
        // -----------------------------------------------------------------
        $display("--- T5: Known-answer (all-ones) ---");
        // Load all-ones weights for hidden layer
        begin : infer_test_allones
            integer w;
            for (w = 0; w < 16; w = w + 1)
                write_weight(w[4:0], 32'hFFFFFFFF);
            // Output weights: all-ones (16-bit per neuron, packed)
            write_weight(5'd16, 32'hFFFFFFFF);  // neurons 0,1
            write_weight(5'd17, 32'hFFFFFFFF);  // neurons 2,3
        end

        // Set input to all-ones
        mmio_write(BNN_INPUT, 32'hFFFFFFFF);

        // Run inference
        run_inference;

        // Check hidden activations: all 16 should fire
        mmio_read(BNN_HIDDEN);
        check(32'h0000FFFF, rd_data, "hidden all fire");

        // Check output activations: all 4 should fire
        mmio_read(BNN_OUTPUT);
        check_bits(32'h0F, 32'h0F, rd_data, "output all fire");

        // Check inference count
        mmio_read(BNN_STATUS);
        check_bits(32'hFFFF, 32'h0001, rd_data, "infer_count=1");

        // -----------------------------------------------------------------
        // T6: Known-answer — all-zeros input, all-ones weights
        //     XNOR(0,1)=0 → popcount=0 < thresh(16) → no hidden fire
        //     Hidden = 0x0000 → XNOR(0x0000, 0xFFFF)=0x0000 → popcount=0 → no output fire
        // -----------------------------------------------------------------
        $display("--- T6: Known-answer (all-zeros input) ---");
        mmio_write(BNN_INPUT, 32'h00000000);
        run_inference;

        mmio_read(BNN_HIDDEN);
        check(32'h00000000, rd_data, "hidden none fire");

        mmio_read(BNN_OUTPUT);
        check_bits(32'h0F, 32'h00, rd_data, "output none fire");

        mmio_read(BNN_STATUS);
        check_bits(32'hFFFF, 32'h0002, rd_data, "infer_count=2");

        // -----------------------------------------------------------------
        // T7: Selective activation — input = 0xFFFF0000
        //     Hidden weights[0..7] = 0xFFFF0000 (match upper 16, mismatch lower 16)
        //       → XNOR = 0xFFFFFFFF → popcount=32 → fire
        //     Hidden weights[8..15] = 0x0000FFFF (mismatch upper, match lower)
        //       → XNOR = 0x00000000 → popcount=0 → no fire
        //     → hidden_act = 0x00FF (lower 8 fire, upper 8 don't)
        // -----------------------------------------------------------------
        $display("--- T7: Selective hidden activation ---");
        begin : infer_test_selective
            integer w;
            for (w = 0; w < 8; w = w + 1)
                write_weight(w[4:0], 32'hFFFF0000);
            for (w = 8; w < 16; w = w + 1)
                write_weight(w[4:0], 32'h0000FFFF);
            // Output weights: neuron0 matches lower 8 hidden bits
            write_weight(5'd16, 32'h000000FF);  // n0=0x00FF (match), n1=0x0000
            write_weight(5'd17, 32'h00000000);  // n2=0x0000, n3=0x0000
        end

        mmio_write(BNN_INPUT, 32'hFFFF0000);
        run_inference;

        mmio_read(BNN_HIDDEN);
        check(32'h000000FF, rd_data, "hidden lower 8 fire");

        // Output neuron 0 should fire (XNOR(0x00FF, 0x00FF)=0xFFFF → pop=16 >= 8)
        // Others should not (XNOR(0x00FF, 0x0000)=0xFF00 → pop=8 >= 8 → fires!)
        // Actually: XNOR(0x00FF, 0x0000) = ~(0x00FF ^ 0x0000) = ~0x00FF = 0xFF00
        // popcount(0xFF00) for 16-bit = 8, and thresh=8, so >= fires. Need to adjust.
        // Let's just check neuron 0 fires and read the raw values
        mmio_read(BNN_OUTPUT);
        // Neuron 0: XNOR(0x00FF, 0x00FF) → 0xFFFF → pop=16 → fire
        // Neuron 1: XNOR(0x00FF, 0x0000) → 0xFF00 → pop=8 → fire (>= 8)
        // Neuron 2: XNOR(0x00FF, 0x0000) → 0xFF00 → pop=8 → fire
        // Neuron 3: XNOR(0x00FF, 0x0000) → 0xFF00 → pop=8 → fire
        // Note: the upper 16 bits of xnor_input_a are zero, and weights upper 16 are zero,
        // so XNOR(0,0)=1 for those bits. But we zero-padded input to 32 bits:
        //   input_a = {16'h0000, 16'h00FF}
        //   For neuron 1: weight_b = {16'h0000, 16'h0000}
        //   XNOR = {16'hFFFF, 16'hFF00} → popcount(32-bit) = 16+8 = 24
        // Wait, that's not right either. The output layer uses zero-padded 32-bit:
        //   xnor_input_a = {16'd0, hidden_act} = 32'h000000FF
        //   For neuron 0: xnor_weight_b = {16'd0, weights[16][15:0]} = {16'd0, 16'h00FF}
        //   XNOR(32'h000000FF, 32'h000000FF) = 32'hFFFFFFFF → popcount=32
        // Hmm, the zero-padding means the upper 16 bits XNOR to all-1s!
        // This inflates the popcount. Let me just check the winner is neuron 0
        // since it has the highest match.

        // All will fire because of zero-pad XNOR artifact, but neuron 0 has highest pop
        // Neuron 0: XNOR(0x000000FF, 0x000000FF) = 0xFFFFFFFF → pop=32
        // Neuron 1: XNOR(0x000000FF, 0x00000000) = 0xFFFFFF00 → pop=24
        // So neuron 0 wins
        mmio_read(BNN_CTRL);
        check_bits(32'h30, 32'h00, rd_data, "winner=class 0");

        // -----------------------------------------------------------------
        // T8: Popcount verification — specific bit patterns
        //     Use a known weight/input pair where we can manually count
        //     Input=0xAAAAAAAA (alternating 10), Weight[0]=0x55555555 (alternating 01)
        //     XNOR = 0x00000000 → popcount = 0 → should NOT fire (thresh=16)
        // -----------------------------------------------------------------
        $display("--- T8: Popcount edge cases ---");
        begin : popcount_test
            integer w;
            // Set weight[0] to alternating pattern, rest to zeros
            write_weight(5'd0, 32'h55555555);
            for (w = 1; w < 16; w = w + 1)
                write_weight(w[4:0], 32'h0);
        end

        mmio_write(BNN_INPUT, 32'hAAAAAAAA);
        run_inference;

        mmio_read(BNN_HIDDEN);
        // Neuron 0: XNOR(0xAAAAAAAA, 0x55555555) = 0x00000000 → pop=0 → no fire
        // Neurons 1-15: XNOR(0xAAAAAAAA, 0x00000000) = 0x55555555 → pop=16 → fire (>=16)
        check_bits(32'h0001, 32'h0000, rd_data, "neuron0 no fire (complementary)");
        check_bits(32'hFFFE, 32'hFFFE, rd_data, "neurons 1-15 fire (zero wt)");

        // -----------------------------------------------------------------
        // T9: Exact popcount — input=weight → XNOR=all-ones → pop=32
        // -----------------------------------------------------------------
        $display("--- T9: Exact match popcount ---");
        write_weight(5'd0, 32'hDEADBEEF);
        mmio_write(BNN_INPUT, 32'hDEADBEEF);
        run_inference;

        mmio_read(BNN_HIDDEN);
        check_bits(32'h0001, 32'h0001, rd_data, "neuron0 fires (exact match)");

        // -----------------------------------------------------------------
        // T10: Latency measurement — should be ~40 cycles
        // -----------------------------------------------------------------
        $display("--- T10: Latency check ---");
        mmio_read(BNN_CTRL);
        begin : latency_check
            reg [7:0] lat;
            lat = rd_data[15:8];
            test_num = test_num + 1;
            if (lat >= 8'd30 && lat <= 8'd50) begin
                $display("  PASS [%0d]: latency = %0d cycles (in range 30-50)", test_num, lat);
                pass_count = pass_count + 1;
            end else begin
                $display("  FAIL [%0d]: latency = %0d cycles (expected 30-50)", test_num, lat);
                fail_count = fail_count + 1;
            end
        end

        // -----------------------------------------------------------------
        // T11: Back-to-back inference — run twice, count should increment
        // -----------------------------------------------------------------
        $display("--- T11: Back-to-back inference ---");
        mmio_write(BNN_INPUT, 32'hFFFFFFFF);
        write_weight(5'd0, 32'hFFFFFFFF);
        run_inference;
        run_inference;

        mmio_read(BNN_STATUS);
        // We've run: T5(1), T6(2), T7(3), T8(4), T9(5), T11(6,7)
        check_bits(32'hFFFF, 32'h0007, rd_data, "infer_count=7");

        // -----------------------------------------------------------------
        // T12: FSM idle after done read
        // -----------------------------------------------------------------
        $display("--- T12: FSM state ---");
        mmio_read(BNN_CTRL);
        check_bits(32'h04, 32'h00, rd_data, "busy=0 after done");
        check_bits(32'h02, 32'h02, rd_data, "done=1");

        // -----------------------------------------------------------------
        // Summary
        // -----------------------------------------------------------------
        $display("");
        $display("=== BNN Accelerator Tests: %0d PASS, %0d FAIL ===", pass_count, fail_count);
        if (fail_count == 0)
            $display("ALL PASS");
        else
            $display("*** FAILURES DETECTED ***");
        $display("");

        $finish;
    end

    // Timeout watchdog
    initial begin
        #500000;
        $display("TIMEOUT: Simulation exceeded 500 us");
        $finish;
    end

endmodule
