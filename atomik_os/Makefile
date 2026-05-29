# ATOMiK OS — cross-compile from laptop to riscv64-linux

CROSS    ?= riscv64-linux-gnu-
CC        = $(CROSS)gcc
STRIP     = $(CROSS)strip

CFLAGS    = -O2 -Wall -Wextra -Wno-unused-parameter \
            -ffunction-sections -fdata-sections \
            -MMD -MP \
            -Iinclude
LDFLAGS   = -Wl,--gc-sections -lm

# Board build uses fb.c; the host preview build (below) uses fb_host.c
# instead, so exclude fb_host.c from the default source set.
SRCS      = $(filter-out src/fb_host.c, $(wildcard src/*.c))
OBJS      = $(SRCS:.c=.o)
DEPS      = $(SRCS:.c=.d)
-include $(DEPS)

BIN       = build/atomik_os

all: $(BIN) tools

# ---- Host preview build (laptop) --------------------------------------
# Renders the REAL UI code to a PNG on the laptop in seconds — no board.
# Swaps fb.c -> fb_host.c (PNG dump), native gcc, -DATOMIK_HOST (arch guards
# in perf_counter/perf_bench/atomik_batch fall back to portable code).
# LAYOUT/VISUAL verification only: on-screen numbers are scenario/seed data,
# never a hardware measurement.  Use a real board fb2png for proof artifacts.
HOST_CC     ?= gcc
HOST_SRCS    = $(filter-out src/fb.c, $(wildcard src/*.c))
HOST_CFLAGS  = -O2 -Wall -Wno-unused-parameter -DATOMIK_HOST -Iinclude
HOST_BIN     = build/atomik_os_host
SHOT        ?= /tmp/atomik_host_shot.png

host: | build
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_SRCS) -o $(HOST_BIN) -lm
	@ls -la $(HOST_BIN)

# Build + stage real fonts/wallpaper to /tmp + render one frame to $(SHOT).
host-shot: host
	@mkdir -p /tmp/atomik_fonts /tmp/atomik_assets
	@cp -f assets/fonts/*.atomik_font /tmp/atomik_fonts/ 2>/dev/null || true
	@cp -f assets/topology_tile.atomik_asset \
	       /tmp/atomik_assets/topology_wallpaper.atomik_asset 2>/dev/null || true
	@rm -f /tmp/atomik_os.lock
	ATOMIK_PREVIEW=1 ATOMIK_SHOT=$(SHOT) $(HOST_BIN) || true
	@echo "host preview frame -> $(SHOT)"

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

.PHONY: all clean host host-shot
