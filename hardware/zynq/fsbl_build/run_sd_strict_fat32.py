#!/usr/bin/env python3
"""Write a strict-FAT32 partition (512 MB / 4 KB clusters / 130K clusters)
to the SD card. Only the allocated sectors (~5 MB) are actually written —
the rest of the 512 MB partition is left as garbage. BootROM only ever
reads what BPB tells it about.

This is the cluster-count test:
- 64 MB / 32 KB clusters (previous tests) = 2046 clusters (sub-FAT32)
- 512 MB / 4 KB clusters (this test)     = 130811 clusters (strict FAT32)

If this STILL fails identically to the previous tests, cluster count is
ruled out and we're looking at card or hardware path.
"""
import argparse, os, re, struct, subprocess, sys, time

ZYNQ_DIR = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB     = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
SDBI_ELF = f"{ZYNQ_DIR}/ps_loader/build/sd_boot_image_writer.elf"

MBR_BIN  = "/tmp/sd_meta_strict_mbr.bin"
PART_BIN = os.environ.get(
    "STRICT_PART_BIN",
    "/tmp/fat_uart1/strict_partition.bin")

DDR_MBR  = 0x14000000
DDR_PART = 0x14001000  # 5.14 MB → ends at ~0x144E7000

SCRATCH_BASE = 0x10100000
S_STATUS     = SCRATCH_BASE + 0x00
S_LBA        = SCRATCH_BASE + 0x04
S_BYTES      = SCRATCH_BASE + 0x08
S_ERR        = SCRATCH_BASE + 0x0C
S_PHASE      = SCRATCH_BASE + 0x20
S_BOOT_BYTES = SCRATCH_BASE + 0x24
S_PART_LBA   = SCRATCH_BASE + 0x28
S_PART_BYTES = SCRATCH_BASE + 0x2C
S_BOOT_LBA   = SCRATCH_BASE + 0x30

PART_LBA   = 8192
DONE_MAGIC = 0xC0DEC0DE

PHASE_NAMES = {
    0x10: "controller_init", 0x20: "card_init",
    0x30: "test_write", 0x31: "test_read", 0x32: "test_compare",
    0x40: "write_MBR", 0x41: "write_FAT32_metadata",
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


def build_mbr():
    """Create an MBR pointing to a 512 MB FAT32 partition at LBA 8192."""
    mbr = bytearray(512)
    # bytes 0x1BE..0x1FD = partition entry
    e = mbr  # write at offset 0x1BE
    o = 0x1BE
    e[o + 0x00] = 0x80      # bootable
    e[o + 0x01] = 0         # CHS start head
    e[o + 0x02] = 0
    e[o + 0x03] = 0
    e[o + 0x04] = 0x0C      # FAT32 LBA
    e[o + 0x05] = 0
    e[o + 0x06] = 0
    e[o + 0x07] = 0
    # LBA start = 8192
    struct.pack_into("<I", e, o + 0x08, 8192)
    # LBA sectors = 1048576 (512 MB)
    struct.pack_into("<I", e, o + 0x0C, 1048576)
    # boot signature
    mbr[0x1FE] = 0x55
    mbr[0x1FF] = 0xAA
    with open(MBR_BIN, "wb") as f:
        f.write(bytes(mbr))
    print(f"  Wrote {MBR_BIN}: 512 B (partition LBA 8192, 1,048,576 sectors)")


def xsdb_run(script, timeout=600):
    return subprocess.run([XSDB], input=script, text=True,
                          capture_output=True, timeout=timeout)


def load_and_start(part_bytes):
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
puts "-- dow sparse partition ({part_bytes:,} B) --"
dow -data {PART_BIN} 0x{DDR_PART:08x}
puts "-- dow sd_boot_image_writer.elf --"
dow {SDBI_ELF}
mwr -force 0x{S_STATUS:08x} 0
mwr -force 0x{S_BOOT_BYTES:08x} 0
mwr -force 0x{S_PART_LBA:08x} 0x{PART_LBA:08x}
mwr -force 0x{S_PART_BYTES:08x} 0x{part_bytes:08x}
mwr -force 0x{S_BOOT_LBA:08x} 0
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
    if not os.path.exists(PART_BIN):
        print(f"missing: {PART_BIN}"); sys.exit(1)
    build_mbr()
    part_bytes = os.path.getsize(PART_BIN)

    print(f"=== Strict-FAT32 sparse writer ===")
    print(f"  Strategy: 512 MB / 4 KB cluster FAT32, ~130K clusters (strict)")
    print(f"  Sparse partition: {PART_BIN} ({part_bytes:,} B = {part_bytes//1024//1024} MB)")
    print(f"  PART_LBA: {PART_LBA}  PART_BYTES: {part_bytes:,}")

    print("\n--- Load + start ---")
    t0 = time.time()
    r = load_and_start(part_bytes)
    for ln in r.stdout.splitlines():
        if ln.strip().startswith("--") or "error" in ln.lower():
            print(f"  {ln.strip()}")
    if r.returncode != 0:
        print("LOAD FAILED:", r.stdout[-1000:], r.stderr[-500:]); sys.exit(1)
    print(f"  took {time.time()-t0:.0f}s")

    print("\n--- Polling ---")
    last = -1
    deadline = time.time() + 600
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
            print(f"\n  *** SUCCESS — strict-FAT32 layout written ***")
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
