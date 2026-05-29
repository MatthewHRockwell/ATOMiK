#!/usr/bin/env python3
"""Flash the FSBL-only BOOT.bin to the board's QSPI NOR via JTAG.

WHY QSPI: SD boot fails at the BootROM stage with error 0x200A — the BootROM
cannot cold-initialize this SD card (see results/SD_BOOT_0x200A_DIAGNOSIS.md).
The data on the card is provably correct; only the BootROM's own SD driver is
the problem. QSPI BootROM init is deterministic (factory QSPI image boots fine).

HYBRID BOOT MODEL:
  BootROM (QSPI) --> loads 15 KB FSBL from QSPI flash @ 0x0
  FSBL           --> ps7_init, then reads from the SD FAT (our SD driver works):
                       nax64.bit.bin      -> PCAP program PL (NaxRiscv)
                       fw_jump_nax64.bin  -> DDR 0x01000000
                       linux_nax64.dtb    -> DDR 0x00FF0000
                       ubuntu_rv64.cpio.gz-> DDR 0x02100000  (31 MB Ubuntu rootfs)
                       Image_nax64        -> DDR 0x00100000
                     writes DONE_MAGIC -> NaxRiscv LiteX BIOS auto-boots Linux

So QSPI only needs the tiny FSBL; the 31 MB Ubuntu rootfs stays on SD (it can
never fit the 32 MB QSPI alongside kernel+bitstream). This keeps Ubuntu (per
feedback_no_compromises) while bypassing the broken BootROM-SD path.

BOARD PREP (operator):
  - DIP boot strap BOOT[1:0] = 10 (QSPI) is only needed to *boot*; for *flashing*
    leave straps at JTAG (00) or QSPI — program_flash drives the PS over JTAG.
  - SD card must hold the 5 files above in the FAT root (NOT BOOT.bin — that now
    lives in QSPI). Use stage_sd_files.py / run_sd_*; nax64.bit.bin is a renamed
    copy of litex-build-nax64-l2off-fb/gateware/hamgeek_rk7020f.bit.bin.

DRIVER-FSBL CAVEAT: program_flash uses --fsbl only to ps7_init the PS so its
flash-writer stub can run; it halts the core after init. Our minimal fsbl.elf
does ps7_init first thing in main(), so it works as the driver. If program_flash
rejects it (handshake/timeout), fall back to a ps7_init-only stub FSBL.
"""
import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_IMAGE = os.path.join(HERE, "BOOT_fsbl_only.bin")
DEFAULT_FSBL = os.path.join(HERE, "minimal_fsbl", "fsbl.elf")
PROGRAM_FLASH = "/opt/Xilinx/2025.2/Vitis/bin/program_flash"
SETTINGS = "/opt/Xilinx/2025.2/Vivado/settings64.sh"
# program_flash also exists at Vitis/bin/scripts/program_flash; the bin/ wrapper
# is preferred (sets up the loader env). Fall back to the scripts path if needed.
PROGRAM_FLASH_FALLBACK = "/opt/Xilinx/2025.2/Vitis/bin/scripts/program_flash"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--image", default=DEFAULT_IMAGE, help="boot image to write to QSPI @ offset 0")
    ap.add_argument("--fsbl", default=DEFAULT_FSBL, help="driver FSBL for program_flash PS init")
    ap.add_argument("--flash-type", default="qspi_single",
                    help="qspi_single (default). If 0x200A-equivalent QSPI read errors appear, "
                         "try the explicit part name, e.g. mt25ql256-spi-x1_x2_x4 or "
                         "s25fl256s-spi-x1_x2_x4 (256 Mbit parts).")
    ap.add_argument("--url", default="TCP:127.0.0.1:3121",
                    help="hw_server URL (start hw_server first, or Vivado open_hw)")
    ap.add_argument("--offset", default="0")
    ap.add_argument("--no-verify", action="store_true")
    ap.add_argument("--dry-run", action="store_true", help="print the command, do not run")
    args = ap.parse_args()

    for p, label in [(args.image, "image"), (args.fsbl, "fsbl")]:
        if not os.path.exists(p):
            print(f"ERROR: {label} not found: {p}", file=sys.stderr)
            return 2

    pf = PROGRAM_FLASH if os.path.exists(PROGRAM_FLASH) else PROGRAM_FLASH_FALLBACK
    cmd = [
        pf,
        "-f", args.image,
        "-offset", args.offset,
        "-flash_type", args.flash_type,
        "-fsbl", args.fsbl,
        "-cable", "type", "xilinx_tcf", "url", args.url,
    ]
    if not args.no_verify:
        cmd.append("-verify")

    sz = os.path.getsize(args.image)
    print("=== QSPI FSBL flash ===")
    print(f"  image      : {args.image} ({sz} bytes)")
    print(f"  driver fsbl: {args.fsbl}")
    print(f"  flash_type : {args.flash_type}")
    print(f"  offset     : {args.offset}")
    print(f"  hw_server  : {args.url}")
    # Run under the Vivado/Vitis environment so program_flash finds its loaders.
    shell_cmd = f"source {SETTINGS} >/dev/null 2>&1; exec " + " ".join(
        f"'{c}'" if " " in c else c for c in cmd)
    print(f"  command    : {shell_cmd}\n")
    if args.dry_run:
        return 0

    proc = subprocess.run(["bash", "-lc", shell_cmd])
    if proc.returncode == 0:
        print("\n=== QSPI flash OK. Set BOOT[1:0]=10 (QSPI), keep SD inserted, cold power-cycle. ===")
        print("Then probe with probe_sd_boot_pc.py (expect FSBL to run; OCM no longer zero).")
    else:
        print(f"\nprogram_flash failed (rc={proc.returncode}). If FSBL handshake is the cause, "
              f"build a ps7_init-only stub FSBL and pass it via --fsbl.", file=sys.stderr)
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
