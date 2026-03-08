"""
test_atomik_zynq.py - ATOMiK Python Bindings Test Suite

Tests the ATOMiK Python library using the mock backend.
Mirrors the C test suite (test_libatomik.c) for consistency.

Run: python3 test_atomik_zynq.py
  or: pytest test_atomik_zynq.py -v

ATOMiK Project — March 2026
SPDX-License-Identifier: MIT
"""

import sys
import os

# Add parent directory so atomik_zynq can be imported
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from atomik_zynq import ATOMiK


pass_count = 0
fail_count = 0
test_num = 0


def check64(expected: int, actual: int, name: str):
    global pass_count, fail_count, test_num
    test_num += 1
    if actual == expected:
        print(f"  PASS [{test_num}]: {name} = 0x{actual:016x}")
        pass_count += 1
    else:
        print(f"  FAIL [{test_num}]: {name} = 0x{actual:016x} "
              f"(expected 0x{expected:016x})")
        fail_count += 1


def check_bool(expected: bool, actual: bool, name: str):
    global pass_count, fail_count, test_num
    test_num += 1
    if actual == expected:
        print(f"  PASS [{test_num}]: {name} = {actual}")
        pass_count += 1
    else:
        print(f"  FAIL [{test_num}]: {name} = {actual} (expected {expected})")
        fail_count += 1


def test_open_close():
    print("\n=== Test: Open/Close (Mock) ===")
    a = ATOMiK.mock()
    check_bool(True, a is not None, "mock() succeeds")
    check_bool(True, a.version == 1, "version = 1")
    check_bool(True, a.n_banks == 1, "n_banks = 1")
    a.close()


def test_context_manager():
    print("\n=== Test: Context Manager ===")
    with ATOMiK.mock() as a:
        a.load(0, 0x1234)
        check64(0x1234, a.read(), "context manager works")


def test_config():
    print("\n=== Test: CONFIG Enable/Disable ===")
    a = ATOMiK.mock()
    check_bool(True, a.enabled, "initially enabled")

    a.enabled = False
    check_bool(False, a.enabled, "disabled after set")

    a.enabled = True
    check_bool(True, a.enabled, "re-enabled after set")
    a.close()


def test_load_read_roundtrip():
    print("\n=== Test: LOAD + READ Roundtrip ===")
    a = ATOMiK.mock()
    a.load(0, 0xDEADBEEFCAFEBABE)
    check64(0xDEADBEEFCAFEBABE, a.read(), "load/read roundtrip")
    a.close()


def test_accum_correctness():
    print("\n=== Test: ACCUM Correctness ===")
    a = ATOMiK.mock()
    a.load(0, 0xDEADBEEFCAFEBABE)
    a.accum(0x00000000000000FF)
    expected = 0xDEADBEEFCAFEBABE ^ 0x00000000000000FF
    check64(expected, a.read(), "state = init ^ delta")
    a.close()


def test_accum_multiple():
    print("\n=== Test: ACCUM Multiple ===")
    a = ATOMiK.mock()
    a.load(0, 0)
    a.accum(0x0000000000000001)
    a.accum(0x0000000000000002)
    a.accum(0x0000000000000004)
    check64(0x0000000000000007, a.read(), "1^2^4 = 7")
    a.close()


def test_xor_cancellation():
    print("\n=== Test: XOR Cancellation (Self-Inverse) ===")
    a = ATOMiK.mock()
    a.load(0, 0)
    a.accum(0xFEDCBA9876543210)
    a.accum(0xFEDCBA9876543210)
    check64(0, a.read(), "delta ^ delta = 0")
    check_bool(True, a.acc_zero, "acc_zero after cancellation")
    a.close()


def test_commutativity():
    print("\n=== Test: Commutativity ===")
    d1 = 0xAAAAAAAA55555555
    d2 = 0x55555555AAAAAAAA
    d3 = 0x1234567890ABCDEF

    a = ATOMiK.mock()

    # Order A: d1, d2, d3
    a.load(0, 0)
    a.accum(d1)
    a.accum(d2)
    a.accum(d3)
    result_a = a.read()

    # Order B: d3, d1, d2
    a.load(0, 0)
    a.accum(d3)
    a.accum(d1)
    a.accum(d2)
    result_b = a.read()

    # Order C: d2, d3, d1
    a.load(0, 0)
    a.accum(d2)
    a.accum(d3)
    a.accum(d1)
    result_c = a.read()

    check64(result_a, result_b, "order A == order B")
    check64(result_a, result_c, "order A == order C")
    a.close()


def test_identity():
    print("\n=== Test: Identity (Zero Delta) ===")
    a = ATOMiK.mock()
    a.load(0, 0xDEADBEEFCAFEBABE)
    a.accum(0)
    check64(0xDEADBEEFCAFEBABE, a.read(), "state unchanged after zero delta")
    check_bool(True, a.acc_zero, "acc_zero with zero delta")
    a.close()


def test_multi_address():
    print("\n=== Test: Multi-Address Isolation ===")
    a = ATOMiK.mock()
    a.load(0, 0x1111111111111111)
    a.load(1, 0x2222222222222222)
    a.load(2, 0x3333333333333333)

    check64(0x3333333333333333, a.read(), "addr2 active after last load")

    a.swap(0)
    check64(0x1111111111111111, a.read(), "addr0 preserved after swap")

    a.swap(1)
    check64(0x2222222222222222, a.read(), "addr1 preserved after swap")
    a.close()


