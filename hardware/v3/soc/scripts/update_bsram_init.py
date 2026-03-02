#!/usr/bin/env python3
"""
Update Gowin BSRAM IP block .v files with new initialization data from .mi files.

Usage: python3 update_bsram_init.py <mi_file_base> <ip_dir_base>
Example: python3 update_bsram_init.py build-atomik/test_atomik_brom ../gowin_ip/bootram_2kx8

NOTE: This script expects .mix4 format files (_0.mi through _3.mi) where each file
contains ONE BYTE LANE of the 32-bit words. It will reconstruct the full byte stream
before splitting into INIT_RAM chunks.
"""

import sys
import re

def read_mi_file(mi_path):
    """Read a .mi file and return list of hex bytes."""
    bytes_data = []
    with open(mi_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('#'):
                # Each line is a 2-digit hex byte (for 8-bit .mi files)
                bytes_data.append(int(line, 16))
    return bytes_data

def bytes_to_init_rams(bytes_data):
    """Convert byte array to INIT_RAM_xx defparam strings.

    bytes_data should be sequential firmware bytes (NOT byte-lane-separated).
    Each INIT_RAM_xx holds 32 bytes (256 bits).
    BSRAM is 2048 bytes total = 64 INIT_RAM parameters (00-3F).
    """
    init_rams = {}

    # Process 32 bytes at a time
    for ram_idx in range(64):
        start_byte = ram_idx * 32
        end_byte = start_byte + 32

        # Get 32 bytes, pad with zeros if needed
        chunk = bytes_data[start_byte:end_byte]
        while len(chunk) < 32:
            chunk.append(0)

        # Convert to 256-bit hex string (64 hex digits)
        # Bytes are stored LSB first in the defparam
        hex_str = ''.join(f'{b:02X}' for b in reversed(chunk))

        init_rams[f'{ram_idx:02X}'] = hex_str

    return init_rams

def update_verilog_file(v_path, init_rams):
    """Update the INIT_RAM_xx defparams in a .v file."""
    with open(v_path, 'r') as f:
        lines = f.readlines()

    # Find and replace INIT_RAM_xx lines
    for i, line in enumerate(lines):
        match = re.match(r'^defparam\s+sp_inst_0\.INIT_RAM_([0-9A-F]{2})\s*=', line)
        if match:
            ram_idx = match.group(1)
            if ram_idx in init_rams:
                lines[i] = f"defparam sp_inst_0.INIT_RAM_{ram_idx} = 256'h{init_rams[ram_idx]};\n"

    with open(v_path, 'w') as f:
        f.writelines(lines)

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    mi_base = sys.argv[1]  # e.g., "build-atomik/test_atomik_brom"
    ip_base = sys.argv[2]   # e.g., "../gowin_ip/bootram_2kx8"

    # Read all 4 byte lanes
    print("Reading byte-lane-separated .mi files...")
    byte_lanes = []
    for i in range(4):
        mi_file = f"{mi_base}_{i}.mi"
        lane_data = read_mi_file(mi_file)
        print(f"  Read {len(lane_data)} bytes from lane {i}: {mi_file}")
        byte_lanes.append(lane_data)

    # Verify all lanes have the same length
    lane_len = len(byte_lanes[0])
    if not all(len(lane) == lane_len for lane in byte_lanes):
        print("ERROR: Byte lanes have different lengths!")
        sys.exit(1)

    # Reconstruct sequential byte stream from byte lanes
    # byte_lanes[0][i] is byte (i*4 + 0) of firmware
    # byte_lanes[1][i] is byte (i*4 + 1) of firmware
    # byte_lanes[2][i] is byte (i*4 + 2) of firmware
    # byte_lanes[3][i] is byte (i*4 + 3) of firmware
    sequential_bytes = []
    for i in range(lane_len):
        sequential_bytes.append(byte_lanes[0][i])  # byte (i*4 + 0)
        sequential_bytes.append(byte_lanes[1][i])  # byte (i*4 + 1)
        sequential_bytes.append(byte_lanes[2][i])  # byte (i*4 + 2)
        sequential_bytes.append(byte_lanes[3][i])  # byte (i*4 + 3)

    print(f"Reconstructed {len(sequential_bytes)} sequential bytes")
    print(f"First 16 bytes: {' '.join(f'{b:02X}' for b in sequential_bytes[:16])}")

    # Now split the sequential bytes back into 4 byte lanes for each BSRAM block
    # Each BSRAM block gets bytes (lane_idx, lane_idx+4, lane_idx+8, ...)
    for lane_idx in range(4):
        # Extract this lane's bytes
        lane_bytes = [sequential_bytes[i] for i in range(lane_idx, len(sequential_bytes), 4)]

        print(f"\nProcessing BSRAM lane {lane_idx}:")
        print(f"  Extracted {len(lane_bytes)} bytes (every 4th byte starting at {lane_idx})")

        # Convert to INIT_RAM parameters
        init_rams = bytes_to_init_rams(lane_bytes)

        # Update .v file
        ip_dir = f"{ip_base}_{lane_idx}"
        v_file = f"{ip_dir}/bootram_2kx8_{lane_idx}.v"
        update_verilog_file(v_file, init_rams)
        print(f"  Updated {v_file}")

    print("\nDone!")

if __name__ == '__main__':
    main()
