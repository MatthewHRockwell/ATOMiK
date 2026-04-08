/*
 * cfu_passthrough.v — Trivial CFU that returns the raw instruction word
 *
 * Used as a control: proves the VexRiscv Linux+CFU variant boots Linux
 * without any ATOMiK logic. Custom-0 instructions return zero.
 */
module Cfu (
    input  wire        clk,
    input  wire        reset,

    input  wire        cmd_valid,
    output wire        cmd_ready,
    input  wire [9:0]  cmd_payload_function_id,
    input  wire [31:0] cmd_payload_inputs_0,
    input  wire [31:0] cmd_payload_inputs_1,

    output wire        rsp_valid,
    input  wire        rsp_ready,
    output wire [31:0] rsp_payload_outputs_0
);
    /* Combinational pass-through: always ready, immediate response */
    assign cmd_ready = rsp_ready;
    assign rsp_valid = cmd_valid;

    /* Return zero for all custom-0 instructions */
    assign rsp_payload_outputs_0 = 32'h0;
endmodule
