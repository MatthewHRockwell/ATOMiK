#!/usr/bin/env python3
"""Swap BOOT.bin on the strict-FAT32 SD layout.

This is the safe follow-up to run_sd_strict_fat32.py.  The strict image uses a
512 MB / 4 KB-cluster FAT32 partition at LBA 8192; older swap scripts hardcode
the previous 64 MB layout and must not be used for this card.
"""
import argparse
import os
import re
import struct
import subprocess
import sys
import time

ZYNQ_DIR = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
SDBI_ELF = f"{ZYNQ_DIR}/ps_loader/build/sd_boot_image_writer.elf"
SDBI_BIN = f"{ZYNQ_DIR}/ps_loader/build/sd_boot_image_writer.bin"
ELF_ENTRY = 0x10000000  # entry point from readelf -h sd_boot_image_writer.elf

STRICT_PART_BIN = os.environ.get("STRICT_PART_BIN", "/tmp/fat_final/strict_partition.bin")
MBR_BIN = "/tmp/sd_meta_strict_mbr.bin"
META_PATCHED = "/tmp/sd_strict_boot_meta_patched.bin"

DDR_MBR = 0x14000000
DDR_PART = 0x14001000
DDR_BOOT = 0x18100000

SCRATCH_BASE = 0x10100000
S_STATUS = SCRATCH_BASE + 0x00
S_LBA = SCRATCH_BASE + 0x04
S_BYTES = SCRATCH_BASE + 0x08
S_ERR = SCRATCH_BASE + 0x0C
S_PHASE = SCRATCH_BASE + 0x20
S_BOOT_BYTES = SCRATCH_BASE + 0x24
S_PART_LBA = SCRATCH_BASE + 0x28
S_PART_BYTES = SCRATCH_BASE + 0x2C
S_BOOT_LBA = SCRATCH_BASE + 0x30

PART_LBA = 8192
PART_SECTORS = 1048576
SECTOR_SIZE = 512
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


def build_mbr() -> None:
    mbr = bytearray(SECTOR_SIZE)
    o = 0x1BE
    mbr[o + 0x00] = 0x80
    mbr[o + 0x04] = 0x0C
    struct.pack_into("<I", mbr, o + 0x08, PART_LBA)
    struct.pack_into("<I", mbr, o + 0x0C, PART_SECTORS)
    mbr[0x1FE] = 0x55
    mbr[0x1FF] = 0xAA
    with open(MBR_BIN, "wb") as f:
        f.write(mbr)


def parse_strict_image(part: bytearray) -> dict:
    if part[510:512] != b"\x55\xaa":
        raise RuntimeError("strict partition image is missing FAT boot signature")

    bps = struct.unpack_from("<H", part, 11)[0]
    spc = part[13]
    reserved = struct.unpack_from("<H", part, 14)[0]
    fats = part[16]
    spf = struct.unpack_from("<I", part, 36)[0]
    root_cluster = struct.unpack_from("<I", part, 44)[0]
    if bps != SECTOR_SIZE or spc == 0 or fats != 2:
        raise RuntimeError(f"unexpected FAT32 geometry: bps={bps} spc={spc} fats={fats}")

    data_rel_lba = reserved + fats * spf
    root_rel_lba = data_rel_lba + (root_cluster - 2) * spc
    root_off = root_rel_lba * bps
    root_len = spc * bps

    boot_entry_off = None
    for off in range(root_off, root_off + root_len, 32):
        name = bytes(part[off:off + 11])
        if name == b"BOOT    BIN":
            boot_entry_off = off
            break
    if boot_entry_off is None:
        raise RuntimeError("BOOT.BIN short directory entry not found in strict image")

    cluster_hi = struct.unpack_from("<H", part, boot_entry_off + 20)[0]
    cluster_lo = struct.unpack_from("<H", part, boot_entry_off + 26)[0]
    boot_cluster = (cluster_hi << 16) | cluster_lo
    old_size = struct.unpack_from("<I", part, boot_entry_off + 28)[0]
    boot_rel_lba = data_rel_lba + (boot_cluster - 2) * spc

    return {
        "bps": bps,
        "spc": spc,
        "reserved": reserved,
        "fats": fats,
        "spf": spf,
        "data_rel_lba": data_rel_lba,
        "root_end_rel_lba": root_rel_lba + spc,
        "boot_entry_off": boot_entry_off,
        "boot_cluster": boot_cluster,
        "boot_rel_lba": boot_rel_lba,
        "old_size": old_size,
    }


