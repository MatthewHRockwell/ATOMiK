#!/usr/bin/env python3
"""Emergency ATOMiK demo boot path: no-reset JTAG load + UART trigger.

This deliberately bypasses the currently-broken SD BootROM path. It assumes the
board has been physically power-cycled in SD mode, then uses xsdb without
`rst-system` to:

  1. run ps7_init,
  2. program the requested LiteX/NaxRiscv bitstream,
  3. run ps7_post_config,
  4. load Linux/OpenSBI/initramfs/trampoline into PS DDR,
  5. release ARM and send `boot 0x40a00000` to LiteX BIOS.

Default target is the l2off + framebuffer build for the investor demo.
"""

from __future__ import annotations

import argparse
import base64
import os
from pathlib import Path
import subprocess
import sys
import time
from typing import Iterable

try:
    import serial
except ImportError:  # pragma: no cover - useful when invoked on a fresh host
    serial = None


ZYNQ_DIR = Path("/home/mattrock/Projects/ATOMiK/hardware/zynq")
LITEX_BUILD = ZYNQ_DIR / "litex-build"
PS_LOADER = ZYNQ_DIR / "ps_loader"
XSDB = Path("/opt/Xilinx/2025.2/Vivado/bin/xsdb")
PS7_INIT = ZYNQ_DIR / "scripts/ps7_init_rk7020f.tcl"
DEFAULT_ATOMIK_OS = Path("/home/mattrock/Projects/ATOMiK/atomik_os/build/atomik_os")

DEFAULT_BITSTREAM = (
    ZYNQ_DIR / "litex-build-nax64-l2off-fb/gateware/hamgeek_rk7020f.bit"
)
FALLBACK_BITSTREAM = (
    ZYNQ_DIR / "litex-build-nax64-l2off/gateware/hamgeek_rk7020f.bit"
)

NAX_TO_PS_OFFSET = 0x3FF0_0000
BOOT_TARGET_NAX = 0x40A0_0000
TRAMPOLINE_NAX = BOOT_TARGET_NAX

BOOT_FILES = [
    (LITEX_BUILD / "Image_nax64", 0x4000_0000, "Linux kernel"),
    (LITEX_BUILD / "linux_nax64.dtb", 0x40EF_0000, "Device tree"),
    (LITEX_BUILD / "fw_jump_nax64.bin", 0x40F0_0000, "OpenSBI fw_jump"),
    (LITEX_BUILD / "ubuntu_rv64.cpio.gz", 0x4200_0000, "Ubuntu initramfs"),
    (PS_LOADER / "build/trampoline.bin", TRAMPOLINE_NAX, "L2 flush trampoline"),
]


def nax_to_ps(nax_addr: int) -> int:
    return nax_addr - NAX_TO_PS_OFFSET


def tcl_path(path: Path) -> str:
    # Paths here do not contain braces, but escape defensively.
    return "{" + str(path).replace("}", "\\}") + "}"


def require_paths(paths: Iterable[Path]) -> None:
    missing = [str(p) for p in paths if not p.exists()]
    if missing:
        raise SystemExit("missing required file(s):\n  " + "\n  ".join(missing))


