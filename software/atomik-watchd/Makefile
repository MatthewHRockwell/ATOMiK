# atomik-watchd Makefile

CROSS ?=
CC     = $(CROSS)gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=gnu99 -I../libatomik
LDFLAGS =

BUILD = build
LIB   = ../libatomik/libatomik.c

.PHONY: all mock zynq clean

all: mock

$(BUILD):
	mkdir -p $(BUILD)

# Mock build (runs on host, no hardware)
mock: $(BUILD)/atomik-watchd-mock
	@echo "Running 5 ticks in mock mode..."
	@./$(BUILD)/atomik-watchd-mock -t 5

$(BUILD)/atomik-watchd-mock: atomik-watchd.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) -DATOMIK_MOCK -o $@ atomik-watchd.c $(LIB)

# Zynq build (static, rv32ima) — CROSS must be set
zynq: $(BUILD)/atomik-watchd

$(BUILD)/atomik-watchd: atomik-watchd.c $(LIB) | $(BUILD)
ifeq ($(CROSS),)
	$(error CROSS not set. Use: make zynq CROSS=/path/to/riscv32-buildroot-linux-gnu-)
endif
	$(CC) $(CFLAGS) -static -o $@ atomik-watchd.c $(LIB) $(LDFLAGS)
	@file $@ | grep -q "RISC-V" || { echo "ERROR: not a RISC-V binary"; rm $@; exit 1; }
	@echo "Built for Zynq: $@"

clean:
	rm -rf $(BUILD)
