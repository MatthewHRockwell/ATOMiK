#!/bin/sh
# atomik_os runtime env + launch — baked into initramfs at /aos_launch.sh
chmod +x /atomik_os /fb2png /fbcrop /abench 2>/dev/null
[ -e /dev/mem ] || mknod /dev/mem c 1 1
[ -e /dev/fb0 ] || mknod /dev/fb0 c 29 0
mkdir -p /dev/input
[ -e /dev/input/event0 ] || mknod /dev/input/event0 c 13 64
[ -e /dev/input/event1 ] || mknod /dev/input/event1 c 13 65
[ -e /dev/input/event2 ] || mknod /dev/input/event2 c 13 66
[ -e /dev/input/event3 ] || mknod /dev/input/event3 c 13 67
mountpoint -q /proc 2>/dev/null || mount -t proc proc /proc 2>/dev/null
mountpoint -q /sys  2>/dev/null || mount -t sysfs sys /sys 2>/dev/null
for v in /sys/class/vtconsole/vtcon*/bind; do echo 0 > "$v" 2>/dev/null; done
# AA font atlases: baked at /atomik_fonts (persistent); atomik_os reads them
# from /tmp/atomik_fonts (tmpfs).  Copy them across so the UI uses the real
# typography instead of the oversized pixel fallback.
mkdir -p /tmp/atomik_fonts
cp /atomik_fonts/*.atomik_font /tmp/atomik_fonts/ 2>/dev/null
echo "FONTS:" $(ls /tmp/atomik_fonts/*.atomik_font 2>/dev/null | wc -l)
# Atom assistant poses: baked at /atomik_assets (persistent); atomik_os reads
# them from /tmp/atomik_assets (tmpfs).  Without these the assistant overlay
# falls back to a bare text box (no Atom character) — the bake-into-initramfs
# path must stage them just like the fonts.
mkdir -p /tmp/atomik_assets
cp /atomik_assets/*.atomik_asset /tmp/atomik_assets/ 2>/dev/null
echo "ASSETS:" $(ls /tmp/atomik_assets/*.atomik_asset 2>/dev/null | wc -l)
echo "USB:" $(grep -c Logitech /proc/bus/input/devices 2>/dev/null)
# LIVE parallel-bank sweep producer (real measured order-independence + scaling).
# v0.43 interactive: read the seed from /tmp/atomik_bench_ctl (written by the
# "Shuffle deltas" button) so the audience drives the workload; default until
# the first shuffle.  In-place write (one daemon per boot); parse_file tolerates
# a transient partial read (honest WAITING) and the next loop fixes it.
( while true; do
    S=$(cat /tmp/atomik_bench_ctl 2>/dev/null); [ -n "$S" ] || S=0xDEADBEEF0BADF00D
    /abench 65537 "$S" > /tmp/atomik_bench_live.txt 2>/dev/null
    sleep 0.3
  done ) &
echo "BENCH_DAEMON=$!"
# open the Workloads surface pinned to the Parallel Aggregate scenario
touch /tmp/atomik_open_workloads
export ATOMIK_PREVIEW_WL_SCENARIO=2
pkill -9 atomik_os 2>/dev/null; sleep 1
/atomik_os </dev/null >/tmp/aos.log 2>/tmp/aos.err &
echo "AOS_PID=$!"
sleep 8
pgrep atomik_os >/dev/null && echo AOS_ALIVE || echo AOS_DEAD
head -4 /tmp/aos.err 2>/dev/null
echo "--- DONE ---"
