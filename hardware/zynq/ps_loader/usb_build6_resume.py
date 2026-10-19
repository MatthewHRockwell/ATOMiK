#!/usr/bin/env python3
"""Resume from BIOS prompt (DDR already loaded). 115200 baud."""

import sys, time, serial

PORT = "/dev/ttyUSB2"
BAUD = 115200
BOOT = b"boot 0x40a00000\n"


def log(m): print(f"[r] {m}", flush=True)


def wait_for(ser, needles, timeout):
    if isinstance(needles, (bytes, str)):
        needles = [needles]
    needles = [n.encode() if isinstance(n, str) else n for n in needles]
    deadline = time.time() + timeout
    buf = bytearray()
    while time.time() < deadline:
        chunk = ser.read(4096)
        if chunk:
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
            buf.extend(chunk)
            for n in needles:
                if n in buf:
                    return n, bytes(buf)
    return None, bytes(buf)


def cmd(ser, c, timeout=15):
    sentinel = f"__D_{abs(hash(c))%100000}__"
    ser.write(f"{c}; echo {sentinel}\n".encode())
    ser.flush()
    return wait_for(ser, sentinel, timeout)


def main():
    ser = serial.Serial(PORT, BAUD, timeout=0.5)
    ser.reset_input_buffer()
    log(f"open {PORT}@{BAUD}")
    ser.write(b"\n")
    ser.flush()
    time.sleep(0.3)
    # Drain
    ser.read(4096)
    log("sending boot")
    ser.write(BOOT)
    ser.flush()

    log("waiting for shell (180s)")
    hit, _ = wait_for(ser, [b"login:", b"# ", b"$ ", b"buildroot login"], 180)
    if hit and b"login" in hit:
        ser.write(b"root\n")
        ser.flush()
        time.sleep(1)
        wait_for(ser, [b"# ", b"$ "], 30)
    if not hit:
        log("FAIL: no shell")
        return 2

    log("\n=== USB DIAGNOSTICS ===")
    cmd(ser, "echo ===PORTSC=== && devmem 0x80002184 32")
    cmd(ser, "echo ===OTGSC=== && devmem 0x800021A4 32")
    cmd(ser, "echo ===USBCMD=== && devmem 0x80002100 32")
    cmd(ser, "echo ===USBSTS=== && devmem 0x80002104 32")
    cmd(ser, "echo ===USBINTR=== && devmem 0x80002108 32")
    cmd(ser, "echo ===ID=== && devmem 0x80002000 32")
    cmd(ser, "echo ===DMESG=== && dmesg | grep -iE 'ci_hdrc|chipidea|usb|hub|hid' | head -30", timeout=20)
    cmd(ser, "echo ===SYSUSB=== && ls /sys/bus/usb/devices/ 2>&1 && ls /dev/input/ 2>&1")
    cmd(ser, "echo ===INPUT=== && ls /sys/class/input/ 2>&1 && cat /sys/class/input/*/name 2>&1 | head", timeout=10)
    cmd(ser, "echo ===IRQ=== && cat /proc/interrupts | head -20")
    log("\n=== END ===")
    ser.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
