#!/bin/sh
# board_demo_launch.sh — start the standalone self-driving Workloads demo.
#
# Ship to /tmp on the board (deploy.transfer) and run `sh /tmp/wl.sh`.  Needs
# /tmp/atomik_os, /tmp/fb2png, /tmp/fbcrop, /tmp/aworkload, /tmp/abench,
# /tmp/atomik_bench_daemon.sh, and /tmp/atomik_fonts + /tmp/atomik_assets
# already present (deploy.py ships all of them, verified).
#
# Controls (write before launching):
#   /tmp/atomik_assist_force  = success|thinking|warning|explain  (pin Atom mood/pose)
# This script sets demo mode (auto-cycle scenarios) + opens Workloads on launch
# via the /tmp/atomik_open_workloads startup hook (robust; not the flaky 'w' key).
[ -e /dev/mem ] || mknod /dev/mem c 1 1
[ -e /dev/fb0 ] || mknod /dev/fb0 c 29 0
# USB HID input nodes (minimal initramfs has a static /dev; the input
# layer registers the device but no node is auto-created). Needs the
# kernel booted with 'irqpoll' so the USB controller IRQ is serviced
# (the chipidea IRQ isn't delivered via PS IRQ_P2F -> see DEMO.md).
mkdir -p /dev/input
[ -e /dev/input/event0 ] || mknod /dev/input/event0 c 13 64
[ -e /dev/input/event1 ] || mknod /dev/input/event1 c 13 65
echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null
pkill -9 atomik_os 2>/dev/null
sleep 1
# fresh hardware-verified memory-workload measurements (adapter @0xF0020000)
/tmp/aworkload /tmp/atomik_workloads_live.txt
# live parallel-bank throughput daemon (engine @0xF0021000)
pkill -f atomik_bench_daemon 2>/dev/null
setsid /tmp/atomik_bench_daemon.sh >/dev/null 2>&1 </dev/null &
sleep 3
touch /tmp/atomik_demo            # self-driving auto-cycle
touch /tmp/atomik_open_workloads  # open Workloads at startup (reliable)
# Launch with </dev/null stdin (the EOF-spin fix makes this idle, not 100% CPU).
setsid /tmp/atomik_os > /tmp/aos.out 2> /tmp/aos.err < /dev/null &
sleep 2
pgrep atomik_os >/dev/null && echo WL_DEMO_UP || echo WL_DEMO_FAILED
