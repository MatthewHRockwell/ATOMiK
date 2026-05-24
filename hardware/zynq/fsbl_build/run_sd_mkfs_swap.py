#!/usr/bin/env python3
"""Write a real-mkfs.vfat-produced FAT32 layout to the SD card, with BOOT.bin.

The hand-rolled metadata in /tmp/sd_meta_partition.bin uses non-standard FAT
sizes that BootROM appears to reject. This runner uses /tmp/sd_meta_partition_mkfs.bin
(produced by mkfs.vfat on a loopback image) as the source of truth.

mkfs.vfat layout for this 64 MB FAT32:
    sectors/cluster = 64 (32 KB)
    reserved sectors = 64
    sectors/FAT = 64
    2 FATs
    => FAT1 at partition_LBA + 64
       FAT2 at partition_LBA + 128
       data at partition_LBA + 192
       cluster 3 (BOOT.BIN) at partition_LBA + 256 = LBA 8448

Usage:
    python3 run_sd_mkfs_swap.py /path/to/BOOT.bin
"""
import argparse, os, re, subprocess, sys, time

ZYNQ_DIR = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB     = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
SDBI_ELF = f"{ZYNQ_DIR}/ps_loader/build/sd_boot_image_writer.elf"

MBR_BIN  = "/tmp/sd_meta_mbr.bin"
MKFS_BIN = "/tmp/sd_meta_partition_mkfs.bin"   # 128 KB metadata bundle

DDR_MBR  = 0x14000000
DDR_PART = 0x14001000   # 128 KB room: 0x14001000-0x14020FFF
DDR_BOOT = 0x18100000

SCRATCH_BASE  = 0x10100000
S_STATUS      = SCRATCH_BASE + 0x00
S_LBA         = SCRATCH_BASE + 0x04
S_BYTES       = SCRATCH_BASE + 0x08
S_ERR         = SCRATCH_BASE + 0x0C
S_PHASE       = SCRATCH_BASE + 0x20
S_BOOT_BYTES  = SCRATCH_BASE + 0x24
S_PART_LBA    = SCRATCH_BASE + 0x28
S_PART_BYTES  = SCRATCH_BASE + 0x2C
S_BOOT_LBA    = SCRATCH_BASE + 0x30

# mkfs.vfat-derived layout
PART_LBA   = 8192
PART_BYTES = 128 * 1024      # 256 sectors metadata
BOOT_LBA   = 8192 + 256      # cluster 3 in mkfs layout = LBA 8448

DONE_MAGIC = 0xC0DEC0DE

PHASE_NAMES = {
    0x10: "controller_init",
    0x20: "card_init",
    0x30: "test_write",
    0x31: "test_read",
    0x32: "test_compare",
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


def xsdb_run(script, timeout=600):
    return subprocess.run([XSDB], input=script, text=True,
                          capture_output=True, timeout=timeout)


def load_and_start(boot_bin_path, boot_size):
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
puts "-- dow mkfs partition ({PART_BYTES:,} B) --"
dow -data {MKFS_BIN} 0x{DDR_PART:08x}
puts "-- dow BOOT.bin ({boot_size:,} B) --"
dow -data {boot_bin_path} 0x{DDR_BOOT:08x}
puts "-- dow sd_boot_image_writer.elf --"
dow {SDBI_ELF}
mwr -force 0x{S_STATUS:08x} 0
mwr -force 0x{S_BOOT_BYTES:08x} 0x{boot_size:08x}
mwr -force 0x{S_PART_LBA:08x} 0x{PART_LBA:08x}
mwr -force 0x{S_PART_BYTES:08x} 0x{PART_BYTES:08x}
mwr -force 0x{S_BOOT_LBA:08x} 0x{BOOT_LBA:08x}
con
puts "-- CPU running --"
disconnect
exit
""", timeout=600)


def poll_status():
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
        if m: out[m.group(1)] = int(m.group(2), 16)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("boot_bin", help="Path to BOOT.bin to flash")
    args = ap.parse_args()
    if not os.path.exists(args.boot_bin):
        print(f"missing: {args.boot_bin}"); sys.exit(1)
    if not os.path.exists(MKFS_BIN):
        print(f"missing mkfs partition metadata: {MKFS_BIN}"); sys.exit(1)

    boot_size = os.path.getsize(args.boot_bin)
    n_sec = (boot_size + 511) // 512
    print(f"=== mkfs.vfat layout SD writer ===")
    print(f"  BOOT.bin:      {args.boot_bin}")
    print(f"  Size:          {boot_size:,} B  ({n_sec} sectors)")
    print(f"  Metadata:      {MKFS_BIN} ({os.path.getsize(MKFS_BIN):,} B)")
    print(f"  PART_LBA:      {PART_LBA}")
    print(f"  PART_BYTES:    {PART_BYTES:,}")
    print(f"  BOOT_LBA:      {BOOT_LBA}  (LBA range {BOOT_LBA}..{BOOT_LBA + n_sec - 1})")

    print("\n--- Load + start ---")
    t0 = time.time()
    r = load_and_start(args.boot_bin, boot_size)
    for ln in r.stdout.splitlines():
        if ln.strip().startswith("--") or "error" in ln.lower():
            print(f"  {ln.strip()}")
    if r.returncode != 0:
        print("LOAD FAILED:", r.stdout[-1000:], r.stderr[-500:]); sys.exit(1)
    print(f"  took {time.time()-t0:.0f}s")

    print("\n--- Polling ---")
    last = -1
    deadline = time.time() + 900
    while time.time() < deadline:
        m = poll_status()
        status = m.get("STATUS", 0)
        phase = m.get("PHASE", 0)
        if status != last or m.get("BYTES"):
            ph = PHASE_NAMES.get(phase, f"0x{phase:02x}")
            print(f"  STATUS=0x{status:08x} phase=0x{phase:02x}({ph:<20}) "
                  f"lba=0x{m.get('LBA',0):08x} bytes={m.get('BYTES',0):,}")
            last = status
        if status == DONE_MAGIC:
            print(f"\n  *** SUCCESS — mkfs.vfat layout written ***")
            print(f"\nNEXT: power-cycle into SD mode, then probe:")
            print(f"  python3 hardware/zynq/fsbl_build/probe_sd_boot_pc.py")
            return 0
        if 0xBAD20001 <= status < DONE_MAGIC:
            name = ERR_NAMES.get(status, "unknown")
            print(f"\n  *** FAIL: {name} (err 0x{m.get('ERR',0):08x}) ***")
            return 1
        time.sleep(2)
    print("\n  *** TIMEOUT ***")
    return 1


if __name__ == "__main__":
    sys.exit(main())
