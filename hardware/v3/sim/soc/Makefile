# ATOMiK v3 SoC Simulation Makefile
# Builds Verilator testbenches for SoC-level verification

VERILATOR = verilator
VERILATOR_FLAGS = -Wall -Wno-fatal --cc --exe --build
VERILATOR_FLAGS += -I../../rtl -I../../soc -I../../soc/gowin_ip
VERILATOR_FLAGS += --Mdir obj_bringup
VERILATOR_FLAGS += -CFLAGS "-std=c++14 -O2"

# Optional trace support (generates .fst waveform)
TRACE ?= 0
ifeq ($(TRACE),1)
VERILATOR_FLAGS += --trace-fst --trace-threads 2 -DTRACE
endif

# RTL sources
RTL_DIR = ../../rtl
SOC_DIR = ../../soc
GOWIN_IP_DIR = $(SOC_DIR)/gowin_ip

RTL_SRCS = \
	$(RTL_DIR)/atomik_v3_cpu.v \
	$(RTL_DIR)/atomik_v3_fetch.v \
	$(RTL_DIR)/atomik_v3_decode.v \
	$(RTL_DIR)/atomik_v3_regfile.v \
	$(RTL_DIR)/atomik_v3_alu.v \
	$(RTL_DIR)/atomik_v3_branch.v \
	$(RTL_DIR)/atomik_v3_lsu.v \
	$(RTL_DIR)/atomik_v3_csr.v \
	$(RTL_DIR)/atomik_v3_atomik.v \
	$(RTL_DIR)/atomik_v3_control.v

SOC_SRCS = \
	sim_soc_top.v \
	$(SOC_DIR)/picoperipheral.v \
	$(SOC_DIR)/manual_uart_tx.v

# Boot ROM firmware paths (relative to repo root when running make)
BROM_V32 = ../../soc/firmware/fw-brom/build/fw-brom.v32
FLASH_HEX32 = ../../soc/firmware/fw-flash/build/fw-flash.hex32

.PHONY: all bringup clean

all: bringup

# Bringup test: Boot ROM continuously spams 'T' + GPIO heartbeat
bringup: obj_bringup/Vsim_soc_top $(BROM_V32) fw-brom.hex32 fw-flash.hex32
	@echo "=== Running SoC Bringup Test ==="
	@./obj_bringup/Vsim_soc_top

# Symlink Boot ROM .v32 to .hex32 (same format, different extension)
fw-brom.hex32: $(BROM_V32)
	@ln -sf $(BROM_V32) fw-brom.hex32

# Build Verilator executable
obj_bringup/Vsim_soc_top: soc_bringup_test.cpp $(RTL_SRCS) $(SOC_SRCS)
	$(VERILATOR) $(VERILATOR_FLAGS) \
		-o Vsim_soc_top \
		--top-module sim_soc_top \
		$(RTL_SRCS) $(SOC_SRCS) \
		soc_bringup_test.cpp

# Generate hex32 format from flash firmware .v file (for $readmemh)
# The sim expects 32-bit words in hex, not the 8-bit .v format
fw-flash.hex32:
	@echo "Creating empty fw-flash.hex32 (bringup mode doesn't use flash)"
	@echo "// Empty flash placeholder for bringup mode" > fw-flash.hex32
	@for i in $$(seq 0 16383); do echo "00000000" >> fw-flash.hex32; done

# Check Boot ROM firmware is built
$(BROM_V32):
	@echo "ERROR: Boot ROM not built. Run:"
	@echo "  cd ../../soc/firmware/fw-brom && make"
	@exit 1

clean:
	rm -rf obj_bringup fw-flash.hex32 soc_bringup.fst

# Run with waveform trace
trace:
	@$(MAKE) bringup TRACE=1
	@echo ""
	@echo "Waveform saved to: soc_bringup.fst"
	@echo "View with: gtkwave soc_bringup.fst"
