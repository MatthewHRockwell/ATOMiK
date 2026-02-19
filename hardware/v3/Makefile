# =============================================================================
# ATOMiK v3 Hardware Makefile
#
# Targets:
#   lint          - Verilator lint on all v3 RTL
#   sim-iverilog  - Run iverilog testbenches
#   sim-verilator - Run Verilator C++ testbenches
#   sim-smoke     - Run both iverilog and Verilator smoke tests
#   compliance    - RISC-V compliance tests
#   synth         - Reminder for local Gowin synthesis
#   clean         - Remove simulation artifacts
#   all           - lint + sim-smoke (default)
# =============================================================================

# Tool discovery — prefer oss-cad-suite if available, else system PATH
OSS_CAD := $(HOME)/Tools/oss-cad-suite/bin
VERILATOR     := $(shell command -v $(OSS_CAD)/verilator 2>/dev/null || command -v verilator 2>/dev/null)
VERILATOR_COV := $(shell command -v $(OSS_CAD)/verilator_coverage 2>/dev/null || command -v verilator_coverage 2>/dev/null)
IVERILOG      := $(shell command -v $(OSS_CAD)/iverilog 2>/dev/null || command -v iverilog 2>/dev/null)
VVP           := $(shell command -v $(OSS_CAD)/vvp 2>/dev/null || command -v vvp 2>/dev/null)

# Directories
V3_DIR      := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
RTL_DIR     := $(V3_DIR)rtl
SIM_IV      := $(V3_DIR)sim/iverilog
SIM_VL      := $(V3_DIR)sim/verilator
SIM_COMP    := $(V3_DIR)sim/compliance
BUILD_DIR   := $(V3_DIR)build
COMP_BUILD  := $(BUILD_DIR)/compliance

# RISC-V tests directory
RISCV_TESTS := $(HOME)/Projects/riscv-tests

