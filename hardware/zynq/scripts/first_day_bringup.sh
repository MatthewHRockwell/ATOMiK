#!/bin/bash
# ==============================================================================
# ATOMiK Zynq — First Day Bringup Script
# Run this when the ALINX AX7020 board arrives.
#
# Prerequisites:
#   - Vivado 2025.2 installed at /opt/Xilinx/2025.2/
#   - Board connected via USB-JTAG and USB-UART
#   - Run from hardware/zynq/ directory
# ==============================================================================

set -e
ZYNQ_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ZYNQ_DIR"

echo "=============================================="
echo " ATOMiK Zynq — First Day Bringup"
echo " Board: ALINX AX7020 (XC7Z020-2CLG400I)"
echo "=============================================="
echo ""

# Step 0: Environment
echo "[0] Checking environment..."
source /opt/Xilinx/2025.2/Vivado/settings64.sh
which vivado && echo "  Vivado: OK" || { echo "  ERROR: Vivado not found"; exit 1; }
which arm-linux-gnueabihf-gcc && echo "  ARM GCC: OK" || echo "  WARNING: ARM cross-compiler not installed"
ls /dev/ttyUSB* 2>/dev/null && echo "  USB serial: $(ls /dev/ttyUSB*)" || echo "  WARNING: No USB serial devices found"
echo ""

# Step 1: JTAG connectivity test
echo "[1] Testing JTAG connection..."
vivado -mode batch -nojournal -nolog -source /dev/stdin <<'STEP1_EOF'
open_hw_manager
connect_hw_server
open_hw_target
set device [lindex [get_hw_devices] 0]
puts "JTAG device found: $device"
puts "ID code: [get_property REGISTER.IR.BIT5_0 $device]"
close_hw_target
close_hw_server
close_hw_manager
STEP1_EOF
echo ""

# Step 2: Build block design (Phase A only first)
echo "[2] Building block design..."
echo "  This takes 15-30 minutes on first run."
vivado -mode batch -nojournal -nolog -source vivado/block_design.tcl 2>&1 | tee output/block_design_build.log
echo ""

# Step 3: Program bitstream
echo "[3] Programming bitstream via JTAG..."
vivado -mode batch -nojournal -nolog -source vivado/program.tcl
echo ""

# Step 4: Quick smoke test via UART
echo "[4] UART smoke test..."
echo "  Connect to serial: minicom -D /dev/ttyUSB0 -b 115200"
echo "  Or: screen /dev/ttyUSB0 115200"
echo "  Look for U-Boot or Linux boot messages."
echo ""

echo "=============================================="
echo " Bringup complete. Next steps:"
echo "  1. Verify UART output (U-Boot, Linux kernel)"
echo "  2. If Linux boots: test devmem2 0x43C0001C (STATUS register)"
echo "  3. Add UIO device tree: dts/atomik_uio.dtsi"
echo "  4. Run: ./test_libatomik (33 hardware tests)"
echo "=============================================="