def test_swap_preserves_accumulator():
    print("\n=== Test: SWAP Preserves Accumulator ===")
    a = ATOMiK.mock()
    a.load(0, 0x0000000000001111)
    a.load(1, 0x0000000000002222)

    a.swap(0)
    a.accum(0x0000000000000001)
    check64(0x0000000000001110, a.read(), "addr0: 0x1111^0x0001=0x1110")

    a.swap(1)
    check64(0x0000000000002223, a.read(),
            "addr1: 0x2222^0x0001=0x2223 (acc preserved)")
    a.close()


def test_64bit_boundaries():
    print("\n=== Test: 64-bit Boundary Patterns ===")
    a = ATOMiK.mock()

    a.load(0, 0xFFFFFFFFFFFFFFFF)
    check64(0xFFFFFFFFFFFFFFFF, a.read(), "all ones")

    a.load(0, 0xAAAAAAAA55555555)
    check64(0xAAAAAAAA55555555, a.read(), "alternating")

    a.load(0, 0x8000000000000000)
    check64(0x8000000000000000, a.read(), "MSB only")

    a.load(0, 0x0000000000000001)
    check64(0x0000000000000001, a.read(), "LSB only")

    a.load(0, 0xFFFFFFFF00000000)
    check64(0xFFFFFFFF00000000, a.read(), "upper half only")

    a.load(0, 0x00000000FFFFFFFF)
    check64(0x00000000FFFFFFFF, a.read(), "lower half only")
    a.close()


def test_address_boundaries():
    print("\n=== Test: Address Boundaries ===")
    a = ATOMiK.mock()
    a.load(0, 0xAAAAAAAAAAAAAAAA)
    a.load(127, 0xBBBBBBBBBBBBBBBB)
    a.load(255, 0xCCCCCCCCCCCCCCCC)

    check64(0xCCCCCCCCCCCCCCCC, a.read(), "addr 255")

    a.swap(0)
    check64(0xAAAAAAAAAAAAAAAA, a.read(), "addr 0")

    a.swap(127)
    check64(0xBBBBBBBBBBBBBBBB, a.read(), "addr 127")
    a.close()


def test_config_reset():
    print("\n=== Test: CONFIG Disable Resets Core ===")
    a = ATOMiK.mock()
    a.load(0, 0xDEADBEEFCAFEBABE)
    a.accum(0x00000000000000FF)

    expected = 0xDEADBEEFCAFEBABE ^ 0x00000000000000FF
    check64(expected, a.read(), "state before disable")

    a.enabled = False
    a.enabled = True
    check_bool(True, a.acc_zero, "acc_zero after re-enable")
    a.close()


def test_state_reconstruction():
    print("\n=== Test: State Reconstruction (core algebra) ===")
    a = ATOMiK.mock()
    init = 0x0123456789ABCDEF
    delta = 0xFEDCBA9876543210
    a.load(0, init)
    a.accum(delta)
    check64(0xFFFFFFFFFFFFFFFF, a.read(), "init^delta = all-ones")
    a.close()


def test_burst_accum():
    print("\n=== Test: Burst Accumulate (16 deltas) ===")
    a = ATOMiK.mock()
    a.load(0, 0)

    expected = 0
    for i in range(16):
        delta = ((i + 1) << (i * 4)) & 0xFFFFFFFFFFFFFFFF
        a.accum(delta)
        expected ^= delta

    check64(expected, a.read(), "16-delta burst")
    a.close()


def test_associativity():
    print("\n=== Test: Associativity ===")
    a = ATOMiK.mock()

    d1 = 0x1111111111111111
    d2 = 0x2222222222222222
    d3 = 0x4444444444444444

    # (d1 ^ d2) ^ d3
    a.load(0, 0)
    a.accum(d1)
    a.accum(d2)
    a.accum(d3)
    result_1 = a.read()

    # d1 ^ (d2 ^ d3)
    a.load(0, 0)
    a.accum(d2)
    a.accum(d3)
    a.accum(d1)
    result_2 = a.read()

    check64(result_1, result_2, "associativity: (d1^d2)^d3 == d1^(d2^d3)")
    a.close()


def main():
    print("============================================")
    print("ATOMiK Python Bindings Test Suite (MOCK)")
    print("============================================")

    test_open_close()
    test_context_manager()
    test_config()
    test_load_read_roundtrip()
    test_accum_correctness()
    test_accum_multiple()
    test_xor_cancellation()
    test_commutativity()
    test_identity()
    test_multi_address()
    test_swap_preserves_accumulator()
    test_64bit_boundaries()
    test_address_boundaries()
    test_config_reset()
    test_state_reconstruction()
    test_burst_accum()
    test_associativity()

    print(f"\n============================================")
    print(f"Test Summary: {pass_count}/{pass_count + fail_count} passed")
    print(f"============================================")

    if fail_count > 0:
        print(f"*** {fail_count} TESTS FAILED ***")
        return 1

    print("*** ALL TESTS PASSED ***")
    return 0


if __name__ == "__main__":
    sys.exit(main())
