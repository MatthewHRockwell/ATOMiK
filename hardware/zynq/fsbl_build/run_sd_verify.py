#!/usr/bin/env python3
"""Run sd_verify.elf, dump physical SD sectors via JTAG, and compare to strict-FAT expected bytes.

This verifier is intentionally tied to the strict FAT32 SD-boot layout used by
run_sd_strict_boot_swap.py. It does not trust staged metadata alone: it asks the
Cortex-A9 helper to read physical SD LBAs back into DDR, then JTAG-dumps those
sectors and compares them on the host.
"""
import argparse
import math
import os
import re
import struct
import subprocess
import sys
import time
from pathlib import Path

ZYNQ_DIR = Path(__file__).resolve().parents[1]
XSDB = os.environ.get("XSDB", "/opt/Xilinx/2025.2/Vivado/bin/xsdb")
PS7_INIT = ZYNQ_DIR / "scripts/ps7_init_rk7020f.tcl"
SDV_ELF = ZYNQ_DIR / "ps_loader/build/sd_verify.elf"
STRICT_SWAP = ZYNQ_DIR / "fsbl_build/run_sd_strict_boot_swap.py"
DEFAULT_BOOT_BIN = ZYNQ_DIR / "fsbl_build/BOOT_l2off.bin"

PART_LBA = 8192
SECTOR_SIZE = 512
SCRATCH = 0x10100000
S_STATUS = SCRATCH + 0x00
S_LBA = SCRATCH + 0x04
S_ERR = SCRATCH + 0x08
S_JOB_INDEX = SCRATCH + 0x0C
S_DONE = SCRATCH + 0x18
S_JOB_COUNT = SCRATCH + 0x20
S_HASH_LBA = SCRATCH + 0x24
S_HASH_BYTES = SCRATCH + 0x28
S_HASH_RESULT = SCRATCH + 0x2C
S_HASH_DONE_BYTES = SCRATCH + 0x30
S_JOB_TABLE = SCRATCH + 0x40
DDR_BASE = 0x10200000
DDR_STRIDE = 0x1000
DONE_MAGIC = 0xC0DEC0DE
FNV1A_OFFSET = 2166136261
FNV1A_PRIME = 16777619

MBR_BIN = Path("/tmp/sd_meta_strict_mbr.bin")
META_PATCHED = Path("/tmp/sd_strict_boot_meta_patched.bin")


def run(cmd, timeout=120):
    return subprocess.run(cmd, text=True, capture_output=True, timeout=timeout)


def xsdb_run(script, timeout=120):
    return subprocess.run([XSDB], input=script, text=True, capture_output=True, timeout=timeout)


def refresh_expected(boot_bin: Path):
    r = run([sys.executable, str(STRICT_SWAP), str(boot_bin), "--dry-run"], timeout=60)
    if r.returncode != 0:
        sys.stdout.write(r.stdout)
        sys.stderr.write(r.stderr)
        raise RuntimeError("failed to refresh strict-FAT expected metadata")
    return r.stdout



def fnv1a32(data: bytes) -> int:
    h = FNV1A_OFFSET
    for b in data:
        h ^= b
        h = (h * FNV1A_PRIME) & 0xFFFFFFFF
    return h

def le16(b, off):
    return struct.unpack_from("<H", b, off)[0]


def le32(b, off):
    return struct.unpack_from("<I", b, off)[0]


