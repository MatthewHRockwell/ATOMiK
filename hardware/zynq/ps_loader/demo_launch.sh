#!/bin/bash
# ATOMiK Demo Launcher — one command to start everything
#
# Usage:
#   ./demo_launch.sh              # full boot + demo + bridge
#   ./demo_launch.sh --quick      # skip JTAG boot (board already running)
#   ./demo_launch.sh --reset      # kill everything and restart
#
# Prerequisites:
#   - Board powered on, JTAG USB connected
#   - UART adapter at /dev/ttyUSB2
#   - Python venv at /home/mattrock/Projects/ATOMiK/.venv/

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJ_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
VENV="$PROJ_DIR/.venv"
ZYNQ_DIR="$PROJ_DIR/hardware/zynq"
PORT="${ATOMIK_PORT:-/dev/ttyUSB2}"
BAUD=921600
BITSTREAM="$ZYNQ_DIR/litex-build-nax64/gateware/hamgeek_rk7020f.bit"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

log() { echo -e "${BLUE}[atomik]${NC} $1"; }
ok()  { echo -e "${GREEN}[  OK  ]${NC} $1"; }
err() { echo -e "${RED}[ERROR]${NC} $1"; }

cleanup() {
    log "Stopping bridge..."
    kill $(pgrep -f "bridge.py" 2>/dev/null) 2>/dev/null || true
    log "Stopping atomik_live on board..."
    python3 -c "
import serial, time
s = serial.Serial('$PORT', $BAUD, timeout=1)
s.write(b'\x03\n'); s.flush(); time.sleep(1)
s.close()
" 2>/dev/null || true
}

# Handle --reset
if [ "$1" = "--reset" ]; then
    cleanup
    ok "Everything stopped"
    exit 0
fi

# Activate venv
source "$VENV/bin/activate" || { err "Can't activate venv"; exit 1; }

# Kill any existing bridge
kill $(pgrep -f "bridge.py" 2>/dev/null) 2>/dev/null || true
sleep 1

if [ "$1" != "--quick" ]; then
    # Full JTAG boot
    log "JTAG boot (this takes ~100 seconds)..."
    source /opt/Xilinx/2025.2/Vivado/settings64.sh
    cd "$SCRIPT_DIR"
    BITSTREAM="$BITSTREAM" python3 jtag_boot.py --no-boot 2>&1 | tail -3
    ok "JTAG boot complete"

    # Boot Linux
    log "Booting Linux..."
    python3 -c "
import serial, time
s = serial.Serial('$PORT', $BAUD, timeout=1)
s.reset_input_buffer()
s.write(b'\n\nboot 0x40a00000\n'); s.flush()
time.sleep(120)  # wait for initramfs
s.write(b'root\n'); s.flush()
time.sleep(3)
s.close()
" 2>/dev/null
    ok "Linux booted"
fi

# Transfer atomik_live if needed
log "Checking atomik_live on board..."
cd "$SCRIPT_DIR"
python3 -c "
import serial, time, base64
s = serial.Serial('$PORT', $BAUD, timeout=2)
s.reset_input_buffer(); time.sleep(0.5); s.read(65536)
def slow_send(s, t):
    for c in t: s.write(c.encode()); s.flush(); time.sleep(0.002)
    s.write(b'\n'); s.flush()
def cmd_wait(s, t):
    slow_send(s, t)
    dl = time.time() + 10; buf = b''
    while time.time() < dl:
        d = s.read(1024)
        if d: buf += d
        if b'#' in buf or b'$' in buf: return buf
    return buf

slow_send(s, '\x03'); time.sleep(1); s.read(65536)

# Check if already there
r = cmd_wait(s, 'wc -c /tmp/atomik_live 2>/dev/null')
if b'22' not in r:  # roughly right size
    print('[atomik] Transferring atomik_live...')
    with open('atomik_live_dyn', 'rb') as f: data = f.read()
    b64 = base64.b64encode(data).decode()
    cmd_wait(s, 'rm -f /tmp/demo.b64 /tmp/atomik_live')
    for i in range(0, len(b64), 128):
        cmd_wait(s, f\"printf '%s' '{b64[i:i+128]}' >> /tmp/demo.b64\")
    cmd_wait(s, 'base64 -d /tmp/demo.b64 > /tmp/atomik_live && chmod +x /tmp/atomik_live')
    time.sleep(2)
    print('[atomik] Transfer complete')
else:
    print('[atomik] atomik_live already on board')

# Launch
print('[atomik] Launching atomik_live...')
slow_send(s, '/tmp/atomik_live')
time.sleep(8)
print(s.read(4096).decode(errors='replace')[:200])
s.close()
"
ok "atomik_live running on board"

# Start bridge
log "Starting browser bridge..."
cd "$SCRIPT_DIR"
python3 bridge.py --port "$PORT" --baud "$BAUD" &
sleep 2
ok "Bridge running on ws://localhost:8766"

echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════${NC}"
echo -e "${GREEN}  ATOMiK Demo Ready${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════${NC}"
echo ""
echo "  HDMI:    Hero display (cinematic)"
echo "  LCD:     Replica endpoint"
echo "  Browser: file:///tmp/atomik_replica.html"
echo ""
echo "  Press 1-8 in browser to modify buffers"
echo "  Press 'a' for all, 'r' to reset"
echo ""
echo "  To stop: ./demo_launch.sh --reset"
echo ""
