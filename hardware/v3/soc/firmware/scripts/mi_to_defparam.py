#!/usr/bin/env python3
"""
Convert .mi file (32-bit hex words) to Gowin BSRAM defparam format.

Input: .mi file with 32-bit words (big-endian hex)
Output: 64 INIT_RAM_XX defparams, each containing 256 bits (8 x 32-bit words)

For 4-bank split BROM:
- Bank 0 gets bits [7:0] of each word
- Bank 1 gets bits [15:8]
- Bank 2 gets bits [23:16]
- Bank 3 gets bits [31:24]
"""

import sys

def read_mi_file(filename):
    """Read .mi file and return list of 32-bit words."""
    words = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('#'):
                # Each line is a 32-bit hex word (8 hex chars)
                words.append(line)
    return words

def extract_byte_bank(words, bank_index):
    """Extract one byte lane from 32-bit words."""
    bank_data = []
    for word in words:
        # word is like "0200006F" (32-bit, 8 hex chars)
        # Extract byte based on bank index (little-endian)
        # Bank 0 = [7:0]   = chars[6:8]
        # Bank 1 = [15:8]  = chars[4:6]
        # Bank 2 = [23:16] = chars[2:4]
        # Bank 3 = [31:24] = chars[0:2]
        byte_map = {
            0: word[6:8],  # Bits [7:0]
            1: word[4:6],  # Bits [15:8]
            2: word[2:4],  # Bits [23:16]
            3: word[0:2],  # Bits [31:24]
        }
        bank_data.append(byte_map[bank_index])
    return bank_data

def bytes_to_defparams(byte_data):
    """Convert byte array to 64 INIT_RAM defparams (256 bits each = 32 bytes)."""
    # Pad to 2048 bytes (64 * 32)
    while len(byte_data) < 2048:
        byte_data.append("00")

    defparams = []
    for i in range(64):
        # Each defparam is 256 bits = 32 bytes = 64 hex chars
        start_byte = i * 32
        chunk = byte_data[start_byte:start_byte + 32]
        # Reverse to big-endian for Verilog 256'h notation
        chunk.reverse()
        hex_str = "".join(chunk).upper()
        defparams.append(f"defparam sp_inst_0.INIT_RAM_{i:02X} = 256'h{hex_str};")

    return defparams

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.mi> <bank_index>")
        print("  bank_index: 0-3 for byte lanes [7:0], [15:8], [23:16], [31:24]")
        sys.exit(1)

    mi_file = sys.argv[1]
    bank_index = int(sys.argv[2])

    if bank_index not in [0, 1, 2, 3]:
        print("Error: bank_index must be 0-3")
        sys.exit(1)

    # Read 32-bit words from .mi file
    words = read_mi_file(mi_file)
    print(f"Read {len(words)} words from {mi_file}", file=sys.stderr)

    # Extract byte lane for this bank
    bank_bytes = extract_byte_bank(words, bank_index)
    print(f"Extracted bank {bank_index}: {len(bank_bytes)} bytes", file=sys.stderr)

    # Convert to defparams
    defparams = bytes_to_defparams(bank_bytes)

    # Output defparams
    for dp in defparams:
        print(dp)

if __name__ == "__main__":
    main()
