PROJ_NAME=atomik-fw

SRCS = $(wildcard *.c) $(wildcard *.S)
LDSCRIPT = ./linker_flash.ld

RISCV_NAME ?= riscv64-unknown-elf
RISCV_PATH ?= /usr

MABI=ilp32
MARCH=rv32i

CFLAGS = -march=$(MARCH) -mabi=$(MABI) -ffunction-sections -fdata-sections -g -O3
CFLAGS += -MD -fstrict-volatile-bitfields -fno-builtin
LDFLAGS = -march=$(MARCH) -mabi=$(MABI) -Wl,--gc-sections
LDFLAGS += -nostdlib -lgcc -mcmodel=medany -nostartfiles -ffreestanding
LDFLAGS += -Wl,-Bstatic,-T,$(LDSCRIPT),-Map,$(OBJDIR)/$(PROJ_NAME).map,--print-memory-usage

RISCV_OBJCOPY = $(RISCV_PATH)/bin/$(RISCV_NAME)-objcopy
RISCV_OBJDUMP = $(RISCV_PATH)/bin/$(RISCV_NAME)-objdump
RISCV_CC = $(RISCV_PATH)/bin/$(RISCV_NAME)-gcc

OBJDIR = build
OBJS := $(SRCS)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.S=.o)
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))

all: $(OBJDIR)/$(PROJ_NAME).elf $(OBJDIR)/$(PROJ_NAME).hex $(OBJDIR)/$(PROJ_NAME).v $(OBJDIR)/$(PROJ_NAME).asm

$(OBJDIR)/%.elf: $(OBJS) | $(OBJDIR)
	$(RISCV_CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.hex: %.elf
	$(RISCV_OBJCOPY) -O ihex $^ $@

%.bin: %.elf
	$(RISCV_OBJCOPY) -O binary $^ $@

%.v: %.elf
	$(RISCV_OBJCOPY) -O verilog $^ $@

%.asm: %.elf
	$(RISCV_OBJDUMP) -S -d $^ > $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(RISCV_CC) -c $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.S | $(OBJDIR)
	$(RISCV_CC) -c $(CFLAGS) -o $@ $^ -D__ASSEMBLY__=1

$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR)

.SECONDARY: $(OBJS)
