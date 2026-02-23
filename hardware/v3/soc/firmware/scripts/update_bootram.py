#!/usr/bin/env python3
"""Update Gowin bootram_2kx8 IP files with new BROM firmware init data.

Usage:
    python3 update_bootram.py <mi_dir> <bootram_dir>

Where:
    mi_dir     = directory containing fw-brom_0.mi .. fw-brom_3.mi
    bootram_dir = directory containing bootram_2kx8_0/ .. bootram_2kx8_3/
"""
import sys
import os
import re

def read_mi_file(path):
    """Read a Gowin .mi file and return list of byte values."""
    data = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith('#') or not line:
                continue
            data.append(int(line, 16))
    return data

def bytes_to_init_ram(data):
    """Convert byte array to INIT_RAM_xx parameter strings.

    Each INIT_RAM block is 256 bits = 32 bytes.
    Byte ordering: rightmost = lowest address (byte 0).
    """
    params = []
    for block in range(0x40):  # 64 blocks for 2048 bytes
        start = block * 32
        end = start + 32
        chunk = data[start:end]
        # Pad with zeros if needed
        while len(chunk) < 32:
            chunk.append(0)
        # Build hex string: MSB (byte 31) first, LSB (byte 0) last
        hex_str = ''.join(f'{b:02X}' for b in reversed(chunk))
        params.append(f"defparam sp_inst_0.INIT_RAM_{block:02X} = 256'h{hex_str};")
    return params

def update_bootram_file(verilog_path, init_params):
    """Replace INIT_RAM parameters in a bootram Verilog file."""
    with open(verilog_path) as f:
        content = f.read()

    # Remove existing INIT_RAM lines
    lines = content.split('\n')
    new_lines = []
    for line in lines:
        if 'INIT_RAM_' in line and 'defparam' in line:
            continue
        new_lines.append(line)

    # Find insertion point (after the last defparam that's not INIT_RAM,
    # or after RESET_MODE line)
    content = '\n'.join(new_lines)

    # Insert after RESET_MODE line
    insert_after = 'defparam sp_inst_0.RESET_MODE = "SYNC";'
    if insert_after in content:
        idx = content.index(insert_after) + len(insert_after)
        content = content[:idx] + '\n' + '\n'.join(init_params) + content[idx:]

    with open(verilog_path, 'w') as f:
        f.write(content)

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    mi_dir = sys.argv[1]
    bootram_dir = sys.argv[2]

    for i in range(4):
        mi_path = os.path.join(mi_dir, f'fw-brom_{i}.mi')
        verilog_path = os.path.join(bootram_dir, f'bootram_2kx8_{i}', f'bootram_2kx8_{i}.v')

        if not os.path.exists(mi_path):
            print(f"ERROR: {mi_path} not found")
            sys.exit(1)
        if not os.path.exists(verilog_path):
            print(f"ERROR: {verilog_path} not found")
            sys.exit(1)

        data = read_mi_file(mi_path)
        params = bytes_to_init_ram(data)
        update_bootram_file(verilog_path, params)
        print(f"Updated {verilog_path} with {len(data)} bytes from {mi_path}")

if __name__ == '__main__':
    main()
