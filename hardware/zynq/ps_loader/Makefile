# ============================================================================
# ATOMiK PS Loader — Cortex-A9 baremetal build
# ============================================================================

CROSS   ?= arm-none-eabi-
CC       = $(CROSS)gcc
LD       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy
OBJDUMP  = $(CROSS)objdump
SIZE     = $(CROSS)size

BUILD    = build

CPUFLAGS = -mcpu=cortex-a9 -marm -mfpu=vfpv3 -mfloat-abi=soft
CFLAGS   = $(CPUFLAGS) -Os -g -ffreestanding -nostdlib -fno-builtin \
           -Wall -Wextra -I.
ASFLAGS  = $(CPUFLAGS) -g
LDFLAGS  = $(CPUFLAGS) -T linker.ld -nostartfiles -nostdlib \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/ps_loader.map
LDLIBS   = -lgcc

OBJS     = $(BUILD)/startup.o $(BUILD)/main.o $(BUILD)/sdhc.o $(BUILD)/fat32.o
ELF      = $(BUILD)/ps_loader.elf
BIN      = $(BUILD)/ps_loader.bin
DISASM   = $(BUILD)/ps_loader.disasm

.PHONY: all disasm clean

all: $(ELF) $(BIN)
	@echo ""
	@echo "=== PS Loader Build ==="
	@$(SIZE) $(ELF)

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %.S | $(BUILD)
	$(CC) $(ASFLAGS) -c -o $@ $<

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(ELF): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

disasm: $(ELF)
	$(OBJDUMP) -d -S $< > $(DISASM)

clean:
	rm -rf $(BUILD)
