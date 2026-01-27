#!/usr/bin/env python3
"""
ATOMiK Core v2 Hardware Validation Script

Tests the delta-state architecture via UART:
  - Load initial state
  - Accumulate deltas
  - Read current state
  - Verify delta algebra properties

Usage:
    python test_hardware.py COM6
    python test_hardware.py /dev/ttyUSB0
"""

import sys
import serial
import struct
import time

class ATOMiKHardware:
    """Interface to ATOMiK Core v2 hardware via UART."""
    
    def __init__(self, port, baudrate=115200, timeout=2.0):
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        time.sleep(0.2)  # Wait for connection to stabilize
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        self.debug_mode = True
    
    def close(self):
        self.ser.close()
    
    def _debug(self, msg):
        if self.debug_mode:
            print(f"    [DEBUG] {msg}")
    
    def load(self, initial_state: int) -> None:
        """Load initial state (64-bit)."""
        cmd = b'L' + struct.pack('>Q', initial_state & 0xFFFFFFFFFFFFFFFF)
        self._debug(f"TX: {cmd.hex()}")
        self.ser.write(cmd)
        time.sleep(0.05)  # Give hardware time to process
    
    def accumulate(self, delta: int) -> None:
        """Accumulate a delta (64-bit XOR)."""
        cmd = b'A' + struct.pack('>Q', delta & 0xFFFFFFFFFFFFFFFF)
        self._debug(f"TX: {cmd.hex()}")
        self.ser.write(cmd)
        time.sleep(0.05)
    
    def read(self) -> int:
        """Read current state (returns 64-bit value)."""
        self.ser.reset_input_buffer()
        self._debug("TX: 52 (R)")
        self.ser.write(b'R')
        time.sleep(0.1)  # Wait for response
        
        # Try to read all available data
        data = self.ser.read(8)
        self._debug(f"RX: {data.hex()} ({len(data)} bytes)")
        
        if len(data) < 8:
            # Try reading more
            time.sleep(0.1)
            more = self.ser.read(8 - len(data))
            self._debug(f"RX more: {more.hex()} ({len(more)} bytes)")
            data += more
        
        if len(data) != 8:
            raise TimeoutError(f"Read timeout: got {len(data)} bytes, expected 8. Data: {data.hex()}")
        return struct.unpack('>Q', data)[0]
    
    def read_raw(self, count: int) -> bytes:
        """Read raw bytes for debugging."""
        self.ser.reset_input_buffer()
        self._debug(f"TX: 52 (R)")
        self.ser.write(b'R')
        time.sleep(0.2)
        data = self.ser.read(count)
        self._debug(f"RX raw: {data.hex()} ({len(data)} bytes)")
        return data
    
    def status(self) -> bool:
        """Get accumulator_zero status."""
        self.ser.reset_input_buffer()
        self._debug("TX: 53 (S)")
        self.ser.write(b'S')
        time.sleep(0.05)
        data = self.ser.read(1)
        self._debug(f"RX: {data.hex() if data else 'empty'}")
        if len(data) != 1:
            raise TimeoutError("Status timeout")
        # Flag is in bit 7 (MSB) of the byte
        return (data[0] & 0x80) != 0
    
    def debug_cmd(self) -> int:
        """Get debug info (initial_state)."""
        self.ser.reset_input_buffer()
        self._debug("TX: 44 (D)")
        self.ser.write(b'D')
        time.sleep(0.1)
        data = self.ser.read(8)
        self._debug(f"RX: {data.hex()} ({len(data)} bytes)")
        if len(data) != 8:
            raise TimeoutError(f"Debug timeout: got {len(data)} bytes")
        return struct.unpack('>Q', data)[0]


