#!/usr/bin/env python3
"""
ATOMiK Performance Benchmark UART Capture & Parser

Connects to PicoRV32 SoC via serial, sends commands, and parses
machine-readable ##PERF output into structured dictionaries.

Usage:
    from perf_capture import PerfCapture
    cap = PerfCapture("/dev/ttyUSB1")
    results = cap.run_perf_suite()
    existing = cap.run_existing_tests()
    cap.close()
"""

import re
import time
import serial


class PerfCapture:
    """UART capture and parser for ATOMiK performance data."""

    def __init__(self, port, baudrate=115200, timeout=15):
        """Open serial connection to PicoRV32 SoC.

        Args:
            port: Serial port (e.g., /dev/ttyUSB1)
            baudrate: UART baud rate (default 115200)
            timeout: Read timeout in seconds
        """
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        time.sleep(0.5)
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        # Wake up firmware — send newline to trigger menu reprint
        self.ser.write(b"\n")
        time.sleep(0.3)
        # Drain any menu text that appeared
        self.ser.reset_input_buffer()

    def close(self):
        """Close serial connection."""
        self.ser.close()

    def _send_cmd(self, cmd):
        """Send a single-character command and brief delay."""
        self.ser.reset_input_buffer()
        self.ser.write(cmd.encode('ascii'))
        self.ser.flush()
        time.sleep(0.2)

    def _read_lines_until(self, end_marker, timeout=30):
        """Read lines until end_marker is found or timeout.

        Returns:
            List of decoded lines (stripped).
        """
        lines = []
        deadline = time.time() + timeout
        buf = b""

        while time.time() < deadline:
            chunk = self.ser.read(self.ser.in_waiting or 1)
            if not chunk:
                continue
            buf += chunk

            while b"\n" in buf:
                line_bytes, buf = buf.split(b"\n", 1)
                line = line_bytes.decode("ascii", errors="replace").strip()
                if line:
                    lines.append(line)
                if end_marker in line:
                    return lines

        return lines

    def run_perf_suite(self):
        """Send 'R' command and capture ##PERF benchmark output.

        Returns:
            dict with keys:
                "meta": dict of metadata from ##META line
                "measurements": list of dicts from ##PERF lines
                "raw_lines": all captured lines
        """
        self._send_cmd("R")
        lines = self._read_lines_until("##PERF_END", timeout=30)

        meta = {}
        measurements = []

        for line in lines:
            if line.startswith("##META:"):
                meta = self._parse_tagged_line(line, "##META:")
            elif line.startswith("##PERF:"):
                m = self._parse_tagged_line(line, "##PERF:")
                if m:
                    measurements.append(m)

        return {
            "meta": meta,
            "measurements": measurements,
            "raw_lines": lines,
        }

    def run_existing_tests(self):
        """Run existing X and P test commands and parse results.

        Returns:
            dict with keys:
                "atomik_hw": {"passed": int, "total": int, "t11_cycles": int or None}
                "integration": {"passed": int, "total": int}
        """
        results = {}

        # X command: ATOMiK hardware test (11 tests)
        self._send_cmd("X")
        x_lines = self._read_lines_until("Result:", timeout=10)
        results["atomik_hw"] = self._parse_test_results(x_lines, "T")

        time.sleep(0.5)

        # P command: Integration test (10 tests)
        self._send_cmd("P")
        p_lines = self._read_lines_until("Result:", timeout=10)
        results["integration"] = self._parse_test_results(p_lines, "P")

        return results

    def _parse_tagged_line(self, line, prefix):
        """Parse '##PREFIX:key=val,key=val' into dict.

        Values are converted to int (hex-aware) where possible.
        """
        payload = line[len(prefix):]
        result = {}

        for token in payload.split(","):
            if "=" not in token:
                continue
            key, val = token.split("=", 1)
            key = key.strip()
            val = val.strip()

            # Try int conversion (hex or decimal)
            try:
                if val.startswith("0x") or val.startswith("0X"):
                    result[key] = int(val, 16)
                else:
                    result[key] = int(val)
            except ValueError:
                result[key] = val

        return result

    def _parse_test_results(self, lines, prefix):
        """Parse human-readable test output (T1..T11 or P1..P10).

        Returns:
            dict with "passed", "total", and optionally "t11_cycles".
        """
        passed = 0
        total = 0
        t11_cycles = None

        test_re = re.compile(rf"({prefix}\d+)\s+.*?:\s+(PASS|FAIL)", re.IGNORECASE)
        cycles_re = re.compile(r"(\d+)\s+cycles")
        result_re = re.compile(r"Result:\s*(\d+)/(\d+)")

        for line in lines:
            # Count individual test lines
            m = test_re.search(line)
            if m:
                total += 1
                if m.group(2).upper() == "PASS":
                    passed += 1

            # Extract T11 cycle count
            if prefix == "T" and "T11" in line:
                cm = cycles_re.search(line)
                if cm:
                    t11_cycles = int(cm.group(1))

            # Check for Perf (cycles) lines that count as pass
            if prefix == "T" and "Perf (cycles):" in line:
                total += 1
                passed += 1

            # Summary line overrides individual counts
            rm = result_re.search(line)
            if rm:
                passed = int(rm.group(1))
                total = int(rm.group(2))

        result = {"passed": passed, "total": total}
        if t11_cycles is not None:
            result["t11_cycles"] = t11_cycles
        return result
