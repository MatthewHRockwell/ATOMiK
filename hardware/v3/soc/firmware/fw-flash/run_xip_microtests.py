#!/usr/bin/env python3
"""
XIP Microtest Suite - Systematic Flash Execution Debugging
Tests AUIPC, RAM, and flash latency isolation
"""

import serial
import time
import subprocess
import sys

BITSTREAM = '/home/mattrock/Projects/ATOMiK/hardware/v3/deploy/atomik_v3_soc_isp.fs'
PROGRAMMER = '/home/mattrock/Projects/ATOMiK/hardware/v3/soc/firmware/scripts/pico-programmer.py'
PORT = '/dev/ttyUSB1'
BAUD = 115200

def load_bitstream():
    """Load FPGA bitstream"""
    print("Loading bitstream...")
    result = subprocess.run(['openFPGALoader', '-b', 'tangnano9k', BITSTREAM],
                          capture_output=True, text=True)
    return 'DONE' in result.stdout or 'DONE' in result.stderr

def flash_firmware(fw_file):
    """Flash firmware via ISP"""
    print(f"Flashing {fw_file}...")
    if not load_bitstream():
        return False
    time.sleep(0.3)
    result = subprocess.run(['python3', PROGRAMMER, fw_file, PORT],
                          capture_output=True, text=True)
    return 'Flashing completed' in result.stdout

def read_uart(timeout=3):
    """Read UART output after bitstream load"""
    if not load_bitstream():
        return None
    time.sleep(0.5)

    ser = serial.Serial(PORT, BAUD, timeout=timeout)
    time.sleep(1)  # Wait past Boot ROM timeout
    data = ser.read(500)
    ser.close()

    return data.decode('utf-8', errors='replace') if data else None

def test_boot_rom_consistency():
    """Test 0: Boot ROM timeout consistency (5 trials)"""
    print("\n" + "="*60)
    print("TEST 0: Boot ROM Timeout Consistency")
    print("="*60)
    print("Testing if Boot ROM reliably prints 'JUMP!' on timeout...")

    results = []
    for i in range(5):
        output = read_uart(timeout=2)
        has_jump = 'JUMP!' in output if output else False
        results.append(has_jump)
        print(f"  Trial {i+1}: {'✅ JUMP!' if has_jump else '❌ No output'}")
        time.sleep(0.5)

    success_rate = sum(results) / len(results) * 100
    print(f"\n  Result: {success_rate:.0f}% success rate")
    if success_rate == 100:
        print("  ✅ PASS - Boot ROM timeout is deterministic")
        return True
    else:
        print(f"  ⚠️  INCONSISTENT - volatile waitcnt didn't fully fix it")
        return False

def run_test(test_name, fw_file, expected_chars):
    """Run a single microtest"""
    print(f"\n" + "="*60)
    print(f"TEST: {test_name}")
    print("="*60)

    if not flash_firmware(fw_file):
        print("  ❌ FAIL - Flash programming failed")
        return None

    output = read_uart(timeout=3)

    if not output:
        print("  ❌ FAIL - No UART output at all")
        print("     Interpretation: Test didn't execute (flash XIP completely broken)")
        return {'result': 'NO_OUTPUT', 'chars': []}

    # Check for expected characters
    found_chars = [c for c in expected_chars if c in output]
    missing_chars = [c for c in expected_chars if c not in output]

    print(f"  Output: {repr(output[:50])}")
    print(f"  Expected: {expected_chars}")
    print(f"  Found: {found_chars}")

    if 'JUMP!' not in output:
        print("  ⚠️  WARNING - No 'JUMP!' in output (Boot ROM issue)")

    if found_chars == expected_chars:
        print(f"  ✅ PASS - All expected characters present")
        return {'result': 'PASS', 'chars': found_chars, 'output': output}
    elif found_chars:
        print(f"  ⚠️  PARTIAL - Found {found_chars}, missing {missing_chars}")
        return {'result': 'PARTIAL', 'chars': found_chars, 'output': output}
    else:
        print(f"  ❌ FAIL - None of expected chars found")
        return {'result': 'FAIL', 'chars': [], 'output': output}

def main():
    print("\n" + "🧪"*30)
    print("XIP MICROTEST SUITE - Flash Execution Diagnosis")
    print("🧪"*30)

    # Test 0: Boot ROM consistency
    bootrom_ok = test_boot_rom_consistency()

    # Test 1: AUIPC-only
    test1 = run_test(
        "Test 1: AUIPC-only (no RAM, no stack)",
        "build/test_auipc.v",
        ['A']  # Should print 'A' repeatedly if AUIPC works
    )

    # Test 2: Stack touch
    test2 = run_test(
        "Test 2: Stack/RAM touch (no AUIPC)",
        "build/test_stack.v",
        ['S', 'R']  # 'S' after write, 'R' after read-verify
    )

    # Test 3: NOP-padded CRT
    test3 = run_test(
        "Test 3: NOP-padded CRT prologue",
        "build/test_nop_crt.v",
        ['N', 'G', 'P']  # 'N' start, 'G' GP loaded, 'P' SP loaded
    )

    # Summary and diagnosis
    print("\n" + "="*60)
    print("DIAGNOSTIC SUMMARY")
    print("="*60)

    if not bootrom_ok:
        print("\n⚠️  Boot ROM timeout still inconsistent")
        print("   → Consider increasing FW_WAIT_MAXCNT further")

    if test1 and test1['result'] == 'PASS':
        print("\n✅ AUIPC+ADDI works from flash XIP")
    elif test1 and test1['result'] == 'FAIL':
        print("\n❌ AUIPC+ADDI FAILS from flash XIP")
        print("   → This is the root cause - flash controller or CPU issue")
        print("   → CRT uses 'la gp, __global_pointer$' which is AUIPC+ADDI")

    if test2 and test2['result'] == 'PASS':
        print("\n✅ RAM read/write works")
    elif test2 and test2['result'] == 'FAIL':
        print("\n❌ RAM access FAILS")
        print("   → RAM mapping or timing issue")
        print("   → CRT crashes when initializing stack")

    if test3 and test3['result'] == 'PASS':
        print("\n✅ NOP padding allows CRT to progress")
        print("   → Flash fetch bandwidth/latency is the issue")
        print("   → Need to slow down instruction fetch or improve XIP timing")
    elif test3 and test3['result'] == 'FAIL':
        if test1 and test1['result'] == 'PASS':
            print("\n⚠️  NOP padding didn't help but AUIPC works")
            print("   → Issue is likely in CRT logic itself, not instruction fetch")

    # Decision tree
    print("\n" + "-"*60)
    print("NEXT STEPS:")
    print("-"*60)

    if test1 and test1['result'] != 'PASS':
        print("\n1. AUIPC broken → Investigate flash XIP controller")
        print("   - Check spimemio_puya.v for PC-relative read support")
        print("   - May need to add wait states or change read command")

    elif test2 and test2['result'] != 'PASS':
        print("\n1. RAM broken → Investigate memory controller")
        print("   - Check RAM initialization and timing")
        print("   - Verify address decode for 0x40000000 range")

    elif test3 and test3['result'] == 'PASS':
        print("\n1. Flash latency issue → Slow down instruction fetch")
        print("   - Add wait states to spimemio_puya.v")
        print("   - Or use faster flash read command (0xEB vs 0x03)")
        print("   - Or lower CPU clock from 27 MHz")

    else:
        print("\n1. All microtests failed → Fundamental flash XIP issue")
        print("   - Even simple instructions not executing")
        print("   - Check flash XIP mode configuration")
        print("   - Verify Boot ROM jump mechanism")

    print()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
