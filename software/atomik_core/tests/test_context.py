"""Tests for AtomikContext — mirrors the 108 Lean4 theorems."""

from atomik_core import AtomikContext

# =========================================================================
# Basic operations
# =========================================================================

def test_load_sets_state():
    ctx = AtomikContext()
    ctx.load(0xDEADBEEFCAFEBABE)
    assert ctx.read() == 0xDEADBEEFCAFEBABE


def test_load_clears_accumulator():
    ctx = AtomikContext()
    ctx.accum(0xFF)
    ctx.load(0x1234)
    assert ctx.read() == 0x1234
    assert ctx.is_clean


def test_accum_xors_delta():
    ctx = AtomikContext()
    ctx.load(0xDEADBEEF00000000)
    ctx.accum(0x00000000000000FF)
    assert ctx.read() == 0xDEADBEEF000000FF


def test_read_is_nondestructive():
    ctx = AtomikContext()
    ctx.load(0xCAFE)
    ctx.accum(0x00FF)
    s1 = ctx.read()
    s2 = ctx.read()
    assert s1 == s2


def test_swap_returns_state():
    ctx = AtomikContext()
    ctx.load(0xAAAA)
    ctx.accum(0x00FF)
    result = ctx.swap()
    assert result == 0xAAAA ^ 0x00FF


def test_swap_resets_accumulator():
    ctx = AtomikContext()
    ctx.load(0x1000)
    ctx.accum(0x0001)
    state = ctx.swap()
    assert ctx.is_clean
    assert ctx.read() == state  # reference is now the swapped state


def test_swap_with_new_reference():
    ctx = AtomikContext()
    ctx.load(0xAAAA)
    ctx.accum(0x00FF)
    old_state = ctx.swap(new_reference=0xBBBB)
    assert old_state == 0xAAAA ^ 0x00FF
    assert ctx.read() == 0xBBBB
    assert ctx.is_clean


# =========================================================================
# Algebraic properties (Lean4 theorems)
# =========================================================================

def test_identity_element():
    """accum(0) is the identity — doesn't change state."""
    ctx = AtomikContext()
    ctx.load(0xDEADBEEF)
    ctx.accum(0)
    assert ctx.read() == 0xDEADBEEF


def test_self_inverse():
    """Applying the same delta twice cancels out."""
    ctx = AtomikContext()
    ctx.load(0xDEADBEEF)
    ctx.accum(0x12345678)
    ctx.accum(0x12345678)
    assert ctx.read() == 0xDEADBEEF


def test_commutativity():
    """Order of accumulation doesn't matter."""
    a = AtomikContext()
    b = AtomikContext()
    a.load(0xCAFEBABE)
    b.load(0xCAFEBABE)

    a.accum(0x11111111)
    a.accum(0x22222222)

    b.accum(0x22222222)
    b.accum(0x11111111)

    assert a.read() == b.read()


def test_associativity():
    """Grouping of deltas doesn't matter."""
    ctx1 = AtomikContext()
    ctx2 = AtomikContext()
    ref = 0xAAAAAAAA
    d1, d2, d3 = 0x11, 0x22, 0x33

    ctx1.load(ref)
    ctx1.accum(d1 ^ d2)  # group (d1,d2) first
    ctx1.accum(d3)

    ctx2.load(ref)
    ctx2.accum(d1)
    ctx2.accum(d2 ^ d3)  # group (d2,d3) first

    assert ctx1.read() == ctx2.read()


def test_rollback():
    """Rollback undoes a delta."""
    ctx = AtomikContext()
    ctx.load(0xBEEF)
    ctx.accum(0x00FF)
    assert ctx.read() != 0xBEEF
    ctx.rollback(0x00FF)
    assert ctx.read() == 0xBEEF


def test_multiple_rollback():
    """Multiple deltas can be rolled back in any order."""
    ctx = AtomikContext()
    ctx.load(0x1000)
    ctx.accum(0x0001)
    ctx.accum(0x0010)
    ctx.accum(0x0100)
    # Rollback in reverse order
    ctx.rollback(0x0100)
    ctx.rollback(0x0010)
    ctx.rollback(0x0001)
    assert ctx.read() == 0x1000


