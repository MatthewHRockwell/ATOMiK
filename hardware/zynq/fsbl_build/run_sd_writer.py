#!/usr/bin/env python3
"""Drive sd_writer.elf on Cortex-A9 over JTAG.

Steps:
  1. connect, rst -system, source+run ps7_init (clocks, pinmux, DDR).
  2. JTAG-dow BOOT.bin to PS 0x18100000.
  3. JTAG-dow nax64.bit.bin to PS 0x1C100000.
  4. JTAG-dow sd_writer.elf and start Cortex-A9 #0 at its entry.
  5. Poll scratch at PS 0x10100000 until status == DONE_MAGIC or BAD-code.

If the test-write/readback at the start of sd_writer passes, the bulk file
writes that follow are the same code path on the same hardware — so we know
the SD path is healthy before committing real data.
"""
import os, subprocess, sys, time, re

ZYNQ_DIR  = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB      = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT  = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
SDW_ELF   = f"{ZYNQ_DIR}/ps_loader/build/sd_writer.elf"
BOOT_BIN  = f"{ZYNQ_DIR}/fsbl_build/BOOT.bin"
NAX_BIT   = f"{ZYNQ_DIR}/litex-build-nax64-sdboot/gateware/hamgeek_rk7020f.bit.bin"
MBR_BIN   = "/tmp/sd_meta_mbr.bin"
PART_BIN  = "/tmp/sd_meta_partition.bin"

SRC_MBR_PS    = 0x14000000
SRC_PART_PS   = 0x14001000
SRC_BOOT_PS   = 0x18100000
SRC_NAX_PS    = 0x1C100000
SCRATCH_BASE  = 0x10100000
DONE_MAGIC    = 0xC0DEC0DE

PHASE_NAMES = {
    0x01: "boot",
    0x10: "controller_init",
    0x20: "card_init",
    0x30: "test_write",
    0x31: "test_read",
    0x32: "test_compare",
    0x40: "write_BOOT.bin",
    0x41: "write_nax64.bit.bin",
    0xFF: "done",
}

ERR_NAMES = {
    0xBAD00001: "BOOT (in-progress marker, not a failure)",
    0xBAD00002: "controller_init failed",
    0xBAD00003: "card_init failed",
    0xBAD00004: "self-test sector write failed",
    0xBAD00005: "self-test sector read failed",
    0xBAD00006: "self-test data mismatch",
    0xBAD00007: "BOOT.bin write failed mid-stream",
    0xBAD00008: "nax64.bit.bin write failed mid-stream",
    0xBAD00009: "MBR (LBA 0) write failed",
    0xBAD0000A: "FAT32 partition header (LBA 8192) write failed",
}

PHASE_NAMES.update({
    0x40: "write_MBR",
    0x41: "write_FAT32_header",
    0x42: "write_BOOT.bin",
    0x43: "write_nax64.bit.bin",
})


def build_load_script():
    """Generate the xsdb tcl that resets, loads, and starts sd_writer."""
    lines = [
        "connect", "after 4000",
        'targets -set -filter {name =~ "APU*"}',
        "rst -system", "after 500",
        f"source {PS7_INIT}", "ps7_init",
        'puts "-- ps7_init done --"',
        # Select A9 #0 for dow / con
        'targets -set -filter {name =~ "ARM*Cortex-A9 MPCore #0"}',
        "stop", "after 200",
        f'puts "-- dow MBR to 0x{SRC_MBR_PS:08x} --"',
        f"dow -data {MBR_BIN} 0x{SRC_MBR_PS:08x}",
        f'puts "-- dow partition header to 0x{SRC_PART_PS:08x} --"',
        f"dow -data {PART_BIN} 0x{SRC_PART_PS:08x}",
        f'puts "-- dow BOOT.bin to 0x{SRC_BOOT_PS:08x} --"',
        f"dow -data {BOOT_BIN} 0x{SRC_BOOT_PS:08x}",
        f'puts "-- dow nax64.bit.bin to 0x{SRC_NAX_PS:08x} --"',
        f"dow -data {NAX_BIT} 0x{SRC_NAX_PS:08x}",
        f'puts "-- dow sd_writer.elf --"',
        f"dow {SDW_ELF}",
        # Clear scratch before run
        f"mwr -force 0x{SCRATCH_BASE:08x} 0",
        "con",
        'puts "-- CPU running sd_writer --"',
        "disconnect", "exit",
    ]
    return "\n".join(lines)


