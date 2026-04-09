#!/bin/bash
# =============================================================================
# ATOMiK Demo Run — One-shot launch checklist
#
# Prerequisites:
#   1. Board FULLY power cycled (unplug barrel jack + JTAG USB + debug USB,
#      wait 5 min, reconnect all three)
#   2. Vivado settings sourced: source /opt/Xilinx/2025.2/Vivado/settings64.sh
#   3. Python venv active: source .venv/bin/activate
#
# Usage:
#   cd ~/Projects/ATOMiK
#   ./hardware/zynq/scripts/demo_run.sh           # runs demo_state_monitor
#   ./hardware/zynq/scripts/demo_run.sh --workload # runs workload_change_detect
#   ./hardware/zynq/scripts/demo_run.sh --bench    # runs bench_change_detect
#   ./hardware/zynq/scripts/demo_run.sh --dry-run  # prints plan without touching hardware
#
# This script: programs, DDR-checks, boots, runs one binary, captures, stops.
# DO NOT run JTAG commands after this script starts the serial phase.
# =============================================================================

set -e

ZYNQ=hardware/zynq
LITEX=$ZYNQ/litex-build
BIT=$LITEX/build/gateware/hamgeek_rk7020f.bit
PS7=$LITEX/build/gateware/hamgeek_rk7020f.gen/sources_1/ip/Zynq/ps7_init.tcl

# Parse arguments
MODE="demo"
DRY_RUN=false
for arg in "$@"; do
    case "$arg" in
        --workload) MODE="workload" ;;
        --bench)    MODE="bench" ;;
        --demo)     MODE="demo" ;;
        --dry-run)  DRY_RUN=true ;;
        *) echo "Unknown argument: $arg"; exit 1 ;;
    esac
done

# Select binary and end-marker based on mode
case "$MODE" in
    demo)     BINARY="/root/demo_state_monitor";       ENDMARK="regardless of size" ;;
    workload) BINARY="/root/workload_change_detect";    ENDMARK="size-independent\|overhead" ;;
    bench)    BINARY="/root/bench_change_detect";       ENDMARK="write time\|overhead" ;;
esac

OUTFILE="$ZYNQ/results/${MODE}_$(date +%Y%m%d_%H%M%S).txt"

echo "============================================"
echo "ATOMiK Demo Run"
echo "Git SHA: $(git rev-parse --short HEAD)"
echo "Date:    $(date -Iseconds)"
echo "Mode:    $MODE → $BINARY"
echo "Output:  $OUTFILE"
echo "============================================"
echo ""

if $DRY_RUN; then
    echo "[DRY RUN] Would program $BIT"
    echo "[DRY RUN] Would load images from $LITEX/"
    echo "[DRY RUN] Would boot Linux and run $BINARY"
    echo "[DRY RUN] Would save output to $OUTFILE"
    exit 0
fi

# Check files exist
for f in "$BIT" "$PS7" "$LITEX/Image69" "$LITEX/rootfs69_demo.cpio" "$LITEX/linux69_demo.dtb" "$LITEX/fw_jump69.bin"; do
    [ -f "$f" ] || { echo "MISSING: $f"; exit 1; }
done
echo "[OK] All files present"

# Check board
lsusb | grep -q "0403:6010" || { echo "ERROR: Board not detected. Is it plugged in and powered?"; exit 1; }
echo "[OK] Board detected"

# Kill stale hw_server
sudo killall -9 hw_server 2>/dev/null || true
sleep 2

