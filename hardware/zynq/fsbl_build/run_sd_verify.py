#!/usr/bin/env python3
"""Run sd_verify.elf on Cortex-A9, dump SD sectors via JTAG, compare to staged metadata."""
import os, subprocess, sys, time, re, hashlib

ZYNQ_DIR = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB     = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
SDV_ELF  = f"{ZYNQ_DIR}/ps_loader/build/sd_verify.elf"

# Expected sources for comparison
EXPECTS = [
    ("LBA 0    MBR",  0x10200000, "/tmp/sd_meta_mbr.bin",                                            0),
    ("LBA 8192 VBR",  0x10201000, "/tmp/sd_meta_partition.bin",                                      0),
    ("LBA 8224 FAT1", 0x10202000, "/tmp/sd_meta_partition.bin",                                  32*512),
    ("LBA 8256 RDIR", 0x10203000, "/tmp/sd_meta_partition.bin",                                  64*512),
    ("LBA 8320 BOOT", 0x10204000, f"{ZYNQ_DIR}/fsbl_build/BOOT.bin",                                 0),
]
SCRATCH = 0x10100000
DONE_MAGIC = 0xC0DEC0DE


def xsdb_run(script, timeout=120):
    r = subprocess.run([XSDB], input=script, text=True, capture_output=True, timeout=timeout)
    return r


def load_and_run():
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
dow {SDV_ELF}
mwr -force 0x{SCRATCH:08x} 0
con
puts "-- running --"
disconnect
exit
""")


def poll_status():
    return xsdb_run(f"""
connect
after 1500
targets -set -filter {{name =~ "APU*"}}
set v [mrd -value -force 0x{SCRATCH+0x00:08x} 1]; puts "STATUS=0x[format %08x $v]"
set v [mrd -value -force 0x{SCRATCH+0x04:08x} 1]; puts "LBA=0x[format %08x $v]"
set v [mrd -value -force 0x{SCRATCH+0x08:08x} 1]; puts "ERR=0x[format %08x $v]"
set v [mrd -value -force 0x{SCRATCH+0x18:08x} 1]; puts "DONE=0x[format %08x $v]"
disconnect
exit
""", timeout=30)


def dump_sector(ddr_addr):
    """Read 512 bytes from DDR via JTAG; return as bytes."""
    script = f"""
connect
after 1500
targets -set -filter {{name =~ "APU*"}}
"""
    # 128 dwords
    for i in range(128):
        addr = ddr_addr + i*4
        script += f'set v [mrd -value -force 0x{addr:08x} 1]; puts "D{i:03d}=0x[format %08x $v]"\n'
    script += "disconnect\nexit\n"
    r = xsdb_run(script, timeout=120)
    words = []
    for line in r.stdout.splitlines():
        m = re.match(r"D(\d{3})=0x([0-9a-f]{8})", line.strip())
        if m: words.append(int(m.group(2), 16))
    if len(words) != 128:
        raise RuntimeError(f"only got {len(words)} dwords from JTAG")
    # Convert little-endian dwords back to bytes
    out = bytearray()
    for w in words:
        out += w.to_bytes(4, "little")
    return bytes(out)


def main():
    print("=== sd_verify JTAG runner ===")
    print(f"sd_verify.elf: {os.path.getsize(SDV_ELF):,} B")

    print("\n--- Load + start ---")
    r = load_and_run()
    if r.returncode != 0:
        print("LOAD FAILED:", r.stdout[-500:], r.stderr[-500:]); sys.exit(1)
    for ln in r.stdout.splitlines():
        if ln.strip().startswith("--"): print(" ", ln.strip())

    print("\n--- Polling ---")
    deadline = time.time() + 60
    while time.time() < deadline:
        r = poll_status()
        m = {}
        for ln in r.stdout.splitlines():
            mm = re.match(r"(\w+)=0x([0-9a-f]+)", ln.strip())
            if mm: m[mm.group(1)] = int(mm.group(2), 16)
        status = m.get("STATUS", 0)
        print(f"  STATUS=0x{status:08x} LBA=0x{m.get('LBA',0):08x} "
              f"ERR=0x{m.get('ERR',0):08x} DONE=0x{m.get('DONE',0):08x}")
        if status == DONE_MAGIC:
            print("  *** sd_verify completed ***")
            break
        if status >= 0xBAD10002 and status < DONE_MAGIC:
            print(f"  *** sd_verify FAILED (status=0x{status:08x}) ***")
            sys.exit(1)
        time.sleep(2)
    else:
        print("  *** TIMEOUT ***"); sys.exit(1)

    print("\n--- Reading back 5 sectors via JTAG ---")
    results = []
    for tag, ddr, src_file, src_off in EXPECTS:
        print(f"  reading {tag} from 0x{ddr:08x}...")
        got = dump_sector(ddr)
        with open(src_file, "rb") as f:
            f.seek(src_off)
            want = f.read(512)
        if want == got:
            print(f"    ✓ MATCH ({tag})")
            results.append((tag, "MATCH", None))
        else:
            # find first differing byte
            diff_off = next((i for i in range(512) if got[i] != want[i]), -1)
            print(f"    ✗ MISMATCH at offset 0x{diff_off:03x}: got=0x{got[diff_off]:02x} want=0x{want[diff_off]:02x}")
            results.append((tag, "MISMATCH", diff_off))
        # Save raw read for further inspection
        with open(f"/tmp/sd_read_{tag.split()[1]}.bin", "wb") as f:
            f.write(got)

    print("\n=== SUMMARY ===")
    for tag, result, diff in results:
        if result == "MATCH":
            print(f"  {tag}: ✓ MATCH")
        else:
            print(f"  {tag}: ✗ MISMATCH (first diff at offset 0x{diff:03x})")


if __name__ == "__main__":
    main()
