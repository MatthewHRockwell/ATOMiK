#!/usr/bin/env python3
"""Write the full mkfs.vfat FAT32 image to the SD card with no surgical patches.

This eliminates the hand-built FAT metadata entirely. The image at
/tmp/fat32_mkfs_full.img is the byte-for-byte output of mkfs.vfat on a 64 MB
loopback, with BOOT.bin already copied inside (cluster 3 = LBA 256 within
partition). The only patch is hidden_sectors=8192 (to reflect the MBR offset)
and that's done at file-creation time, not in flight.

Steps:
    1. JTAG ps7_init.
    2. dow MBR (512 B) to 0x14000000.
    3. dow 64 MB FAT image to 0x14001000 — biggest single dow we've done.
    4. Run sd_boot_image_writer.elf with PART_BYTES=64 MB, BOOT_BYTES=0.
    5. Poll until done.
"""
import os, re, subprocess, sys, time

ZYNQ_DIR = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB     = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
SDBI_ELF = f"{ZYNQ_DIR}/ps_loader/build/sd_boot_image_writer.elf"
MBR_BIN  = "/tmp/sd_meta_mbr.bin"
FULL_IMG = "/tmp/fat32_mkfs_full.img"

DDR_MBR  = 0x14000000
DDR_PART = 0x14001000   # 64 MB image → ends at 0x18001000, well before 0x18100000

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

PART_LBA   = 8192
PART_BYTES = 64 * 1024 * 1024   # full 64 MB image
DONE_MAGIC = 0xC0DEC0DE

PHASE_NAMES = {
    0x10: "controller_init", 0x20: "card_init",
    0x30: "test_write", 0x31: "test_read", 0x32: "test_compare",
    0x40: "write_MBR", 0x41: "write_FAT32_metadata",
    0x42: "write_BOOT.bin", 0xFF: "DONE",
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


def xsdb_run(script, timeout=900):
    return subprocess.run([XSDB], input=script, text=True,
                          capture_output=True, timeout=timeout)


def load_and_start():
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
puts "-- dow full FAT image (64 MB, ~2-3 minutes) --"
dow -data {FULL_IMG} 0x{DDR_PART:08x}
puts "-- dow sd_boot_image_writer.elf --"
dow {SDBI_ELF}
mwr -force 0x{S_STATUS:08x} 0
mwr -force 0x{S_BOOT_BYTES:08x} 0
mwr -force 0x{S_PART_LBA:08x} 0x{PART_LBA:08x}
mwr -force 0x{S_PART_BYTES:08x} 0x{PART_BYTES:08x}
mwr -force 0x{S_BOOT_LBA:08x} 0
con
puts "-- CPU running --"
disconnect
exit
""", timeout=900)


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
    for p in (MBR_BIN, FULL_IMG, SDBI_ELF):
        if not os.path.exists(p):
            print(f"missing: {p}"); sys.exit(1)
    print(f"=== Full mkfs.vfat image SD writer ===")
    print(f"  MBR:        {MBR_BIN}     ({os.path.getsize(MBR_BIN):,} B)")
    print(f"  Full image: {FULL_IMG} ({os.path.getsize(FULL_IMG):,} B)")
    print(f"  PART_LBA:   {PART_LBA}")
    print(f"  PART_BYTES: {PART_BYTES:,}")
    print(f"  No surgical patches in flight — image is mkfs.vfat output verbatim")

    print("\n--- Load + start (this takes ~3 minutes for the 64 MB dow) ---")
    t0 = time.time()
    r = load_and_start()
    for ln in r.stdout.splitlines():
        if ln.strip().startswith("--") or "error" in ln.lower():
            print(f"  {ln.strip()}")
    if r.returncode != 0:
        print("LOAD FAILED:", r.stdout[-1500:], r.stderr[-500:]); sys.exit(1)
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
            print(f"\n  *** SUCCESS — full mkfs.vfat image written ***")
            return 0
        if 0xBAD20001 <= status < DONE_MAGIC:
            name = ERR_NAMES.get(status, "unknown")
            print(f"\n  *** FAIL: {name} (err 0x{m.get('ERR',0):08x}) ***")
            return 1
        time.sleep(3)
    print("\n  *** TIMEOUT ***")
    return 1


if __name__ == "__main__":
    sys.exit(main())
