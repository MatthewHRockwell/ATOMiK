#!/usr/bin/env python3
"""
Simple UART loopback/diagnostic test for ATOMiK hardware.
"""

import sys
import serial
import time

def test_loopback(port, baudrate=115200):
    """Test basic UART communication."""
    print(f"Opening {port} at {baudrate} baud...")
    
    ser = serial.Serial(port, baudrate, timeout=2.0)
    time.sleep(0.2)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    print("\n=== Test 1: Single byte status ===")
    ser.write(b'S')
    time.sleep(0.1)
    resp = ser.read(10)
    print(f"Sent: S (0x53)")
    print(f"Received: {resp.hex()} ({len(resp)} bytes)")
    
    print("\n=== Test 2: Load zeros and read ===")
    ser.reset_input_buffer()
    # Load all zeros
    cmd = b'L' + bytes([0]*8)
    ser.write(cmd)
    print(f"Sent LOAD: {cmd.hex()}")
    time.sleep(0.1)
    
    # Read back
    ser.write(b'R')
    time.sleep(0.2)
    resp = ser.read(16)
    print(f"Sent: R")
    print(f"Received: {resp.hex()} ({len(resp)} bytes)")
    
    print("\n=== Test 3: Load pattern and read ===")
    ser.reset_input_buffer()
    # Load a simple pattern
    pattern = bytes([0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA])
    cmd = b'L' + pattern
    ser.write(cmd)
    print(f"Sent LOAD: {cmd.hex()}")
    time.sleep(0.1)
    
    # Read back
    ser.write(b'R')
    time.sleep(0.2)
    resp = ser.read(16)
    print(f"Sent: R")
    print(f"Received: {resp.hex()} ({len(resp)} bytes)")
    if resp == pattern:
        print("MATCH!")
    else:
        print(f"MISMATCH - expected {pattern.hex()}")
    
    print("\n=== Test 4: Load incrementing pattern ===")
    ser.reset_input_buffer()
    pattern = bytes([0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08])
    cmd = b'L' + pattern
    ser.write(cmd)
    print(f"Sent LOAD: {cmd.hex()}")
    time.sleep(0.1)
    
    # Read back  
    ser.write(b'R')
    time.sleep(0.2)
    resp = ser.read(16)
    print(f"Sent: R")
    print(f"Received: {resp.hex()} ({len(resp)} bytes)")
    if resp == pattern:
        print("MATCH!")
    else:
        print(f"MISMATCH - expected {pattern.hex()}")
        if len(resp) == 8:
            # Analyze the mismatch
            for i in range(8):
                print(f"  Byte {i}: sent 0x{pattern[i]:02X}, got 0x{resp[i]:02X}, diff={resp[i] - pattern[i]}")
    
    print("\n=== Test 5: Byte-by-byte send with delays ===")
    ser.reset_input_buffer()
    print("Sending 'L' then bytes one at a time...")
    ser.write(b'L')
    time.sleep(0.01)
    for i, b in enumerate([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88]):
        ser.write(bytes([b]))
        print(f"  Sent byte {i}: 0x{b:02X}")
        time.sleep(0.01)
    
    time.sleep(0.1)
    ser.write(b'R')
    time.sleep(0.2)
    resp = ser.read(16)
    print(f"Sent: R")
    print(f"Received: {resp.hex()} ({len(resp)} bytes)")
    print(f"Expected: 1122334455667788")
    
    ser.close()
    print("\nDone.")

if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else "COM6"
    test_loopback(port)
