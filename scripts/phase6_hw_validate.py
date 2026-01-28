#!/usr/bin/env python3
"""Phase 6: Hardware Validation via UART

Programs the FPGA (if not already done) and validates the parallel
accumulator via UART command protocol.

Protocol:
  L + 8 bytes: Load initial state
  A + 8 bytes: Accumulate delta
  R:           Read current state (returns 8 bytes)
  S:           Status (returns 1 byte, bit[7] = accumulator_zero)
"""

import serial
import sys
import time

PORT = "COM6"
BAUD = 115200
TIMEOUT = 2


def read_state(port):
    """Send Read command and return 64-bit state."""
    port.write(b"R")
    resp = port.read(8)
    if len(resp) != 8:
        return None
    return int.from_bytes(resp, "big")


def load_initial(port, value):
    """Send Load command with 64-bit initial state."""
    port.write(b"L" + value.to_bytes(8, "big"))
    time.sleep(0.1)


def accumulate(port, delta):
    """Send Accumulate command with 64-bit delta."""
    port.write(b"A" + delta.to_bytes(8, "big"))
    time.sleep(0.1)


def read_status(port):
    """Send Status command and return (status_byte, acc_zero)."""
    port.write(b"S")
    resp = port.read(1)
    if len(resp) != 1:
        return None, None
    status = resp[0]
    acc_zero = (status >> 7) & 1
    return status, acc_zero


def check(label, got, expected):
    """Check a value and print result."""
    ok = got == expected
    tag = "PASS" if ok else "FAIL"
    print(f"  {label}: got=0x{got:016X}, expect=0x{expected:016X} -> {tag}")
    return ok


def main():
    port = serial.Serial(PORT, BAUD, timeout=TIMEOUT)
    time.sleep(0.5)
    port.reset_input_buffer()

    print("ATOMiK Phase 6: Hardware Validation")
    print("=" * 60)

    passed = 0
    failed = 0

    # Test 1: Load zeros, read back
    print("\nTest 1: Load initial state = 0, read back")
    load_initial(port, 0)
    state = read_state(port)
    if state is not None and check("state", state, 0):
        passed += 1
    else:
        failed += 1

    # Test 2: Accumulate one delta
    d1 = 0xDEADBEEFCAFEBABE
    print(f"\nTest 2: Accumulate 0x{d1:016X}")
    accumulate(port, d1)
    state = read_state(port)
    if state is not None and check("state", state, d1):
        passed += 1
    else:
        failed += 1

    # Test 3: Self-inverse (accumulate same delta again -> 0)
    print(f"\nTest 3: Self-inverse (accumulate 0x{d1:016X} again)")
    accumulate(port, d1)
    state = read_state(port)
    if state is not None and check("state", state, 0):
        passed += 1
    else:
        failed += 1

    # Test 4: Status check (accumulator zero)
    print("\nTest 4: Status check (accumulator zero)")
    status, acc_zero = read_status(port)
    if status is not None:
        tag = "PASS" if acc_zero == 1 else "FAIL"
        print(f"  status=0x{status:02X}, acc_zero={acc_zero} -> {tag}")
        if acc_zero == 1:
            passed += 1
        else:
            failed += 1
    else:
        print("  ERROR: no response")
        failed += 1

    # Test 5: Two deltas (commutativity)
    d2 = 0x1234567890ABCDEF
    d3 = 0xFEDCBA9876543210
    expected = d2 ^ d3
    print(f"\nTest 5: Accumulate d2=0x{d2:016X}, d3=0x{d3:016X}")
    accumulate(port, d2)
    accumulate(port, d3)
    state = read_state(port)
    if state is not None and check("state", state, expected):
        passed += 1
    else:
        failed += 1

    # Test 6: Load non-zero initial state
    init = 0xAAAABBBBCCCCDDDD
    print(f"\nTest 6: Load initial state = 0x{init:016X}")
    load_initial(port, init)
    state = read_state(port)
    if state is not None and check("state", state, init):
        passed += 1
    else:
        failed += 1

    # Test 7: Accumulate with non-zero initial state
    d4 = 0x1111222233334444
    expected = init ^ d4
    print(f"\nTest 7: Accumulate 0x{d4:016X} with initial=0x{init:016X}")
    accumulate(port, d4)
    state = read_state(port)
    if state is not None and check("state", state, expected):
        passed += 1
    else:
        failed += 1

    # Test 8: Status check (accumulator NOT zero)
    print("\nTest 8: Status check (accumulator not zero)")
    status, acc_zero = read_status(port)
    if status is not None:
        tag = "PASS" if acc_zero == 0 else "FAIL"
        print(f"  status=0x{status:02X}, acc_zero={acc_zero} -> {tag}")
        if acc_zero == 0:
            passed += 1
        else:
            failed += 1
    else:
        print("  ERROR: no response")
        failed += 1

    # Test 9: Multiple deltas verify accumulation
    print("\nTest 9: Accumulate 4 deltas (exercises round-robin across 4 banks)")
    load_initial(port, 0)
    deltas = [0x1000000000000001, 0x0200000000000020,
              0x0030000000000300, 0x0004000000004000]
    expected = 0
    for d in deltas:
        accumulate(port, d)
        expected ^= d
    state = read_state(port)
    if state is not None and check("state", state, expected):
        passed += 1
    else:
        failed += 1

    # Test 10: 8 deltas (two full cycles across 4 banks)
    print("\nTest 10: Accumulate 8 deltas (two full bank cycles)")
    load_initial(port, 0)
    deltas = [0xFF00000000000000, 0x00FF000000000000,
              0x0000FF0000000000, 0x000000FF00000000,
              0x00000000FF000000, 0x0000000000FF0000,
              0x000000000000FF00, 0x00000000000000FF]
    expected = 0
    for d in deltas:
        accumulate(port, d)
        expected ^= d
    state = read_state(port)
    if state is not None and check("state", state, expected):
        passed += 1
    else:
        failed += 1

    # Summary
    print("\n" + "=" * 60)
    total = passed + failed
    print(f"Results: {passed}/{total} passed, {failed}/{total} failed")
    if failed == 0:
        print("ALL TESTS PASSED -- Phase 6 hardware validated")
    else:
        print(f"FAILURES: {failed} test(s) failed")

    port.close()
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
