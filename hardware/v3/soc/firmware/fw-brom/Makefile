PROJ_NAME = fw-brom

SRCS = $(wildcard *.c) $(wildcard *.S)

LDSCRIPT = ./linker_brom.ld

RISCV_PREFIX ?= riscv64-unknown-elf-
CC      = $(RISCV_PREFIX)gcc
OBJCOPY = $(RISCV_PREFIX)objcopy
OBJDUMP = $(RISCV_PREFIX)objdump

PYTHON  ?= python3
VDUMP   ?= ../scripts/vdump8to32.py
MEM_BASEADDR ?= 0x80000000

MARCH = rv64i
MABI  = lp64

CFLAGS  = -march=$(MARCH) -mabi=$(MABI) -Os -fno-builtin
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -MD -fstrict-volatile-bitfields
CFLAGS += -DISP_STAGE1  # ISP minimal: banner + handshake + echo (no flash ops)

LDFLAGS  = -march=$(MARCH) -mabi=$(MABI) -Wl,--gc-sections
LDFLAGS += -nostdlib -lgcc -mcmodel=medany -nostartfiles -ffreestanding
LDFLAGS += -Wl,-Bstatic,-T,$(LDSCRIPT),-Map,$(OBJDIR)/$(PROJ_NAME).map,--print-memory-usage

OBJDIR = build
OBJS := $(SRCS)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.S=.o)
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))

.PHONY: all clean

all: $(OBJDIR)/$(PROJ_NAME).elf $(OBJDIR)/$(PROJ_NAME).hex $(OBJDIR)/$(PROJ_NAME).asm $(OBJDIR)/$(PROJ_NAME).v $(OBJDIR)/$(PROJ_NAME).v32 $(OBJDIR)/$(PROJ_NAME).mi $(OBJDIR)/$(PROJ_NAME).mix4

$(OBJDIR)/%.elf: $(OBJS) | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.hex: %.elf
	$(OBJCOPY) -O ihex $^ $@

%.v: %.elf
	$(OBJCOPY) -O verilog $^ $@

%.v32: %.v
	$(PYTHON) $(VDUMP) $^ $@ v32 $(MEM_BASEADDR)

%.mi: %.v
	$(PYTHON) $(VDUMP) $^ $@ mi $(MEM_BASEADDR)

%.mix4: %.v
	$(PYTHON) $(VDUMP) $^ $@ mix4 $(MEM_BASEADDR)

%.asm: %.elf
	$(OBJDUMP) -S -d $^ > $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.S | $(OBJDIR)
	$(CC) -c $(CFLAGS) -o $@ $^ -D__ASSEMBLY__=1

$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR)
