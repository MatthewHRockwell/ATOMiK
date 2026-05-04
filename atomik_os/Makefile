# ATOMiK OS — cross-compile from laptop to riscv64-linux

CROSS    ?= riscv64-linux-gnu-
CC        = $(CROSS)gcc
STRIP     = $(CROSS)strip

CFLAGS    = -O2 -Wall -Wextra -Wno-unused-parameter \
            -ffunction-sections -fdata-sections \
            -Iinclude
LDFLAGS   = -Wl,--gc-sections -lm

SRCS      = $(wildcard src/*.c)
OBJS      = $(SRCS:.c=.o)

BIN       = build/atomik_os

all: $(BIN)

$(BIN): $(OBJS) | build
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	$(STRIP) $@
	@ls -la $@

build:
	mkdir -p build

clean:
	rm -f src/*.o
	rm -rf build

.PHONY: all clean
