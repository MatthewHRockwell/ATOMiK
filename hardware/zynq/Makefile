# ==============================================================================
# ATOMiK Zynq Build Makefile
# Target: ALINX AX7020 (XC7Z020-2CLG400I)
#
# Usage:
#   make help         Show available targets
#   make sim          Run single-clock iverilog simulation (Phase 1 wrapper)
#   make sim-cdc      Run dual-clock CDC simulation (Phase 2 full path)
#   make blockdesign  Build PS+PL block design (primary flow)
#   make bitstream    Build PL-only bitstream (non-project mode)
#   make program      Program FPGA via JTAG (volatile)
#   make clean        Remove all build artifacts
# ==============================================================================

ZYNQ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
V3_RTL_DIR := $(ZYNQ_DIR)../v3/rtl

# Tools
VIVADO   := vivado -mode batch -source
IVERILOG := $(shell command -v iverilog 2>/dev/null)
VVP      := $(shell command -v vvp 2>/dev/null)

# RTL sources — Phase A (accelerator)
ZYNQ_RTL := $(ZYNQ_DIR)rtl/atomik_axi4lite_wrapper.v \
            $(ZYNQ_DIR)rtl/atomik_cdc_bridge.v \
            $(ZYNQ_DIR)rtl/atomik_core_zynq.v \
            $(ZYNQ_DIR)rtl/atomik_core_zynq_parallel.v \
            $(ZYNQ_DIR)rtl/atomik_zynq_clk.v \
            $(ZYNQ_DIR)rtl/atomik_zynq_top.v

# RTL sources — Phase B (co-processor)
COPROC_RTL := $(ZYNQ_DIR)rtl/atomik_v3_cpu_zynq.v \
              $(ZYNQ_DIR)rtl/atomik_v3_regfile_zynq.v \
              $(ZYNQ_DIR)rtl/atomik_v3_atomik_zynq.v \
              $(ZYNQ_DIR)rtl/atomik_v3_bus_to_axi.v \
              $(ZYNQ_DIR)rtl/atomik_mailbox.v \
              $(ZYNQ_DIR)rtl/atomik_coprocessor_top.v

# v3 vendor-independent submodules (shared with Tang Nano)
V3_RTL := $(V3_RTL_DIR)/atomik_v3_alu.v \
          $(V3_RTL_DIR)/atomik_v3_branch.v \
          $(V3_RTL_DIR)/atomik_v3_control.v \
          $(V3_RTL_DIR)/atomik_v3_csr.v \
          $(V3_RTL_DIR)/atomik_v3_decode.v \
          $(V3_RTL_DIR)/atomik_v3_fetch.v \
          $(V3_RTL_DIR)/atomik_v3_lsu.v

CORE_RTL :=

# Testbenches
TB_CDC   := $(ZYNQ_DIR)sim/tb_cdc_wrapper.v
TB_COPROC := $(ZYNQ_DIR)sim/tb_coprocessor.v

# Build outputs
BUILD_DIR  := $(ZYNQ_DIR)output
SIM_CDC    := $(BUILD_DIR)/tb_cdc_wrapper
SIM_COPROC := $(BUILD_DIR)/tb_coprocessor

.PHONY: all sim sim-cdc sim-coproc bitstream blockdesign program clean help

all: sim-cdc

# ------------------------------------------------------------------------------
# Simulation (iverilog)
# ------------------------------------------------------------------------------

## Run dual-clock CDC testbench (primary validation)
sim-cdc: $(TB_CDC) $(ZYNQ_RTL) $(CORE_RTL)
ifdef IVERILOG
	@mkdir -p $(BUILD_DIR)
	$(IVERILOG) -DSIMULATION -o $(SIM_CDC) $(TB_CDC) $(ZYNQ_RTL) $(CORE_RTL)
	cd $(BUILD_DIR) && $(VVP) $(SIM_CDC)
	@rm -f $(BUILD_DIR)/tb_cdc_wrapper.vcd
else
	@echo "iverilog not found — skipping simulation"
endif

## Run co-processor testbench (Phase B: RV64I + mailbox + DDR3)
sim-coproc: $(TB_COPROC) $(COPROC_RTL) $(V3_RTL)
ifdef IVERILOG
	@mkdir -p $(BUILD_DIR)
	$(IVERILOG) -DSIMULATION -o $(SIM_COPROC) $(TB_COPROC) $(COPROC_RTL) $(V3_RTL)
	cd $(BUILD_DIR) && $(VVP) $(SIM_COPROC)
	@rm -f $(BUILD_DIR)/tb_coprocessor.vcd
else
	@echo "iverilog not found — skipping simulation"
endif

## Alias for backward compatibility
sim: sim-cdc

# ------------------------------------------------------------------------------
# Fmax Sweep (Vivado batch — generates configs, runs synth+impl, parses reports)
# ------------------------------------------------------------------------------

## Quick Fmax sweep: 200, 300, 400 MHz (N=1)
sweep-quick:
	cd $(ZYNQ_DIR) && python3 scripts/fmax_sweep.py --quick

## Full Fmax sweep: 200-500 MHz (N=1)
sweep:
	cd $(ZYNQ_DIR) && python3 scripts/fmax_sweep.py

## Full Fmax + multi-bank sweep
sweep-full:
	cd $(ZYNQ_DIR) && python3 scripts/fmax_sweep.py --banks 1,4,8,16

## Generate sweep files only (no Vivado)
sweep-gen:
	cd $(ZYNQ_DIR) && python3 scripts/fmax_sweep.py --gen-only

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
	rm -rf $(ZYNQ_DIR)sweep/
	rm -f $(ZYNQ_DIR)vivado*.log $(ZYNQ_DIR)vivado*.jou

help:
	@echo "ATOMiK Zynq Build Targets:"
	@echo "  make sim-cdc      - Run dual-clock CDC testbench (Phase A)"
	@echo "  make sim-coproc   - Run co-processor testbench (Phase B)"
	@echo "  make sim          - Alias for sim-cdc"
	@echo "  make blockdesign  - Build Zynq PS+PL block design (primary flow)"
	@echo "  make bitstream    - Build PL-only bitstream (non-project mode)"
	@echo "  make program      - Program FPGA via JTAG (volatile)"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make sweep-quick  - Quick Fmax sweep: 200, 300, 400 MHz"
	@echo "  make sweep        - Full Fmax sweep: 200-500 MHz"
	@echo "  make sweep-full   - Full Fmax + multi-bank sweep (N=1,4,8,16)"
	@echo "  make sweep-gen    - Generate sweep files only (no Vivado)"
	@echo "  make help         - Show this help"