# RTL sources (exclude stub for CPU targets)
RTL_ALL     := $(wildcard $(RTL_DIR)/*.v)
RTL_CPU     := $(filter-out %atomik_v3_stub.v,$(RTL_ALL))

# Testbenches
TB_IV_SRC := $(wildcard $(SIM_IV)/tb_*.v)
TB_VL_SRC := $(wildcard $(SIM_VL)/tb_*.cpp)

# Reset PC for riscv-tests (default entry point)
RESET_PC ?= 0x0000000080000000

COVERAGE_BUILD := $(BUILD_DIR)/coverage
COVERAGE_DATA  := $(BUILD_DIR)/coverage_data
COVERAGE_ANN   := $(BUILD_DIR)/coverage_annotated

.PHONY: all lint sim-iverilog sim-verilator sim-smoke compliance compliance-build coverage coverage-build synth clean

all: lint sim-smoke

# -----------------------------------------------------------------------------
# Lint: Verilator --lint-only on all RTL
# -----------------------------------------------------------------------------
lint:
ifeq ($(VERILATOR),)
	@echo "SKIP: verilator not found"
else ifeq ($(RTL_ALL),)
	@echo "SKIP: no RTL files in $(RTL_DIR)"
else
	@echo "=== Verilator lint ==="
	$(VERILATOR) --lint-only -Wall $(RTL_ALL)
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
		$(IVERILOG) -o $(BUILD_DIR)/$$name.vvp -I$(RTL_DIR) $(RTL_ALL) $$tb && \
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
# compliance-build: Build the compliance runner binary
# -----------------------------------------------------------------------------
compliance-build:
ifeq ($(VERILATOR),)
	@echo "SKIP: verilator not found"
else
	@mkdir -p $(COMP_BUILD)
	@echo "=== Building compliance runner ==="
	$(VERILATOR) --cc --exe --build --trace-fst \
		-DTRACE \
		-CFLAGS "-DTRACE -I$(SIM_COMP)" \
		-GRESET_PC=$(RESET_PC) \
		--top-module atomik_v3_cpu \
		--Mdir $(COMP_BUILD) \
		-I$(RTL_DIR) \
		$(RTL_CPU) \
		$(SIM_COMP)/compliance_runner.cpp \
		-o compliance_runner
	@echo "Built: $(COMP_BUILD)/compliance_runner"
endif

# -----------------------------------------------------------------------------
# compliance: Build and run RISC-V compliance tests
# -----------------------------------------------------------------------------
COMP_TESTS := $(wildcard $(RISCV_TESTS)/isa/rv64ui-p-*)
# Filter out .dump files
COMP_ELFS  := $(filter-out %.dump,$(COMP_TESTS))

compliance: compliance-build
ifeq ($(VERILATOR),)
	@echo "SKIP: verilator not found"
else ifeq ($(COMP_ELFS),)
	@echo "=== RISC-V Compliance Tests ==="
	@echo "SKIP: No test binaries found."
	@echo "Build riscv-tests first:"
	@echo "  cd ~/Projects && git clone https://github.com/riscv-software-src/riscv-tests"
	@echo "  cd riscv-tests && git submodule update --init --recursive"
	@echo "  autoconf && ./configure --prefix=\$$PWD/install && make isa"
else
	@echo "=== RISC-V Compliance Tests ==="
	@passed=0; failed=0; total=0; \
	for elf in $(COMP_ELFS); do \
		name=$$(basename $$elf); \
		total=$$((total + 1)); \
		if $(COMP_BUILD)/compliance_runner $$elf --timeout 500000 2>&1 | tee $(COMP_BUILD)/$$name.log | grep -q "^PASS"; then \
			echo "PASS: $$name"; \
			passed=$$((passed + 1)); \
		else \
			echo "FAIL: $$name"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "=============================================="; \
	echo "Compliance: $$passed/$$total passed"; \
	echo "=============================================="; \
	if [ $$failed -ne 0 ]; then \
		echo "*** $$failed TESTS FAILED ***"; \
		exit 1; \
	else \
		echo "*** ALL TESTS PASSED ***"; \
	fi
endif

# -----------------------------------------------------------------------------
# coverage-build: Build compliance runner with line coverage instrumentation
# -----------------------------------------------------------------------------
coverage-build:
ifeq ($(VERILATOR),)
	@echo "SKIP: verilator not found"
else
	@mkdir -p $(COVERAGE_BUILD)
	@echo "=== Building compliance runner with coverage ==="
	$(VERILATOR) --cc --exe --build --coverage-line \
		-CFLAGS "-I$(SIM_COMP)" \
		-GRESET_PC=$(RESET_PC) \
		--top-module atomik_v3_cpu \
		--Mdir $(COVERAGE_BUILD) \
		-I$(RTL_DIR) \
		$(RTL_CPU) \
		$(SIM_COMP)/compliance_runner.cpp \
		-o compliance_runner_cov
	@echo "Built: $(COVERAGE_BUILD)/compliance_runner_cov"
endif

# -----------------------------------------------------------------------------
# coverage: Run compliance tests with coverage, merge, and annotate
# -----------------------------------------------------------------------------
coverage: coverage-build
ifeq ($(VERILATOR),)
	@echo "SKIP: verilator not found"
else ifeq ($(VERILATOR_COV),)
	@echo "SKIP: verilator_coverage not found"
else ifeq ($(COMP_ELFS),)
	@echo "SKIP: No test binaries found (build riscv-tests first)"
else
	@mkdir -p $(COVERAGE_DATA)
	@echo "=== Running compliance tests with coverage ==="
	@passed=0; failed=0; total=0; \
	for elf in $(COMP_ELFS); do \
		name=$$(basename $$elf); \
		total=$$((total + 1)); \
		if $(COVERAGE_BUILD)/compliance_runner_cov $$elf --timeout 500000 \
			--cov-file $(COVERAGE_DATA)/$$name.dat 2>&1 | \
			grep -q "^PASS"; then \
			passed=$$((passed + 1)); \
		else \
			echo "FAIL: $$name"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo "Compliance: $$passed/$$total passed"
	@echo "=== Merging coverage data ==="
	$(VERILATOR_COV) --annotate $(COVERAGE_ANN) --annotate-all $(COVERAGE_DATA)/*.dat
	@echo "=== Coverage report: $(COVERAGE_ANN)/ ==="
	@echo "Key files to examine:"
	@echo "  atomik_v3_decode.v"
	@echo "  atomik_v3_control.v"
	@echo ""
	@echo "--- Coverage summary ---"
	@for f in $(COVERAGE_ANN)/atomik_v3_decode.v $(COVERAGE_ANN)/atomik_v3_control.v; do \
		if [ -f "$$f" ]; then \
			name=$$(basename $$f); \
			total=$$(grep -c '^ ' $$f 2>/dev/null || echo 0); \
			uncov=$$(grep -c '%000000' $$f 2>/dev/null || echo 0); \
			echo "  $$name: $$uncov uncovered lines"; \
		fi; \
	done
endif

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