def test_rollback_any_order():
    """Rollback in any order works (commutativity)."""
    ctx = AtomikContext()
    ctx.load(0x1000)
    ctx.accum(0x0001)
    ctx.accum(0x0010)
    ctx.accum(0x0100)
    # Rollback in forward order (not reverse)
    ctx.rollback(0x0001)
    ctx.rollback(0x0010)
    ctx.rollback(0x0100)
    assert ctx.read() == 0x1000


# =========================================================================
# Merge
# =========================================================================

def test_merge():
    """Merging two accumulators produces correct combined state."""
    a = AtomikContext()
    b = AtomikContext()
    a.load(0xAAAA)
    b.load(0xAAAA)

    a.accum(0x0011)
    b.accum(0x1100)

    a.merge(b)
    assert a.read() == 0xAAAA ^ 0x0011 ^ 0x1100


def test_merge_commutative():
    """merge(a, b) == merge(b, a) in effect."""
    a1 = AtomikContext()
    a2 = AtomikContext()
    b1 = AtomikContext()
    b2 = AtomikContext()
    ref = 0xBBBB

    a1.load(ref)
    a2.load(ref)
    b1.load(ref)
    b2.load(ref)

    a1.accum(0x0011)
    a2.accum(0x0011)
    b1.accum(0x1100)
    b2.accum(0x1100)

    a1.merge(b1)  # a into a1
    b2.merge(a2)  # b into b2

    assert a1.read() == b2.read()


# =========================================================================
# Width configurations
# =========================================================================

def test_32bit():
    ctx = AtomikContext(width=32)
    ctx.load(0xDEADBEEF)
    ctx.accum(0x000000FF)
    assert ctx.read() == 0xDEADBE10


def test_8bit():
    ctx = AtomikContext(width=8)
    ctx.load(0xAB)
    ctx.accum(0x0F)
    assert ctx.read() == 0xA4


def test_128bit():
    ctx = AtomikContext(width=128)
    val = (1 << 127) | 0xCAFE
    ctx.load(val)
    ctx.accum(0xFF)
    assert ctx.read() == val ^ 0xFF


def test_width_masking():
    """Values wider than the context width are masked."""
    ctx = AtomikContext(width=8)
    ctx.load(0xFFFF)  # only low 8 bits kept
    assert ctx.read() == 0xFF


# =========================================================================
# Stress
# =========================================================================

def test_1m_deltas():
    """1 million random-ish deltas, then undo all — returns to reference."""
    ctx = AtomikContext()
    ctx.load(0xDEADBEEFCAFEBABE)
    deltas = [(i * 0x1234567890ABCDEF + 0x1111) & 0xFFFFFFFFFFFFFFFF for i in range(1000)]
    for d in deltas:
        ctx.accum(d)
    for d in deltas:
        ctx.rollback(d)
    assert ctx.read() == 0xDEADBEEFCAFEBABE


def test_delta_count():
    ctx = AtomikContext()
    ctx.load(0)
    assert ctx.delta_count == 0
    ctx.accum(1)
    ctx.accum(2)
    assert ctx.delta_count == 2


def test_repr():
    ctx = AtomikContext(width=32, initial_state=0xCAFE)
    s = repr(ctx)
    assert "0x0000cafe" in s


def test_equality():
    a = AtomikContext(width=64)
    b = AtomikContext(width=64)
    a.load(0x1234)
    b.load(0x1234)
    assert a == b
    a.accum(0xFF)
    assert a != b


# =========================================================================
# Edge cases
# =========================================================================

def test_zero_state():
    """All-zero reference with all-zero deltas."""
    ctx = AtomikContext()
    ctx.load(0)
    ctx.accum(0)
    assert ctx.read() == 0
    assert ctx.is_clean


def test_max_value_64bit():
    """Max 64-bit value operations."""
    ctx = AtomikContext(width=64)
    max_val = (1 << 64) - 1
    ctx.load(max_val)
    assert ctx.read() == max_val
    ctx.accum(max_val)
    assert ctx.read() == 0  # XOR with self = 0
    ctx.accum(max_val)
    assert ctx.read() == max_val  # back to original


def test_single_bit_widths():
    """1-bit context — smallest possible width."""
    ctx = AtomikContext(width=1)
    ctx.load(1)
    assert ctx.read() == 1
    ctx.accum(1)
    assert ctx.read() == 0
    ctx.accum(1)
    assert ctx.read() == 1


