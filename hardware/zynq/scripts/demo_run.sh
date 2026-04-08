#!/bin/bash
# =============================================================================
# ATOMiK Demo Run — One-shot launch checklist
#
# Prerequisites:
#   1. Board power cycled (unplug BOTH barrel jack AND USB, wait 10s, reconnect)
#   2. Vivado settings sourced: source /opt/Xilinx/2025.2/Vivado/settings64.sh
#   3. Python venv active: source .venv/bin/activate
#
# Usage:
#   cd ~/Projects/ATOMiK
#   ./hardware/zynq/scripts/demo_run.sh
#
# This script: programs, boots, runs workload, captures output, stops.
# DO NOT run JTAG commands after this script starts the serial phase.
# =============================================================================

set -e

ZYNQ=hardware/zynq
LITEX=$ZYNQ/litex-build
ADAPTER=$ZYNQ/litex-build-adapter
BIT=$ADAPTER/gateware/hamgeek_rk7020f.bit
PS7=$ADAPTER/gateware/hamgeek_rk7020f.gen/sources_1/ip/Zynq/ps7_init.tcl
SERIAL=/dev/ttyUSB2  # FT2232H channel B

echo "============================================"
echo "ATOMiK Demo Run"
echo "Git SHA: $(git rev-parse --short HEAD)"
echo "Date: $(date -Iseconds)"
echo "============================================"
echo ""

# Check files exist
for f in "$BIT" "$PS7" "$LITEX/Image69" "$LITEX/rootfs69_demo.cpio" "$LITEX/linux69_demo.dtb" "$LITEX/fw_jump69.bin"; do
    [ -f "$f" ] || { echo "MISSING: $f"; exit 1; }
done
echo "[OK] All files present"

# Check board
lsusb | grep -q "0403:6010" || { echo "ERROR: Board not detected"; exit 1; }
echo "[OK] Board detected"

# Kill stale hw_server
sudo killall -9 hw_server 2>/dev/null || true
sleep 2

# Step 1: JTAG program + load
echo ""
echo "[1/4] Programming FPGA + loading images..."
source /opt/Xilinx/2025.2/Vivado/settings64.sh
xsdb -eval "
connect; after 3000
targets -set -filter {name =~ \"xc7z*\"}
fpga $BIT
after 2000
targets -set -filter {name =~ \"ARM*#0\"}
source $PS7
ps7_init; ps7_post_config
dow -data $LITEX/Image69 0x00100000
dow -data $LITEX/rootfs69_demo.cpio 0x02100000
dow -data $LITEX/linux69_demo.dtb 0x00ff0000
dow -data $LITEX/fw_jump69.bin 0x01000000
mwr 0xF8F0277C 0xFFFF
puts LOADED
" 2>&1 | grep -q "LOADED" || { echo "JTAG FAILED"; exit 1; }
echo "[OK] FPGA programmed, images loaded"

# Step 2: Boot Linux (keep hw_server alive!)
echo ""
echo "[2/4] Booting Linux..."
python3 -c "
import serial, time, sys
ser=serial.Serial('$SERIAL',115200,timeout=2)
ser.reset_input_buffer(); time.sleep(3)
for c in 'boot 0x40f00000': ser.write(c.encode()); time.sleep(0.02)
ser.write(b'\n')
buf=b'';t0=time.time()
while time.time()-t0<120:
    c=ser.read(4096)
    if c: buf+=c
    if b'login:' in buf or b'syslogd' in buf: break
    time.sleep(0.1)
if len(buf)<5000: print('BOOT_FAILED'); sys.exit(1)
print('BOOT_OK %dB %ds' % (len(buf), time.time()-t0))
" || { echo "Boot failed"; exit 1; }
echo "[OK] Linux booted"

# Step 3: Login + setup
echo ""
echo "[3/4] Logging in..."
python3 -c "
import serial, time
ser=serial.Serial('$SERIAL',115200,timeout=5)
ser.reset_input_buffer()
time.sleep(2)
for c in 'root': ser.write(c.encode()); time.sleep(0.02)
ser.write(b'\n'); time.sleep(5); ser.read(8192)
for c in 'mknod /dev/mem c 1 1 2>/dev/null': ser.write(c.encode()); time.sleep(0.02)
ser.write(b'\n'); time.sleep(1); ser.read(4096)
ser.reset_input_buffer()
for c in 'echo READY': ser.write(c.encode()); time.sleep(0.02)
ser.write(b'\n'); time.sleep(2)
out=ser.read(4096).decode('utf-8','replace')
if 'READY' in out: print('LOGIN_OK')
else: print('LOGIN_FAILED'); import sys; sys.exit(1)
ser.close()
" || { echo "Login failed"; exit 1; }
echo "[OK] Logged in, /dev/mem ready"

# Step 4: Run workload
echo ""
echo "[4/4] Running workload..."
OUTFILE="hardware/zynq/results/workload_$(date +%Y%m%d_%H%M%S).txt"
mkdir -p hardware/zynq/results
python3 -c "
import serial, time, sys
ser=serial.Serial('$SERIAL',115200,timeout=5)
ser.reset_input_buffer()
for c in '/root/workload_change_detect': ser.write(c.encode()); time.sleep(0.02)
ser.write(b'\n')
buf=b'';t0=time.time()
while time.time()-t0<600:
    c=ser.read(4096)
    if c: buf+=c
    if b'overhead' in buf or b'size-independent' in buf:
        time.sleep(3);buf+=ser.read(8192);break
    time.sleep(0.5)
import re
out=re.sub(r'\x1b\[[0-9;]*m','',buf.decode('utf-8','replace'))
with open('$OUTFILE','w') as f: f.write(out)
print(out)
ser.close()
" || echo "Workload may have timed out"

echo ""
echo "============================================"
echo "Output saved to: $OUTFILE"
echo "============================================"
