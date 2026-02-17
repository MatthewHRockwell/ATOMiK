# =============================================================================
# ATOMiK v3 Hardware Makefile
#
# Targets:
#   lint          - Verilator lint on all v3 RTL
#   sim-iverilog  - Run iverilog testbenches
#   sim-verilator - Run Verilator C++ testbenches
#   sim-smoke     - Run both iverilog and Verilator smoke tests
#   compliance    - RISC-V compliance tests (placeholder)
#   synth         - Reminder for local Gowin synthesis
#   clean         - Remove simulation artifacts
#   all           - lint + sim-smoke (default)
# =============================================================================

# Tool discovery — prefer oss-cad-suite if available, else system PATH
OSS_CAD := $(HOME)/Tools/oss-cad-suite/bin
VERILATOR := $(shell command -v $(OSS_CAD)/verilator 2>/dev/null || command -v verilator 2>/dev/null)
IVERILOG  := $(shell command -v $(OSS_CAD)/iverilog 2>/dev/null || command -v iverilog 2>/dev/null)
VVP       := $(shell command -v $(OSS_CAD)/vvp 2>/dev/null || command -v vvp 2>/dev/null)

# Directories
V3_DIR    := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
RTL_DIR   := $(V3_DIR)rtl
SIM_IV    := $(V3_DIR)sim/iverilog
SIM_VL    := $(V3_DIR)sim/verilator
BUILD_DIR := $(V3_DIR)build

# RTL sources
RTL_SRC := $(wildcard $(RTL_DIR)/*.v)

# Testbenches
TB_IV_SRC := $(wildcard $(SIM_IV)/tb_*.v)
TB_VL_SRC := $(wildcard $(SIM_VL)/tb_*.cpp)

.PHONY: all lint sim-iverilog sim-verilator sim-smoke compliance synth clean

all: lint sim-smoke

# -----------------------------------------------------------------------------
# Lint: Verilator --lint-only on all RTL
# -----------------------------------------------------------------------------
lint:
ifeq ($(VERILATOR),)
	@echo "SKIP: verilator not found"
else ifeq ($(RTL_SRC),)
	@echo "SKIP: no RTL files in $(RTL_DIR)"
else
	@echo "=== Verilator lint ==="
	$(VERILATOR) --lint-only -Wall $(RTL_SRC)
	@echo "PASS: lint clean"
endif

# -----------------------------------------------------------------------------
# sim-iverilog: Compile and run all tb_*.v testbenches
# -----------------------------------------------------------------------------
sim-iverilog:
ifeq ($(IVERILOG),)
	@echo "SKIP: iverilog not found"
else ifeq ($(TB_IV_SRC),)
	@echo "SKIP: no iverilog testbenches in $(SIM_IV)"
else
	@mkdir -p $(BUILD_DIR)
	@failed=0; \
	for tb in $(TB_IV_SRC); do \
		name=$$(basename $$tb .v); \
		echo "=== iverilog: $$name ==="; \
		$(IVERILOG) -o $(BUILD_DIR)/$$name.vvp -I$(RTL_DIR) $(RTL_SRC) $$tb && \
		$(VVP) $(BUILD_DIR)/$$name.vvp | tee $(BUILD_DIR)/$$name.log; \
		if grep -q "ALL TESTS PASSED" $(BUILD_DIR)/$$name.log; then \
			echo "RESULT: $$name PASSED"; \
		else \
			echo "RESULT: $$name FAILED"; \
			failed=1; \
		fi; \
		echo ""; \
	done; \
	if [ $$failed -ne 0 ]; then exit 1; fi
endif

# -----------------------------------------------------------------------------
# sim-verilator: Verilate, compile, and run C++ testbenches
# -----------------------------------------------------------------------------
sim-verilator:
ifeq ($(VERILATOR),)
	@echo "SKIP: verilator not found"
else ifeq ($(TB_VL_SRC),)
	@echo "SKIP: no Verilator testbenches in $(SIM_VL)"
else
	@mkdir -p $(BUILD_DIR)
	@failed=0; \
	for tb in $(TB_VL_SRC); do \
		name=$$(basename $$tb .cpp); \
		mod=$$(echo $$name | sed 's/^tb_//'); \
		echo "=== Verilator: $$name ==="; \
		$(VERILATOR) --cc --exe --build --trace-fst \
			-DTRACE \
			-CFLAGS "-DTRACE" \
			--top-module atomik_v3_stub \
			--Mdir $(BUILD_DIR)/V$$name \
			-I$(RTL_DIR) \
			$(RTL_DIR)/atomik_v3_stub.v \
			$$tb \
			-o $$name && \
		$(BUILD_DIR)/V$$name/$$name | tee $(BUILD_DIR)/$$name.log; \
		if grep -q "ALL TESTS PASSED" $(BUILD_DIR)/$$name.log; then \
			echo "RESULT: $$name PASSED"; \
		else \
			echo "RESULT: $$name FAILED"; \
			failed=1; \
		fi; \
		echo ""; \
	done; \
	if [ $$failed -ne 0 ]; then exit 1; fi
endif

# -----------------------------------------------------------------------------
# sim-smoke: Run both iverilog and Verilator smoke tests
# -----------------------------------------------------------------------------
sim-smoke: sim-iverilog sim-verilator

# -----------------------------------------------------------------------------
# compliance: RISC-V compliance tests (placeholder)
# -----------------------------------------------------------------------------
compliance:
	@echo "=== RISC-V Compliance Tests ==="
	@echo "SKIP: Not yet implemented (scaffold only)"
	@echo "See hardware/v3/sim/compliance/README.md for setup instructions"

# -----------------------------------------------------------------------------
# synth: Gowin synthesis (local only, requires licensed EDA)
# -----------------------------------------------------------------------------
synth:
	@echo "=== Gowin Synthesis ==="
	@echo "Synthesis requires Gowin EDA (local only, not available in CI)"
	@echo "Run: gw_sh hardware/v3/synth/synth_v3.tcl"

# -----------------------------------------------------------------------------
# clean: Remove build artifacts
# -----------------------------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)
	rm -f *.vcd *.fst
