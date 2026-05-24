#!/usr/bin/env python3
"""Probe a failed SD boot without resetting the Zynq.

Use this after setting J13 to SD mode, power-cycling, and waiting for the
silent boot attempt to settle. The script intentionally does not run ps7_init,
does not program the PL, and does not issue rst-system. It connects with xsdb,
halts Cortex-A9 #0 where it is, and dumps enough state to distinguish:

  - BootROM never jumped to FSBL
  - BootROM jumped and FSBL is executing/hung in OCM
  - FSBL reached post-DDR scratch markers
"""

import argparse
import os
import subprocess
import sys
import time


DEFAULT_XSDB = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"


def xsdb_script(include_ddr_markers: bool) -> str:
    ddr_probe = ""
    if include_ddr_markers:
        ddr_probe = r'''
safe "DDR scratch 0x10100000..0x1010001f" {
    mrd -force 0x10100000 8
}
'''

    return r'''
proc safe {label body} {
    puts ""
    puts [format "-- %s --" $label]
    if {[catch {uplevel 1 $body} result]} {
        puts [format "ERR: %s" $result]
    } elseif {[string length $result] != 0} {
        puts $result
    }
}

connect
after 500

safe "targets before halt" {
    targets
}

safe "select Cortex-A9 #0" {
    targets -set -filter {name =~ "ARM*Cortex-A9 MPCore #0"}
}

safe "halt current PC (no reset)" {
    stop
}

safe "core registers" {
    rrd
}

safe "BOOT_MODE 0xF800025C" {
    mrd -force 0xF800025C 1
}

safe "REBOOT_STATUS 0xF8000258" {
    mrd -force 0xF8000258 1
}

safe "PLL_STATUS 0xF800010C" {
    mrd -force 0xF800010C 1
}

safe "SDIO_RST_CTRL 0xF8000218" {
    mrd -force 0xF8000218 1
}

safe "APER_CLK_CTRL 0xF800012C" {
    mrd -force 0xF800012C 1
}

safe "SDIO_CLK_CTRL 0xF8000150" {
    mrd -force 0xF8000150 1
}

safe "SDIO0 state 0xE0100000" {
    foreach a {
        0xE0100008
        0xE010000C
        0xE0100010
        0xE0100024
        0xE0100028
        0xE010002C
        0xE0100030
        0xE0100034
        0xE0100038
        0xE0100040
        0xE01000FE
    } {
        set v [mrd -value -force $a 1]
        puts [format "0x%08x: 0x%08x" $a $v]
    }
}

safe "MIO40..47 SDIO0 pin config" {
    foreach a {
        0xF80007A0
        0xF80007A4
        0xF80007A8
        0xF80007AC
        0xF80007B0
        0xF80007B4
        0xF80007B8
        0xF80007BC
    } {
        set v [mrd -value -force $a 1]
        puts [format "0x%08x: 0x%08x" $a $v]
    }
}

safe "DEVCFG_STATUS 0xF8007014" {
    mrd -force 0xF8007014 1
}

safe "DEVCFG_INT_STS 0xF800700C" {
    mrd -force 0xF800700C 1
}

safe "OCM vectors / FSBL first words 0x00000000" {
    mrd -force 0x00000000 16
}
''' + ddr_probe + r'''

safe "targets after halt" {
    targets
}

disconnect
exit
'''


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Halt and inspect a failed Zynq SD boot without resetting it."
    )
    ap.add_argument(
        "--xsdb",
        default=os.environ.get("XSDB", DEFAULT_XSDB),
        help=f"xsdb path (default: {DEFAULT_XSDB})",
    )
    ap.add_argument(
        "--delay",
        type=float,
        default=0.0,
        help="seconds to wait before connecting, useful for an automated timed snapshot",
    )
    ap.add_argument(
        "--ddr-markers",
        action="store_true",
        help="also read 0x10100000 scratch markers; do this last because DDR may be uninitialized",
    )
    args = ap.parse_args()

    if args.delay > 0:
        print(f"Waiting {args.delay:.1f}s before xsdb attach...")
        time.sleep(args.delay)

    script = xsdb_script(args.ddr_markers)
    try:
        proc = subprocess.run(
            [args.xsdb],
            input=script,
            text=True,
            capture_output=True,
            timeout=60,
        )
    except FileNotFoundError:
        print(f"xsdb not found: {args.xsdb}", file=sys.stderr)
        return 127
    except subprocess.TimeoutExpired as exc:
        print("xsdb probe timed out", file=sys.stderr)
        if exc.stdout:
            print(exc.stdout)
        if exc.stderr:
            print(exc.stderr, file=sys.stderr)
        return 124

    if proc.stdout:
        print(proc.stdout, end="")
    if proc.stderr:
        print(proc.stderr, end="", file=sys.stderr)
    return proc.returncode


if __name__ == "__main__":
    raise SystemExit(main())
