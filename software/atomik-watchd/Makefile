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

# Zynq build (static, rv32ima)
zynq: $(BUILD)/atomik-watchd
	@echo "Built for Zynq: $(BUILD)/atomik-watchd"

$(BUILD)/atomik-watchd: atomik-watchd.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) -static -o $@ atomik-watchd.c $(LIB) $(LDFLAGS)

clean:
	rm -rf $(BUILD)
