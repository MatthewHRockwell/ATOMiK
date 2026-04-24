#!/bin/bash
# Program Zynq PL via OpenOCD + ps7_init via ARM core
# Usage: ./program_openocd.sh <bitfile>
# Requires: openocd, ftdi_sio unbound from interface 0

BITFILE="${1:?Usage: $0 <bitfile>}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ZYNQ_DIR="$(dirname "$SCRIPT_DIR")"

if [ ! -f "$BITFILE" ]; then
    echo "ERROR: $BITFILE not found"
    exit 1
fi

# Ensure ftdi_sio is not on Channel A
echo "1-2:1.0" | sudo tee /sys/bus/usb/drivers/ftdi_sio/unbind 2>/dev/null

OPENOCD_CFG="/tmp/rk7020f_program.cfg"
cat > "$OPENOCD_CFG" << 'EOF'
adapter driver ftdi
ftdi vid_pid 0x0403 0x6010
ftdi channel 0
ftdi layout_init 0x0008 0x000b
ftdi tdo_sample_edge falling
transport select jtag
adapter speed 1000

jtag newtap zynq_pl tap -irlen 6 -expected-id 0x23727093
jtag newtap zynq_ps cpu -irlen 4 -expected-id 0x4ba00477

dap create zynq_ps.dap -chain-position zynq_ps.cpu
target create zynq_ps.cpu cortex_a -dap zynq_ps.dap -dbgbase 0x80090000
EOF

echo "============================================"
echo " ATOMiK Zynq Programming (OpenOCD)"
echo " Bitfile: $BITFILE"
echo "============================================"

# Convert .bit to raw .bin for PCAP loading
# Xilinx .bit has a header; strip it to get raw bitstream
BINFILE="/tmp/zynq_bitstream.bin"
python3 -c "
import struct, sys
with open('$BITFILE', 'rb') as f:
    data = f.read()
# Find sync word 0xAA995566 (big-endian)
sync = bytes([0xAA, 0x99, 0x55, 0x66])
idx = data.find(sync)
if idx < 0:
    print('ERROR: No sync word found in bitstream')
    sys.exit(1)
# Bitstream starts 1 word before sync (the bus width detect pattern)
# Actually, for PCAP, start from the sync word
with open('$BINFILE', 'wb') as f:
    f.write(data[idx:])
print(f'Extracted {len(data)-idx} bytes (sync at offset {idx})')
"

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to extract bitstream"
    exit 1
fi

# Program via OpenOCD
openocd -f "$OPENOCD_CFG" -c "
init
adapter speed 10000
halt

# === ps7_init register writes (essential subset) ===
# SLCR unlock
mww 0xF8000008 0x0000DF0D

# ARM PLL
mww 0xF8000110 0x000FA3C0
mww 0xF8000100 0x0002E010
mww 0xF8000100 0x0002E011
mww 0xF8000100 0x0002E010
mww 0xF8000100 0x0002E000

# IO PLL
mww 0xF8000118 0x001452C0
mww 0xF8000108 0x0001E010
mww 0xF8000108 0x0001E011
mww 0xF8000108 0x0001E010
mww 0xF8000108 0x0001E000

# Wait for PLLs to lock
sleep 100

# Clock dividers
mww 0xF8000120 0x1F000200
mww 0xF8000128 0x00700F01
mww 0xF800014C 0x00000501
mww 0xF8000154 0x00000A01
mww 0xF800012C 0x01DC4C4D

# FPGA clock enable
mww 0xF80001C4 0x00000001

# Level shifters ON
mww 0xF8000900 0x0000000F

# FPGA resets OFF
mww 0xF8000240 0x00000000

# Program PL via PCAP (DevC)
# Enable PCAP
mww 0xF8007000 0x4C00E07F
sleep 10

# Load bitstream to PL
load_image $BINFILE 0x00100000
# DMA source = 0x00100000, DMA dest = PCAP, length from file
# ... actually this approach requires DDR3 init which we skipped

# Simpler: use SVF/XSVF programming through JTAG PL tap
# For now, just verify the connection works
echo \"Connection verified. Use Vivado for full programming.\"

shutdown
" 2>&1

echo ""
echo "To complete programming, use Vivado or convert to SVF."