def build_poll_script():
    """Generate a fresh xsdb tcl that reads scratch in one shot."""
    addrs = [SCRATCH_BASE + 0x00,  # STATUS
             SCRATCH_BASE + 0x04,  # LBA
             SCRATCH_BASE + 0x08,  # BYTES
             SCRATCH_BASE + 0x0C,  # ERR
             SCRATCH_BASE + 0x18,  # DONE_MAGIC
             SCRATCH_BASE + 0x20]  # PHASE
    lines = ["connect", "after 1500",
             'targets -set -filter {name =~ "APU*"}']
    for a in addrs:
        lines.append(
            f"set v [mrd -value -force 0x{a:08x} 1]; "
            f"puts \"R 0x{a:08x} = 0x[format %08x $v]\"")
    lines += ["disconnect", "exit"]
    return "\n".join(lines)


def run_xsdb(script, timeout=300):
    return subprocess.run([XSDB], input=script, text=True,
                          capture_output=True, timeout=timeout)


def parse_poll(out):
    vals = {}
    for ln in out.splitlines():
        m = re.match(r"R 0x([0-9a-f]+) = 0x([0-9a-f]+)", ln.strip())
        if m:
            vals[int(m.group(1), 16)] = int(m.group(2), 16)
    return vals


def main():
    print("=== sd_writer JTAG runner ===")
    for p in (MBR_BIN, PART_BIN, BOOT_BIN, NAX_BIT, SDW_ELF):
        if not os.path.exists(p):
            print(f"missing: {p}"); sys.exit(1)
        print(f"  {os.path.basename(p):<32} {os.path.getsize(p):>10,} B")

    print("\n--- Load + start ---")
    t0 = time.time()
    r = run_xsdb(build_load_script(), timeout=300)
    for ln in r.stdout.splitlines():
        if ln.strip().startswith("--") or "error" in ln.lower():
            print(f"  {ln.strip()}")
    if r.returncode != 0:
        print("LOAD FAILED stdout:", r.stdout[-1500:])
        print("LOAD FAILED stderr:", r.stderr[-1500:])
        sys.exit(1)
    print(f"  load+start took {time.time()-t0:.0f}s")

    print("\n--- Polling scratch via JTAG ---")
    last_status = -1
    deadline = time.time() + 600
    while time.time() < deadline:
        r = run_xsdb(build_poll_script(), timeout=30)
        vals = parse_poll(r.stdout)
        status = vals.get(SCRATCH_BASE + 0x00, 0)
        lba    = vals.get(SCRATCH_BASE + 0x04, 0)
        nbytes = vals.get(SCRATCH_BASE + 0x08, 0)
        err    = vals.get(SCRATCH_BASE + 0x0C, 0)
        done   = vals.get(SCRATCH_BASE + 0x18, 0)
        phase  = vals.get(SCRATCH_BASE + 0x20, 0)
        phn    = PHASE_NAMES.get(phase, f"0x{phase:02x}")
        if status != last_status or nbytes:
            print(f"  status=0x{status:08x}  phase=0x{phase:02x}({phn:<22}) "
                  f"lba=0x{lba:08x}  bytes={nbytes:,}  err={err}")
            last_status = status

        if status == DONE_MAGIC and done == DONE_MAGIC:
            print("\n  *** SUCCESS — all writes completed ***")
            return 0
        # 0xBAD00001 = SDW_BOOT = in-progress, not a failure
        if status >= 0xBAD00002 and status < DONE_MAGIC:
            name = ERR_NAMES.get(status, "unknown")
            print(f"\n  *** FAIL: {name} (err code from driver: {err}) ***")
            return 1
        time.sleep(2)
    print("\n  *** TIMEOUT after 10 min ***")
    return 1


if __name__ == "__main__":
    sys.exit(main())