def build_xsdb_script(bitstream: Path, connect_url: str | None) -> str:
    connect_cmd = f"connect -url {connect_url}" if connect_url else "connect"
    lines = [
        "proc must {label body} {",
        "    puts \"-- $label --\"",
        "    if {[catch {uplevel 1 $body} result]} {",
        "        puts \"ERROR: $label: $result\"",
        "        exit 1",
        "    } elseif {[string length $result] != 0} {",
        "        puts $result",
        "    }",
        "}",
        "proc soft {label body} {",
        "    puts \"-- $label --\"",
        "    if {[catch {uplevel 1 $body} result]} {",
        "        puts \"WARN: $label: $result\"",
        "        return 1",
        "    } elseif {[string length $result] != 0} {",
        "        puts $result",
        "    }",
        "    return 0",
        "}",
        "",
        f"must \"connect hw_server\" {{{connect_cmd}}}",
        "after 1000",
        "soft \"targets before init\" {targets}",
        "must \"select APU\" {targets -set -filter {name =~ \"APU*\"}}",
        f"must \"source ps7_init\" {{source {tcl_path(PS7_INIT)}}}",
        "must \"run ps7_init\" {ps7_init}",
        "must \"select FPGA target\" {targets -set -filter {name =~ \"xc7z*\"}}",
        f"must \"program bitstream\" {{fpga -file {tcl_path(bitstream)}}}",
        "must \"select APU for post-config\" {targets -set -filter {name =~ \"APU*\"}}",
        "must \"ps7_post_config\" {ps7_post_config}",
        "soft \"halt Cortex-A9 #0\" {targets -set -filter {name =~ \"ARM*Cortex-A9 MPCore #0\"}; stop}",
        "after 5000",
        "soft \"A9 registers after stop\" {rrd pc}",
    ]

    for path, nax_addr, desc in BOOT_FILES:
        ps_addr = nax_to_ps(nax_addr)
        size = path.stat().st_size
        lines.append(
            'puts "-- dow -data {} -> PS 0x{:08x} (Nax 0x{:08x}, {:,} bytes) --"'.format(
                path.name, ps_addr, nax_addr, size
            )
        )
        lines.append(f"must \"load {path.name}\" {{dow -data {tcl_path(path)} 0x{ps_addr:08x}}}")

    lines += [
        "soft \"clean/invalidate ARM PL310 L2\" {",
        "    set clean_inv 0xF8F027FC",
        "    set sync_reg  0xF8F02730",
        "    mwr -force $clean_inv 0x0000FFFF",
        "    after 100",
        "    for {set i 0} {$i < 20} {incr i} {",
        "        set val [mrd -force -value $clean_inv]",
        "        if {$val == 0} {break}",
        "        after 50",
        "    }",
        "    mwr -force $sync_reg 0",
        "}",
        "soft \"USB0 debug state\" {",
        "    puts \"  USB0_CLK_CTRL = [format 0x%08x [mrd -force -value 0xF8000130]]\"",
        "    puts \"  MIO_PIN_46    = [format 0x%08x [mrd -force -value 0xF8000748]]\"",
        "    puts \"  USB ID        = [format 0x%08x [mrd -force -value 0xE0002000]]\"",
        "    puts \"  PORTSC        = [format 0x%08x [mrd -force -value 0xE0002184]]\"",
        "}",
        "soft \"release ARM\" {targets -set -filter {name =~ \"ARM*Cortex-A9 MPCore #0\"}; con}",
        "disconnect",
        "exit",
        "",
    ]
    return "\n".join(lines)