def run_tests(port: str):
    """Run hardware validation tests."""
    print("=" * 60)
    print("ATOMiK Core v2 Hardware Validation")
    print("=" * 60)
    print(f"Port: {port}")
    print()
    
    try:
        hw = ATOMiKHardware(port)
    except Exception as e:
        print(f"ERROR: Could not open {port}: {e}")
        return False
    
    tests_passed = 0
    tests_failed = 0
    
    def check(name: str, condition: bool, details: str = ""):
        nonlocal tests_passed, tests_failed
        if condition:
            print(f"  PASS: {name}")
            tests_passed += 1
        else:
            print(f"  FAIL: {name} {details}")
            tests_failed += 1
    
    try:
        # Test 0: Raw read diagnostic
        print("--- Test 0: Raw Read Diagnostic ---")
        hw.load(0x0102030405060708)
        time.sleep(0.1)
        raw = hw.read_raw(16)
        print(f"    Raw bytes received: {len(raw)}")
        if len(raw) > 0:
            print(f"    Hex: {raw.hex()}")
        
        # Test 1: Basic communication
        print("\n--- Test 1: Basic Communication ---")
        hw.load(0)
        time.sleep(0.05)
        status = hw.status()
        check("Status returns valid response", status in [True, False])
        
        # Test 2: Load and Read
        print("\n--- Test 2: Load and Read ---")
        test_value = 0x123456789ABCDEF0
        hw.load(test_value)
        time.sleep(0.1)
        try:
            result = hw.read()
            check(f"Load/Read roundtrip", result == test_value,
                  f"(got 0x{result:016X}, expected 0x{test_value:016X})")
        except TimeoutError as e:
            check("Load/Read roundtrip", False, str(e))
        
        # Test 3: Accumulator Zero after Load
        print("\n--- Test 3: Accumulator Zero Detection ---")
        hw.load(0xDEADBEEF)
        time.sleep(0.05)
        status = hw.status()
        check("Accumulator zero after load", status == True,
              f"(got {status})")
        
        # Test 4: Single Delta Accumulation
        print("\n--- Test 4: Single Delta Accumulation ---")
        hw.load(0x0000000000000000)
        hw.accumulate(0xAAAAAAAAAAAAAAAA)
        time.sleep(0.1)
        try:
            result = hw.read()
            expected = 0xAAAAAAAAAAAAAAAA
            check(f"Single delta", result == expected,
                  f"(got 0x{result:016X}, expected 0x{expected:016X})")
        except TimeoutError as e:
            check("Single delta", False, str(e))
        
        # Test 5: Accumulator not zero after delta
        print("\n--- Test 5: Accumulator Status ---")
        status = hw.status()
        check("Accumulator not zero after delta", status == False,
              f"(got {status})")
        
        # Test 6: Self-Inverse Property (δ ⊕ δ = 0)
        print("\n--- Test 6: Self-Inverse Property ---")
        hw.load(0xCAFEBABE12345678)
        hw.accumulate(0x5555555555555555)
        hw.accumulate(0x5555555555555555)  # Same delta twice
        time.sleep(0.1)
        try:
            result = hw.read()
            expected = 0xCAFEBABE12345678  # Should return to initial
            check("Self-inverse (d XOR d = 0)", result == expected,
                  f"(got 0x{result:016X}, expected 0x{expected:016X})")
        except TimeoutError as e:
            check("Self-inverse", False, str(e))
        
        # Accumulator should be zero
        status = hw.status()
        check("Accumulator zero after self-inverse", status == True,
              f"(got {status})")
        
        # Test 7: Identity Property (δ = 0)
        print("\n--- Test 7: Identity Property ---")
        hw.load(0xFEDCBA9876543210)
        hw.accumulate(0x0000000000000000)  # Zero delta
        time.sleep(0.1)
        try:
            result = hw.read()
            expected = 0xFEDCBA9876543210
            check("Identity (d = 0 is no-op)", result == expected,
                  f"(got 0x{result:016X}, expected 0x{expected:016X})")
        except TimeoutError as e:
            check("Identity", False, str(e))
        
        # Test 8: Multiple Deltas
        print("\n--- Test 8: Multiple Deltas ---")
        hw.load(0x0000000000000000)
        hw.accumulate(0x1111111111111111)
        hw.accumulate(0x2222222222222222)
        hw.accumulate(0x4444444444444444)
        time.sleep(0.1)
        try:
            result = hw.read()
            # 0x1111... ^ 0x2222... ^ 0x4444... = 0x7777...
            expected = 0x7777777777777777
            check(f"Multiple deltas", result == expected,
                  f"(got 0x{result:016X}, expected 0x{expected:016X})")
        except TimeoutError as e:
            check("Multiple deltas", False, str(e))
        
        # Test 9: State Reconstruction Formula
        print("\n--- Test 9: State Reconstruction ---")
        initial = 0xFF00FF00FF00FF00
        delta = 0x00FF00FF00FF00FF
        hw.load(initial)
        hw.accumulate(delta)
        time.sleep(0.1)
        try:
            result = hw.read()
            expected = initial ^ delta  # Should be 0xFFFFFFFFFFFFFFFF
            check("Reconstruction (S XOR d)", result == expected,
                  f"(got 0x{result:016X}, expected 0x{expected:016X})")
        except TimeoutError as e:
            check("Reconstruction", False, str(e))
        
    except TimeoutError as e:
        print(f"\nERROR: Communication timeout - {e}")
        tests_failed += 1
    except Exception as e:
        print(f"\nERROR: {e}")
        import traceback
        traceback.print_exc()
        tests_failed += 1
    finally:
        hw.close()
    
    # Summary
    print()
    print("=" * 60)
    print("Test Summary")
    print("=" * 60)
    print(f"  Passed: {tests_passed}")
    print(f"  Failed: {tests_failed}")
    print()
    
    if tests_failed == 0:
        print("*** ALL HARDWARE TESTS PASSED ***")
        print()
        print("ATOMiK Core v2 delta architecture validated!")
        return True
    else:
        print("*** SOME TESTS FAILED ***")
        return False


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python test_hardware.py <COM_PORT>")
        print("Example: python test_hardware.py COM6")
        sys.exit(1)
    
    port = sys.argv[1]
    success = run_tests(port)
    sys.exit(0 if success else 1)