def parse_expected(boot_bin: Path):
    part = META_PATCHED.read_bytes()
    boot = boot_bin.read_bytes()
    if len(part) < SECTOR_SIZE or part[510:512] != b"\x55\xaa":
        raise RuntimeError(f"bad FAT VBR in {META_PATCHED}")

    bps = le16(part, 11)
    spc = part[13]
    reserved = le16(part, 14)
    fats = part[16]
    spf = le32(part, 36)
    root_cluster = le32(part, 44)
    if bps != SECTOR_SIZE or spc == 0 or fats != 2:
        raise RuntimeError(f"unexpected FAT geometry: bps={bps} spc={spc} fats={fats}")

    data_rel_lba = reserved + fats * spf
    root_rel_lba = data_rel_lba + (root_cluster - 2) * spc
    root_off = root_rel_lba * bps
    root_len = spc * bps

    entry_off = None
    for off in range(root_off, root_off + root_len, 32):
        if part[off:off + 11] == b"BOOT    BIN":
            entry_off = off
            break
    if entry_off is None:
        raise RuntimeError("BOOT.BIN entry not found in expected root directory")

    boot_cluster = (le16(part, entry_off + 20) << 16) | le16(part, entry_off + 26)
    dir_size = le32(part, entry_off + 28)
    boot_rel_lba = data_rel_lba + (boot_cluster - 2) * spc
    boot_abs_lba = PART_LBA + boot_rel_lba
    boot_size = len(boot)
    if dir_size != boot_size:
        raise RuntimeError(f"directory BOOT.BIN size {dir_size} != file size {boot_size}")

    fat1_off = reserved * bps
    chain = []
    seen = set()
    cur = boot_cluster
    while cur < 0x0FFFFFF8:
        if cur in seen:
            raise RuntimeError("BOOT.BIN FAT chain loops")
        seen.add(cur)
        chain.append(cur)
        nxt = le32(part, fat1_off + cur * 4) & 0x0FFFFFFF
        if nxt == 0:
            break
        cur = nxt
    clusters_needed = math.ceil(boot_size / (spc * bps))
    if len(chain) < clusters_needed:
        raise RuntimeError(f"BOOT.BIN needs {clusters_needed} clusters, chain has {len(chain)}")

    contiguous = all(chain[i + 1] == chain[i] + 1 for i in range(min(len(chain), clusters_needed) - 1))
    file_sectors = math.ceil(boot_size / SECTOR_SIZE)
    last_file_lba = boot_abs_lba + file_sectors - 1
    last_file_len = boot_size - (file_sectors - 1) * SECTOR_SIZE
    if last_file_len == 0:
        last_file_len = SECTOR_SIZE
    last_cluster = chain[clusters_needed - 1]
    chain_tail_lba = PART_LBA + data_rel_lba + (last_cluster - 2) * spc + (spc - 1)

    return {
        "part": part,
        "boot": boot,
        "bps": bps,
        "spc": spc,
        "reserved": reserved,
        "fats": fats,
        "spf": spf,
        "data_rel_lba": data_rel_lba,
        "root_cluster": root_cluster,
        "root_rel_lba": root_rel_lba,
        "root_abs_lba": PART_LBA + root_rel_lba,
        "fat1_rel_lba": reserved,
        "fat1_abs_lba": PART_LBA + reserved,
        "fat2_rel_lba": reserved + spf,
        "fat2_abs_lba": PART_LBA + reserved + spf,
        "boot_cluster": boot_cluster,
        "boot_abs_lba": boot_abs_lba,
        "boot_size": boot_size,
        "file_sectors": file_sectors,
        "last_file_lba": last_file_lba,
        "last_file_len": last_file_len,
        "clusters_needed": clusters_needed,
        "chain_len": len(chain),
        "chain_tail_lba": chain_tail_lba,
        "contiguous": contiguous,
    }


def make_jobs(info):
    jobs = []

    def add(tag, lba, source, offset=0, compare_len=SECTOR_SIZE, required=True):
        jobs.append({
            "tag": tag,
            "lba": lba,
            "ddr": DDR_BASE + len(jobs) * DDR_STRIDE,
            "source": source,
            "offset": offset,
            "compare_len": compare_len,
            "required": required,
        })

    add("MBR", 0, "mbr", 0)
    add("VBR", PART_LBA, "meta", 0)
    add("FAT1_0", info["fat1_abs_lba"], "meta", info["fat1_rel_lba"] * SECTOR_SIZE)
    add("FAT2_0", info["fat2_abs_lba"], "meta", info["fat2_rel_lba"] * SECTOR_SIZE)
    add("ROOT", info["root_abs_lba"], "meta", info["root_rel_lba"] * SECTOR_SIZE)
    add("BOOT_FIRST", info["boot_abs_lba"], "boot", 0)
    if info["last_file_lba"] != info["boot_abs_lba"]:
        add("BOOT_LAST_FILE", info["last_file_lba"], "boot",
            (info["file_sectors"] - 1) * SECTOR_SIZE, info["last_file_len"])
    if info["chain_tail_lba"] not in {j["lba"] for j in jobs}:
        add("BOOT_CHAIN_TAIL", info["chain_tail_lba"], None, 0, 0, required=False)
    if len(jobs) > 16:
        raise RuntimeError("too many sd_verify jobs")
    return jobs


