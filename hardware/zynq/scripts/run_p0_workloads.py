#!/usr/bin/env python3
"""Run ATOMiK P0 validation workloads through the board serial shell.

The board rootfs used for the demo may not include the latest workload scripts,
so this tool can package `hardware/zynq/workloads/`, upload it over UART using
base64, extract it with Python on the board, and run `run_all.sh`.
"""

from __future__ import annotations

import argparse
import base64
import io
from pathlib import Path
import sys
import tarfile
import time

try:
    import serial
except ImportError:  # pragma: no cover
    serial = None


ZYNQ_DIR = Path("/home/mattrock/Projects/ATOMiK/hardware/zynq")
WORKLOADS_DIR = ZYNQ_DIR / "workloads"
DEFAULT_LOG = ZYNQ_DIR / "results/p0_workloads_serial.log"


def parse_ports(value: str) -> list[str]:
    return [p.strip() for p in value.split(",") if p.strip()]


def open_console(ports: list[str], baud: int):
    if serial is None:
        raise SystemExit("pyserial is required: python3 -m pip install pyserial")

    last_error = None
    for port in ports:
        if not Path(port).exists():
            continue
        try:
            ser = serial.Serial(port, baud, timeout=0.05)
            ser.reset_input_buffer()
            print(f"[uart] opened {port} @ {baud}")
            return ser
        except Exception as exc:
            last_error = exc
    raise SystemExit(f"could not open serial console; last error: {last_error}")


def write_line(ser, line: str = "") -> None:
    ser.write(line.encode("utf-8") + b"\n")
    ser.flush()


def read_until_prompt(ser, timeout: int) -> str:
    print("[uart] waiting for Linux shell prompt")
    rolling = ""
    end = time.time() + timeout
    last_ping = 0.0
    while time.time() < end:
        now = time.time()
        if now - last_ping > 2.0:
            write_line(ser)
            last_ping = now
        data = ser.read(4096)
        if data:
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()
            rolling = (rolling + data.decode("utf-8", errors="replace"))[-8000:]
            low = rolling.lower()
            if any(t in low for t in ("root@", "/ #", "~#", "bash", "atomik | ubuntu")):
                print("\n[uart] shell appears ready")
                return rolling
    raise SystemExit("timed out waiting for shell prompt")


def build_workload_archive() -> bytes:
    if not WORKLOADS_DIR.exists():
        raise SystemExit(f"missing workloads dir: {WORKLOADS_DIR}")

    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w:gz") as tf:
        tf.add(WORKLOADS_DIR, arcname="workloads")
    return buf.getvalue()


def upload_workloads(ser, archive: bytes) -> None:
    encoded = base64.b64encode(archive).decode("ascii")
    print(f"[uart] uploading workloads archive: {len(archive) // 1024} KiB binary, {len(encoded) // 1024} KiB base64")

    write_line(ser, "stty -echo 2>/dev/null || true")
    write_line(ser, "rm -rf /root/atomik_p0 /tmp/atomik_p0_workloads.tar.gz.b64")
    write_line(ser, "cat > /tmp/atomik_p0_workloads.tar.gz.b64 <<'ATOMIK_P0_B64'")
    for i in range(0, len(encoded), 4096):
        ser.write(encoded[i : i + 4096].encode("ascii") + b"\n")
        ser.flush()
        if i and i % (4096 * 32) == 0:
            print(f"  upload {i * 100 // len(encoded)}%")
            time.sleep(0.05)
    write_line(ser, "ATOMIK_P0_B64")

    extract = r"""
python3 - <<'PY'
import base64, io, pathlib, tarfile
b64 = pathlib.Path('/tmp/atomik_p0_workloads.tar.gz.b64').read_text()
data = base64.b64decode(''.join(b64.split()))
root = pathlib.Path('/root/atomik_p0')
root.mkdir(parents=True, exist_ok=True)
with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as tf:
    tf.extractall(root)
print('ATOMIK_P0_DEPLOYED', len(data))
PY
""".strip()
    for line in extract.splitlines():
        write_line(ser, line)
        time.sleep(0.02)
    write_line(ser, "stty echo 2>/dev/null || true")


def run_workloads(ser) -> None:
    commands = r"""
export PATH=/usr/sbin:/usr/bin:/sbin:/bin
mount | grep -q '^proc' || mount -t proc proc /proc 2>/dev/null || true
mount | grep -q '^sysfs' || mount -t sysfs sysfs /sys 2>/dev/null || true
[ -e /dev/mem ] || mknod /dev/mem c 1 1
chmod 666 /dev/mem 2>/dev/null || true
cd /root/atomik_p0/workloads
chmod +x run_all.sh
if command -v bash >/dev/null 2>&1; then bash run_all.sh; else sh run_all.sh; fi
echo __ATOMIK_P0_DONE:$?
""".strip()
    print("[uart] starting P0 workload suite")
    for line in commands.splitlines():
        write_line(ser, line)
        time.sleep(0.03)


def monitor(ser, timeout: int, log_path: Path) -> int:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    rolling = ""
    end = time.time() + timeout
    rc = 124
    with log_path.open("ab") as log:
        while time.time() < end:
            data = ser.read(4096)
            if not data:
                continue
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()
            log.write(data)
            log.flush()
            rolling = (rolling + data.decode("utf-8", errors="replace"))[-12000:]
            marker = "__ATOMIK_P0_DONE:"
            if marker in rolling:
                tail = rolling.split(marker, 1)[1]
                digits = ""
                for ch in tail:
                    if ch.isdigit():
                        digits += ch
                    else:
                        break
                rc = int(digits or "0")
                break
    print(f"\n[uart] workload monitor finished rc={rc}; log={log_path}")
    return rc


def main() -> int:
    ap = argparse.ArgumentParser(description="Run P0 workloads through the Zynq Linux serial shell")
    ap.add_argument("--ports", default="/dev/ttyUSB2,/dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--prompt-timeout", type=int, default=120)
    ap.add_argument("--timeout", type=int, default=900)
    ap.add_argument("--no-deploy", action="store_true", help="assume /root/atomik_p0/workloads already exists")
    ap.add_argument("--log", default=str(DEFAULT_LOG))
    args = ap.parse_args()

    ser = open_console(parse_ports(args.ports), args.baud)
    read_until_prompt(ser, args.prompt_timeout)
    if not args.no_deploy:
        upload_workloads(ser, build_workload_archive())
        time.sleep(2)
    run_workloads(ser)
    return monitor(ser, args.timeout, Path(args.log))


if __name__ == "__main__":
    raise SystemExit(main())
