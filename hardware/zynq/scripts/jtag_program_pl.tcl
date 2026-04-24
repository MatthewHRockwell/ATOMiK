# Direct JTAG PL programming for Xilinx 7-series via OpenOCD
# Uses raw irscan/drscan — no PLD driver needed
# Reference: UG470 (7 Series FPGA Configuration) Table 12-6

set bitfile "/tmp/hdmi_raw.bin"

# Read raw bitstream
set fp [open $bitfile rb]
set bitdata [read $fp]
close $fp
set nbytes [string length $bitdata]
set nbits [expr {$nbytes * 8}]
echo "Bitstream: $nbytes bytes ($nbits bits)"

# Step 1: JPROGRAM (IR=0x0B) — pulse program
echo "JPROGRAM..."
irscan zynq_pl.tap 0x0B
runtest 100000

# Step 2: CFG_IN (IR=0x05) — select config data register
echo "CFG_IN..."
irscan zynq_pl.tap 0x05

# Step 3: Shift bitstream into DR
echo "Shifting bitstream ($nbytes bytes)..."
# OpenOCD drscan wants hex string
# Convert binary data to hex for drscan
binary scan $bitdata H* hexdata
drscan zynq_pl.tap $nbits 0x$hexdata

# Step 4: JSTART (IR=0x0C) — start configuration sequence
echo "JSTART..."
irscan zynq_pl.tap 0x0C
runtest 100

# Step 5: Check status — read STAT register
# ISC_NOOP and read back
irscan zynq_pl.tap 0x14
set stat [drscan zynq_pl.tap 32 0x00000000]
echo "FPGA Status: $stat"

echo "=== PL Programming Complete ==="
