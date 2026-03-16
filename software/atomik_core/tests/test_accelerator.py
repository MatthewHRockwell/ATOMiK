"""Tests for the C accelerator module."""

import struct
from atomik_core._accelerator import c_accum, c_read, c_fp_reduce, is_available


def test_is_available_returns_bool():
    """is_available() returns a boolean regardless of C library status."""
    result = is_available()
    assert isinstance(result, bool)


def test_c_accum():
    """c_accum computes XOR correctly."""
    mask = 0xFFFFFFFFFFFFFFFF
    assert c_accum(0, 0, 0xFF, mask) == 0xFF
    assert c_accum(0, 0xFF, 0xFF, mask) == 0  # self-inverse
    assert c_accum(0, 0xAAAA, 0x5555, mask) == 0xFFFF


def test_c_accum_masking():
    """c_accum respects the mask."""
    mask = 0xFF
    assert c_accum(0, 0xFF, 0x100, mask) == 0xFF  # 0x100 masked to 0


def test_c_read():
    """c_read computes reference XOR accumulator."""
    mask = 0xFFFFFFFFFFFFFFFF
    assert c_read(0xDEADBEEF, 0x000000FF, mask) == 0xDEADBE10
    assert c_read(0, 0, mask) == 0
    assert c_read(0xFFFF, 0xFFFF, mask) == 0  # self-inverse


def test_c_fp_reduce_empty():
    """c_fp_reduce on empty data returns 0."""
    assert c_fp_reduce(b"") == 0


def test_c_fp_reduce_8_bytes():
    """c_fp_reduce on exactly 8 bytes returns that value."""
    data = struct.pack("<Q", 0xDEADBEEFCAFEBABE)
    result = c_fp_reduce(data)
    assert result == 0xDEADBEEFCAFEBABE


def test_c_fp_reduce_16_bytes():
    """c_fp_reduce on 16 bytes XORs two 8-byte chunks."""
    a = 0x1111111111111111
    b = 0x2222222222222222
    data = struct.pack("<QQ", a, b)
    result = c_fp_reduce(data)
    assert result == a ^ b


def test_c_fp_reduce_unaligned():
    """c_fp_reduce handles data not aligned to 8 bytes."""
    data = b"Hello!!"  # 7 bytes
    result = c_fp_reduce(data)
    # Should not crash, should return some value
    assert isinstance(result, int)


def test_c_fp_reduce_large():
    """c_fp_reduce on large data produces consistent results."""
    data = bytes(range(256)) * 4  # 1024 bytes
    r1 = c_fp_reduce(data)
    r2 = c_fp_reduce(data)
    assert r1 == r2  # deterministic


def test_c_fp_reduce_matches_python():
    """C accelerator produces the same result as pure Python for 64-bit."""
    from atomik_core import Fingerprint

    data = b"The quick brown fox jumps over the lazy dog" * 10
    fp = Fingerprint(width=64)
    fp.load(data)

    # The reduce result should match c_fp_reduce
    # (fp.load sets the reference to the reduce result)
    c_result = c_fp_reduce(data)
    assert fp._ctx.reference == c_result
