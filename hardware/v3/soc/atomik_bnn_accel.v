// =============================================================================
// ATOMiK BNN (Binary Neural Network) Inference Accelerator
//
// Time-multiplexed XNOR-popcount engine for binary neural network inference.
// Processes one neuron per 2 clock cycles using a shared compute unit.
//
// Network topology: 32 binary inputs → 16 hidden neurons → 4 output classes
//   - Hidden layer: 16 neurons × 32-bit weights = 512 binary weights
//   - Output layer:  4 neurons × 16-bit weights =  64 binary weights
//   - Total weights: 576 bits = 18 × 32-bit registers
//
// Inference latency: ~40 clock cycles (~1.85 μs at 21.6 MHz)
//   → ~540K inferences/second
//
// BNN compute: activation = popcount(XNOR(input, weight)) > threshold
//   - XNOR: matching bits contribute +1, mismatches contribute 0
//   - Popcount: count matching bits (6-bit result for 32-bit input)
//   - Threshold: if matches > N/2, neuron fires (binary activation)
//
// MMIO at 0xC000_0200 (S3 sub-slot, addr[9:8]=10):
//   +0x00  BNN_CTRL      [0] run (W: start, auto-clears), [1] done (RO),
//                         [2] busy (RO), [7:4] output_class (RO),
//                         [15:8] last_latency (RO, cycles)
//   +0x04  BNN_INPUT     [31:0] binary input vector (RW)
//   +0x08  BNN_HIDDEN    [15:0] hidden activations (RO)
//   +0x0C  BNN_OUTPUT    [3:0] output activations (RO),
//                         [9:4] popcount[0], [15:10] popcount[1],
//                         [21:16] popcount[2], [27:22] popcount[3]
//   +0x10  BNN_WGHT_ADDR [4:0] weight register index 0-17 (RW)
//   +0x14  BNN_WGHT_DATA [31:0] weight at current index (RW)
//   +0x18  BNN_THRESH    [5:0] hidden threshold (default 16),
//                         [13:8] output threshold (default 8)
//   +0x1C  BNN_STATUS    [15:0] total inference count (RO)
// =============================================================================