def load_and_run(jobs, info, reset=True):
    reset_block = "rst -system\nafter 500\n" if reset else ""
    job_lines = [f"mwr -force 0x{S_JOB_COUNT:08x} 0x{len(jobs):08x}"]
    for i, job in enumerate(jobs):
        base = S_JOB_TABLE + i * 8
        job_lines.append(f"mwr -force 0x{base:08x} 0x{job['lba']:08x}")
        job_lines.append(f"mwr -force 0x{base + 4:08x} 0x{job['ddr']:08x}")

    script = f"""
connect
after 3000
targets -set -filter {{name =~ "APU*"}}
{reset_block}source {PS7_INIT}
ps7_init
puts "-- ps7_init done --"
targets -set -filter {{name =~ "ARM*Cortex-A9 MPCore #0"}}
stop
after 200
dow {SDV_ELF}
mwr -force 0x{SCRATCH:08x} 0
mwr -force 0x{S_DONE:08x} 0
mwr -force 0x{S_HASH_LBA:08x} 0x{info["boot_abs_lba"]:08x}
mwr -force 0x{S_HASH_BYTES:08x} 0x{info["boot_size"]:08x}
mwr -force 0x{S_HASH_RESULT:08x} 0
mwr -force 0x{S_HASH_DONE_BYTES:08x} 0
{chr(10).join(job_lines)}
con
puts "-- running --"
disconnect
exit
"""
    return xsdb_run(script, timeout=120)


def poll_status():
    r = xsdb_run(f"""
connect
after 1500
targets -set -filter {{name =~ "APU*"}}
set v [mrd -value -force 0x{S_STATUS:08x} 1]; puts "STATUS=0x[format %08x $v]"
set v [mrd -value -force 0x{S_LBA:08x} 1]; puts "LBA=0x[format %08x $v]"
set v [mrd -value -force 0x{S_ERR:08x} 1]; puts "ERR=0x[format %08x $v]"
set v [mrd -value -force 0x{S_DONE:08x} 1]; puts "DONE=0x[format %08x $v]"
set v [mrd -value -force 0x{S_JOB_INDEX:08x} 1]; puts "JOB=0x[format %08x $v]"
set v [mrd -value -force 0x{S_HASH_RESULT:08x} 1]; puts "HASH=0x[format %08x $v]"
set v [mrd -value -force 0x{S_HASH_DONE_BYTES:08x} 1]; puts "HBYTES=0x[format %08x $v]"
disconnect
exit
""", timeout=30)
    out = {}
    for line in r.stdout.splitlines():
        m = re.match(r"(\w+)=0x([0-9a-fA-F]+)", line.strip())
        if m:
            out[m.group(1)] = int(m.group(2), 16)
    return out


def dump_sector(ddr_addr):
    lines = ["connect", "after 1500", "targets -set -filter {name =~ \"APU*\"}"]
    for i in range(128):
        addr = ddr_addr + i * 4
        lines.append(f"set v [mrd -value -force 0x{addr:08x} 1]; puts \"D{i:03d}=0x[format %08x $v]\"")
    lines += ["disconnect", "exit"]
    r = xsdb_run("\n".join(lines) + "\n", timeout=120)
    words = []
    for line in r.stdout.splitlines():
        m = re.match(r"D(\d{3})=0x([0-9a-fA-F]{8})", line.strip())
        if m:
            words.append(int(m.group(2), 16))
    if len(words) != 128:
        raise RuntimeError(f"only got {len(words)} dwords from JTAG")
    out = bytearray()
    for word in words:
        out += word.to_bytes(4, "little")
    return bytes(out)


