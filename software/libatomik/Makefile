# ==============================================================================
# libatomik Makefile
#
# Builds the ATOMiK UIO userspace library and test suite.
#
# Targets:
#   make test-mock     Build and run tests with mock backend (no hardware)
#   make lib           Build libatomik.so for ARM target
#   make test-hw       Build test binary for ARM target (real UIO)
#   make clean         Remove build artifacts
#   make help          Show this help
#
# Cross-compilation:
#   make lib CROSS=arm-linux-gnueabihf-
#   make test-hw CROSS=arm-linux-gnueabihf-
#
# ATOMiK Project — March 2026
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
LIB_SRC = libatomik.c
LIB_HDR = libatomik.h
TEST_SRC = test_libatomik.c

# Targets
LIB_SO   = $(BUILD)/libatomik.so
LIB_A    = $(BUILD)/libatomik.a
TEST_MOCK = $(BUILD)/test_libatomik_mock
TEST_HW   = $(BUILD)/test_libatomik_hw

.PHONY: all test-mock lib test-hw clean help

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
# Hardware test binary (for ARM target, links against libatomik)
# ==============================================================================
test-hw: $(TEST_HW)

$(TEST_HW): $(TEST_SRC) $(LIB_A) $(LIB_HDR) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILD) -latomik

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
	@echo "libatomik — ATOMiK UIO Userspace Library"
	@echo ""
	@echo "Targets:"
	@echo "  make test-mock     Build + run tests with mock backend (default)"
	@echo "  make lib           Build libatomik.so/.a (cross-compile with CROSS=)"
	@echo "  make test-hw       Build test binary for hardware target"
	@echo "  make clean         Remove build artifacts"
	@echo "  make help          Show this help"
	@echo ""
	@echo "Cross-compilation example:"
	@echo "  make lib CROSS=arm-linux-gnueabihf-"
