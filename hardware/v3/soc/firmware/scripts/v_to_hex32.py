#!/usr/bin/env python3
"""Convert Verilog hex output (objcopy -O verilog) to $readmemh format.

Produces a flat hex file with 32-bit words suitable for $readmemh.
Address segments (@ADDR) are handled by zero-filling gaps.

Usage: v_to_hex32.py <input.v> <output.hex32> [base_addr_hex]
"""
import sys

def convert(inpath, outpath, base_addr=0):
    # Read all bytes into a flat buffer
    buf = {}
    addr = 0

    with open(inpath) as f:
        for line in f:
            line = line.strip()
            if line.startswith('@'):
                addr = int(line[1:], 16)
            elif line:
                for b in line.split():
                    buf[addr] = int(b, 16)
                    addr += 1

    if not buf:
        print("Warning: empty input", file=sys.stderr)
        with open(outpath, 'w') as f:
            pass
        return

    min_addr = min(buf.keys())
    max_addr = max(buf.keys())

    # Align to word boundary
    min_word = (min_addr - base_addr) // 4
    max_word = (max_addr - base_addr) // 4

    with open(outpath, 'w') as f:
        for w in range(max_word + 1):
            byte_addr = base_addr + w * 4
            word = 0
            for b in range(4):
                word |= buf.get(byte_addr + b, 0) << (8 * b)
            f.write(f"{word:08X}\n")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.v> <output.hex32> [base_addr]")
        sys.exit(1)
    base = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0
    convert(sys.argv[1], sys.argv[2], base)
