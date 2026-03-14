# ==============================================================================
# ATOMiK ASIC — OpenLane Synthesis Flow
#
# Targets:
#   make setup          Install OpenLane 2 + Sky130 PDK (Docker-based)
#   make synth          Run full RTL-to-GDSII flow
#   make lint           Verilator lint check (no PDK needed)
#   make sim            Run BIST testbench (no PDK needed)
#   make clean          Remove build artifacts
#   make help           Show this help
#
# Prerequisites:
#   - Docker (for OpenLane)
#   - Python 3.8+ (for OpenLane CLI)
#   - iverilog (for simulation, apt install iverilog)
#   - verilator (for lint, apt install verilator)
#
# ATOMiK Project — March 2026
# ==============================================================================

RTL_DIR    = ../rtl
SIM_DIR    = ../sim
BUILD_DIR  = runs

# RTL source files (vendor-neutral, no primitives)
RTL_FILES  = $(RTL_DIR)/atomik_asic_top.v \
             $(RTL_DIR)/atomik_core_asic.v \
             $(RTL_DIR)/atomik_sram_dp.v \
             $(RTL_DIR)/atomik_sram_sp.v \
             $(RTL_DIR)/atomik_bist.v \
             $(RTL_DIR)/atomik_dft_wrapper.v

TB_FILE    = $(SIM_DIR)/tb_asic_top.v

.PHONY: all setup synth lint sim clean help

all: lint sim
	@echo ""
	@echo "=== All pre-synthesis checks passed ==="
	@echo "Run 'make synth' to execute OpenLane RTL-to-GDSII flow."

# ==============================================================================
# OpenLane Setup
# ==============================================================================
setup:
	@echo "Installing OpenLane 2 via pip..."
	pip install openlane
	@echo ""
	@echo "OpenLane installed. Ensure Docker is running for PDK download."
	@echo "Run 'make synth' to start the flow (PDK downloads automatically)."

# ==============================================================================
# Full Synthesis Flow (requires OpenLane + Docker + PDK)
# ==============================================================================
synth:
	@echo "=== Running OpenLane RTL-to-GDSII ==="
	openlane config.json
	@echo ""
	@echo "Results in $(BUILD_DIR)/latest/"
	@echo "  GDSII:  $(BUILD_DIR)/latest/final/gds/"
	@echo "  LEF:    $(BUILD_DIR)/latest/final/lef/"
	@echo "  STA:    $(BUILD_DIR)/latest/reports/sta/"

# ==============================================================================
# Verilator Lint (no PDK needed)
# ==============================================================================
lint:
	@echo "=== Verilator Lint Check ==="
	verilator --lint-only -Wall --top-module atomik_asic_top $(RTL_FILES)
	@echo "LINT: PASS (0 warnings)"

# ==============================================================================
# Simulation (no PDK needed)
# ==============================================================================
sim:
	@echo "=== BIST + Functional Simulation ==="
	iverilog -DSIMULATION -o /tmp/tb_atomik_asic $(TB_FILE) $(RTL_FILES)
	vvp /tmp/tb_atomik_asic

# ==============================================================================
# Yosys Standalone Synthesis (area estimate without full flow)
# ==============================================================================
synth-yosys:
	@echo "=== Yosys Gate-Level Synthesis (generic cells) ==="
	yosys -p "read_verilog $(RTL_FILES); \
	          synth -top atomik_asic_top; \
	          stat; \
	          write_json $(BUILD_DIR)/atomik_synth.json" \
	      2>&1 | tee $(BUILD_DIR)/yosys_synth.log
	@echo "Gate count in $(BUILD_DIR)/yosys_synth.log"

# ==============================================================================
# Clean
# ==============================================================================
clean:
	rm -rf $(BUILD_DIR) /tmp/tb_atomik_asic

# ==============================================================================
# Help
# ==============================================================================
help:
	@echo "ATOMiK ASIC — OpenLane Synthesis Flow"
	@echo ""
	@echo "Targets:"
	@echo "  make lint           Verilator lint (no tools needed beyond verilator)"
	@echo "  make sim            Run BIST + functional testbench (needs iverilog)"
	@echo "  make synth-yosys    Gate-level synthesis with Yosys (generic cells)"
	@echo "  make setup          Install OpenLane 2 via pip"
	@echo "  make synth          Full RTL-to-GDSII with OpenLane + Sky130"
	@echo "  make clean          Remove build artifacts"
	@echo "  make help           Show this help"
	@echo ""
	@echo "Quick start (no ASIC tools needed):"
	@echo "  make lint sim       Run lint + simulation checks"
