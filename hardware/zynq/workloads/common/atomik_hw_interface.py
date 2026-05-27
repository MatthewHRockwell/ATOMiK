#!/usr/bin/env python3
"""ATOMiK hardware interface for Zynq NaxRiscv SoC.

CSR base: 0xf0020000 (from soc_nax64_atomik.py atomik_adapter region)
Access: /dev/mem via mmap

Register map (word-addressed, 4 bytes each):
  0x00: load_addr    W  address for LOAD (8-bit)
  0x04: load_data_lo W  initial state [31:0]
  0x08: load_data_hi W  initial state [63:32] — triggers LOAD
  0x0C: accum_lo     W  delta [31:0]
  0x10: accum_hi     W  delta [63:32] — triggers ACCUM
  0x14: swap_addr    W  address — triggers SWAP
  0x18: config       W  core enable (bit 0, default=1)
  0x1C: state_lo     R  current state [31:0]
  0x20: state_hi     R  current state [63:32]
  0x24: status       R  version[23:16], n_banks[15:8], acc_zero[0]

MMIO ordering: after CSR writes, issue fence iorw,iorw + dummy STATUS read
before reading STATE_LO/HI to flush pipeline.
"""
import mmap
import struct
import time
import ctypes

# ATOMiK adapter address in NaxRiscv address space
ATOMIK_BASE = 0xf0020000
ATOMIK_SIZE = 0x40  # 16 registers × 4 bytes

# Register offsets
REG_LOAD_ADDR   = 0x00
REG_LOAD_LO     = 0x04
REG_LOAD_HI     = 0x08   # triggers LOAD
REG_ACCUM_LO    = 0x0C
REG_ACCUM_HI    = 0x10   # triggers ACCUM
REG_SWAP_ADDR   = 0x14   # triggers SWAP
REG_CONFIG      = 0x18
REG_STATE_LO    = 0x1C
REG_STATE_HI    = 0x20
REG_STATUS      = 0x24


class ATOMiKHW:
    """Low-level ATOMiK hardware interface via /dev/mem."""

    def __init__(self, base=ATOMIK_BASE, dev="/dev/mem"):
        self._fd = open(dev, "r+b")
        page = ATOMIK_BASE & ~(mmap.PAGESIZE - 1)
        offset = ATOMIK_BASE - page
        self._mm = mmap.mmap(
            self._fd.fileno(), ATOMIK_SIZE + offset,
            mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE,
            offset=page
        )
        self._base_off = offset

    def close(self):
        self._mm.close()
        self._fd.close()

    def _wr32(self, off, val):
        self._mm[self._base_off + off : self._base_off + off + 4] = struct.pack("<I", val & 0xFFFFFFFF)

    def _rd32(self, off):
        return struct.unpack("<I", self._mm[self._base_off + off : self._base_off + off + 4])[0]

    def _fence(self):
        # Issue dummy STATUS read to flush Wishbone pipeline
        _ = self._rd32(REG_STATUS)

    def status(self):
        v = self._rd32(REG_STATUS)
        return {
            "version": (v >> 16) & 0xFF,
            "n_banks": (v >> 8) & 0xFF,
            "acc_zero": v & 1,
            "raw": v,
        }

    def load(self, addr, state64):
        """LOAD: write initial state to address."""
        self._wr32(REG_LOAD_ADDR, addr & 0xFF)
        self._wr32(REG_LOAD_LO,   state64 & 0xFFFFFFFF)
        self._wr32(REG_LOAD_HI,   (state64 >> 32) & 0xFFFFFFFF)  # triggers

    def accum(self, delta64):
        """ACCUM: XOR a delta into the current accumulator."""
        self._wr32(REG_ACCUM_LO,  delta64 & 0xFFFFFFFF)
        self._wr32(REG_ACCUM_HI,  (delta64 >> 32) & 0xFFFFFFFF)  # triggers

    def read(self):
        """READ: reconstruct current state (reference XOR accumulated delta)."""
        self._fence()
        lo = self._rd32(REG_STATE_LO)
        hi = self._rd32(REG_STATE_HI)
        return (hi << 32) | lo

    def swap(self, addr):
        """SWAP: commit clean epoch boundary at address."""
        self._wr32(REG_SWAP_ADDR, addr & 0xFF)
        self._fence()

    def smoke_test(self):
        """Quick 4-op correctness check. Returns (pass, details)."""
        st = self.status()
        if st["version"] < 1:
            return False, f"bad version {st['version']}"

        ref = 0xCAFEBABEDEADBEEF
        delta = 0x0F0F0F0F0F0F0F0F
        expected = ref ^ delta

        self.load(0, ref)
        self.accum(delta)
        got = self.read()
        if got != expected:
            return False, f"expected 0x{expected:016X} got 0x{got:016X}"

        # Self-inverse: applying delta again should restore ref
        self.accum(delta)
        got2 = self.read()
        if got2 != ref:
            return False, f"self-inverse failed: expected 0x{ref:016X} got 0x{got2:016X}"

        return True, f"OK version={st['version']} banks={st['n_banks']}"

    def __enter__(self): return self
    def __exit__(self, *a): self.close()
