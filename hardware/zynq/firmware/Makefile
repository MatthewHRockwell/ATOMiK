# ==============================================================================
# ATOMiK Zynq Co-Processor Firmware Build
#
# Builds RV64I firmware for the co-processor in Zynq PL.
# Output: raw binary suitable for ARM firmware loader (coproc_load_firmware).
#
# Usage:
#   make                Build demo_atomik firmware (default)
#   make FW=my_app      Build custom firmware (my_app.c)
#   make disasm         Disassemble firmware ELF
#   make clean          Remove build artifacts
#   make help           Show this help
#
# Prerequisites:
#   riscv64-unknown-elf-gcc (apt install gcc-riscv64-unknown-elf)
#
# ATOMiK Project — March 2026
# ==============================================================================

# Cross-compiler prefix
CROSS   ?= riscv64-unknown-elf-

CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump
SIZE    = $(CROSS)size

# Default firmware source (without .c extension)
FW      ?= demo_atomik

# Architecture: RV64I + Zicsr (for CSR access: mcycle, mtvec, etc.)
MARCH   = rv64i_zicsr
MABI    = lp64

# Compiler flags
CFLAGS  = -march=$(MARCH) -mabi=$(MABI) -Os -g \
          -nostdlib -nostartfiles -ffreestanding \
          -fno-builtin \
          -Wall -Wextra \
          -I.

# Linker flags
LDFLAGS = -T linker_ddr3.ld -nostdlib -Wl,--gc-sections

# Output directory
BUILD   = build

# Source files
CRT_SRC = crt_zynq.S
FW_SRC  = $(FW).c
SRCS    = $(CRT_SRC) $(FW_SRC)

# Output files
ELF     = $(BUILD)/$(FW).elf
BIN     = $(BUILD)/$(FW).bin
MAP     = $(BUILD)/$(FW).map
DISASM  = $(BUILD)/$(FW).disasm

.PHONY: all disasm clean help

all: $(BIN)
	@echo ""
	@echo "=== Firmware Build Complete ==="
	@$(SIZE) $(ELF)
	@echo "Binary: $(BIN) ($$(wc -c < $(BIN)) bytes)"

$(ELF): $(SRCS) linker_ddr3.ld atomik_v3.h mailbox.h | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$(MAP) -o $@ $(SRCS)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

disasm: $(ELF)
	$(OBJDUMP) -d -M no-aliases $< > $(DISASM)
	@echo "Disassembly: $(DISASM)"

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

help:
	@echo "ATOMiK Zynq Co-Processor Firmware"
	@echo ""
	@echo "Targets:"
	@echo "  make              Build demo_atomik firmware (default)"
	@echo "  make FW=my_app    Build custom firmware (my_app.c)"
	@echo "  make disasm       Disassemble firmware ELF"
	@echo "  make clean        Remove build artifacts"
	@echo "  make help         Show this help"
	@echo ""
	@echo "Output: build/<name>.bin (raw binary for ARM firmware loader)"