def fat_entry(part: bytearray, fat_base: int, cluster: int) -> int:
    return struct.unpack_from("<I", part, fat_base + cluster * 4)[0] & 0x0FFFFFFF


def set_fat_entry(part: bytearray, fat_base: int, cluster: int, value: int) -> None:
    struct.pack_into("<I", part, fat_base + cluster * 4, value & 0x0FFFFFFF)


def patch_boot_metadata(part: bytearray, info: dict, boot_size: int) -> int:
    cluster_bytes = info["spc"] * info["bps"]
    clusters_needed = (boot_size + cluster_bytes - 1) // cluster_bytes
    fat1 = info["reserved"] * info["bps"]
    fat2 = fat1 + info["spf"] * info["bps"]

    chain = []
    seen = set()
    cur = info["boot_cluster"]
    while cur < 0x0FFFFFF8:
        if cur in seen:
            raise RuntimeError("BOOT.BIN FAT chain loops")
        seen.add(cur)
        chain.append(cur)
        cur = fat_entry(part, fat1, cur)
        if cur == 0:
            break

    if clusters_needed > len(chain):
        raise RuntimeError(
            f"new BOOT.bin needs {clusters_needed} clusters, strict image chain has {len(chain)}"
        )

    struct.pack_into("<I", part, info["boot_entry_off"] + 28, boot_size)
    for fat in (fat1, fat2):
        for i, cluster in enumerate(chain):
            if i < clusters_needed - 1:
                set_fat_entry(part, fat, cluster, chain[i + 1])
            elif i == clusters_needed - 1:
                set_fat_entry(part, fat, cluster, 0x0FFFFFFF)
            else:
                set_fat_entry(part, fat, cluster, 0)

    metadata_bytes = info["root_end_rel_lba"] * info["bps"]
    with open(META_PATCHED, "wb") as f:
        f.write(part[:metadata_bytes])
    return metadata_bytes


def xsdb_run(script: str, timeout: int = 120) -> subprocess.CompletedProcess:
    return subprocess.run([XSDB], input=script, text=True, capture_output=True, timeout=timeout)


