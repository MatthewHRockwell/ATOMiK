#!/usr/bin/env python3
"""launch_bench_demo.py — bring up the live PARALLEL BANKS demo on the board.

Run AFTER the atomik_os binary is on the board (e.g. via
`deploy.py --no-launch --no-assets --no-fonts`).  This:
  1. uploads the proven standalone sweep tool to /tmp/abench,
  2. starts a tiny daemon that re-measures the engine every 2s and writes the
     result table to /tmp/atomik_bench_live.txt (atomik_os reads that file —
     no MMIO inside the compositor),
  3. sets up the framebuffer env and launches atomik_os DETACHED (console stays
     free),
  4. pulls /tmp/atomik_bench_dbg.txt to confirm the panel parsed live data.

Usage:  ATOMIK_PORT=/dev/ttyUSB1 python3 launch_bench_demo.py
"""
import os, sys, time, serial

HERE    = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "atomik_os"))
import deploy  # noqa: E402

TINY  = os.path.join(HERE, "atomik_bench_sweep_tiny")
PORT  = os.environ.get("ATOMIK_PORT", "/dev/ttyUSB1")
SEED  = "0xDEADBEEF0BADF00D"
COUNT = "65537"   # must match BENCH_COUNT in atomik_os/src/bench.c

def main():
    s = serial.Serial(PORT, deploy.BAUD, timeout=0.5)
    time.sleep(0.3)
    s.write(b"\x03\r\n"); time.sleep(0.5); s.read(4000)

    # sanity: atomik_os binary present?
    sz = deploy.cmd_capture(s, "stat -c%s /tmp/atomik_os 2>/dev/null; echo __1__", t=10)
    print("[demo] /tmp/atomik_os size:", (sz or "").strip().split("\n")[0])

    print("[demo] uploading proven sweep tool -> /tmp/abench")
    deploy.transfer(s, TINY, "/tmp/abench", "abench")

    # kill any prior demo bits
    deploy.cmd(s, "pkill -9 atomik_os 2>/dev/null; pkill -f abench_loop 2>/dev/null; "
                  "rm -f /tmp/atomik_bench_live.txt /tmp/atomik_bench_dbg.txt; true", log=False)

    # daemon: measure every 2s into the file atomik_os reads
    deploy.cmd(s, "printf '%s\\n' '#!/bin/sh' > /tmp/abench_loop.sh", log=False)
    deploy.cmd(s, f"printf '%s\\n' 'while true; do /tmp/abench {COUNT} {SEED} "
                  ">/tmp/atomik_bench_live.txt 2>&1; sleep 2; done' >> /tmp/abench_loop.sh", log=False)
    deploy.cmd(s, "nohup sh /tmp/abench_loop.sh >/dev/null 2>&1 &", log=False)
    time.sleep(3)
    print("[demo] daemon wrote live file? ->")
    print(deploy.cmd_capture(s, "head -7 /tmp/atomik_bench_live.txt 2>/dev/null; echo __2__", t=12))

    # framebuffer env + DETACHED launch (stdin /dev/null, no fifo)
    deploy.cmd(s, "[ -e /dev/fb0 ]||mknod /dev/fb0 c 29 0; "
                  "echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null; "
                  "echo DEMO > /tmp/atomik_mode; rm -f /tmp/atomik_demo; true", log=False)
    time.sleep(1); s.read(4000)
    deploy.cmd(s, "nohup /tmp/atomik_os </dev/null >/tmp/aos.out 2>/tmp/aos.err & ", log=False)
    time.sleep(8)

    print("[demo] alive?")
    print(deploy.cmd_capture(s, "pgrep atomik_os >/dev/null && echo ALIVE || echo DEAD; echo __3__", t=10))
    print("[demo] panel parse breadcrumb (proves panel sees live data):")
    print(deploy.cmd_capture(s, "cat /tmp/atomik_bench_dbg.txt 2>/dev/null; echo __4__", t=12))
    print("[demo] aos.err (should be empty):")
    print(deploy.cmd_capture(s, "wc -c </tmp/aos.err; tail -3 /tmp/aos.err; echo __5__", t=10))
    print("\n[demo] >>> look at the HDMI monitor: PARALLEL BANKS should read VERIFIED <<<")

if __name__ == "__main__":
    main()
