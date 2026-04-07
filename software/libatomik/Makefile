# ==============================================================================
# libatomik Makefile
#
# Builds the ATOMiK userspace library, tests, benchmarks, and examples.
#
# Quick start (Zynq/VexRiscv rv32ima):
#   make zynq    Build all binaries for Zynq Linux (requires buildroot toolchain)
#
# Targets:
#   make test-mock     Build and run tests with mock backend (no hardware)
#   make lib           Build libatomik.so/.a
#   make zynq          Build all Zynq binaries (test, bench, workload, example)
#   make test-hw       Build test binary (real UIO or /dev/mem)
#   make clean         Remove build artifacts
#   make help          Show this help
#
# Cross-compilation:
#   make lib CROSS=riscv32-buildroot-linux-gnu-
#   make zynq CROSS=/home/mattrock/buildroot-litex/output/host/bin/riscv32-buildroot-linux-gnu-
#
# ATOMiK Project — April 2026
# ==============================================================================

# Cross-compilation prefix (empty for native build)
CROSS ?=

CC      = $(CROSS)gcc
AR      = $(CROSS)ar
CFLAGS  = -Wall -Wextra -Werror -O2 -std=c99 -I.
LDFLAGS =

# Output directory
BUILD   = build

# Sources
LIB_SRC     = libatomik.c
LIB_HDR     = libatomik.h
COPROC_SRC  = atomik_coprocessor.c
COPROC_HDR  = atomik_coprocessor.h
TEST_SRC    = test_libatomik.c
ZYNQ_TEST   = ../../hardware/zynq/test

# Targets
LIB_SO    = $(BUILD)/libatomik.so
LIB_A     = $(BUILD)/libatomik.a
COPROC_SO = $(BUILD)/libatomik_coproc.so
COPROC_A  = $(BUILD)/libatomik_coproc.a
TEST_MOCK = $(BUILD)/test_libatomik_mock
TEST_HW   = $(BUILD)/test_libatomik_hw
BENCH     = $(BUILD)/bench_change_detect
WORKLOAD  = $(BUILD)/workload_change_detect
EXAMPLE   = $(BUILD)/example_atomik

.PHONY: all test-mock lib lib-coproc test-hw zynq clean help

all: test-mock

# ==============================================================================
# Mock test (runs locally, no hardware needed)
# ==============================================================================
test-mock: $(TEST_MOCK)
	@echo ""
	@echo "=== Running mock tests ==="
	@./$(TEST_MOCK)

$(TEST_MOCK): $(TEST_SRC) $(LIB_HDR) | $(BUILD)
	$(CC) $(CFLAGS) -DATOMIK_MOCK -o $@ $<

# ==============================================================================
# Shared library (for ARM target)
# ==============================================================================
lib: $(LIB_SO) $(LIB_A)

$(BUILD)/libatomik.o: $(LIB_SRC) $(LIB_HDR) | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

$(LIB_SO): $(BUILD)/libatomik.o
	$(CC) -shared -o $@ $< $(LDFLAGS)

$(LIB_A): $(BUILD)/libatomik.o
	$(AR) rcs $@ $<

# ==============================================================================
# Co-processor dispatch library (for ARM target)
# ==============================================================================
lib-coproc: $(COPROC_SO) $(COPROC_A)

$(BUILD)/atomik_coprocessor.o: $(COPROC_SRC) $(COPROC_HDR) | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

$(COPROC_SO): $(BUILD)/atomik_coprocessor.o
	$(CC) -shared -o $@ $< $(LDFLAGS)

$(COPROC_A): $(BUILD)/atomik_coprocessor.o
	$(AR) rcs $@ $<

# ==============================================================================
# Hardware test binary (links against libatomik)
# ==============================================================================
test-hw: $(TEST_HW)

$(TEST_HW): $(TEST_SRC) $(LIB_A) $(LIB_HDR) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILD) -latomik

# ==============================================================================
# Zynq/VexRiscv: all binaries for Linux userspace (static, rv32ima)
# ==============================================================================
zynq: $(BENCH) $(WORKLOAD) $(EXAMPLE)
	@echo ""
	@echo "Built for Zynq Linux:"
	@ls -la $(BENCH) $(WORKLOAD) $(EXAMPLE)
	@echo ""
	@echo "Copy to rootfs overlay and boot."

$(BENCH): $(ZYNQ_TEST)/bench_change_detect.c $(LIB_SRC) $(LIB_HDR) | $(BUILD)
	$(CC) $(CFLAGS) -static -o $@ $< $(LIB_SRC) -I.

$(WORKLOAD): $(ZYNQ_TEST)/workload_change_detect.c $(LIB_SRC) $(LIB_HDR) | $(BUILD)
	$(CC) $(CFLAGS) -static -o $@ $< $(LIB_SRC) -I.

$(EXAMPLE): example_atomik.c $(LIB_SRC) $(LIB_HDR) | $(BUILD)
	$(CC) $(CFLAGS) -static -o $@ $< $(LIB_SRC) -I.

# ==============================================================================
# Build directory
# ==============================================================================
$(BUILD):
	mkdir -p $(BUILD)

# ==============================================================================
# Clean
# ==============================================================================
clean:
	rm -rf $(BUILD)

# ==============================================================================
# Help
# ==============================================================================
help:
	@echo "libatomik — ATOMiK Userspace Library"
	@echo ""
	@echo "Targets:"
	@echo "  make test-mock     Build + run tests with mock backend (default)"
	@echo "  make lib           Build libatomik.so/.a (cross-compile with CROSS=)"
	@echo "  make zynq          Build all Zynq binaries (bench, workload, example)"
	@echo "  make test-hw       Build test binary for hardware target"
	@echo "  make clean         Remove build artifacts"
	@echo "  make help          Show this help"
	@echo ""
	@echo "Zynq quick start:"
	@echo "  make zynq CROSS=/path/to/riscv32-buildroot-linux-gnu-"
