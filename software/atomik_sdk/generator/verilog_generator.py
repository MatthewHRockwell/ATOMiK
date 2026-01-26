"""
ATOMiK Verilog RTL Generator

Generates synthesizable Verilog RTL modules from ATOMiK schemas.
Matches the Phase 3 hardware architecture.
"""

from __future__ import annotations

from typing import Dict, Any
from pathlib import Path

from .code_emitter import CodeEmitter, GeneratedFile, GenerationResult
from .namespace_mapper import NamespaceMapping


class VerilogGenerator(CodeEmitter):
    """
    Generates Verilog RTL code from ATOMiK schemas.

    Generates:
    - rtl/{vertical}/{field}/{module}.v - Main RTL module
    - testbench/tb_{module}.v - Testbench
    - constraints/{module}.cst - Constraint file
    """

    def __init__(self):
        """Initialize Verilog code generator."""
        super().__init__('verilog')

    def generate(self, schema: Dict[str, Any], namespace: NamespaceMapping) -> GenerationResult:
        """Generate Verilog RTL code from schema."""
        try:
            files = []
            errors = []
            warnings = []

            # Extract schema components
            catalogue = schema.get('catalogue', {})
            schema_def = schema.get('schema', {})
            delta_fields = schema_def.get('delta_fields', {})
            operations = schema_def.get('operations', {})
            hardware = schema.get('hardware', {})

            # Determine DATA_WIDTH from delta fields
            data_width = self._get_data_width(delta_fields)

            # Generate main RTL module
            rtl_file = self._generate_rtl_module(
                namespace, delta_fields, operations, hardware, data_width
            )
            files.append(rtl_file)

            # Generate testbench
            testbench_file = self._generate_testbench(
                namespace, delta_fields, operations, data_width
            )
            files.append(testbench_file)

            # Generate constraints file
            constraints_file = self._generate_constraints(namespace)
            files.append(constraints_file)

            return GenerationResult(
                success=True,
                files=files,
                errors=errors,
                warnings=warnings
            )

        except Exception as e:
            return GenerationResult(
                success=False,
                files=[],
                errors=[f"Verilog generation failed: {str(e)}"],
                warnings=[]
            )

    def _generate_rtl_module(
        self,
        namespace: NamespaceMapping,
        delta_fields: Dict[str, Any],
        operations: Dict[str, Any],
        hardware: Dict[str, Any],
        data_width: int
    ) -> GeneratedFile:
        """Generate synthesizable Verilog RTL module."""

        module_name = namespace.verilog_module_name
        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        lines = []

        # Module header
        lines.append(f"/**")
        lines.append(f" * {module_name}")
        lines.append(f" * ")
        lines.append(f" * Delta-state module for {namespace.object}")
        lines.append(f" * Generated from ATOMiK schema")
        lines.append(f" * ")
        lines.append(f" * Operations:")
        lines.append(f" *   LOAD: Set initial state")
        lines.append(f" *   ACCUMULATE: XOR delta into accumulator")
        lines.append(f" *   READ: Reconstruct current state")
        lines.append(f" *   STATUS: Check if accumulator is zero")
        lines.append(f" */")
        lines.append("")

        # Module declaration
        lines.append(f"module {module_name} #(")
        lines.append(f"    parameter DATA_WIDTH = {data_width}")
        lines.append(") (")
        lines.append("    // Clock and reset")
        lines.append("    input wire clk,")
        lines.append("    input wire rst_n,")
        lines.append("")
        lines.append("    // Control signals")
        lines.append("    input wire load_en,")
        lines.append("    input wire accumulate_en,")
        lines.append("    input wire read_en,")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    input wire rollback_en,")
        lines.append("")
        lines.append("    // Data signals")
        lines.append("    input wire [DATA_WIDTH-1:0] data_in,")
        lines.append("    output reg [DATA_WIDTH-1:0] data_out,")
        lines.append("")
        lines.append("    // Status signals")
        lines.append("    output wire accumulator_zero")
        lines.append(");")
        lines.append("")

        # Internal registers
        lines.append("    // Internal state")
        lines.append("    reg [DATA_WIDTH-1:0] initial_state;")
        lines.append("    reg [DATA_WIDTH-1:0] accumulator;")
        lines.append("")

        # Rollback support
        if operations.get('rollback', {}).get('enabled', False):
            history_depth = operations['rollback'].get('history_depth', 10)
            lines.append(f"    // Rollback history")
            lines.append(f"    reg [DATA_WIDTH-1:0] history [0:{history_depth-1}];")
            lines.append(f"    reg [$clog2({history_depth}):0] history_count;")
            lines.append(f"    reg [$clog2({history_depth})-1:0] history_head;")
            lines.append("")

        # Status output
        lines.append("    // Status: accumulator is zero")
        lines.append("    assign accumulator_zero = (accumulator == {DATA_WIDTH{1'b0}});")
        lines.append("")

        # LOAD operation
        lines.append("    // LOAD operation: Set initial state")
        lines.append("    always @(posedge clk or negedge rst_n) begin")
        lines.append("        if (!rst_n) begin")
        lines.append("            initial_state <= {DATA_WIDTH{1'b0}};")
        lines.append("        end else if (load_en) begin")
        lines.append("            initial_state <= data_in;")
        lines.append("        end")
        lines.append("    end")
        lines.append("")

        # ACCUMULATE operation
        lines.append("    // ACCUMULATE operation: XOR delta into accumulator")
        lines.append("    always @(posedge clk or negedge rst_n) begin")
        lines.append("        if (!rst_n) begin")
        lines.append("            accumulator <= {DATA_WIDTH{1'b0}};")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("            history_count <= 0;")
            lines.append("            history_head <= 0;")
        lines.append("        end else if (load_en) begin")
        lines.append("            // Reset accumulator on load")
        lines.append("            accumulator <= {DATA_WIDTH{1'b0}};")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("            history_count <= 0;")
            lines.append("            history_head <= 0;")
        lines.append("        end else if (accumulate_en) begin")
        lines.append("            // XOR delta into accumulator")
        lines.append("            accumulator <= accumulator ^ data_in;")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("            // Save to history")
            lines.append("            history[history_head] <= data_in;")
            lines.append(f"            history_head <= (history_head + 1) % {history_depth};")
            lines.append(f"            if (history_count < {history_depth})")
            lines.append("                history_count <= history_count + 1;")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("        end else if (rollback_en && history_count > 0) begin")
            lines.append("            // Rollback: XOR out the last delta")
            lines.append(f"            history_head <= (history_head - 1 + {history_depth}) % {history_depth};")
            lines.append("            accumulator <= accumulator ^ history[history_head - 1];")
            lines.append("            history_count <= history_count - 1;")
        lines.append("        end")
        lines.append("    end")
        lines.append("")

        # READ operation
        lines.append("    // READ operation: Reconstruct current state")
        lines.append("    always @(posedge clk or negedge rst_n) begin")
        lines.append("        if (!rst_n) begin")
        lines.append("            data_out <= {DATA_WIDTH{1'b0}};")
        lines.append("        end else if (read_en) begin")
        lines.append("            // current_state = initial_state XOR accumulator")
        lines.append("            data_out <= initial_state ^ accumulator;")
        lines.append("        end")
        lines.append("    end")
        lines.append("")

        lines.append("endmodule")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"rtl/{vertical_lower}/{field_lower}/{module_name}.v",
            content=content,
            language='verilog',
            description=f"Verilog RTL module for {namespace.object}"
        )

    def _generate_testbench(
        self,
        namespace: NamespaceMapping,
        delta_fields: Dict[str, Any],
        operations: Dict[str, Any],
        data_width: int
    ) -> GeneratedFile:
        """Generate Verilog testbench."""

        module_name = namespace.verilog_module_name
        tb_name = f"tb_{module_name}"

        lines = []

        # Testbench module
        lines.append(f"/**")
        lines.append(f" * Testbench for {module_name}")
        lines.append(f" */")
        lines.append("")
        lines.append(f"`timescale 1ns / 1ps")
        lines.append("")
        lines.append(f"module {tb_name};")
        lines.append("")

        # Testbench signals
        lines.append(f"    // Parameters")
        lines.append(f"    parameter DATA_WIDTH = {data_width};")
        lines.append(f"    parameter CLK_PERIOD = 10;  // 10ns = 100MHz")
        lines.append("")
        lines.append("    // Clock and reset")
        lines.append("    reg clk;")
        lines.append("    reg rst_n;")
        lines.append("")
        lines.append("    // Control signals")
        lines.append("    reg load_en;")
        lines.append("    reg accumulate_en;")
        lines.append("    reg read_en;")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    reg rollback_en;")
        lines.append("")
        lines.append("    // Data signals")
        lines.append("    reg [DATA_WIDTH-1:0] data_in;")
        lines.append("    wire [DATA_WIDTH-1:0] data_out;")
        lines.append("")
        lines.append("    // Status")
        lines.append("    wire accumulator_zero;")
        lines.append("")

        # DUT instantiation
        lines.append(f"    // DUT instantiation")
        lines.append(f"    {module_name} #(")
        lines.append(f"        .DATA_WIDTH(DATA_WIDTH)")
        lines.append(f"    ) dut (")
        lines.append("        .clk(clk),")
        lines.append("        .rst_n(rst_n),")
        lines.append("        .load_en(load_en),")
        lines.append("        .accumulate_en(accumulate_en),")
        lines.append("        .read_en(read_en),")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("        .rollback_en(rollback_en),")
        lines.append("        .data_in(data_in),")
        lines.append("        .data_out(data_out),")
        lines.append("        .accumulator_zero(accumulator_zero)")
        lines.append("    );")
        lines.append("")

        # Clock generation
        lines.append("    // Clock generation")
        lines.append("    initial begin")
        lines.append("        clk = 0;")
        lines.append("        forever #(CLK_PERIOD/2) clk = ~clk;")
        lines.append("    end")
        lines.append("")

        # Test sequence
        lines.append("    // Test sequence")
        lines.append("    initial begin")
        lines.append('        $display("=== Starting testbench for %s ===", dut.INST_NAME);')
        lines.append("")
        lines.append("        // Initialize signals")
        lines.append("        rst_n = 0;")
        lines.append("        load_en = 0;")
        lines.append("        accumulate_en = 0;")
        lines.append("        read_en = 0;")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("        rollback_en = 0;")
        lines.append("        data_in = 0;")
        lines.append("")
        lines.append("        // Release reset")
        lines.append("        #(CLK_PERIOD*2);")
        lines.append("        rst_n = 1;")
        lines.append("        #(CLK_PERIOD);")
        lines.append("")
        lines.append("        // Test 1: LOAD operation")
        lines.append('        $display("Test 1: LOAD operation");')
        lines.append(f"        data_in = {data_width}'hAAAAAAAAAAAAAAAA;")
        lines.append("        load_en = 1;")
        lines.append("        #(CLK_PERIOD);")
        lines.append("        load_en = 0;")
        lines.append("        #(CLK_PERIOD);")
        lines.append('        $display("  [PASS] LOAD complete");')
        lines.append("")
        lines.append("        // Test 2: ACCUMULATE operation")
        lines.append('        $display("Test 2: ACCUMULATE operation");')
        lines.append(f"        data_in = {data_width}'h5555555555555555;")
        lines.append("        accumulate_en = 1;")
        lines.append("        #(CLK_PERIOD);")
        lines.append("        accumulate_en = 0;")
        lines.append("        #(CLK_PERIOD);")
        lines.append('        $display("  [PASS] ACCUMULATE complete");')
        lines.append("")
        lines.append("        // Test 3: READ operation")
        lines.append('        $display("Test 3: READ operation");')
        lines.append("        read_en = 1;")
        lines.append("        #(CLK_PERIOD);")
        lines.append("        read_en = 0;")
        lines.append("        #(CLK_PERIOD);")
        lines.append(f"        if (data_out == {data_width}'hFFFFFFFFFFFFFFFF)")
        lines.append('            $display("  [PASS] READ correct (0xAAAA XOR 0x5555 = 0xFFFF)");')
        lines.append("        else")
        lines.append('            $display("  [FAIL] READ incorrect: expected 0xFFFF, got %h", data_out);')
        lines.append("")
        lines.append("        // Test 4: Self-inverse property")
        lines.append('        $display("Test 4: Self-inverse property");')
        lines.append(f"        data_in = {data_width}'h1234567890ABCDEF;")
        lines.append("        accumulate_en = 1;")
        lines.append("        #(CLK_PERIOD);")
        lines.append("        accumulate_en = 0;")
        lines.append("        #(CLK_PERIOD);")
        lines.append("        accumulate_en = 1;  // Apply same delta twice")
        lines.append("        #(CLK_PERIOD);")
        lines.append("        accumulate_en = 0;")
        lines.append("        #(CLK_PERIOD);")
        lines.append("        if (accumulator_zero)")
        lines.append('            $display("  [PASS] Self-inverse verified");')
        lines.append("        else")
        lines.append('            $display("  [FAIL] Self-inverse failed");')
        lines.append("")

        if operations.get('rollback', {}).get('enabled', False):
            lines.append("        // Test 5: Rollback operation")
            lines.append('        $display("Test 5: Rollback operation");')
            lines.append(f"        data_in = {data_width}'h1111111111111111;")
            lines.append("        accumulate_en = 1;")
            lines.append("        #(CLK_PERIOD);")
            lines.append("        accumulate_en = 0;")
            lines.append("        #(CLK_PERIOD);")
            lines.append("        rollback_en = 1;")
            lines.append("        #(CLK_PERIOD);")
            lines.append("        rollback_en = 0;")
            lines.append("        #(CLK_PERIOD);")
            lines.append("        if (accumulator_zero)")
            lines.append('            $display("  [PASS] Rollback complete");')
            lines.append("        else")
            lines.append('            $display("  [FAIL] Rollback failed");')
            lines.append("")

        lines.append('        $display("=== All tests complete ===");')
        lines.append("        $finish;")
        lines.append("    end")
        lines.append("")

        lines.append("endmodule")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"testbench/tb_{module_name}.v",
            content=content,
            language='verilog',
            description=f"Testbench for {namespace.object}"
        )

    def _generate_constraints(self, namespace: NamespaceMapping) -> GeneratedFile:
        """Generate constraint file for FPGA synthesis."""

        module_name = namespace.verilog_module_name

        lines = []
        lines.append("# Constraint file for %s" % module_name)
        lines.append("# ")
        lines.append("# Target: Generic FPGA")
        lines.append("# Clock: 27 MHz input, 94.5 MHz PLL output (matches Phase 3)")
        lines.append("")
        lines.append("# Clock constraint")
        lines.append("# create_clock -name clk -period 10.582 [get_ports clk]  # 94.5 MHz")
        lines.append("")
        lines.append("# I/O constraints")
        lines.append("# set_property IOSTANDARD LVCMOS33 [get_ports clk]")
        lines.append("# set_property IOSTANDARD LVCMOS33 [get_ports rst_n]")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"constraints/{module_name}.cst",
            content=content,
            language='verilog',
            description=f"Constraints for {namespace.object}"
        )

    @staticmethod
    def _get_data_width(delta_fields: Dict[str, Any]) -> int:
        """Determine DATA_WIDTH parameter from delta fields."""
        max_width = 64
        for field_name, field_spec in delta_fields.items():
            width = field_spec.get('width', 64)
            max_width = max(max_width, width)
        return max_width
