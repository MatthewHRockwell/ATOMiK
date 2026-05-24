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

SDW_OBJS = $(BUILD)/startup.o $(BUILD)/sd_writer.o $(BUILD)/sdhc.o
SDW_ELF  = $(BUILD)/sd_writer.elf
SDW_BIN  = $(BUILD)/sd_writer.bin

SDV_OBJS = $(BUILD)/startup.o $(BUILD)/sd_verify.o $(BUILD)/sdhc.o
SDV_ELF  = $(BUILD)/sd_verify.elf

.PHONY: all disasm clean sd_writer sd_verify

all: $(ELF) $(BIN)
	@echo ""
	@echo "=== PS Loader Build ==="
	@$(SIZE) $(ELF)

sd_writer: $(SDW_ELF) $(SDW_BIN)
	@echo ""
	@echo "=== SD Writer Build ==="
	@$(SIZE) $(SDW_ELF)

$(SDW_ELF): $(SDW_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(SDW_OBJS) $(LDLIBS)

$(SDW_BIN): $(SDW_ELF)
	$(OBJCOPY) -O binary $< $@

sd_verify: $(SDV_ELF)
	@echo ""
	@echo "=== SD Verify Build ==="
	@$(SIZE) $(SDV_ELF)

$(SDV_ELF): $(SDV_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(SDV_OBJS) $(LDLIBS)

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
