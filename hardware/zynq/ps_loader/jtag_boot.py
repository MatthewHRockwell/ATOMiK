#!/usr/bin/env python3
"""
ATOMiK JTAG-direct boot — single-xsdb-session DDR load + UART boot trigger.

Why this exists:
  The PS→SD path is fully built (see sd_boot.py + ps_loader.elf) but the SD
  card is in the board and there is no microSD adapter on the host laptop, so
  we cannot populate the SD from outside the board. Until we either (a) swap
  cards or (b) extend the PS loader with SD-write support, we use this:

    1. ONE xsdb session (avoids the multi-session DAP errors that broke the
       earlier JTAG attempts):
         rst -system → ps7_init → fpga -file <bitstream> → ps7_post_config →
         dow -data <each boot file at its PS DDR address>
    2. Open /dev/ttyUSB1 and send `boot 0x40f00000\n` to the LiteX BIOS, then
       passthrough the boot console.

  Total elapsed: ~bitstream load (5–10 s) + 4 dow -data calls (~1–3 min for
  ~40 MB at JTAG speed) + boot.  Beats the 42-minute SFL upload cleanly.

Usage:
    python3 jtag_boot.py
"""

import argparse
import os
import subprocess
import sys
import time

ZYNQ_DIR    = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
LITEX_BUILD = f"{ZYNQ_DIR}/litex-build"
PS7_INIT    = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
# Default: pinned known-good baseline (PHASE_9_1_MILESTONE.md). Override via
# `BITSTREAM=...` env var when testing a different build (e.g. FB experiment
# at litex-build-nax64-fb) so the proven baseline stays authoritative and
# test builds cannot silently become the default.
BITSTREAM   = os.environ.get(
    "BITSTREAM",
    f"{ZYNQ_DIR}/litex/build/hamgeek_rk7020f/gateware/hamgeek_rk7020f.bit",
)
XSDB        = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"

# (file, NaxRiscv address). PS-side DDR address = NaxRiscv - 0x3FF00000.
PSLD = "/home/mattrock/Projects/ATOMiK/hardware/zynq/ps_loader"
BOOT_FILES = [
    ("Image_nax64",         0x4000_0000),   # kernel
    ("linux_nax64.dtb",     0x40EF_0000),   # device tree
    ("fw_jump_nax64.bin",   0x40F0_0000),   # OpenSBI
    ("ubuntu_rv64.cpio.gz", 0x4200_0000),   # initramfs (rootfs)
]
# L2 flush trampoline (built from trampoline.S) — placed beyond the 2 MB
# memtest range, so its own bytes are NEVER stale in L2. It evicts every L2
# line by writing 256 KB to a scratch region, then jumps to OpenSBI.
TRAMPOLINE_FILE = "build/trampoline.bin"
TRAMPOLINE_NAX  = 0x40A0_0000
BOOT_TARGET_NAX = TRAMPOLINE_NAX        # send `boot 0x40a00000`, not 0x40f00000
NAX_TO_PS_OFFSET = 0x3FF0_0000


def nax_to_ps(nax_addr: int) -> int:
    """LiteX wishbone remapper: NaxRiscv 0x40000000 → PS DDR 0x00100000."""
    return nax_addr - NAX_TO_PS_OFFSET