def expected_bytes(job, info):
    if job["source"] == "mbr":
        src = MBR_BIN.read_bytes()
    elif job["source"] == "meta":
        src = info["part"]
    elif job["source"] == "boot":
        src = info["boot"]
    else:
        return None
    off = job["offset"]
    want = src[off:off + job["compare_len"]]
    if len(want) < job["compare_len"] and job["source"] != "boot":
        raise RuntimeError(f"expected source too short for {job['tag']}")
    return want


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("boot_bin", nargs="?", default=str(DEFAULT_BOOT_BIN),
                    help="BOOT.bin image expected on strict FAT layout")
    ap.add_argument("--no-refresh", action="store_true",
                    help="do not run run_sd_strict_boot_swap.py --dry-run first")
    ap.add_argument("--no-reset", action="store_true",
                    help="skip xsdb rst-system before running sd_verify.elf")
    ap.add_argument("--timeout", type=float, default=75.0)
    args = ap.parse_args()

    boot_bin = Path(args.boot_bin).resolve()
    if not boot_bin.exists():
        print(f"missing BOOT.bin: {boot_bin}")
        return 1
    if not SDV_ELF.exists():
        print(f"missing {SDV_ELF}; run: make -C {ZYNQ_DIR / 'ps_loader'} sd_verify")
        return 1

    print("=== strict SD physical readback verifier ===")
    print(f"BOOT expected: {boot_bin} ({boot_bin.stat().st_size:,} B)")
    if not args.no_refresh:
        out = refresh_expected(boot_bin)
        for line in out.splitlines():
            if "BOOT cluster" in line or "Metadata" in line or "Strict image" in line:
                print("  " + line.strip())

    info = parse_expected(boot_bin)
    print("Geometry:")
    print(f"  reserved={info['reserved']} spf={info['spf']} spc={info['spc']} data_rel_lba={info['data_rel_lba']}")
    print(f"  root_abs_lba={info['root_abs_lba']} boot_abs_lba={info['boot_abs_lba']} last_file_lba={info['last_file_lba']}")
    print(f"  boot_clusters={info['clusters_needed']} chain_len={info['chain_len']} contiguous={info['contiguous']} chain_tail_lba={info['chain_tail_lba']}")
    expected_hash = fnv1a32(info["boot"])
    print(f"  full_boot_fnv1a32=0x{expected_hash:08x} ({info['boot_size']:,} B)")

    jobs = make_jobs(info)
    print("Readback jobs:")
    for i, job in enumerate(jobs):
        print(f"  {i:02d}: {job['tag']:<16} LBA={job['lba']:<8} DDR=0x{job['ddr']:08x}")

    print("\n--- Load + start sd_verify.elf ---")
    r = load_and_run(jobs, info, reset=not args.no_reset)
    if r.returncode != 0:
        print(r.stdout[-2000:])
        print(r.stderr[-1000:], file=sys.stderr)
        return 1
    for line in r.stdout.splitlines():
        if line.strip().startswith("--"):
            print("  " + line.strip())

    print("\n--- Polling ---")
    deadline = time.time() + args.timeout
    while time.time() < deadline:
        m = poll_status()
        status = m.get("STATUS", 0)
        job_idx = m.get("JOB", 0)
        print(f"  STATUS=0x{status:08x} JOB={job_idx} LBA=0x{m.get('LBA',0):08x} ERR=0x{m.get('ERR',0):08x} DONE=0x{m.get('DONE',0):08x} HASH=0x{m.get('HASH',0):08x} HBYTES={m.get('HBYTES',0):,}")
        if status == DONE_MAGIC:
            break
        if 0xBAD10002 <= status < DONE_MAGIC:
            print("sd_verify.elf reported failure")
            return 1
        time.sleep(2)
    else:
        print("timeout waiting for sd_verify.elf")
        return 1

    final = poll_status()
    got_hash = final.get("HASH", 0)
    got_hbytes = final.get("HBYTES", 0)
    if got_hash != expected_hash or got_hbytes != info["boot_size"]:
        print(f"\nFAIL: full BOOT.BIN hash mismatch: got hash=0x{got_hash:08x} bytes={got_hbytes:,}; want hash=0x{expected_hash:08x} bytes={info['boot_size']:,}")
        return 1
    print(f"\nPASS: full BOOT.BIN physical SD hash matches expected (0x{got_hash:08x}, {got_hbytes:,} B)")

    print("\n--- JTAG dump + compare ---")
    failures = []
    import hashlib
    for job in jobs:
        got = dump_sector(job["ddr"])
        safe_tag = re.sub(r"[^A-Za-z0-9_.-]", "_", job["tag"])
        out_path = Path(f"/tmp/sd_read_{safe_tag}.bin")
        out_path.write_bytes(got)
        want = expected_bytes(job, info)
        if want is None:
            print(f"  {job['tag']:<16} LBA={job['lba']:<8} saved {out_path} sha256={hashlib.sha256(got).hexdigest()[:16]}")
            continue
        got_cmp = got[:len(want)]
        if got_cmp == want:
            print(f"  {job['tag']:<16} LBA={job['lba']:<8} MATCH ({len(want)} B)")
        else:
            diff = next((i for i, (a, b) in enumerate(zip(got_cmp, want)) if a != b), -1)
            print(f"  {job['tag']:<16} LBA={job['lba']:<8} MISMATCH at +0x{diff:03x}: got=0x{got_cmp[diff]:02x} want=0x{want[diff]:02x}")
            failures.append(job["tag"])

    if failures:
        print("\nFAIL: physical SD readback mismatches: " + ", ".join(failures))
        return 1
    print("\nPASS: physical SD readback matches strict FAT metadata and BOOT.BIN file bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
