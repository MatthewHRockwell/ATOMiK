#!/bin/sh
# atomik_os autostart — board-side init script.
#
# Runs atomik_os at boot with stdin redirected from a FIFO so external
# tools (atomik_ai_daemon.py, atomik_pull.py, capture_screenshots.py)
# can inject keystrokes without a controlling terminal.
#
# Today this is invoked manually after each cold boot:
#   python3 jtag_boot.py --autostart
# which pushes this script over UART and runs it.
#
# v0.30+ path: bake this into the buildroot initramfs as
# /etc/init.d/S99atomik so it runs at every cold boot without a
# laptop-side trigger. That requires a buildroot rebuild; deferred.

PIDFILE=/tmp/atomik_os.pid
FIFO=/tmp/aos_keys
LOG_OUT=/tmp/aos.out
LOG_ERR=/tmp/aos.err
BIN=/tmp/atomik_os

# 1. Kill any stale instance so we own /dev/fb0 cleanly.
if [ -f "$PIDFILE" ]; then
    kill -9 "$(cat $PIDFILE)" 2>/dev/null || true
    rm -f "$PIDFILE"
fi
pkill -9 atomik_os 2>/dev/null
sleep 1

# 2. Drop the framebuffer console so atomik_os owns scanout.
echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null || true

# 3. Set up the keystroke FIFO. A long-running cat keeps it open from
#    the writer side so atomik_os's open(O_RDONLY) returns immediately.
rm -f "$FIFO"
mkfifo "$FIFO"
( while true; do sleep 3600; done ) > "$FIFO" &
echo $! > /tmp/aos_fifo_writer.pid

# 4. Launch.
nohup "$BIN" > "$LOG_OUT" 2> "$LOG_ERR" < "$FIFO" &
PID=$!
echo "$PID" > "$PIDFILE"

# 5. Verify.
sleep 2
if kill -0 "$PID" 2>/dev/null; then
    echo "atomik_os autostart: PID=$PID, FIFO=$FIFO"
    cat /tmp/atomik_os_version 2>/dev/null
else
    echo "atomik_os autostart FAILED — see $LOG_ERR"
    cat "$LOG_ERR" | head -20
    exit 1
fi