`timescale 1ns/1ps

module atomik_bnn_accel (
    input  wire        clk,
    input  wire        rst_n,

    // MMIO bus (CPU domain, no CDC needed)
    input  wire        mmio_valid,
    output reg         mmio_ready,
    input  wire [31:0] mmio_addr,
    input  wire [31:0] mmio_wdata,
    input  wire [3:0]  mmio_wstrb,
    output reg  [31:0] mmio_rdata
);

    // =========================================================================
    // Weight storage: 18 × 32-bit registers
    //   [0..15]  = hidden layer weights (neuron i has 32-bit weight word i)
    //   [16..17] = output layer weights (packed: [16]={neuron1[15:0], neuron0[15:0]},
    //                                            [17]={neuron3[15:0], neuron2[15:0]})
    // =========================================================================
    reg [31:0] weights [0:17];

    // =========================================================================
    // Configuration & state registers
    // =========================================================================
    reg [31:0] input_reg;         // 32-bit binary input vector
    reg [4:0]  wght_addr_reg;     // Weight register index for MMIO access
    reg [5:0]  thresh_hidden;     // Hidden layer threshold (default 16 = 32/2)
    reg [5:0]  thresh_output;     // Output layer threshold (default 8 = 16/2)

    // Inference results
    reg [15:0] hidden_act;        // Hidden layer activations (1 bit per neuron)
    reg [3:0]  output_act;        // Output layer activations (1 bit per neuron)
    reg [5:0]  output_pop [0:3];  // Raw popcount for each output neuron
    reg [7:0]  last_latency;      // Cycles for last inference
    reg [15:0] infer_count;       // Total inferences completed

    // =========================================================================
    // FSM
    // =========================================================================
    localparam FSM_IDLE        = 3'd0;
    localparam FSM_HIDDEN_XNOR = 3'd1;
    localparam FSM_HIDDEN_POP  = 3'd2;
    localparam FSM_OUTPUT_XNOR = 3'd3;
    localparam FSM_OUTPUT_POP  = 3'd4;
    localparam FSM_DONE        = 3'd5;

    reg [2:0]  fsm_state;
    reg [4:0]  neuron_idx;        // Current neuron being computed
    reg [7:0]  cycle_count;       // Latency counter

    wire fsm_busy = (fsm_state != FSM_IDLE) && (fsm_state != FSM_DONE);
    wire fsm_done = (fsm_state == FSM_DONE);

    // =========================================================================
    // XNOR-Popcount datapath (shared compute unit)
    //
    // Computes: popcount(input_a XNOR weight_b) in one combinational stage.
    // Result is registered on the next clock edge (FSM_*_POP states).
    // =========================================================================
    reg  [31:0] xnor_input_a;     // Input vector (or hidden activations, zero-padded)
    reg  [31:0] xnor_weight_b;    // Weight vector for current neuron
    reg         output_layer;     // 1 when computing output layer (mask upper 16 bits)

    // Mask upper 16 bits during output layer to prevent zero-pad inflation.
    // Without this, XNOR(0,0)=1 in the unused upper bits adds 16 to popcount.
    wire [31:0] xnor_raw    = xnor_input_a ~^ xnor_weight_b;
    wire [31:0] xnor_result = output_layer ? {16'd0, xnor_raw[15:0]} : xnor_raw;

    // 32-bit popcount using hierarchical adder tree
    // Stage 1: count pairs (16 × 2-bit sums)
    wire [1:0] pop_s1 [0:15];
    genvar gi;
    generate
        for (gi = 0; gi < 16; gi = gi + 1) begin : pop_pairs
            assign pop_s1[gi] = {1'b0, xnor_result[2*gi]} + {1'b0, xnor_result[2*gi+1]};
        end
    endgenerate

    // Stage 2: sum pairs of pairs (8 × 3-bit sums)
    wire [2:0] pop_s2 [0:7];
    generate
        for (gi = 0; gi < 8; gi = gi + 1) begin : pop_quads
            assign pop_s2[gi] = {1'b0, pop_s1[2*gi]} + {1'b0, pop_s1[2*gi+1]};
        end
    endgenerate

    // Stage 3: sum quads (4 × 4-bit sums)
    wire [3:0] pop_s3 [0:3];
    generate
        for (gi = 0; gi < 4; gi = gi + 1) begin : pop_octs
            assign pop_s3[gi] = {1'b0, pop_s2[2*gi]} + {1'b0, pop_s2[2*gi+1]};
        end
    endgenerate

    // Stage 4: sum octets (2 × 5-bit sums)
    wire [4:0] pop_s4_lo = {1'b0, pop_s3[0]} + {1'b0, pop_s3[1]};
    wire [4:0] pop_s4_hi = {1'b0, pop_s3[2]} + {1'b0, pop_s3[3]};

    // Stage 5: final 6-bit popcount
    wire [5:0] popcount = {1'b0, pop_s4_lo} + {1'b0, pop_s4_hi};

    // Registered popcount result (captured in FSM_*_POP states)
    reg [5:0] popcount_reg;

    // Run trigger: set by MMIO write, consumed by FSM
    reg run_trigger;

    // =========================================================================
    // FSM Logic
    // =========================================================================
    always @(posedge clk) begin
        if (!rst_n) begin
            fsm_state    <= FSM_IDLE;
            neuron_idx   <= 5'd0;
            cycle_count  <= 8'd0;
            hidden_act   <= 16'd0;
            output_act   <= 4'd0;
            output_layer <= 1'b0;
            popcount_reg <= 6'd0;
            output_pop[0] <= 6'd0;
            output_pop[1] <= 6'd0;
            output_pop[2] <= 6'd0;
            output_pop[3] <= 6'd0;
            last_latency <= 8'd0;
            infer_count  <= 16'd0;
        end else begin
            case (fsm_state)

                FSM_IDLE: begin
                    if (run_trigger) begin
                        fsm_state   <= FSM_HIDDEN_XNOR;
                        neuron_idx  <= 5'd0;
                        cycle_count <= 8'd0;
                        hidden_act  <= 16'd0;
                        output_act  <= 4'd0;
                    end
                end

                FSM_HIDDEN_XNOR: begin
                    // Set up XNOR inputs for hidden neuron[neuron_idx]
                    xnor_input_a  <= input_reg;
                    xnor_weight_b <= weights[neuron_idx[3:0]]; // [0..15]
                    output_layer  <= 1'b0;
                    fsm_state     <= FSM_HIDDEN_POP;
                    cycle_count   <= cycle_count + 8'd1;
                end

                FSM_HIDDEN_POP: begin
                    // Capture registered popcount, apply threshold, store activation
                    popcount_reg <= popcount;
                    hidden_act[neuron_idx[3:0]] <= (popcount >= thresh_hidden);
                    cycle_count <= cycle_count + 8'd1;

                    if (neuron_idx == 5'd15) begin
                        // Hidden layer complete, start output layer
                        neuron_idx <= 5'd0;
                        fsm_state  <= FSM_OUTPUT_XNOR;
                    end else begin
                        neuron_idx <= neuron_idx + 5'd1;
                        fsm_state  <= FSM_HIDDEN_XNOR;
                    end
                end

                FSM_OUTPUT_XNOR: begin
                    // Set up XNOR inputs for output neuron[neuron_idx]
                    // Output neurons use 16-bit weights from hidden activations
                    output_layer  <= 1'b1;  // Mask upper 16 bits in XNOR result
                    xnor_input_a  <= {16'd0, hidden_act};  // Zero-pad to 32 bits
                    // Unpack 16-bit weights from 32-bit registers [16] and [17]
                    case (neuron_idx[1:0])
                        2'd0: xnor_weight_b <= {16'd0, weights[16][15:0]};
                        2'd1: xnor_weight_b <= {16'd0, weights[16][31:16]};
                        2'd2: xnor_weight_b <= {16'd0, weights[17][15:0]};
                        2'd3: xnor_weight_b <= {16'd0, weights[17][31:16]};
                    endcase
                    fsm_state   <= FSM_OUTPUT_POP;
                    cycle_count <= cycle_count + 8'd1;
                end

                FSM_OUTPUT_POP: begin
                    // Capture popcount for output neuron
                    // Note: upper 16 bits are zero-padded, so popcount only counts
                    // matches in the lower 16 bits (max value = 16)
                    popcount_reg <= popcount;
                    output_act[neuron_idx[1:0]] <= (popcount >= thresh_output);
                    output_pop[neuron_idx[1:0]] <= popcount;
                    cycle_count <= cycle_count + 8'd1;

                    if (neuron_idx == 5'd3) begin
                        // Inference complete
                        fsm_state    <= FSM_DONE;
                        last_latency <= cycle_count + 8'd1;
                        infer_count  <= infer_count + 16'd1;
                    end else begin
                        neuron_idx <= neuron_idx + 5'd1;
                        fsm_state  <= FSM_OUTPUT_XNOR;
                    end
                end

                FSM_DONE: begin
                    if (run_trigger) begin
                        fsm_state   <= FSM_HIDDEN_XNOR;
                        neuron_idx  <= 5'd0;
                        cycle_count <= 8'd0;
                        hidden_act  <= 16'd0;
                        output_act  <= 4'd0;
                    end
                end

                default: fsm_state <= FSM_IDLE;

            endcase
        end
    end

    // =========================================================================
    // Winning class: highest popcount among output neurons
    // Simple priority comparison (combinational, off critical path)
    // =========================================================================
    wire [5:0] pop0 = output_pop[0];
    wire [5:0] pop1 = output_pop[1];
    wire [5:0] pop2 = output_pop[2];
    wire [5:0] pop3 = output_pop[3];

    wire [5:0] max01 = (pop1 > pop0) ? pop1 : pop0;
    wire       sel01 = (pop1 > pop0);
    wire [5:0] max23 = (pop3 > pop2) ? pop3 : pop2;
    wire       sel23 = (pop3 > pop2);
    wire [5:0] max_all = (max23 > max01) ? max23 : max01;
    wire       sel_hi  = (max23 > max01);

    wire [1:0] winner = sel_hi ? (sel23 ? 2'd3 : 2'd2) : (sel01 ? 2'd1 : 2'd0);

    // =========================================================================
    // MMIO Register Interface
    // =========================================================================
    wire mmio_is_write = mmio_valid & |mmio_wstrb;
    wire [2:0] mmio_reg_sel = mmio_addr[4:2];  // 8 registers, 4-byte aligned

    always @(posedge clk) begin
        if (!rst_n) begin
            mmio_ready     <= 1'b0;
            mmio_rdata     <= 32'h0;
            input_reg      <= 32'h0;
            wght_addr_reg  <= 5'd0;
            thresh_hidden  <= 6'd16;  // Default: 32/2
            thresh_output  <= 6'd8;   // Default: 16/2
            run_trigger    <= 1'b0;
        end else begin
            mmio_ready  <= 1'b0;
            run_trigger <= 1'b0;  // Auto-clear (pulse)

            if (mmio_valid && !mmio_ready) begin
                mmio_ready <= 1'b1;

                case (mmio_reg_sel)
                    3'b000: begin  // +0x00 BNN_CTRL
                        if (mmio_is_write && mmio_wdata[0]) begin
                            // Set run trigger pulse — FSM block consumes it
                            if (fsm_state == FSM_IDLE || fsm_state == FSM_DONE)
                                run_trigger <= 1'b1;
                        end
                        mmio_rdata <= {16'd0, last_latency, winner, 2'b0,
                                       output_act[winner], fsm_busy, fsm_done, 1'b0};
                    end

                    3'b001: begin  // +0x04 BNN_INPUT
                        if (mmio_is_write)
                            input_reg <= mmio_wdata;
                        mmio_rdata <= input_reg;
                    end

                    3'b010: begin  // +0x08 BNN_HIDDEN
                        mmio_rdata <= {16'd0, hidden_act};
                    end

                    3'b011: begin  // +0x0C BNN_OUTPUT
                        mmio_rdata <= {output_pop[3][3:0], output_pop[2],
                                       output_pop[1], output_pop[0],
                                       output_act};
                    end

                    3'b100: begin  // +0x10 BNN_WGHT_ADDR
                        if (mmio_is_write)
                            wght_addr_reg <= mmio_wdata[4:0];
                        mmio_rdata <= {27'd0, wght_addr_reg};
                    end

                    3'b101: begin  // +0x14 BNN_WGHT_DATA
                        if (mmio_is_write)
                            weights[wght_addr_reg] <= mmio_wdata;
                        mmio_rdata <= weights[wght_addr_reg];
                    end

                    3'b110: begin  // +0x18 BNN_THRESH
                        if (mmio_is_write) begin
                            thresh_hidden <= mmio_wdata[5:0];
                            thresh_output <= mmio_wdata[13:8];
                        end
                        mmio_rdata <= {18'd0, thresh_output, 2'b0, thresh_hidden};
                    end

                    3'b111: begin  // +0x1C BNN_STATUS
                        mmio_rdata <= {16'd0, infer_count};
                    end
                endcase
            end
        end
    end

    // =========================================================================
    // Initialize weights to zero
    // =========================================================================
    integer i;
    initial begin
        for (i = 0; i < 18; i = i + 1)
            weights[i] = 32'h0;
    end

endmodule