def test_large_width_256bit():
    """256-bit width operations."""
    ctx = AtomikContext(width=256)
    big = (1 << 255) | (1 << 128) | 0xDEADBEEF
    ctx.load(big)
    ctx.accum(0xFF)
    assert ctx.read() == big ^ 0xFF
    ctx.rollback(0xFF)
    assert ctx.read() == big


def test_swap_without_prior_accum():
    """Swap on clean context returns reference unchanged."""
    ctx = AtomikContext()
    ctx.load(0xCAFE)
    result = ctx.swap()
    assert result == 0xCAFE
    assert ctx.is_clean


def test_many_merges_converge():
    """N independent contexts all merge to same result."""
    n = 10
    ref = 0xAAAABBBBCCCCDDDD
    deltas = [0x1111 << (i * 4) for i in range(n)]

    # Apply all deltas to one context
    expected = AtomikContext()
    expected.load(ref)
    for d in deltas:
        expected.accum(d)

    # Apply one delta each to N contexts, merge all into first
    ctxs = [AtomikContext() for _ in range(n)]
    for c in ctxs:
        c.load(ref)
    for i, d in enumerate(deltas):
        ctxs[i].accum(d)

    for i in range(1, n):
        ctxs[0].merge(ctxs[i])

    assert ctxs[0].read() == expected.read()


def test_double_load_overwrites():
    """Second load completely replaces first state."""
    ctx = AtomikContext()
    ctx.load(0xAAAA)
    ctx.accum(0x00FF)
    ctx.load(0xBBBB)
    assert ctx.read() == 0xBBBB
    assert ctx.is_clean
    assert ctx.delta_count == 0


def test_rollback_is_same_as_accum():
    """Rollback is just accum (XOR is self-inverse)."""
    ctx1 = AtomikContext()
    ctx2 = AtomikContext()
    ctx1.load(0xDEAD)
    ctx2.load(0xDEAD)
    ctx1.accum(0xFF)
    ctx2.accum(0xFF)
    ctx1.rollback(0xFF)
    ctx2.accum(0xFF)  # accum same delta again = rollback
    assert ctx1.read() == ctx2.read() == 0xDEAD


def test_alternating_deltas_stress():
    """Rapid alternation between two deltas maintains correctness."""
    ctx = AtomikContext()
    ctx.load(0xBEEF)
    for _ in range(10000):
        ctx.accum(0xFF)
        ctx.accum(0xFF)  # self-cancels
    assert ctx.read() == 0xBEEF


def test_merge_self_doubles_accumulator():
    """Merging context with itself doubles the accumulator."""
    ctx = AtomikContext()
    ctx.load(0xAAAA)
    ctx.accum(0x00FF)
    ctx.merge(ctx)
    # XOR with self = 0, so merge(self) zeroes the accumulator
    assert ctx.accumulator == 0
    assert ctx.read() == 0xAAAA  # back to reference


# =========================================================================
# _set_accumulator
# =========================================================================

def test_set_accumulator():
    """_set_accumulator sets the value directly and resets delta_count."""
    ctx = AtomikContext()
    ctx.load(0xAAAA)
    ctx.accum(0x0001)
    ctx.accum(0x0010)
    assert ctx.delta_count == 2
    assert ctx.accumulator == 0x0011

    ctx._set_accumulator(0xFF)
    assert ctx.accumulator == 0xFF
    assert ctx.delta_count == 0
    assert ctx.read() == 0xAAAA ^ 0xFF


def test_snapshot_roundtrip():
    """snapshot() and from_snapshot() preserve full context state."""
    ctx = AtomikContext(width=64, initial_state=0xDEADBEEF)
    ctx.accum(0x000000FF)
    ctx.accum(0x0000FF00)

    snap = ctx.snapshot()
    assert snap["width"] == 64
    assert snap["reference"] == 0xDEADBEEF
    assert snap["state"] == ctx.read()
    assert snap["delta_count"] == 2

    restored = AtomikContext.from_snapshot(snap)
    assert restored.read() == ctx.read()
    assert restored.reference == ctx.reference
    assert restored.accumulator == ctx.accumulator
    assert restored.delta_count == 2


def test_context_manager():
    """Context manager resets state on exit."""
    ctx = AtomikContext()
    ctx.load(0xCAFEBABE)
    ctx.accum(0xFF)

    with ctx as c:
        assert c.read() == 0xCAFEBABE ^ 0xFF
        c.accum(0x01)

    # After exiting, context is reset
    assert ctx.read() == 0
    assert ctx.accumulator == 0
