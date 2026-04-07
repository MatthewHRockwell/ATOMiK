#!/usr/bin/env python3
"""
ATOMiK Zynq Linux Boot — JTAG program + litex_term serialboot

Phase 1: JTAG programs bitstream + ps7_init (fast, ~30s)
Phase 2: litex_term uploads images via SFL and boots (interactive terminal)

Images from boot_linux.json:
  Image32      → 0x40000000  (Linux kernel, rv32ima)
  rootfs.cpio  → 0x41000000  (BusyBox rootfs)
  linux32.dtb  → 0x40ef0000  (device tree with initrd params)
  fw_jump.bin  → 0x40f00000  (OpenSBI, boots last → jumps to kernel)
"""

import argparse
import subprocess
import sys
import os
import time

ZYNQ_DIR  = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
BITSTREAM = f"{ZYNQ_DIR}/litex-build/build/gateware/hamgeek_rk7020f.bit"
PS7_INIT  = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
BOOT_JSON = "boot_linux.json"  # relative to litex-build/
SERIAL    = "/dev/ttyUSB0"
BAUD      = 115200

def jtag_program():
    """Program FPGA bitstream and run ps7_init via JTAG."""
    xsdb_script = f"""
connect
after 3000

puts "JTAG: System reset..."
targets -set -filter {{name =~ "APU*"}}
rst -system
after 1000

puts "JTAG: PS7 init..."
source {PS7_INIT}
ps7_init

puts "JTAG: Program bitstream..."
targets -set -filter {{name =~ "xc7z*"}}
fpga -file {BITSTREAM}

puts "JTAG: Post-config..."
targets -set -filter {{name =~ "APU*"}}
ps7_post_config
after 500

puts "JTAG: Done."
disconnect
"""
    # Kill stale hw_server
    subprocess.run(["sudo", "killall", "-9", "hw_server"],
                   capture_output=True, timeout=5)
    time.sleep(1)

    # Unbind ftdi_sio from JTAG channel (if onboard FTDI)
    subprocess.run(["sudo", "sh", "-c",
                    "echo 1-2:1.0 > /sys/bus/usb/drivers/ftdi_sio/unbind"],
                   capture_output=True, timeout=5)
    time.sleep(1)

    print("[JTAG] Programming FPGA...")
    result = subprocess.run(
        ["xsdb"], input=xsdb_script,
        capture_output=True, text=True, timeout=300,
        env={**os.environ, "PATH": f"/opt/Xilinx/2025.2/Vitis/bin:{os.environ['PATH']}"}
    )
    for line in result.stdout.strip().split('\n'):
        if line.strip().startswith("JTAG:"):
            print(f"  {line.strip()}")
    if result.returncode != 0:
        print(f"[JTAG] ERROR: {result.stderr[-300:]}")
        return False
    print("[JTAG] Bitstream loaded, PS7 initialized.")
    return True


def serial_boot(boot_json=BOOT_JSON, serial_port=SERIAL):
    """Run litex_term with serialboot to upload images and boot Linux."""
    print(f"\n[SERIAL] Starting litex_term on {serial_port} @ {BAUD}")
    print(f"[SERIAL] Images: {boot_json}")
    print(f"[SERIAL] Ctrl-C to abort\n")

    os.chdir(f"{ZYNQ_DIR}/litex-build")
    try:
        subprocess.run([
            sys.executable, "-m", "litex.tools.litex_term",
            serial_port,
            "--speed", str(BAUD),
            "--images", boot_json,
            "--serial-boot",
        ])
    except KeyboardInterrupt:
        print("\n[SERIAL] Aborted by user.")


def jtag_load_images(image_tag="69"):
    """Load images to DDR via JTAG (~10s vs ~22min over serial)."""
    tcl_script = f"{ZYNQ_DIR}/scripts/jtag_load_images.tcl"
    if not os.path.exists(tcl_script):
        print(f"[ERROR] Missing: {tcl_script}")
        return False

    print(f"[JTAG] Loading images to DDR (tag={image_tag})...")
    result = subprocess.run(
        ["xsdb", tcl_script, image_tag],
        capture_output=True, text=True, timeout=120,
        env={**os.environ, "PATH": f"/opt/Xilinx/2025.2/Vitis/bin:{os.environ['PATH']}"}
    )
    print(result.stdout)
    if result.returncode != 0:
        print(f"[JTAG] ERROR: {result.stderr[-300:]}")
        return False
    return True


def serial_terminal(serial_port=SERIAL):
    """Open interactive serial terminal (no SFL upload)."""
    print(f"\n[SERIAL] Terminal on {serial_port} @ {BAUD}")
    print(f"[SERIAL] Type 'boot 0x40f00000' at the litex> prompt")
    print(f"[SERIAL] Ctrl-C to exit\n")

    os.chdir(f"{ZYNQ_DIR}/litex-build")
    try:
        subprocess.run([
            sys.executable, "-m", "litex.tools.litex_term",
            serial_port,
            "--speed", str(BAUD),
        ])
    except KeyboardInterrupt:
        print("\n[SERIAL] Closed.")


def main():
    parser = argparse.ArgumentParser(description="ATOMiK Zynq Linux Boot")
    parser.add_argument("--images", default=BOOT_JSON,
                        help=f"Boot JSON file (relative to litex-build/, default: {BOOT_JSON})")
    parser.add_argument("--serial", default=SERIAL,
                        help=f"Serial port (default: {SERIAL})")
    parser.add_argument("--skip-jtag", action="store_true",
                        help="Skip JTAG programming (FPGA already loaded)")
    parser.add_argument("--fast", metavar="TAG", nargs="?", const="69",
                        help="JTAG fast-load images to DDR instead of SFL serial "
                             "(optional tag: '69' or '32', default: 69)")
    args = parser.parse_args()

    boot_json = args.images
    serial_port = args.serial

    print("=" * 50)
    print(" ATOMiK Linux Boot (VexRiscv SMP + rootfs)")
    print(" Board: HamGeek RK-ZYNQ7020-F (CLG484)")
    if args.fast:
        print(f" Mode: JTAG fast-load ({args.fast})")
    else:
        print(f" Images: {boot_json}")
    print("=" * 50)

    # Phase 1: JTAG program bitstream
    if not args.skip_jtag:
        for name, path in [
            ("Bitstream", BITSTREAM),
            ("PS7 init",  PS7_INIT),
        ]:
            if not os.path.exists(path):
                print(f"[ERROR] Missing {name}: {path}")
                sys.exit(1)

        if not jtag_program():
            print("[ERROR] JTAG programming failed.")
            sys.exit(1)
    else:
        print("[JTAG] Skipped (--skip-jtag)")

    # Phase 2: Load images
    if args.fast:
        # Fast path: JTAG DDR load (~10s) + interactive terminal
        print("\n[WAIT] Waiting 12s for LiteX BIOS init + memtest...")
        time.sleep(12)
        if not jtag_load_images(args.fast):
            print("[ERROR] JTAG image load failed.")
            sys.exit(1)
        serial_terminal(serial_port=serial_port)
    else:
        # Standard path: SFL serial upload (~22 min)
        if not os.path.exists(f"{ZYNQ_DIR}/litex-build/{boot_json}"):
            print(f"[ERROR] Missing Boot JSON: {ZYNQ_DIR}/litex-build/{boot_json}")
            sys.exit(1)
        serial_boot(boot_json=boot_json, serial_port=serial_port)


if __name__ == "__main__":
    main()
