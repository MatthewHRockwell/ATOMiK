#!/usr/bin/env python3
"""Walk and compare Zynq-7000 boot image structure (boot header, image header
table, partition header table) between our BOOT.bin and an ALINX reference.

Goal: find why BootROM rejects our standalone SD image (REBOOT_STATUS=0x0040200A)
when the on-card bytes verify byte-perfect. The defect is somewhere in the full
BOOT.bin construction, not just the FSBL header magic (which already matches).
"""
import struct, sys

def u32(b, off):
    return struct.unpack_from("<I", b, off)[0]

def load(path):
    with open(path, "rb") as f:
        return f.read()

def parse_boot_header(b):
    h = {}
    h["vector_table"]   = [u32(b, i*4) for i in range(8)]   # 0x00-0x1F
    h["width_detect"]   = u32(b, 0x20)   # expect 0xAA995566
    h["image_id"]       = u32(b, 0x24)   # expect 0x584C4E58 'XLNX'
    h["enc_status"]     = u32(b, 0x28)
    h["fsbl_userdef"]   = u32(b, 0x2C)
    h["source_offset"]  = u32(b, 0x30)
    h["fsbl_length"]    = u32(b, 0x34)   # length copied to OCM
    h["fsbl_load"]      = u32(b, 0x38)
    h["fsbl_exec"]      = u32(b, 0x3C)
    h["total_fsbl_len"] = u32(b, 0x40)
    h["qspi_cfg"]       = u32(b, 0x44)
    h["header_cksum"]   = u32(b, 0x48)
    # bootgen places IHT/PHT pointers at 0x98 / 0x9C (byte offsets)
    h["iht_offset"]     = u32(b, 0x98)
    h["pht_offset"]     = u32(b, 0x9C)
    return h

def calc_header_cksum(b):
    s = 0
    for off in range(0x20, 0x48, 4):
        s = (s + u32(b, off)) & 0xFFFFFFFF
    return (~s) & 0xFFFFFFFF

def parse_iht(b, off):
    # Image Header Table
    return {
        "version":        u32(b, off+0x00),
        "image_count":    u32(b, off+0x04),
        "pht_first_word": u32(b, off+0x08),   # word offset
        "ih_first_word":  u32(b, off+0x0C),   # word offset
        "ac_offset":      u32(b, off+0x10),
    }

def parse_partition_header(b, off):
    return {
        "enc_data_words":   u32(b, off+0x00),
        "unenc_data_words": u32(b, off+0x04),
        "total_words":      u32(b, off+0x08),
        "dst_load_addr":    u32(b, off+0x0C),
        "dst_exec_addr":    u32(b, off+0x10),
        "data_word_off":    u32(b, off+0x14),   # word offset into image
        "attributes":       u32(b, off+0x18),
        "section_count":    u32(b, off+0x1C),
        "cksum_word_off":   u32(b, off+0x20),
        "ih_word_off":      u32(b, off+0x24),
        "ac_word_off":      u32(b, off+0x28),
        "checksum":         u32(b, off+0x3C),
    }

def part_cksum(b, off):
    # PHT checksum = ~sum(word[0..14])
    s = 0
    for i in range(0, 0x3C, 4):
        s = (s + u32(b, off+i)) & 0xFFFFFFFF
    return (~s) & 0xFFFFFFFF

def decode_attr(a):
    # bootgen partition attribute bit layout
    owner   = (a >> 16) & 0x3
    rsa     = (a >> 15) & 0x1
    dest_cpu= (a >> 8) & 0xF
    enc     = (a >> 7) & 0x1
    dest_dev= (a >> 4) & 0x7   # 1=PS,2=PL,3=INT
    a32     = (a >> 1) & 0x1   # ARM exec state-ish
    dev_map = {0:"none",1:"PS",2:"PL",3:"INT"}
    cpu_map = {0:"none",1:"A9-0",2:"A9-1"}
    return f"dest_dev={dev_map.get(dest_dev,dest_dev)} dest_cpu={cpu_map.get(dest_cpu,dest_cpu)} enc={enc} rsa={rsa} owner={owner} a32={a32} raw=0x{a:08X}"

