#!/usr/bin/env python3
"""
ISP Flash Programmer for ATOMiK v3 SoC
Programs firmware to SPI flash via UART ISP protocol
"""

import serial
import subprocess
import time
import sys
from pathlib import Path

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
        if b'A' not in resp or b'B' not in resp:
            raise RuntimeError(f"WPAG failed: {resp}")

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

    def read_verilog_hex(self, vfile_path):
        """Read Verilog hex file and return binary data"""
        print(f"Reading firmware: {vfile_path}")

        data = bytearray()
        with open(vfile_path, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('@'):
                    # Address line - ignore for now (assume sequential)
                    continue
                elif line:
                    # Hex data line
                    hex_bytes = line.split()
                    for hb in hex_bytes:
                        data.append(int(hb, 16))

        print(f"✓ Read {len(data)} bytes\n")
        return bytes(data)


def main():
    if len(sys.argv) < 2:
        print("Usage: isp_flash_programmer.py <firmware.v>")
        print("\nExample:")
        print("  ./isp_flash_programmer.py ../soc/firmware/fw-flash/test_flash_minimal.v")
        sys.exit(1)

    firmware_path = Path(sys.argv[1])
    if not firmware_path.exists():
        print(f"ERROR: Firmware file not found: {firmware_path}")
        sys.exit(1)

    bitstream_path = Path("atomik_v3_soc.fs")
    if not bitstream_path.exists():
        print(f"ERROR: Bitstream not found: {bitstream_path}")
        print("Run synthesis first or copy bitstream to current directory")
        sys.exit(1)

    prog = ISPFlashProgrammer()

    try:
        # Step 1: Load bitstream with ISP firmware
        prog.load_bitstream(bitstream_path)

        # Step 2: Open UART and handshake
        prog.open_uart()
        prog.handshake()

        # Step 3: Read firmware
        firmware_data = prog.read_verilog_hex(firmware_path)

        # Step 4: Program to flash
        prog.program_firmware(firmware_data, base_addr=0x00000000)

        # Step 5: Reset and test
        print("=== Programming Complete ===")
        print("\nTo test persistent boot:")
        print("1. Power cycle the FPGA (unplug USB)")
        print("2. Reload bitstream: openFPGALoader -b tangnano9k atomik_v3_soc.fs")
        print("3. Wait for ISP timeout (~200ms)")
        print("4. Flash firmware should execute (look for 'F!F!' on UART)")

    except Exception as e:
        print(f"\nERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
    finally:
        prog.close()

if __name__ == "__main__":
    main()