def build_xsdb_script() -> str:
    """Return a single xsdb script that does everything in one session."""
    lines = [
        "connect",
        "after 500",
        'targets -set -filter {name =~ "APU*"}',
        "rst -system",
        "after 500",
        'puts "-- ps7_init --"',
        f"source {PS7_INIT}",
        "ps7_init",
        f'puts "-- fpga -file {BITSTREAM} --"',
        'targets -set -filter {name =~ "xc7z*"}',
        f"fpga -file {BITSTREAM}",
        'targets -set -filter {name =~ "APU*"}',
        "ps7_post_config",
        'targets -set -filter {name =~ "ARM*Cortex-A9 MPCore #0"}',
        "stop",
        # CRITICAL: wait for NaxRiscv BIOS memtest to finish before dowing.
        # Without this, dow writes to the first ~2 MB of main_ram collide
        # with BIOS memtest patterns and the kernel image is corrupted in
        # flight → OpenSBI hangs at MEDELEG trying to jump to garbage.
        "after 5000",
    ]
    for name, nax_addr in BOOT_FILES:
        path = f"{LITEX_BUILD}/{name}"
        ps_addr = nax_to_ps(nax_addr)
        size = os.path.getsize(path)
        lines.append(
            f'puts "-- dow -data {name} → PS 0x{ps_addr:08x} '
            f'(NaxRiscv 0x{nax_addr:08x}, {size:,} bytes) --"'
        )
        lines.append(f"dow -data {path} 0x{ps_addr:08x}")

    # Also dow the L2-flush trampoline
    tpath = f"{PSLD}/{TRAMPOLINE_FILE}"
    tps   = nax_to_ps(TRAMPOLINE_NAX)
    tsz   = os.path.getsize(tpath)
    lines.append(
        f'puts "-- dow -data trampoline → PS 0x{tps:08x} '
        f'(NaxRiscv 0x{TRAMPOLINE_NAX:08x}, {tsz} bytes) --"'
    )
    lines.append(f"dow -data {tpath} 0x{tps:08x}")
    lines.append('puts "-- DDR loaded; releasing CPU --"')
    lines.append("con")
    lines.append("disconnect")
    lines.append("exit")
    return "\n".join(lines) + "\n"


def run_xsdb(script: str) -> None:
    print("[xsdb] starting unified session …")
    t0 = time.time()
    p = subprocess.Popen([XSDB], stdin=subprocess.PIPE, text=True,
                         bufsize=1)  # line-buffered, stream stdout to console
    try:
        p.communicate(script, timeout=900)  # 15 min ceiling for big DDR loads
    except subprocess.TimeoutExpired:
        p.kill()
        sys.exit("xsdb timed out (>15 min). Check JTAG / bitstream / files.")
    if p.returncode != 0:
        sys.exit(f"xsdb returned {p.returncode}")
    print(f"[xsdb] done in {time.time() - t0:.1f} s")


def trigger_boot(port: str, baud: int = 921600) -> None:
    import serial
    print(f"[uart] {port} @ {baud} — sending boot command")
    with serial.Serial(port, baud, timeout=1) as s:
        s.reset_input_buffer()
        # Wake up BIOS prompt with a couple of newlines, then issue boot.
        s.write(b"\n\n")
        s.flush()
        time.sleep(0.3)
        # Boot to the L2-flush trampoline, which evicts stale L2 entries
        # then jumps to OpenSBI. Skipping this jumps straight to OpenSBI
        # (will hang in the kernel hand-off due to LiteX flush_l2 bug).
        s.write(f"boot 0x{BOOT_TARGET_NAX:08x}\n".encode())
        s.flush()
        print("[uart] passthrough — Ctrl-C to exit")
        try:
            while True:
                data = s.read(512)
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
        except KeyboardInterrupt:
            pass


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyUSB2",
                    help="LiteX BIOS UART (FT232R adapter; FT2232H ch B is "
                         "not wired to the PL UART on this board)")
    ap.add_argument("--baud", type=int, default=921600)
    ap.add_argument("--no-boot", action="store_true",
                    help="Skip the UART boot trigger; just leave DDR loaded "
                         "and CPU released so you can drive the BIOS manually.")
    args = ap.parse_args()

    # Sanity check inputs before launching xsdb.
    for name, _ in BOOT_FILES:
        path = f"{LITEX_BUILD}/{name}"
        if not os.path.exists(path):
            sys.exit(f"missing boot file: {path}")
    if not os.path.exists(BITSTREAM):
        sys.exit(f"missing bitstream: {BITSTREAM}")

    script = build_xsdb_script()
    run_xsdb(script)

    if args.no_boot:
        print("--no-boot set; type `boot 0x40f00000` on the LiteX BIOS prompt.")
        return

    trigger_boot(args.port, args.baud)


if __name__ == "__main__":
    main()