def run_xsdb(script: str, timeout: int) -> None:
    print("[xsdb] no rst-system will be issued. Board should be freshly power-cycled.")
    print(f"[xsdb] using {XSDB}")
    start = time.time()
    proc = subprocess.Popen(
        [str(XSDB)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    assert proc.stdin is not None
    assert proc.stdout is not None
    proc.stdin.write(script)
    proc.stdin.close()

    deadline = time.time() + timeout
    try:
        for line in proc.stdout:
            print(line.rstrip())
            if time.time() > deadline:
                proc.kill()
                raise TimeoutError
        rc = proc.wait(timeout=5)
    except TimeoutError:
        proc.kill()
        raise SystemExit(f"xsdb timed out after {timeout}s")

    if rc != 0:
        raise SystemExit(f"xsdb failed with exit code {rc}")
    print(f"[xsdb] done in {time.time() - start:.1f}s")


def parse_ports(value: str) -> list[str]:
    return [p.strip() for p in value.split(",") if p.strip()]


def open_console(ports: list[str], bauds: list[int], probe_seconds: float):
    if serial is None:
        raise SystemExit("pyserial is required: python3 -m pip install pyserial")

    last_error = None
    best = None
    for port in ports:
        if not Path(port).exists():
            continue
        for baud in bauds:
            try:
                ser = serial.Serial(port, baud, timeout=0.05)
                ser.reset_input_buffer()
                ser.write(b"\n")
                ser.flush()
                time.sleep(0.15)
                data = bytearray()
                end = time.time() + probe_seconds
                while time.time() < end:
                    chunk = ser.read(1024)
                    if chunk:
                        data += chunk
                text = data.decode("utf-8", errors="replace").lower()
                score = 0
                for token in ("litex", "bios", "memtest", "ubuntu", "root@", "# "):
                    if token in text:
                        score += 1
                if score:
                    print(f"[uart] selected {port} @ {baud} ({len(data)} probe bytes)")
                    return ser
                if best is None:
                    best = ser
                    print(f"[uart] {port} @ {baud} opened but no prompt yet")
                else:
                    ser.close()
            except Exception as exc:  # pyserial raises platform-specific errors
                last_error = exc

    if best is not None:
        print(f"[uart] using first available port {best.port} @ {best.baudrate}")
        return best
    raise SystemExit(f"could not open any serial console; last error: {last_error}")


def write_line(ser, line: str) -> None:
    ser.write(line.encode("utf-8") + b"\n")
    ser.flush()


def launch_atomik_boot(ser) -> None:
    cmd = r"""
echo ATOMIK_BOOT_PROBE
export PATH=/usr/sbin:/usr/bin:/sbin:/bin
if [ -x /usr/sbin/atomik_boot.sh ]; then
  echo ATOMIK_BOOT_STARTED:/usr/sbin/atomik_boot.sh
  /usr/sbin/atomik_boot.sh >/dev/null 2>&1 &
elif [ -x /sbin/atomik_boot.sh ]; then
  echo ATOMIK_BOOT_STARTED:/sbin/atomik_boot.sh
  /sbin/atomik_boot.sh >/dev/null 2>&1 &
elif [ -x /root/atomik_os ]; then
  echo ATOMIK_BOOT_STARTED:/root/atomik_os
  /root/atomik_os >/dev/null 2>&1 &
else
  echo ATOMIK_BOOT_MISSING
fi
""".strip()
    for line in cmd.splitlines():
        write_line(ser, line)
        time.sleep(0.03)


def upload_atomik_os(ser, local_path: Path) -> None:
    data = base64.b64encode(local_path.read_bytes()).decode("ascii")
    print(f"[uart] uploading {local_path} via base64 ({len(data) // 1024} KiB text)")
    write_line(ser, "stty -echo 2>/dev/null || true")
    write_line(ser, "cat > /tmp/atomik_os.b64 <<'ATOMIK_OS_B64'")
    for i in range(0, len(data), 4096):
        ser.write(data[i : i + 4096].encode("ascii") + b"\n")
        ser.flush()
        if i and i % (4096 * 32) == 0:
            print(f"  uploaded {i * 100 // len(data)}%")
            time.sleep(0.05)
    write_line(ser, "ATOMIK_OS_B64")
    write_line(
        ser,
        "base64 -d /tmp/atomik_os.b64 > /root/atomik_os && chmod +x /root/atomik_os && rm /tmp/atomik_os.b64",
    )
    write_line(ser, "stty echo 2>/dev/null || true")
    write_line(ser, "/root/atomik_os >/dev/null 2>&1 & echo ATOMIK_OS_DEPLOYED")


def monitor_and_boot(args) -> None:
    bauds = [args.baud] if args.baud else [115200, 921600]
    ser = open_console(parse_ports(args.ports), bauds, args.probe_seconds)
    log_path = Path(args.log)
    log_path.parent.mkdir(parents=True, exist_ok=True)

    print(f"[uart] logging to {log_path}")
    print(f"[uart] waiting for BIOS, then sending boot 0x{BOOT_TARGET_NAX:08x}")

    boot_sent = False
    boot_confirmed = False
    atomik_started = False
    missing_seen = False
    last_boot_try = 0.0
    start = time.time()
    bios_deadline = start + args.bios_timeout
    end = start + args.linux_timeout
    rolling = ""

    with log_path.open("ab") as log:
        while time.time() < end:
            data = ser.read(4096)
            now = time.time()
            if data:
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
                log.write(data)
                log.flush()
                text = data.decode("utf-8", errors="replace")
                rolling = (rolling + text)[-12000:]
                low = rolling.lower()

                if not boot_sent and any(t in low for t in ("litex", "bios", "memtest", "litex>")):
                    write_line(ser, f"boot 0x{BOOT_TARGET_NAX:08x}")
                    boot_sent = True
                    last_boot_try = now
                    print(f"\n[uart] sent boot 0x{BOOT_TARGET_NAX:08x}")

                if "liftoff" in low or "opensbi" in low:
                    boot_confirmed = True

                linux_ready = any(
                    t in low
                    for t in (
                        "atomik | ubuntu",
                        "ubuntu 24.04",
                        "root@",
                        "bash",
                        "/ #",
                        "~#",
                    )
                )
                if linux_ready and not atomik_started and not args.no_auto_atomik:
                    print("\n[uart] Linux shell/banner detected; launching atomik_boot")
                    time.sleep(args.shell_settle_seconds)
                    write_line(ser, "")
                    launch_atomik_boot(ser)
                    atomik_started = True

                if "atomik_boot_missing" in low and not missing_seen:
                    missing_seen = True
                    if args.deploy_atomik_os:
                        upload_atomik_os(ser, Path(args.deploy_atomik_os))

            if not boot_sent and now > bios_deadline:
                write_line(ser, f"boot 0x{BOOT_TARGET_NAX:08x}")
                boot_sent = True
                last_boot_try = now
                print(f"\n[uart] BIOS prompt not seen; sent boot 0x{BOOT_TARGET_NAX:08x} anyway")

            if boot_sent and not boot_confirmed and now - last_boot_try > args.boot_retry_seconds:
                write_line(ser, f"boot 0x{BOOT_TARGET_NAX:08x}")
                last_boot_try = now
                print(f"\n[uart] retried boot 0x{BOOT_TARGET_NAX:08x}")

    print("\n[uart] monitor timeout reached")


def main() -> int:
    ap = argparse.ArgumentParser(description="No-reset JTAG boot for ATOMiK Zynq demo")
    ap.add_argument("--bitstream", default=str(DEFAULT_BITSTREAM), help="LiteX bitstream to program")
    ap.add_argument(
        "--fallback-no-fb",
        action="store_true",
        help=f"use no-framebuffer l2off bitstream instead: {FALLBACK_BITSTREAM}",
    )
    ap.add_argument("--connect-url", default=os.environ.get("HW_SERVER_URL"), help="optional xsdb connect URL")
    ap.add_argument("--xsdb-timeout", type=int, default=900)
    ap.add_argument("--ports", default="/dev/ttyUSB2,/dev/ttyUSB0", help="comma-separated serial candidates")
    ap.add_argument("--baud", type=int, default=0, help="0 = probe 115200 then 921600")
    ap.add_argument("--probe-seconds", type=float, default=1.5)
    ap.add_argument("--bios-timeout", type=int, default=45)
    ap.add_argument("--boot-retry-seconds", type=int, default=8)
    ap.add_argument("--linux-timeout", type=int, default=300)
    ap.add_argument("--shell-settle-seconds", type=float, default=1.5)
    ap.add_argument("--no-auto-atomik", action="store_true")
    ap.add_argument(
        "--deploy-atomik-os",
        default=str(DEFAULT_ATOMIK_OS) if DEFAULT_ATOMIK_OS.exists() else None,
        help="optional local atomik_os binary to deploy over UART if missing",
    )
    ap.add_argument(
        "--log",
        default=str(ZYNQ_DIR / "results/boot_demo_uart.log"),
        help="serial boot log path",
    )
    args = ap.parse_args()

    bitstream = FALLBACK_BITSTREAM if args.fallback_no_fb else Path(args.bitstream)
    required = [XSDB, PS7_INIT, bitstream] + [p for p, _, _ in BOOT_FILES]
    if args.deploy_atomik_os:
        required.append(Path(args.deploy_atomik_os))
    require_paths(required)

    print("=== ATOMiK emergency demo boot ===")
    print("Mode: no rst-system; physical power-cycle first")
    print(f"Bitstream: {bitstream}")
    for path, nax_addr, desc in BOOT_FILES:
        print(f"  {desc:<18} {path.name:<24} Nax 0x{nax_addr:08x} -> PS 0x{nax_to_ps(nax_addr):08x}")

    run_xsdb(build_xsdb_script(bitstream, args.connect_url), args.xsdb_timeout)
    monitor_and_boot(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