# ═══════════════════════════════════════════════════════
# Step 1: JTAG program + load
# ═══════════════════════════════════════════════════════
echo ""
echo "[1/5] Programming FPGA + loading images..."
source /opt/Xilinx/2025.2/Vivado/settings64.sh
xsdb -eval "
connect; after 3000
targets -set -filter {name =~ \"xc7z*\"}
fpga $BIT
after 2000
targets -set -filter {name =~ \"ARM*#0\"}
source $PS7
ps7_init; ps7_post_config
after 1000
puts PROGRAMMED
" 2>&1 | grep -q "PROGRAMMED" || { echo "JTAG FAILED"; exit 1; }
echo "[OK] FPGA programmed"

# Wait for FTDI ports to stabilize AND BIOS serialboot timeout (~15s)
sleep 15

# ═══════════════════════════════════════════════════════
# Step 2: Find BIOS serial port + DDR sanity check
# ═══════════════════════════════════════════════════════
echo ""
echo "[2/5] Finding BIOS and checking DDR..."
SERIAL=$(python3 -c "
import serial, time, sys, re, glob

# Wait for ports to appear
for attempt in range(10):
    ports = sorted(glob.glob('/dev/ttyUSB*'))
    if ports:
        break
    time.sleep(1)

print(f'Scanning ports: {ports}', file=sys.stderr)

for p in ports:
    try:
        ser=serial.Serial(p,115200,timeout=5)
        ser.reset_input_buffer()
        # Wait for BIOS to finish serialboot
        time.sleep(10)
        ser.write(b'\n')
        time.sleep(5)
        d=ser.read(4096)
        if b'litex' in d:
            print(f'  BIOS found on {p}', file=sys.stderr)
            # DDR sanity check
            ser.reset_input_buffer()
            for c in 'mem_test 0x40000000 0x100':
                ser.write(c.encode()); time.sleep(0.02)
            ser.write(b'\n')
            time.sleep(6)
            out=re.sub(r'\x1b\[[0-9;]*m','',ser.read(8192).decode('utf-8','replace'))
            ser.close()
            if 'Memtest OK' in out:
                print(f'  DDR OK', file=sys.stderr)
                print(p)
                sys.exit(0)
            else:
                print(f'  DDR FAILED: {out[-50:]}', file=sys.stderr)
                sys.exit(2)
        else:
            print(f'  {p}: no litex prompt ({len(d)}B)', file=sys.stderr)
        ser.close()
    except Exception as e:
        print(f'  {p}: {e}', file=sys.stderr)
print('NO_BIOS', file=sys.stderr)
sys.exit(1)
" 2>&1 | tee /dev/stderr | grep -E "^/dev/ttyUSB") || {
    echo "ERROR: Could not find BIOS or DDR memtest failed."
    echo ""
    echo "If DDR failed: board needs full cold rest."
    echo "  1. Unplug barrel jack + JTAG USB + debug USB"
    echo "  2. Wait 5 minutes"
    echo "  3. Reconnect all three"
    echo "  4. Re-run this script"
    exit 1
}
echo "[OK] BIOS on $SERIAL, DDR healthy"

# ═══════════════════════════════════════════════════════
# Step 3: Load images + boot
# ═══════════════════════════════════════════════════════
echo ""
echo "[3/5] Loading images + booting Linux..."
xsdb -eval "
connect; after 1000
targets -set -filter {name =~ \"ARM*#0\"}
dow -data $LITEX/Image69 0x00100000
dow -data $LITEX/rootfs69_demo.cpio 0x02100000
dow -data $LITEX/linux69_demo.dtb 0x00ff0000
dow -data $LITEX/fw_jump69.bin 0x01000000
mwr 0xF8F0277C 0xFFFF
after 500
puts LOADED
" 2>&1 | grep -q "LOADED" || { echo "Image load FAILED"; exit 1; }

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
ser.close()
" || { echo "Boot failed"; exit 1; }
echo "[OK] Linux booted"

# ═══════════════════════════════════════════════════════
# Step 4: Login + run binary (chained to avoid getty race)
# ═══════════════════════════════════════════════════════
echo ""
echo "[4/5] Logging in + running $BINARY..."
mkdir -p "$ZYNQ/results"
python3 -c "
import serial, time, sys, re

ser = serial.Serial('$SERIAL', 115200, timeout=5)
ser.reset_input_buffer()

def slow(cmd):
    for c in cmd: ser.write(c.encode()); time.sleep(0.02)
    ser.write(b'\n')

# Wait for login prompt explicitly
time.sleep(5)
for attempt in range(30):
    d = ser.read(4096)
    if b'login:' in d:
        break
    time.sleep(2)
else:
    print('TIMEOUT waiting for login prompt'); sys.exit(1)

# Send login + chained command with minimal gap
slow('root')
time.sleep(8)
ser.read(8192)  # flush MOTD
slow('mknod /dev/mem c 1 1 2>/dev/null; chmod 666 /dev/mem; $BINARY')

# Capture output
buf = b''; t0 = time.time()
while time.time()-t0 < 600:
    c = ser.read(4096)
    if c: buf += c
    if b'$ENDMARK' in buf:
        time.sleep(3); buf += ser.read(8192)
        break
    time.sleep(0.5)

out = re.sub(r'\x1b\[[0-9;]*m', '', buf.decode('utf-8','replace'))

# Save
with open('$OUTFILE', 'w') as f:
    f.write(out)

# Print
print(out)
ser.close()
" || echo "WARNING: Binary may have timed out"

# ═══════════════════════════════════════════════════════
# Step 5: Summary
# ═══════════════════════════════════════════════════════
echo ""
echo "============================================"
echo "Demo complete."
echo "Output: $OUTFILE"
echo "SHA:    $(git rev-parse --short HEAD)"
echo "============================================"
