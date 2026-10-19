#!/usr/bin/env python3
"""run_bench_on_board.py — upload + run the parallel-bank sweep on the board.

Assumes the board is already booted to a Linux shell over UART (after
jtag_load_all_then_boot.py with ATOMIK_BITSTREAM pointed at the bench build).

Reuses atomik_os/deploy.py's proven base64-over-UART transfer.  Uploads the
tiny nostdlib sweep tool, runs it as root, and prints the measured
throughput-vs-banks table.

Usage:
    ATOMIK_PORT=/dev/ttyUSB1 python3 run_bench_on_board.py [count] [seed_hex]
"""
import os, sys, time, serial

HERE     = os.path.dirname(os.path.abspath(__file__))
AOS_DIR  = os.path.join(HERE, "..", "..", "..", "atomik_os")
sys.path.insert(0, AOS_DIR)
import deploy  # noqa: E402  (cmd / cmd_capture / transfer / BAUD)

TINY   = os.path.join(HERE, "atomik_bench_sweep_tiny")
REMOTE = "/tmp/abench"
PORT   = os.environ.get("ATOMIK_PORT", "/dev/ttyUSB1")

def main():
    count = sys.argv[1] if len(sys.argv) > 1 else "65537"
    seed  = sys.argv[2] if len(sys.argv) > 2 else "0xDEADBEEF0BADF00D"

    if not os.path.exists(TINY):
        print(f"missing {TINY} — build it first:")
        print("  riscv64-linux-gnu-gcc -O2 -fno-builtin -ffreestanding "
              "-nostdlib -nostartfiles -static -march=rv64gc -mabi=lp64d "
              "-o atomik_bench_sweep_tiny atomik_bench_sweep_tiny.c")
        return 1

    baud = int(os.environ.get("ATOMIK_BAUD", deploy.BAUD))
    print(f"[bench] opening {PORT} @ {baud}")
    s = serial.Serial(PORT, baud, timeout=0.5)
    time.sleep(0.3)

    # wait for the shell to come up (systemd can take several minutes)
    wait_s = int(os.environ.get("ATOMIK_SHELL_WAIT", "300"))
    deadline = time.time() + wait_s
    ready = False
    print(f"[bench] waiting up to {wait_s}s for a responsive shell...")
    while time.time() < deadline:
        probe = deploy.cmd_capture(s, "echo BENCH_READY_$((6*7))", t=8)
        if "BENCH_READY_42" in (probe or ""):
            ready = True
            print("[bench] shell is up.")
            break
        time.sleep(6)
    if not ready:
        print("[bench] WARNING: never saw a clean shell response; last got:")
        print(repr(probe))
        print("[bench] trying the upload anyway.")

    print("[bench] uploading sweep tool...")
    deploy.transfer(s, TINY, REMOTE, "abench")

    print(f"[bench] running: {REMOTE} {count} {seed}\n")
    out = deploy.cmd_capture(s, f"{REMOTE} {count} {seed}", t=120)
    print("================= BOARD OUTPUT =================")
    print(out)
    print("===============================================")

    ok = "ALL MATCH" in (out or "")
    print("[bench] RESULT:", "VERIFIED ✓" if ok else "needs review")
    return 0 if ok else 2

if __name__ == "__main__":
    sys.exit(main())
