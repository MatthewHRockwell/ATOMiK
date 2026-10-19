#!/usr/bin/env python3
"""Build #6 USB enumeration test — single JTAG session.

1. Load DDR + release CPU via jtag_boot.py --no-boot
2. Drive boot from this process so we keep one UART owner
3. After kernel comes up, capture dmesg | usb, lsusb, PORTSC/OTGSC
4. Print structured PASS/FAIL block
"""

import subprocess
import sys
import time

import serial

PORT  = "/dev/ttyUSB2"
BAUD  = 921600
BOOT  = b"boot 0x40a00000\n"

JTAG  = ["python3", "/home/mattrock/Projects/ATOMiK/hardware/zynq/ps_loader/jtag_boot.py",
         "--no-boot", "--port", PORT, "--baud", str(BAUD)]


def log(msg):
    print(f"[test] {msg}", flush=True)


def run_jtag():
    log("running jtag_boot.py --no-boot (loads DDR, releases CPU)")
    t0 = time.time()
    p = subprocess.run(JTAG, capture_output=True, text=True, timeout=900)
    log(f"jtag_boot exited rc={p.returncode} in {time.time()-t0:.1f}s")
    if p.returncode != 0:
        log("jtag_boot stderr (tail):")
        sys.stdout.write(p.stderr[-1500:])
        sys.exit(1)
    sys.stdout.write(p.stdout[-800:])


def wait_for(ser, needle, timeout):
    deadline = time.time() + timeout
    buf = bytearray()
    while time.time() < deadline:
        chunk = ser.read(512)
        if chunk:
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
            buf.extend(chunk)
            if needle in buf:
                return True, bytes(buf)
    return False, bytes(buf)


def shell_cmd(ser, cmd, marker, timeout=15):
    """Send shell cmd, echo a marker after, read until marker seen."""
    sentinel = f"__DONE_{marker}__"
    line = f"{cmd}; echo {sentinel}\n"
    ser.write(line.encode())
    ser.flush()
    ok, buf = wait_for(ser, sentinel.encode(), timeout)
    return ok, buf


def main():
    run_jtag()

    log(f"opening {PORT} @ {BAUD}")
    ser = serial.Serial(PORT, BAUD, timeout=0.5)
    ser.reset_input_buffer()

    log("sending newlines to wake BIOS")
    ser.write(b"\n\n")
    ser.flush()
    time.sleep(0.5)

    log(f"sending: {BOOT.decode().strip()}")
    ser.write(BOOT)
    ser.flush()

    log("waiting for kernel + shell (up to 180s)…")
    # Buildroot typically auto-launches a getty/shell. Look for `# ` prompt
    # or `login:` (in which case autologin should still drop us in shell).
    ok, buf = wait_for(ser, b"login:", 60)
    if ok:
        log("login prompt — sending root")
        ser.write(b"root\n")
        ser.flush()
        time.sleep(1.0)
    # Wait for any shell prompt
    ok, _ = wait_for(ser, b"# ", 90)
    if not ok:
        log("FAIL: never reached shell prompt")
        ser.close()
        sys.exit(2)

    log("\n========= USB DIAGNOSTICS =========")

    # 1. PORTSC and OTGSC — primary test
    shell_cmd(ser, "echo === PORTSC OTGSC ===", "hdr1")
    shell_cmd(ser, "devmem 0x80002184 32", "portsc")
    shell_cmd(ser, "devmem 0x800021A4 32", "otgsc")
    shell_cmd(ser, "devmem 0x80002100 32", "usbcmd")
    shell_cmd(ser, "devmem 0x80002104 32", "usbsts")

    # 2. Kernel driver bind
    shell_cmd(ser, "echo === DMESG USB ===", "hdr2")
    shell_cmd(ser, "dmesg | grep -iE 'ci_hdrc|usb|chipidea|hid|ehci|hub' | head -40",
              "dmesg", timeout=20)

    # 3. /sys USB devices
    shell_cmd(ser, "echo === SYS USB ===", "hdr3")
    shell_cmd(ser, "ls /sys/bus/usb/devices/ 2>&1; ls /dev/input/ 2>&1; "
                   "ls /sys/class/input/ 2>&1", "sysusb")

    # 4. Interrupt arrivals
    shell_cmd(ser, "echo === IRQ COUNTS ===", "hdr4")
    shell_cmd(ser, "cat /proc/interrupts | head -20", "irq")

    log("\n========= END =========\n")
    ser.close()


if __name__ == "__main__":
    main()
