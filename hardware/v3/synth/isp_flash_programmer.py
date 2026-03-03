#!/usr/bin/env python3
"""
ISP Flash Programmer for ATOMiK v3 SoC
Programs firmware to SPI flash via UART ISP protocol.
Supports --boot-test mode for automated hardware regression.
"""

import argparse
import serial
import struct
import subprocess
import time
import sys
import zlib
from pathlib import Path

FLASH_HDR_MAGIC = 0x4D4F5441  # "ATOM" little-endian
FLASH_HDR_SIZE = 16

class ISPFlashProgrammer:
    def __init__(self, port='/dev/ttyUSB1', baud=115200):
        self.port = port
        self.baud = baud
        self.ser = None

    def load_bitstream(self, bitstream_path):
        """Load FPGA bitstream to SRAM"""
        print(f"Loading bitstream: {bitstream_path}")
        result = subprocess.run([
            "openFPGALoader", "-b", "tangnano9k",
            str(bitstream_path)
        ], capture_output=True, text=True)

        if result.returncode != 0:
            raise RuntimeError(f"Bitstream load failed: {result.stderr}")
        print("✓ Bitstream loaded\n")

    def open_uart(self):
        """Open UART connection"""
        print(f"Opening UART: {self.port} @ {self.baud}")
        self.ser = serial.Serial(self.port, self.baud, timeout=2)
        time.sleep(0.2)
        self.ser.read(100)  # Clear boot messages
        print("✓ UART open\n")

    def handshake(self):
        """ISP handshake"""
        print("ISP handshake...")
        self.ser.write(bytes([0x55]))
        time.sleep(0.1)

        ack = self.ser.read(10)
        if b'V' not in ack and 0x56 not in ack:
            raise RuntimeError(f"Handshake failed: {ack}")
        print("✓ Handshake ACK received\n")

    def erase_sector(self, addr):
        """Erase 4KB sector at address"""
        print(f"Erasing sector at 0x{addr:06x}...")

        addr_bytes = [
            (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF,
            addr & 0xFF
        ]

        self.ser.write(bytes([0x30] + addr_bytes))
        time.sleep(0.6)  # Sector erase takes time

        resp = self.ser.read(10)
        if b'1' not in resp or b'2' not in resp:
            raise RuntimeError(f"ESEC failed: {resp}")
        print(f"✓ Sector erased\n")

    def write_buffer(self, data):
        """Write data to page buffer (max 256 bytes)"""
        if len(data) > 256:
            raise ValueError(f"Data too large: {len(data)} > 256")

        length = len(data) - 1  # Protocol uses len-1
        self.ser.write(bytes([0x10, length]))
        self.ser.write(data)
        time.sleep(0.1)

        resp = self.ser.read(10)
        if len(resp) < 2 or resp[0] != 0x11:
            raise RuntimeError(f"WBUF failed: {resp}")

        expected_chksum = sum(data) & 0xFF
        actual_chksum = resp[1]

        if actual_chksum != expected_chksum:
            raise RuntimeError(f"Checksum mismatch: 0x{actual_chksum:02x} != 0x{expected_chksum:02x}")

        return actual_chksum

    def program_page(self, addr):
        """Program page at address from buffer"""
        addr_bytes = [
            (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF,
            addr & 0xFF
        ]

        self.ser.write(bytes([0x40] + addr_bytes))
        time.sleep(0.4)  # Page program takes time

        resp = self.ser.read(10)
        if 0x4F in resp:
            raise RuntimeError(f"WPAG NACK: no buffer data loaded (send WBUF first)")
        if b'A' not in resp or b'B' not in resp:
            raise RuntimeError(f"WPAG failed: {resp}")

    def read_flash(self, addr, length):
        """Read flash data for verification (max 256 bytes)"""
        if length > 256:
            raise ValueError(f"Read too large: {length} > 256")

        addr_bytes = [
            (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF,
            addr & 0xFF
        ]

        self.ser.write(bytes([0x50] + addr_bytes + [length - 1]))
        time.sleep(0.2)

        # Read ACK byte
        ack = self.ser.read(1)
        if not ack or ack[0] != 0x51:
            raise RuntimeError(f"RDBK ACK failed: {ack}")

        # Read data + checksum
        data = self.ser.read(length)
        if len(data) != length:
            raise RuntimeError(f"RDBK short read: {len(data)}/{length}")

        chksum_byte = self.ser.read(1)
        if not chksum_byte:
            raise RuntimeError("RDBK checksum missing")

        expected_chksum = sum(data) & 0xFF
        if chksum_byte[0] != expected_chksum:
            raise RuntimeError(
                f"RDBK checksum: 0x{chksum_byte[0]:02x} != 0x{expected_chksum:02x}")

        return bytes(data)

    def verify_firmware(self, firmware_data, base_addr=0x00000000):
        """Verify programmed data by reading back and comparing"""
        total_bytes = len(firmware_data)
        print(f"Verifying {total_bytes} bytes at 0x{base_addr:06x}...")

        offset = 0
        while offset < total_bytes:
            chunk_size = min(256, total_bytes - offset)
            read_addr = base_addr + offset

            readback = self.read_flash(read_addr, chunk_size)
            expected = firmware_data[offset:offset + chunk_size]

            if readback != expected:
                for i, (got, exp) in enumerate(zip(readback, expected)):
                    if got != exp:
                        raise RuntimeError(
                            f"Verify FAIL at 0x{read_addr + i:06x}: "
                            f"read 0x{got:02x}, expected 0x{exp:02x}")

            offset += chunk_size

        print(f"✓ Verified {total_bytes} bytes\n")

    def program_firmware(self, firmware_data, base_addr=0x00000000):
        """Program firmware to flash"""
        total_bytes = len(firmware_data)
        print(f"Programming {total_bytes} bytes to 0x{base_addr:06x}...")
        print(f"Pages required: {(total_bytes + 255) // 256}\n")

        # Erase required sectors (4KB each = 16 pages of 256 bytes)
        sectors_needed = (total_bytes + 4095) // 4096
        for sector in range(sectors_needed):
            sector_addr = base_addr + sector * 4096
            self.erase_sector(sector_addr)

        # Program pages
        offset = 0
        page_num = 0
        while offset < total_bytes:
            page_data = firmware_data[offset:offset+256]

            # Pad last page to 256 bytes if needed
            if len(page_data) < 256:
                page_data = page_data + bytes([0xFF] * (256 - len(page_data)))

            page_addr = base_addr + offset

            print(f"Page {page_num}: 0x{page_addr:06x} ({len(page_data)} bytes)")
            chksum = self.write_buffer(page_data)
            print(f"  WBUF: checksum 0x{chksum:02x}")

            self.program_page(page_addr)
            print(f"  WPAG: ✓")

            offset += 256
            page_num += 1

        print(f"\n✓ Programmed {total_bytes} bytes in {page_num} pages\n")

    def generate_header(self, firmware_data):
        """Generate 16-byte flash header with CRC32"""
        image_len = len(firmware_data)
        crc = zlib.crc32(firmware_data) & 0xFFFFFFFF
        entry = 0x00000010  # Right after header
        header = struct.pack('<IIII', FLASH_HDR_MAGIC, image_len, crc, entry)
        print(f"Flash header:")
        print(f"  Magic:     0x{FLASH_HDR_MAGIC:08X} ('ATOM')")
        print(f"  Image len: {image_len} bytes (0x{image_len:X})")
        print(f"  CRC32:     0x{crc:08X}")
        print(f"  Entry:     0x{entry:08X}")
        print()
        return header

    def reset(self):
        """Reset to BROM"""
        print("Resetting to BROM...")
        self.ser.write(bytes([0xF0]))
        time.sleep(0.1)
        resp = self.ser.read(10)
        print(f"Reset response: {resp}\n")

    def close(self):
        """Close UART"""
        if self.ser:
            self.ser.close()
            self.ser = None

    def boot_and_capture(self, bitstream_path, timeout_s=25):
        """Reload bitstream and capture UART output from firmware boot.

        After programming, this reloads the bitstream to trigger a fresh boot:
        BROM starts → ISP timeout (~16s) → CRC gate validates → JUMP! → firmware runs.
        Captures all UART output until timeout or early termination (OK/FAIL detected).
        """
        print("\n=== Boot Test ===\n")

        # Close UART before bitstream reload
        self.close()

        # Reload bitstream (triggers fresh BROM boot)
        self.load_bitstream(bitstream_path)

        # Reopen UART and start capturing
        print(f"Opening UART: {self.port} @ {self.baud}")
        self.ser = serial.Serial(self.port, self.baud, timeout=0.5)
        time.sleep(0.2)
        self.ser.read(100)  # Flush stale data

        print(f"Waiting for ISP timeout + firmware boot (up to {timeout_s}s)...")
        captured = bytearray()
        start = time.time()
        while time.time() - start < timeout_s:
            chunk = self.ser.read(256)
            if chunk:
                captured.extend(chunk)
                # Early termination: firmware printed final result
                text = captured.decode('ascii', errors='replace')
                if '\nOK\n' in text or 'OK\r' in text:
                    break
                if '\nFAIL\n' in text or 'FAIL\r' in text:
                    break
            else:
                # Print progress dot every ~0.5s of silence
                elapsed = time.time() - start
                sys.stdout.write('.')
                sys.stdout.flush()

        elapsed = time.time() - start
        print(f"\nCapture complete ({elapsed:.1f}s, {len(captured)} bytes)\n")

        # Decode and clean up
        text = captured.decode('ascii', errors='replace')
        # Strip non-printable except newline/CR
        clean = ''.join(c for c in text if c == '\n' or c == '\r' or (32 <= ord(c) < 127))
        return clean

    def read_verilog_hex(self, vfile_path):
        """Read Verilog hex file and return binary data.

        Respects @address segments: fills gaps with 0xFF and returns
        a flat binary starting from the lowest address.
        """
        print(f"Reading firmware: {vfile_path}")

        segments = []
        current_addr = 0
        current_data = bytearray()

        with open(vfile_path, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('@'):
                    if current_data:
                        segments.append((current_addr, bytes(current_data)))
                        current_data = bytearray()
                    current_addr = int(line[1:], 16)
                elif line:
                    hex_bytes = line.split()
                    for hb in hex_bytes:
                        current_data.append(int(hb, 16))
        if current_data:
            segments.append((current_addr, bytes(current_data)))

        if not segments:
            raise ValueError("No data found in Verilog hex file")

        # Build flat binary from lowest address
        base_addr = segments[0][0]
        last_seg = segments[-1]
        total_size = (last_seg[0] + len(last_seg[1])) - base_addr

        data = bytearray([0xFF] * total_size)
        for addr, seg_data in segments:
            offset = addr - base_addr
            data[offset:offset + len(seg_data)] = seg_data

        print(f"✓ Read {len(data)} bytes (base 0x{base_addr:X}, {len(segments)} segments)\n")
        return bytes(data), base_addr


def main():
    parser = argparse.ArgumentParser(
        description='ISP Flash Programmer for ATOMiK v3 SoC')
    parser.add_argument('firmware', type=Path,
                        help='Path to firmware .v file (Verilog hex)')
    parser.add_argument('--port', default='/dev/ttyUSB1',
                        help='UART port (default: /dev/ttyUSB1)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='UART baud rate (default: 115200)')
    parser.add_argument('--boot-test', action='store_true',
                        help='After programming, reload bitstream and capture boot output')
    parser.add_argument('--expect', type=str, default=None,
                        help='Expected substring in boot output (requires --boot-test)')
    parser.add_argument('--timeout', type=int, default=25,
                        help='Boot capture timeout in seconds (default: 25)')
    args = parser.parse_args()

    if args.expect and not args.boot_test:
        parser.error("--expect requires --boot-test")

    firmware_path = args.firmware
    if not firmware_path.exists():
        print(f"ERROR: Firmware file not found: {firmware_path}")
        sys.exit(1)

    bitstream_path = Path("atomik_v3_soc.fs")
    if not bitstream_path.exists():
        print(f"ERROR: Bitstream not found: {bitstream_path}")
        print("Run synthesis first or copy bitstream to current directory")
        sys.exit(1)

    prog = ISPFlashProgrammer(port=args.port, baud=args.baud)

    try:
        # Step 1: Open UART FIRST (before bitstream load)
        # CH552T drops data if port not open; we need the ISP handshake window
        prog.open_uart()

        # Step 2: Load bitstream (triggers CPU reset → ISP firmware starts)
        prog.load_bitstream(bitstream_path)

        # Step 3: Wait for FPGA boot then handshake within ISP timeout window
        time.sleep(0.5)  # Let FPGA initialize
        prog.ser.read(1000)  # Flush any leftover data from previous boot
        prog.handshake()

        # Step 4: Read firmware and generate header
        firmware_data, fw_base = prog.read_verilog_hex(firmware_path)
        print(f"Firmware base address: 0x{fw_base:X}")
        header = prog.generate_header(firmware_data)
        full_image = header + firmware_data

        # Step 5: Program header + firmware to flash
        prog.program_firmware(full_image, base_addr=0x00000000)

        # Step 6: Readback verify
        prog.verify_firmware(full_image, base_addr=0x00000000)

        print("=== Programming + Verify Complete ===")

        # Step 7: Boot test (optional)
        if args.boot_test:
            output = prog.boot_and_capture(bitstream_path,
                                           timeout_s=args.timeout)
            print("=== Boot Test Output ===")
            print(output)
            print("========================")

            if args.expect:
                if args.expect in output:
                    print(f"\nPASS: found expected output")
                    sys.exit(0)
                else:
                    print(f"\nFAIL: expected substring not found")
                    print(f"  Expected: {args.expect!r}")
                    sys.exit(1)
        else:
            print("\nTo test persistent boot:")
            print("1. Power cycle the FPGA (unplug USB)")
            print("2. Reload bitstream: openFPGALoader -b tangnano9k atomik_v3_soc.fs")
            print("3. Wait for ISP timeout (~16s at 21.6 MHz)")
            print("4. Flash firmware should execute (look for test output on UART)")

    except Exception as e:
        print(f"\nERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
    finally:
        prog.close()

if __name__ == "__main__":
    main()
