"""Tests for AtomikTable — multi-context state management."""

import pytest
from atomik_core import AtomikTable


def test_basic_load_read():
    t = AtomikTable(num_contexts=4)
    t.load(0, 0xAAAA)
    assert t.read(0) == 0xAAAA


def test_address_isolation():
    t = AtomikTable(num_contexts=4)
    t.load(0, 0x1111)
    t.load(1, 0x2222)
    t.accum(0, 0x00FF)
    assert t.read(0) == 0x1111 ^ 0x00FF
    assert t.read(1) == 0x2222  # untouched


def test_all_addresses():
    t = AtomikTable(num_contexts=256)
    for i in range(256):
        t.load(i, i * 0x0101010101010101)
    for i in range(256):
        assert t.read(i) == i * 0x0101010101010101


def test_swap():
    t = AtomikTable(num_contexts=4)
    t.load(0, 0xBEEF)
    t.accum(0, 0x00FF)
    old = t.swap(0)
    assert old == 0xBEEF ^ 0x00FF
    assert t.read(0) == old  # reference updated


def test_batch_accum():
    t = AtomikTable(num_contexts=4)
    t.load(0, 0x1000)
    t.load(1, 0x2000)
    t.load(2, 0x3000)
    t.batch_accum({0: 0x0001, 1: 0x0002, 2: 0x0003})
    assert t.read(0) == 0x1001
    assert t.read(1) == 0x2002
    assert t.read(2) == 0x3003


def test_snapshot():
    t = AtomikTable(num_contexts=4)
    t.load(1, 0xCAFE)
    t.load(3, 0xBEEF)
    snap = t.snapshot()
    assert 1 in snap
    assert 3 in snap
    assert 0 not in snap  # zero state


def test_out_of_range():
    t = AtomikTable(num_contexts=4)
    with pytest.raises(IndexError):
        t.read(4)
    with pytest.raises(IndexError):
        t.load(256, 0)


def test_context_access():
    t = AtomikTable(num_contexts=4)
    t.load(2, 0xDEAD)
    ctx = t.context(2)
    assert ctx.read() == 0xDEAD
    ctx.accum(0x00FF)
    assert t.read(2) == 0xDEAD ^ 0x00FF


def test_repr():
    t = AtomikTable(num_contexts=8)
    t.load(0, 0x1234)
    s = repr(t)
    assert "contexts=8" in s
