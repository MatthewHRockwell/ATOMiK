#!/usr/bin/env python3
"""Swap BOOT.bin on the SD card for a reference image (e.g. ALINX known-good).

Reuses the staged MBR + FAT32 partition header on disk (/tmp/sd_meta_mbr.bin,
/tmp/sd_meta_partition.bin) and only patches the root-dir BOOT.BIN entry to
reflect the new file size. Then drives sd_boot_image_writer.elf on the
Cortex-A9 over JTAG to push the three sectors to the card.

Does NOT touch sd_writer.c or its hardcoded ATOMiK BOOT.bin size — keeps the
swap test independent.

Usage:
    python3 run_sd_boot_image_swap.py /path/to/reference/BOOT.bin
"""
import argparse
import os
import re
import struct
import subprocess
import sys
import time

ZYNQ_DIR = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB     = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
SDBI_ELF = f"{ZYNQ_DIR}/ps_loader/build/sd_boot_image_writer.elf"

MBR_BIN  = "/tmp/sd_meta_mbr.bin"
PART_BIN = "/tmp/sd_meta_partition.bin"
PART_PATCHED = "/tmp/sd_meta_partition_patched.bin"

# DDR PS addresses (must match sd_boot_image_writer.c)
DDR_MBR     = 0x14000000
DDR_PART    = 0x14001000
DDR_BOOT    = 0x18100000

# Scratch addresses
SCRATCH_BASE       = 0x10100000
S_STATUS           = SCRATCH_BASE + 0x00
S_LBA              = SCRATCH_BASE + 0x04
S_BYTES            = SCRATCH_BASE + 0x08
S_ERR              = SCRATCH_BASE + 0x0C
S_PHASE            = SCRATCH_BASE + 0x20
S_BOOT_BYTES       = SCRATCH_BASE + 0x24

DONE_MAGIC = 0xC0DEC0DE

PHASE_NAMES = {
    0x10: "controller_init",
    0x20: "card_init",
    0x30: "self_test_write",
    0x31: "self_test_read",
    0x32: "self_test_compare",
    0x40: "write_MBR",
    0x41: "write_FAT32_metadata",
    0x42: "write_BOOT.bin",
    0xFF: "DONE",
}

ERR_NAMES = {
    0xBAD20001: "boot size out of range",
    0xBAD20002: "controller init failed",
    0xBAD20003: "card init failed",
    0xBAD20004: "self-test write failed",
    0xBAD20005: "self-test read failed",
    0xBAD20006: "self-test data mismatch",
    0xBAD20007: "MBR write failed",
    0xBAD20008: "FAT32 metadata write failed",
    0xBAD20009: "BOOT.bin write failed",
}


def patch_partition_root_dir(part_bytes: bytearray, new_boot_size: int) -> None:
    """In-place: update the root-dir BOOT.BIN entry's file-size field.

    Root dir starts at byte offset 64*512 = 0x8000 in the partition header
    (1 cluster of 64 sectors = 32 KiB after the 32-reserved + 32-FAT sectors).
    Each dir entry is 32 bytes; BOOT.BIN entry is the first one. Its 8.3 name
    is 'BOOT    BIN' (8 + 3 = 11 bytes), followed by a 21-byte trailer; the
    file size is the last 4 bytes at offset +28 (0x1C) of the entry.
    """
    root_off = 64 * 512  # 0x8000
    # Sanity-check we're looking at the right entry
    name = bytes(part_bytes[root_off:root_off + 11])
    if name != b"BOOT    BIN":
        raise RuntimeError(f"expected 'BOOT    BIN' at root dir, got {name!r}")
    size_off = root_off + 0x1C
    old_size = struct.unpack("<I", bytes(part_bytes[size_off:size_off + 4]))[0]
    part_bytes[size_off:size_off + 4] = struct.pack("<I", new_boot_size)
    print(f"  Root-dir patch: BOOT.BIN size {old_size:,} -> {new_boot_size:,}")


def xsdb_run(script: str, timeout: int = 120) -> subprocess.CompletedProcess:
    return subprocess.run(
        [XSDB], input=script, text=True,
        capture_output=True, timeout=timeout
    )


