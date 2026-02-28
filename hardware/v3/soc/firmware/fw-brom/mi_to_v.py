#!/usr/bin/env python3
"""
Update BSRAM .v files with firmware data from .mi files.

Reads boot_rom_N.mi and updates the corresponding bootram_2kx8_N.v file
with INIT_RAM_XX defparams.
"""

import sys
import os
import re

def parse_mi_file(mi_file):
    """Parse .mi file and return list of bytes"""
    bytes_data = []

    with open(mi_file, 'r') as f:
        for line in f:
            line = line.strip()
            # Skip comments and directives
            if line.startswith('//') or line.startswith('#') or not line:
                continue

            # Parse hex bytes (space or tab separated)
            hex_bytes = line.split()
            for hex_byte in hex_bytes:
                if len(hex_byte) == 2:
                    bytes_data.append(int(hex_byte, 16))

    return bytes_data

def bytes_to_init_params(bytes_data, size=2048):
    """
    Convert byte array to INIT_RAM_XX parameters.

    Gowin SP BSRAM has 2048 bytes, organized as 64 banks of 32 bytes each.
    Each INIT_RAM_XX parameter holds 32 bytes (256 bits).

    INIT_RAM_00 = bytes[0:31]    (little-endian: byte 0 is LSB)
    INIT_RAM_01 = bytes[32:63]
    ...
    INIT_RAM_3F = bytes[2016:2047]
    """

    # Pad to 2048 bytes
    while len(bytes_data) < size:
        bytes_data.append(0)

    init_params = {}

    for bank in range(64):  # 64 banks (0x00 to 0x3F)
        start = bank * 32
        end = start + 32
        bank_bytes = bytes_data[start:end]

        # Convert to 256-bit hex string (little-endian: first byte is LSB)
        hex_str = ''.join(f'{b:02x}' for b in reversed(bank_bytes))
        init_params[f'INIT_RAM_{bank:02X}'] = f"256'h{hex_str}"

    return init_params

def update_bsram_v_file(v_file, init_params):
    """Update .v file with new INIT_RAM_XX defparams"""

    with open(v_file, 'r') as f:
        content = f.read()

    # Replace each INIT_RAM_XX line
    for param_name, param_value in init_params.items():
        pattern = rf'defparam sp_inst_0\.{param_name} = 256\'h[0-9A-Fa-f]+;'
        replacement = f'defparam sp_inst_0.{param_name} = {param_value};'
        content = re.sub(pattern, replacement, content)

    with open(v_file, 'w') as f:
        f.write(content)

def main():
    if len(sys.argv) < 2:
        print('Usage: mi_to_v.py <mi_dir> <v_dir>')
        print('Example: mi_to_v.py bootram_2kx8_0 .')
        sys.exit(1)

    mi_dir = sys.argv[1]
    v_dir = sys.argv[2] if len(sys.argv) > 2 else mi_dir

    # Process all 4 BSRAM blocks
    for i in range(4):
        mi_file = os.path.join(mi_dir, f'boot_rom_{i}.mi')
        v_file = os.path.join(v_dir, f'bootram_2kx8_{i}', f'bootram_2kx8_{i}.v')

        if not os.path.exists(mi_file):
            print(f'Error: {mi_file} not found')
            continue

        if not os.path.exists(v_file):
            print(f'Error: {v_file} not found')
            continue

        print(f'Processing {mi_file} → {v_file}...')

        # Parse .mi file
        bytes_data = parse_mi_file(mi_file)
        print(f'  Read {len(bytes_data)} bytes from .mi')

        # Convert to INIT_RAM parameters
        init_params = bytes_to_init_params(bytes_data)

        # Update .v file
        update_bsram_v_file(v_file, init_params)
        print(f'  Updated {v_file}')

    print('Done!')

if __name__ == '__main__':
    main()
