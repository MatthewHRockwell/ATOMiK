# ATOMiK OS — cross-compile from laptop to riscv64-linux

CROSS    ?= riscv64-linux-gnu-
CC        = $(CROSS)gcc
STRIP     = $(CROSS)strip

CFLAGS    = -O2 -Wall -Wextra -Wno-unused-parameter \
            -ffunction-sections -fdata-sections \
            -MMD -MP \
            -Iinclude
LDFLAGS   = -Wl,--gc-sections -lm

SRCS      = $(wildcard src/*.c)
OBJS      = $(SRCS:.c=.o)
DEPS      = $(SRCS:.c=.d)
-include $(DEPS)

BIN       = build/atomik_os

all: $(BIN) tools

tools: build/fb2png build/perf_bench

build/fb2png: tools/fb2png.c | build
	$(CC) -O2 -Wall -o $@ tools/fb2png.c
	$(STRIP) $@
	@ls -la $@

# Standalone perf-bench runner (v0.33-D).  Cross-compiles for the
# board with the standard ATOMiK toolchain.  Links the perf/batch/
# bench sources directly — no anim.c (uses CLOCK_MONOTONIC stub in
# tools/perf_bench_main.c).
build/perf_bench: tools/perf_bench_main.c src/perf_counter.c src/atomik_batch.c src/perf_bench.c | build
	$(CC) $(CFLAGS) -o $@ $^
	$(STRIP) $@
	@ls -la $@

$(BIN): $(OBJS) | build
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	$(STRIP) $@
	@ls -la $@

build:
	mkdir -p build

clean:
	rm -f src/*.o src/*.d
	rm -rf build

.PHONY: all clean