def load_start_and_poll(boot_bin: str, boot_size: int, metadata_bytes: int, boot_lba: int):
    """Single xsdb session: load ELF, start it, wait 60 s, halt, read scratch once.
    One session avoids FT2232H multi-session degradation.
    60 s >> observed SD write time (~10 s).
    """
    tcl = (
        "connect\n"
        "after 3000\n"
        f'targets -set -filter {{name =~ "APU*"}}\n'
        "rst -system\n"
        "after 500\n"
        f"source {PS7_INIT}\n"
        "ps7_init\n"
        'puts "-- ps7_init done --"\n'
        f'targets -set -filter {{name =~ "ARM*Cortex-A9 MPCore #0"}}\n'
        "stop\n"
        "after 200\n"
        'puts "-- dow strict MBR --"\n'
        f"dow -data {MBR_BIN} 0x{DDR_MBR:08x}\n"
        f'puts "-- dow strict metadata ({metadata_bytes:,} B) --"\n'
        f"dow -data {META_PATCHED} 0x{DDR_PART:08x}\n"
        f'puts "-- dow BOOT.bin ({boot_size:,} B) --"\n'
        f"dow -data {boot_bin} 0x{DDR_BOOT:08x}\n"
        # dow ELF — xsdb sets the CPU entry point via the download protocol,
        # not via DAP rwr (which doesn't work before con in this board state).
        # mwr -force (writes) work fine; mrd (reads) and rwr pc do not.
        f'puts "-- dow sd_boot_image_writer.elf --"\n'
        f"dow {SDBI_ELF}\n"
        f"mwr -force 0x{S_STATUS:08x} 0\n"
        f"mwr -force 0x{S_BOOT_BYTES:08x} 0x{boot_size:08x}\n"
        f"mwr -force 0x{S_PART_LBA:08x} 0x{PART_LBA:08x}\n"
        f"mwr -force 0x{S_PART_BYTES:08x} 0x{metadata_bytes:08x}\n"
        f"mwr -force 0x{S_BOOT_LBA:08x} 0x{boot_lba:08x}\n"
        'puts "-- con --"\n'
        "con\n"
        'puts "-- CPU running, waiting 60 s --"\n'
        "after 60000\n"
        "stop\n"
        "after 300\n"
        'puts "-- reading scratch + CPU state --"\n'
        'puts "PC=[rrd pc]"\n'
        f'set v [mrd -value -force 0x{ELF_ENTRY:08x} 1]; puts "ELF0=0x[format %08x $v]"\n'
        f'set v [mrd -value -force 0x{S_STATUS:08x} 1]; puts "STATUS=0x[format %08x $v]"\n'
        f'set v [mrd -value -force 0x{S_PHASE:08x}  1]; puts "PHASE=0x[format %02x $v]"\n'
        f'set v [mrd -value -force 0x{S_LBA:08x}    1]; puts "LBA=0x[format %08x $v]"\n'
        f'set v [mrd -value -force 0x{S_BYTES:08x}  1]; puts "BYTES=[format %d $v]"\n'
        f'set v [mrd -value -force 0x{S_ERR:08x}    1]; puts "ERR=0x[format %08x $v]"\n'
        "disconnect\n"
        "exit\n"
    )
    return xsdb_run(tcl, timeout=120)


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
    for line in r.stdout.splitlines():
        m = re.match(r"(\w+)=0x([0-9a-f]+)", line.strip())
        if m:
            out[m.group(1)] = int(m.group(2), 16)
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "boot_bin",
        nargs="?",
        default=f"{ZYNQ_DIR}/fsbl_build/BOOT.bin",
        help="BOOT.bin to write; defaults to fsbl_build/BOOT.bin",
    )
    ap.add_argument("--dry-run", action="store_true",
                    help="patch metadata and print geometry, but do not run xsdb")
    args = ap.parse_args()

    if not os.path.exists(args.boot_bin):
        print(f"missing BOOT.bin: {args.boot_bin}")
        return 1
    if not os.path.exists(STRICT_PART_BIN):
        print(f"missing strict FAT32 image: {STRICT_PART_BIN}")
        return 1

    with open(STRICT_PART_BIN, "rb") as f:
        part = bytearray(f.read())
    info = parse_strict_image(part)
    boot_size = os.path.getsize(args.boot_bin)
    metadata_bytes = patch_boot_metadata(part, info, boot_size)
    boot_lba = PART_LBA + info["boot_rel_lba"]
    build_mbr()

    print("=== Strict-FAT32 BOOT.bin swap ===")
    print(f"  Strict image: {STRICT_PART_BIN}")
    print(f"  BOOT.bin:     {args.boot_bin} ({boot_size:,} B)")
    print(f"  Old size:     {info['old_size']:,} B")
    print(f"  BOOT cluster: {info['boot_cluster']}  abs LBA: {boot_lba}")
    print(f"  Metadata:     {metadata_bytes:,} B -> {META_PATCHED}")
    if args.dry_run:
        return 0

    print("\n--- Load + start + poll (single JTAG session) ---")
    t0 = time.time()
    r = load_start_and_poll(args.boot_bin, boot_size, metadata_bytes, boot_lba)
    elapsed = time.time() - t0

    # Print all xsdb output
    scratch = {}
    for line in r.stdout.splitlines():
        ls = line.strip()
        if ls:
            print(f"  {ls}")
        # Parse STATUS=, PHASE=, LBA=, BYTES=, ERR= lines
        for key in ("STATUS", "PHASE", "LBA", "ERR"):
            if ls.startswith(f"{key}=0x"):
                try:
                    scratch[key] = int(ls.split("=")[1], 16)
                except ValueError:
                    pass
        if ls.startswith("BYTES="):
            try:
                scratch["BYTES"] = int(ls.split("=")[1])
            except ValueError:
                pass

    if r.returncode != 0:
        print(f"  xsdb exit {r.returncode}")
        if r.stderr:
            print(f"  stderr: {r.stderr[-500:]}")

    print(f"  total: {elapsed:.0f}s")

    status = scratch.get("STATUS", 0)
    phase  = scratch.get("PHASE", 0)
    phase_name = PHASE_NAMES.get(phase, f"0x{phase:02x}")

    if status == DONE_MAGIC:
        print(f"\n  *** SUCCESS — strict BOOT.bin written ***  (phase={phase_name})")
        print("\nNEXT: power-cycle into SD mode (switches 11), listen on /dev/ttyUSB2 @ 115200")
        return 0
    if 0xBAD20001 <= status <= DONE_MAGIC:
        name = ERR_NAMES.get(status, "unknown")
        print(f"\n  *** FAIL: {name} (STATUS=0x{status:08x}, phase={phase_name}) ***")
        return 1
    if status == 0:
        print(f"\n  STATUS=0 — ELF never ran or scratch not written (board/JTAG issue)")
        return 1
    print(f"\n  Unexpected STATUS=0x{status:08x} (phase={phase_name}) — check board")
    return 1


if __name__ == "__main__":
    sys.exit(main())
