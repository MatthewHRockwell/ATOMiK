# ==============================================================================
# ATOMiK Zynq Build Makefile
# Target: ALINX AX7020 (XC7Z020-2CLG400I)
#
# Usage:
#   make help         Show available targets
#   make sim          Run iverilog simulation (52 tests)
#   make blockdesign  Build PS+PL block design (primary flow)
#   make bitstream    Build PL-only bitstream (non-project mode)
#   make program      Program FPGA via JTAG (volatile)
#   make clean        Remove all build artifacts
# ==============================================================================

ZYNQ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
V3_RTL   := $(ZYNQ_DIR)../v3/rtl

# Tools
VIVADO   := vivado -mode batch -source
IVERILOG := $(shell command -v iverilog 2>/dev/null)
VVP      := $(shell command -v vvp 2>/dev/null)

# RTL sources
ZYNQ_RTL := $(ZYNQ_DIR)rtl/atomik_axi4lite_wrapper.v \
            $(ZYNQ_DIR)rtl/atomik_zynq_clk.v \
            $(ZYNQ_DIR)rtl/atomik_zynq_top.v
CORE_RTL := $(V3_RTL)/atomik_v3_atomik.v
TB_SRC   := $(ZYNQ_DIR)sim/tb_axi4lite_wrapper.v

# Build outputs
BUILD_DIR  := $(ZYNQ_DIR)output
SIM_BIN    := $(BUILD_DIR)/tb_axi4lite

.PHONY: all sim bitstream blockdesign program clean help

all: sim

# ------------------------------------------------------------------------------
# Simulation (iverilog)
# ------------------------------------------------------------------------------

sim: $(TB_SRC) $(ZYNQ_RTL) $(CORE_RTL)
ifdef IVERILOG
	@mkdir -p $(BUILD_DIR)
	$(IVERILOG) -o $(SIM_BIN) $(TB_SRC) $(ZYNQ_RTL) $(CORE_RTL)
	cd $(BUILD_DIR) && $(VVP) $(SIM_BIN)
	@rm -f $(BUILD_DIR)/tb_axi4lite_wrapper.vcd
else
	@echo "iverilog not found — skipping simulation"
endif

# ------------------------------------------------------------------------------
# Vivado Builds (require Vivado installation)
# ------------------------------------------------------------------------------

## Build PL-only bitstream (non-project mode)
bitstream:
	cd $(ZYNQ_DIR) && $(VIVADO) vivado/build.tcl

## Build PS+PL block design (primary flow, exports XSA)
blockdesign:
	cd $(ZYNQ_DIR) && $(VIVADO) vivado/block_design.tcl

## Program FPGA via JTAG (volatile)
program:
	cd $(ZYNQ_DIR) && $(VIVADO) vivado/program.tcl

# ------------------------------------------------------------------------------
# Housekeeping
# ------------------------------------------------------------------------------

clean:
	rm -rf $(ZYNQ_DIR)output/ $(ZYNQ_DIR)reports/
	rm -rf $(ZYNQ_DIR).Xil/ $(ZYNQ_DIR)vivado/atomik_zynq/
	rm -f $(ZYNQ_DIR)vivado*.log $(ZYNQ_DIR)vivado*.jou

help:
	@echo "ATOMiK Zynq Build Targets:"
	@echo "  make sim          - Run iverilog AXI4-Lite testbench (52 tests)"
	@echo "  make blockdesign  - Build Zynq PS+PL block design (primary flow)"
	@echo "  make bitstream    - Build PL-only bitstream (non-project mode)"
	@echo "  make program      - Program FPGA via JTAG (volatile)"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make help         - Show this help"
