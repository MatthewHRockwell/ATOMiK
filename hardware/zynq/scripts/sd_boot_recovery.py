#!/usr/bin/env python3
"""Strict SD-boot recovery and diagnosis flow for the HamGeek RK-ZYNQ7020-F.

This is intentionally SD-first. It writes the strict FAT32 BOOT.BIN image,
physically reads the same SD sectors back through the Zynq SDIO0 controller,
then asks for one real cold power-cycle and probes BootROM state without
issuing rst-system.
"""
import argparse
import glob
import os
import re
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

ZYNQ_DIR = Path(__file__).resolve().parents[1]
PSLD_DIR = ZYNQ_DIR / "ps_loader"
FSBL_DIR = ZYNQ_DIR / "fsbl_build"
DEFAULT_BOOT_BIN = FSBL_DIR / "BOOT_l2off.bin"
STRICT_SWAP = FSBL_DIR / "run_sd_strict_boot_swap.py"
STRICT_VERIFY = FSBL_DIR / "run_sd_verify.py"
PROBE = FSBL_DIR / "probe_sd_boot_pc.py"


def run_step(name, cmd, timeout):
    print(f"\n=== {name} ===")
    print("$ " + " ".join(str(c) for c in cmd))
    try:
        proc = subprocess.run(cmd, text=True, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired as exc:
        print(f"TIMEOUT after {timeout}s")
        if exc.stdout:
            print(exc.stdout)
        if exc.stderr:
            print(exc.stderr, file=sys.stderr)
        return 124, (exc.stdout or "") + (exc.stderr or "")
    if proc.stdout:
        print(proc.stdout, end="")
    if proc.stderr:
        print(proc.stderr, end="", file=sys.stderr)
    if proc.returncode != 0:
        print(f"FAILED: {name} rc={proc.returncode}")
    return proc.returncode, proc.stdout + proc.stderr


def tty_snapshot():
    return sorted(glob.glob("/dev/ttyUSB*"))


def cold_power_cycle_prompt(no_wait):
    before = tty_snapshot()
    print("\n=== REQUIRED COLD POWER-CYCLE ===")
    print("Definition for this board/debug loop:")
    print("  A cold power-cycle means PS_POR_B is asserted and all board/card rails lose power.")
    print("  It is not xsdb rst-system, not the reset button, and not a quick off/on while USB/JTAG/HDMI can back-power the board.")
    print("")
    print("Claude/operator steps:")
    print("  1. Stop serial monitors and xsdb/openFPGALoader sessions that hold the cable.")
    print("  2. Set BOOT[1:0] switches to SD mode: 11.")
    print("  3. Turn off/remove the board's main supply.")
    print("  4. Unplug Debug USB-C/JTAG/UART, USB OTG/Host, HDMI, and any other cable that can source 5V.")
    print("  5. Confirm every board LED is dark and the FTDI ttyUSB devices disappear from Linux.")
    print("  6. Wait 30 seconds after the LEDs are dark.")
    print("  7. Reconnect main board power first. Do not press reset.")
    print("  8. Reconnect Debug USB-C only after main power is up, then wait 10 seconds for ttyUSB re-enumeration.")
    print("  9. Do not run xsdb rst-system before the BootROM probe.")
    print("")
    print(f"ttyUSB before cycle: {before if before else '(none)'}")
    if no_wait:
        print("--no-wait set; continuing without operator confirmation")
        return
    input("Press Enter only after the exact cold power-cycle above is complete...")
    after = tty_snapshot()
    print(f"ttyUSB after cycle:  {after if after else '(none)'}")


def parse_probe_register(output, label):
    lines = output.splitlines()
    for i, line in enumerate(lines):
        if label in line:
            for probe in lines[i + 1:i + 8]:
                m = re.search(r"(?:0x)?[0-9a-fA-F]+:\s+([0-9a-fA-F]{8})", probe)
                if m:
                    return int(m.group(1), 16)
                if re.fullmatch(r"\s*(?:0x)?[0-9a-fA-F]{8}\s*", probe):
                    return int(probe.strip().removeprefix("0x"), 16)
    return None


def classify_probe(output):
    boot_mode = parse_probe_register(output, "BOOT_MODE")
    reboot_status = parse_probe_register(output, "REBOOT_STATUS")
    print("\n=== Probe classification ===")
    if boot_mode is None:
        print("BOOT_MODE: not parsed")
    else:
        print(f"BOOT_MODE: 0x{boot_mode:08x}" + (" (SD)" if boot_mode == 0x5 else " (not the expected SD value 0x5)"))
    if reboot_status is None:
        print("REBOOT_STATUS: not parsed")
    else:
        print(f"REBOOT_STATUS: 0x{reboot_status:08x}")
        if reboot_status & 0x00400000:
            print("Result: BootROM reached SD media but rejected the image/header according to the current repo decode.")
            print("Next evidence gate: physical SD readback must be PASS; if it is, inspect BootROM header/image construction, not FAT geometry.")
        elif reboot_status == 0:
            print("Result: no BootROM error bit parsed. If serial is still silent, inspect whether FSBL ran and where it handed off.")
        else:
            print("Result: nonzero reboot status. Decode the bitfield before changing media again.")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--boot-bin", default=str(DEFAULT_BOOT_BIN),
                    help="BOOT.BIN payload to write and verify")
    ap.add_argument("--no-build", action="store_true",
                    help="skip make -C ps_loader sd_boot_image_writer sd_verify")
    ap.add_argument("--no-write", action="store_true",
                    help="skip writing BOOT.BIN to SD")
    ap.add_argument("--no-verify", action="store_true",
                    help="skip physical SD readback verification")
    ap.add_argument("--no-probe", action="store_true",
                    help="skip post-cold-cycle BootROM probe")
    ap.add_argument("--no-wait", action="store_true",
                    help="do not pause for manual cold power-cycle confirmation")
    ap.add_argument("--probe-delay", type=float, default=12.0,
                    help="seconds to wait after operator confirmation before no-reset BootROM probe")
    args = ap.parse_args()

    boot_bin = Path(args.boot_bin).resolve()
    if not boot_bin.exists():
        print(f"missing BOOT.BIN payload: {boot_bin}", file=sys.stderr)
        return 1

    print("=== ATOMiK strict SD boot recovery ===")
    print(f"Zynq dir: {ZYNQ_DIR}")
    print(f"BOOT payload: {boot_bin} ({boot_bin.stat().st_size:,} B)")
    print("Scope: SD BootROM path only; no JTAG/SFL Linux bypass is used here.")

    if not args.no_build:
        rc, _ = run_step("build SD writer/verifier helpers",
                         ["make", "-C", str(PSLD_DIR), "sd_boot_image_writer", "sd_verify"], 120)
        if rc != 0:
            return rc

    if not args.no_write:
        rc, _ = run_step("write strict FAT metadata + BOOT.BIN to physical SD",
                         [sys.executable, str(STRICT_SWAP), str(boot_bin)], 180)
        if rc != 0:
            return rc

    if not args.no_verify:
        rc, _ = run_step("physical SD readback verify",
                         [sys.executable, str(STRICT_VERIFY), str(boot_bin)], 180)
        if rc != 0:
            print("Stopping: do not trust BootROM conclusions until physical readback passes.")
            return rc

    if args.no_probe:
        print("\nSkipping cold-cycle BootROM probe by request.")
        return 0

    cold_power_cycle_prompt(args.no_wait)

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = Path(f"/tmp/atomik_sd_boot_probe_{stamp}.log")
    rc, out = run_step("no-reset BootROM probe after cold SD boot",
                       [sys.executable, str(PROBE), "--delay", str(args.probe_delay)], 90)
    log_path.write_text(out)
    print(f"Probe log: {log_path}")
    classify_probe(out)
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