def load_and_start(boot_bin_path: str, boot_size: int):
    """JTAG: rst -system, ps7_init, dow all source buffers, start sd_boot_image_writer."""
    return xsdb_run(f"""
connect
after 3000
targets -set -filter {{name =~ "APU*"}}
rst -system
after 500
source {PS7_INIT}
ps7_init
puts "-- ps7_init done --"
targets -set -filter {{name =~ "ARM*Cortex-A9 MPCore #0"}}
stop
after 200
puts "-- dow MBR --"
dow -data {MBR_BIN} 0x{DDR_MBR:08x}
puts "-- dow partition (patched) --"
dow -data {PART_PATCHED} 0x{DDR_PART:08x}
puts "-- dow BOOT.bin ({boot_size:,} B) --"
dow -data {boot_bin_path} 0x{DDR_BOOT:08x}
puts "-- dow sd_boot_image_writer.elf --"
dow {SDBI_ELF}
mwr -force 0x{S_STATUS:08x} 0
mwr -force 0x{S_BOOT_BYTES:08x} 0x{boot_size:08x}
con
puts "-- CPU running --"
disconnect
exit
""", timeout=600)


def poll_status() -> dict:
    r = xsdb_run(f"""
connect
after 1500
targets -set -filter {{name =~ "APU*"}}
set v [mrd -value -force 0x{S_STATUS:08x} 1]; puts "STATUS=0x[format %08x $v]"
set v [mrd -value -force 0x{S_LBA:08x} 1]; puts "LBA=0x[format %08x $v]"
set v [mrd -value -force 0x{S_BYTES:08x} 1]; puts "BYTES=0x[format %08x $v]"
set v [mrd -value -force 0x{S_ERR:08x} 1]; puts "ERR=0x[format %08x $v]"
set v [mrd -value -force 0x{S_PHASE:08x} 1]; puts "PHASE=0x[format %08x $v]"
disconnect
exit
""", timeout=30)
    out = {}
    for ln in r.stdout.splitlines():
        m = re.match(r"(\w+)=0x([0-9a-f]+)", ln.strip())
        if m:
            out[m.group(1)] = int(m.group(2), 16)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reference_boot_bin",
                    help="Path to reference BOOT.bin (e.g. ALINX known-good)")
    args = ap.parse_args()

    if not os.path.exists(args.reference_boot_bin):
        print(f"missing: {args.reference_boot_bin}"); sys.exit(1)

    boot_size = os.path.getsize(args.reference_boot_bin)
    print(f"=== Reference BOOT.bin swap test ===")
    print(f"  Reference: {args.reference_boot_bin}")
    print(f"  Size:      {boot_size:,} bytes")
    print(f"  Sectors:   {(boot_size + 511) // 512} (LBA 8320..{8320 + (boot_size+511)//512 - 1})")

    # Patch partition header root-dir BOOT.BIN size
    with open(PART_BIN, "rb") as f:
        part = bytearray(f.read())
    patch_partition_root_dir(part, boot_size)
    with open(PART_PATCHED, "wb") as f:
        f.write(part)
    print(f"  Wrote patched partition header: {PART_PATCHED}")

    print("\n--- Load + start ---")
    t0 = time.time()
    r = load_and_start(args.reference_boot_bin, boot_size)
    for ln in r.stdout.splitlines():
        if ln.strip().startswith("--") or "error" in ln.lower():
            print(f"  {ln.strip()}")
    if r.returncode != 0:
        print("LOAD FAILED:", r.stdout[-1000:], r.stderr[-500:])
        sys.exit(1)
    print(f"  took {time.time()-t0:.0f}s")

    print("\n--- Polling ---")
    last_status = -1
    deadline = time.time() + 600
    while time.time() < deadline:
        m = poll_status()
        status = m.get("STATUS", 0)
        phase = m.get("PHASE", 0)
        if status != last_status or m.get("BYTES"):
            ph_name = PHASE_NAMES.get(phase, f"0x{phase:02x}")
            print(f"  STATUS=0x{status:08x} phase=0x{phase:02x}({ph_name:<20}) "
                  f"lba=0x{m.get('LBA',0):08x} bytes={m.get('BYTES',0):,} err=0x{m.get('ERR',0):08x}")
            last_status = status
        if status == DONE_MAGIC:
            print("\n  *** SUCCESS — reference BOOT.bin written ***")
            print("\nNEXT: power-cycle into SD mode, then run:")
            print("  python3 hardware/zynq/fsbl_build/probe_sd_boot_pc.py")
            print("Expected good: PC leaves 0xFFFFxxxx, OCM contains FSBL bytes.")
            return 0
        # Allow the in-progress marker SDBI_BOOT = 0xBAD20000 to pass; treat >= 0xBAD20001 as failure
        if 0xBAD20001 <= status < DONE_MAGIC:
            name = ERR_NAMES.get(status, "unknown")
            print(f"\n  *** FAIL: {name} (err code: 0x{m.get('ERR',0):08x}) ***")
            return 1
        time.sleep(2)
    print("\n  *** TIMEOUT ***")
    return 1


if __name__ == "__main__":
    sys.exit(main())