def walk(path):
    b = load(path)
    print(f"\n{'='*78}\nFILE: {path}\n  size = {len(b)} bytes (0x{len(b):X})\n{'='*78}")
    bh = parse_boot_header(b)
    print("-- BOOT HEADER --")
    print(f"  vector[0]      = 0x{bh['vector_table'][0]:08X}")
    print(f"  width_detect   = 0x{bh['width_detect']:08X}  ({'OK' if bh['width_detect']==0xAA995566 else 'BAD'})")
    print(f"  image_id       = 0x{bh['image_id']:08X}  ({'OK XLNX' if bh['image_id']==0x584C4E58 else 'BAD'})")
    print(f"  enc_status     = 0x{bh['enc_status']:08X}")
    print(f"  fsbl_userdef   = 0x{bh['fsbl_userdef']:08X}")
    print(f"  source_offset  = 0x{bh['source_offset']:08X}")
    print(f"  fsbl_length    = 0x{bh['fsbl_length']:08X}  ({bh['fsbl_length']} bytes to OCM)")
    print(f"  fsbl_load      = 0x{bh['fsbl_load']:08X}")
    print(f"  fsbl_exec      = 0x{bh['fsbl_exec']:08X}")
    print(f"  total_fsbl_len = 0x{bh['total_fsbl_len']:08X}")
    print(f"  qspi_cfg       = 0x{bh['qspi_cfg']:08X}")
    calc = calc_header_cksum(b)
    print(f"  header_cksum   = 0x{bh['header_cksum']:08X}  (calc 0x{calc:08X} {'OK' if calc==bh['header_cksum'] else 'MISMATCH'})")
    print(f"  iht_offset@98  = 0x{bh['iht_offset']:08X}")
    print(f"  pht_offset@9C  = 0x{bh['pht_offset']:08X}")

    iht_off = bh["iht_offset"]
    if iht_off == 0 or iht_off > len(b):
        print("  !! IHT offset implausible; trying common 0x8C0")
        iht_off = 0x8C0
    iht = parse_iht(b, iht_off)
    print(f"-- IMAGE HEADER TABLE @0x{iht_off:X} --")
    print(f"  version        = 0x{iht['version']:08X}")
    print(f"  image_count    = {iht['image_count']}")
    print(f"  pht_first_word = 0x{iht['pht_first_word']:08X}  -> byte 0x{iht['pht_first_word']*4:X}")
    print(f"  ih_first_word  = 0x{iht['ih_first_word']:08X}  -> byte 0x{iht['ih_first_word']*4:X}")
    print(f"  ac_offset      = 0x{iht['ac_offset']:08X}")

    # walk partition headers
    pht_off = iht["pht_first_word"]*4
    print(f"-- PARTITION HEADERS @0x{pht_off:X} --")
    idx = 0
    while pht_off+0x40 <= len(b) and idx < 8:
        ph = parse_partition_header(b, pht_off)
        if ph["total_words"]==0 and ph["unenc_data_words"]==0 and ph["dst_load_addr"]==0 and ph["data_word_off"]==0:
            break
        cc = part_cksum(b, pht_off)
        print(f"  [P{idx}] @0x{pht_off:X}")
        print(f"     enc_data_words   = 0x{ph['enc_data_words']:08X} ({ph['enc_data_words']*4} B)")
        print(f"     unenc_data_words = 0x{ph['unenc_data_words']:08X} ({ph['unenc_data_words']*4} B)")
        print(f"     total_words      = 0x{ph['total_words']:08X} ({ph['total_words']*4} B)")
        print(f"     dst_load_addr    = 0x{ph['dst_load_addr']:08X}")
        print(f"     dst_exec_addr    = 0x{ph['dst_exec_addr']:08X}")
        print(f"     data_word_off    = 0x{ph['data_word_off']:08X} -> byte 0x{ph['data_word_off']*4:X}")
        print(f"     attributes       = {decode_attr(ph['attributes'])}")
        print(f"     section_count    = {ph['section_count']}")
        print(f"     cksum_word_off   = 0x{ph['cksum_word_off']:08X} -> byte 0x{ph['cksum_word_off']*4:X}")
        print(f"     ih_word_off      = 0x{ph['ih_word_off']:08X} -> byte 0x{ph['ih_word_off']*4:X}")
        print(f"     ac_word_off      = 0x{ph['ac_word_off']:08X}")
        print(f"     checksum         = 0x{ph['checksum']:08X} (calc 0x{cc:08X} {'OK' if cc==ph['checksum'] else 'MISMATCH'})")
        pht_off += 0x40
        idx += 1
    print(f"  ({idx} partitions)")

if __name__ == "__main__":
    for p in sys.argv[1:]:
        walk(p)
