"""Tests for Fingerprint — change detection."""

from atomik_core import Fingerprint


def test_no_change():
    data = b"Hello, ATOMiK!"
    fp = Fingerprint()
    fp.load(data)
    fp.update(data)
    assert not fp.changed


def test_single_byte_change():
    original = b"Hello, ATOMiK!"
    modified = b"Hello, ATOMIK!"  # lowercase k → uppercase K
    fp = Fingerprint()
    fp.load(original)
    fp.update(modified)
    assert fp.changed


def test_large_buffer():
    data = bytes(range(256)) * 100  # 25.6 KB
    fp = Fingerprint()
    fp.load(data)
    fp.update(data)
    assert not fp.changed


def test_large_buffer_change():
    data = bytearray(range(256)) * 100
    fp = Fingerprint()
    fp.load(bytes(data))
    data[12345] ^= 0x01  # flip one bit
    fp.update(bytes(data))
    assert fp.changed


def test_incremental_update():
    fp = Fingerprint()
    fp.load(b"\x00" * 64)
    # Change chunk at offset 0 from 0x0000000000000000 to 0x00000000000000FF
    fp.accumulate_delta(0x0000000000000000, 0x00000000000000FF)
    assert fp.changed
    # Undo the change
    fp.accumulate_delta(0x00000000000000FF, 0x0000000000000000)
    assert not fp.changed


def test_reset():
    fp = Fingerprint()
    fp.load(b"aaa")
    fp.update(b"bbb")
    assert fp.changed
    fp.reset()
    assert not fp.changed


def test_32bit_fingerprint():
    fp = Fingerprint(width=32)
    data = b"test data here!!"
    fp.load(data)
    fp.update(data)
    assert not fp.changed


def test_empty_data():
    fp = Fingerprint()
    fp.load(b"")
    fp.update(b"")
    assert not fp.changed


def test_repr():
    fp = Fingerprint()
    fp.load(b"test")
    s = repr(fp)
    assert "changed=" in s
