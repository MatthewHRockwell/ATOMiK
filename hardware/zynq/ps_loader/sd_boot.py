#!/usr/bin/env python3
"""
ATOMiK SD Boot — end-to-end orchestrator (replaces the 42-min UART SFL upload)

Flow:
  1. Use xsdb to: ps7_init → fpga -file <bitstream> → ps7_post_config →
     dow ps_loader.elf → con
  2. Poll the loader's DONE flag (PS DDR 0x10100000) for up to 30 s
  3. On DONE, send `serialboot\n` then a `boot 0x40f00000\n` over the LiteX
     BIOS UART (default /dev/ttyUSB1) to make NaxRiscv jump to OpenSBI

Usage:
    sudo python3 sd_boot.py [--port /dev/ttyUSB1] [--bitstream <bit>]

Why root: hw_server needs ftdi access; pyserial doesn't but PATH consistency.
"""

import argparse
import os
import subprocess
import sys
import time

ZYNQ_DIR  = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
PSLD_DIR  = f"{ZYNQ_DIR}/ps_loader"
ELF       = f"{PSLD_DIR}/build/ps_loader.elf"
PS7_INIT  = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
DEFAULT_BITSTREAM = (
    f"{ZYNQ_DIR}/litex-build-nax64/gateware/hamgeek_rk7020f.bit"
)
XSDB = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"

DONE_ADDR  = 0x10100000
DONE_MAGIC = 0xC0DEC0DE


def run_xsdb_load(bitstream: str | None) -> None:
    """Phase A: program PL (optional), bring up PS, run PS loader, leave it
    running (the loader signals completion via PS DDR 0x10100000)."""
    bit_block = ""
    if bitstream:
        bit_block = f"""
puts "-- fpga -file {bitstream} --"
targets -set -filter {{name =~ "xc7z*"}}
fpga -file {bitstream}
targets -set -filter {{name =~ "APU*"}}
ps7_post_config
"""

    script = f"""
connect
after 500
targets -set -filter {{name =~ "APU*"}}
rst -system
after 500

puts "-- ps7_init --"
source {PS7_INIT}
ps7_init
{bit_block}
targets -set -filter {{name =~ "ARM*Cortex-A9 MPCore #0"}}
stop
mwr -force {DONE_ADDR:#x} 0x00000000

puts "-- dow {ELF} --"
dow {ELF}
puts "-- con --"
con
disconnect
exit
"""
    print("[xsdb] loading PS init + bitstream + PS loader …")
    r = subprocess.run([XSDB], input=script, text=True,
                       capture_output=True, timeout=120)
    if r.returncode != 0:
        print(r.stdout)
        print(r.stderr, file=sys.stderr)
        sys.exit(1)
    for ln in r.stdout.splitlines():
        if ln.strip().startswith("--"):
            print(f"  {ln.strip()}")


def poll_done(timeout_s: float = 30.0) -> bool:
    """Phase B: read PS DDR 0x10100000 via xsdb until it equals DONE_MAGIC
    or we time out. Use a fresh xsdb session per probe (cheap, ~1s)."""
    deadline = time.time() + timeout_s
    print(f"[poll] waiting up to {timeout_s:.0f} s for loader DONE …")
    last = None
    while time.time() < deadline:
        script = f"""
connect
targets -set -filter {{name =~ "ARM*Cortex-A9 MPCore #0"}}
puts [format "%08x" [mrd -value -force {DONE_ADDR:#x}]]
disconnect
exit
"""
        r = subprocess.run([XSDB], input=script, text=True,
                           capture_output=True, timeout=15)
        for ln in r.stdout.splitlines():
            ln = ln.strip()
            if len(ln) == 8 and all(c in "0123456789abcdefABCDEF" for c in ln):
                val = int(ln, 16)
                if val != last:
                    print(f"  done={val:#x}")
                    last = val
                if val == DONE_MAGIC:
                    return True
        time.sleep(0.5)
    return False


def trigger_boot(port: str) -> None:
    """Phase C: ask LiteX BIOS to jump to OpenSBI."""
    import serial
    print(f"[uart] opening {port} @ 921600 baud and sending boot command …")
    with serial.Serial(port, 921600, timeout=1) as s:
        # In case BIOS is mid-prompt, send a couple of newlines first
        s.write(b"\n\n")
        time.sleep(0.2)
        s.write(b"boot 0x40f00000\n")
        s.flush()
        # Then act as a passthrough so the user can see boot messages
        print("[uart] now in passthrough mode — Ctrl-C to exit")
        try:
            while True:
                data = s.read(256)
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
        except KeyboardInterrupt:
            pass


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyUSB1",
                    help="LiteX BIOS serial port")
    ap.add_argument("--bitstream", default=DEFAULT_BITSTREAM,
                    help="LiteX bitstream (.bit). Pass '' to skip PL load.")
    ap.add_argument("--no-bitstream", action="store_true",
                    help="Don't program the PL (assume bitstream already loaded)")
    ap.add_argument("--timeout", type=float, default=30.0,
                    help="Loader DONE poll timeout (s)")
    args = ap.parse_args()

    bitstream = None if args.no_bitstream or args.bitstream == "" else args.bitstream

    if not os.path.exists(ELF):
        print(f"ELF not found: {ELF} — run `make` in {PSLD_DIR}")
        sys.exit(1)
    if bitstream and not os.path.exists(bitstream):
        print(f"bitstream not found: {bitstream}")
        sys.exit(1)

    run_xsdb_load(bitstream)
    if not poll_done(args.timeout):
        print("loader did NOT signal DONE — run load_smoke.tcl to inspect state")
        sys.exit(2)

    trigger_boot(args.port)


if __name__ == "__main__":
    main()
