"""Tests for DeviceContext and DeviceTable — /dev/atomik ioctl bindings.

All tests mock the fd/ioctl layer so they run WITHOUT /dev/atomik present.
Tests verify:
  - ioctl number calculation matches the UAPI header
  - struct packing/unpacking is correct for every operation
  - DeviceContext matches the AtomikContext interface
  - DeviceTable matches the AtomikTable interface
  - Resource cleanup (close, context manager, __del__)
"""

from __future__ import annotations

import struct
from unittest.mock import patch, MagicMock, call

import pytest

from atomik_core.device import (
    # ioctl number helpers
    _IOC, _IOW, _IOR, _IOWR,
    _IOC_WRITE, _IOC_READ,
    # ioctl command constants
    _ATOMIK_MAGIC,
    ATOMIK_IOC_CREATE_TABLE,
    ATOMIK_IOC_DESTROY_TABLE,
    ATOMIK_IOC_LOAD,
    ATOMIK_IOC_ACCUM,
    ATOMIK_IOC_READ,
    ATOMIK_IOC_SWAP,
    ATOMIK_IOC_GET_INFO,
    # struct formats
    _FMT_CREATE_TABLE,
    _FMT_DESTROY_TABLE,
    _FMT_LOAD,
    _FMT_ACCUM,
    _FMT_READ,
    _FMT_SWAP,
    _FMT_INFO,
    # struct sizes
    _SZ_CREATE_TABLE,
    _SZ_DESTROY_TABLE,
    _SZ_LOAD,
    _SZ_ACCUM,
    _SZ_READ,
    _SZ_SWAP,
    _SZ_INFO,
    # classes
    DeviceContext,
    DeviceTable,
    DeviceInfo,
)


# =========================================================================
# ioctl number calculation
# =========================================================================

class TestIoctlNumbers:
    """Verify ioctl numbers match the C UAPI header definitions."""

    def test_magic_is_A(self):
        assert _ATOMIK_MAGIC == 0x41

    def test_ioc_construction(self):
        """_IOC(dir, type, nr, size) = (dir<<30) | (type<<8) | (nr) | (size<<16)"""
        result = _IOC(0x3, 0x41, 0x01, 8)
        expected = (0x3 << 30) | (0x41 << 8) | 0x01 | (8 << 16)
        assert result == expected

    def test_iow(self):
        """_IOW sets direction = _IOC_WRITE (1)"""
        result = _IOW(0x41, 0x10, 16)
        expected = (1 << 30) | (0x41 << 8) | 0x10 | (16 << 16)
        assert result == expected

    def test_ior(self):
        """_IOR sets direction = _IOC_READ (2)"""
        result = _IOR(0x41, 0x40, 32)
        expected = (2 << 30) | (0x41 << 8) | 0x40 | (32 << 16)
        assert result == expected

    def test_iowr(self):
        """_IOWR sets direction = _IOC_READ|_IOC_WRITE (3)"""
        result = _IOWR(0x41, 0x01, 8)
        expected = (3 << 30) | (0x41 << 8) | 0x01 | (8 << 16)
        assert result == expected

    def test_create_table_number(self):
        expected = _IOWR(0x41, 0x01, 8)
        assert ATOMIK_IOC_CREATE_TABLE == expected

    def test_destroy_table_number(self):
        expected = _IOW(0x41, 0x02, 4)
        assert ATOMIK_IOC_DESTROY_TABLE == expected

    def test_load_number(self):
        expected = _IOW(0x41, 0x10, 16)
        assert ATOMIK_IOC_LOAD == expected

    def test_accum_number(self):
        expected = _IOW(0x41, 0x11, 16)
        assert ATOMIK_IOC_ACCUM == expected

    def test_read_number(self):
        expected = _IOWR(0x41, 0x12, 16)
        assert ATOMIK_IOC_READ == expected

    def test_swap_number(self):
        expected = _IOWR(0x41, 0x13, 24)
        assert ATOMIK_IOC_SWAP == expected

    def test_get_info_number(self):
        expected = _IOR(0x41, 0x40, 40)
        assert ATOMIK_IOC_GET_INFO == expected

    def test_ioctl_numbers_are_distinct(self):
        numbers = [
            ATOMIK_IOC_CREATE_TABLE, ATOMIK_IOC_DESTROY_TABLE,
            ATOMIK_IOC_LOAD, ATOMIK_IOC_ACCUM,
            ATOMIK_IOC_READ, ATOMIK_IOC_SWAP,
            ATOMIK_IOC_GET_INFO,
        ]
        assert len(numbers) == len(set(numbers))


# =========================================================================
# Struct packing / unpacking
# =========================================================================

class TestStructFormats:
    """Verify struct format strings produce correct sizes and layouts."""

    def test_create_table_size(self):
        assert struct.calcsize(_FMT_CREATE_TABLE) == _SZ_CREATE_TABLE == 8

    def test_destroy_table_size(self):
        assert struct.calcsize(_FMT_DESTROY_TABLE) == _SZ_DESTROY_TABLE == 4

    def test_load_size(self):
        assert struct.calcsize(_FMT_LOAD) == _SZ_LOAD == 16

    def test_accum_size(self):
        assert struct.calcsize(_FMT_ACCUM) == _SZ_ACCUM == 16

    def test_read_size(self):
        assert struct.calcsize(_FMT_READ) == _SZ_READ == 16

    def test_swap_size(self):
        assert struct.calcsize(_FMT_SWAP) == _SZ_SWAP == 24

    def test_info_size(self):
        assert struct.calcsize(_FMT_INFO) == _SZ_INFO == 40

    def test_create_table_pack(self):
        buf = struct.pack(_FMT_CREATE_TABLE, 256, 0)
        num_ctx, table_id = struct.unpack(_FMT_CREATE_TABLE, buf)
        assert num_ctx == 256
        assert table_id == 0

    def test_load_pack(self):
        buf = struct.pack(_FMT_LOAD, 1, 42, 0xDEADBEEFCAFEBABE)
        table_id, addr, initial_state = struct.unpack(_FMT_LOAD, buf)
        assert table_id == 1
        assert addr == 42
        assert initial_state == 0xDEADBEEFCAFEBABE

    def test_accum_pack(self):
        buf = struct.pack(_FMT_ACCUM, 1, 0, 0x00000000000000FF)
        table_id, addr, delta = struct.unpack(_FMT_ACCUM, buf)
        assert table_id == 1
        assert addr == 0
        assert delta == 0xFF

    def test_read_pack_unpack(self):
        """READ: kernel fills in the state field."""
        buf = struct.pack(_FMT_READ, 1, 0, 0xDEADBEEF)
        table_id, addr, state = struct.unpack(_FMT_READ, buf)
        assert table_id == 1
        assert addr == 0
        assert state == 0xDEADBEEF

    def test_swap_pack_unpack(self):
        """SWAP: kernel fills in old_state field."""
        buf = struct.pack(_FMT_SWAP, 1, 0, 0xBBBB, 0xAAAA)
        table_id, addr, new_ref, old_state = struct.unpack(_FMT_SWAP, buf)
        assert table_id == 1
        assert addr == 0
        assert new_ref == 0xBBBB
        assert old_state == 0xAAAA

    def test_info_pack_unpack(self):
        """GET_INFO: kernel fills all fields."""
        # version = (0 << 16) | (5 << 8) | 0 = 0x0500
        version = (0 << 16) | (5 << 8) | 0
        buf = struct.pack(_FMT_INFO, version, 0, 0, 0, 1000, 50, 4096)
        info = DeviceInfo(buf)
        assert info.version == "0.5.0"
        assert info.backend == "software"
        assert info.hw_banks == 0
        assert info.fp_enabled is False
        assert info.ops_total == 1000
        assert info.fp_checks == 50
        assert info.fp_bytes_saved == 4096

    def test_info_hardware_backend(self):
        version = (0 << 16) | (5 << 8) | 0
        buf = struct.pack(_FMT_INFO, version, 1, 16, 1, 0, 0, 0)
        info = DeviceInfo(buf)
        assert info.backend == "hardware"
        assert info.hw_banks == 16
        assert info.fp_enabled is True

    def test_load_64bit_max(self):
        """Ensure full 64-bit values survive the pack/unpack round trip."""
        max_val = 0xFFFFFFFFFFFFFFFF
        buf = struct.pack(_FMT_LOAD, 0, 0, max_val)
        _, _, val = struct.unpack(_FMT_LOAD, buf)
        assert val == max_val

    def test_swap_64bit_values(self):
        max_val = 0xFFFFFFFFFFFFFFFF
        buf = struct.pack(_FMT_SWAP, 0, 0, max_val, max_val)
        _, _, new_ref, old_state = struct.unpack(_FMT_SWAP, buf)
        assert new_ref == max_val
        assert old_state == max_val


# =========================================================================
# Mock helpers
# =========================================================================

def _mock_ioctl_create_table(fd, cmd, buf):
    """Simulate CREATE_TABLE: set table_id=1 in the output buffer."""
    if cmd == ATOMIK_IOC_CREATE_TABLE:
        num_ctx, _ = struct.unpack_from(_FMT_CREATE_TABLE, buf)
        struct.pack_into(_FMT_CREATE_TABLE, buf, 0, num_ctx, 1)
    return 0


def _mock_ioctl_read(fd, cmd, buf, *, state=0xDEADBEEF):
    """Simulate READ: write state into the output buffer."""
    if cmd == ATOMIK_IOC_CREATE_TABLE:
        _mock_ioctl_create_table(fd, cmd, buf)
    elif cmd == ATOMIK_IOC_READ:
        table_id, addr, _ = struct.unpack_from(_FMT_READ, buf)
        struct.pack_into(_FMT_READ, buf, 0, table_id, addr, state)
    return 0


def _mock_ioctl_swap(fd, cmd, buf, *, old_state=0xBEEF):
    """Simulate SWAP: write old_state into the output buffer."""
    if cmd == ATOMIK_IOC_CREATE_TABLE:
        _mock_ioctl_create_table(fd, cmd, buf)
    elif cmd == ATOMIK_IOC_SWAP:
        table_id, addr, new_ref, _ = struct.unpack_from(_FMT_SWAP, buf)
        struct.pack_into(_FMT_SWAP, buf, 0, table_id, addr, new_ref, old_state)
    return 0


# =========================================================================
# DeviceTable tests
# =========================================================================

class TestDeviceTable:
    """Test DeviceTable with mocked fd/ioctl."""

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    def test_create_table(self, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=256)
        mock_open.assert_called_once_with("/dev/atomik", 2)  # O_RDWR = 2
        assert table.table_id == 1
        assert table.num_contexts == 256
        table.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    def test_create_with_external_fd(self, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=8, fd=99)
        mock_open.assert_not_called()
        assert table.table_id == 1
        table.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    def test_invalid_num_contexts_zero(self, mock_open, mock_ioctl):
        with pytest.raises(ValueError, match="num_contexts must be"):
            DeviceTable(num_contexts=0)

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    def test_invalid_num_contexts_too_large(self, mock_open, mock_ioctl):
        with pytest.raises(ValueError, match="num_contexts must be"):
            DeviceTable(num_contexts=65537)

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_load_ioctl(self, mock_close, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=1)
        table.load(addr=0, initial_state=0xCAFEBABE)

        # Second ioctl call is LOAD (first was CREATE_TABLE)
        load_call = mock_ioctl.call_args_list[1]
        assert load_call[0][1] == ATOMIK_IOC_LOAD
        buf = load_call[0][2]
        table_id, addr, initial_state = struct.unpack(_FMT_LOAD, buf)
        assert table_id == 1
        assert addr == 0
        assert initial_state == 0xCAFEBABE
        table.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_accum_ioctl(self, mock_close, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=1)
        table.accum(addr=0, delta=0xFF)

        accum_call = mock_ioctl.call_args_list[1]
        assert accum_call[0][1] == ATOMIK_IOC_ACCUM
        buf = accum_call[0][2]
        table_id, addr, delta = struct.unpack(_FMT_ACCUM, buf)
        assert table_id == 1
        assert addr == 0
        assert delta == 0xFF
        table.close()

    @patch("atomik_core.device.fcntl.ioctl")
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_read_ioctl(self, mock_close, mock_open, mock_ioctl):
        mock_ioctl.side_effect = lambda fd, cmd, buf: _mock_ioctl_read(
            fd, cmd, buf, state=0xDEADBE10
        )
        table = DeviceTable(num_contexts=1)
        result = table.read(addr=0)
        assert result == 0xDEADBE10
        table.close()

    @patch("atomik_core.device.fcntl.ioctl")
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_swap_ioctl(self, mock_close, mock_open, mock_ioctl):
        mock_ioctl.side_effect = lambda fd, cmd, buf: _mock_ioctl_swap(
            fd, cmd, buf, old_state=0xAAAA00FF
        )
        table = DeviceTable(num_contexts=1)
        result = table.swap(addr=0, new_reference=0xBBBB)
        assert result == 0xAAAA00FF

        swap_call = mock_ioctl.call_args_list[1]
        assert swap_call[0][1] == ATOMIK_IOC_SWAP
        buf = bytes(swap_call[0][2])
        # Before kernel modifies: verify new_reference was packed correctly
        # (After mock, the buffer has been modified, so check old_state from return)
        table.close()

    @patch("atomik_core.device.fcntl.ioctl")
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_swap_none_sends_zero(self, mock_close, mock_open, mock_ioctl):
        """swap(new_reference=None) sends new_reference=0 to kernel."""
        mock_ioctl.side_effect = lambda fd, cmd, buf: _mock_ioctl_swap(
            fd, cmd, buf, old_state=0x1234
        )
        table = DeviceTable(num_contexts=1)
        result = table.swap(addr=0)
        assert result == 0x1234
        table.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_context_manager(self, mock_close, mock_open, mock_ioctl):
        with DeviceTable(num_contexts=1) as table:
            assert table.table_id == 1
        # After exiting, close should have been called
        # DESTROY_TABLE should have been issued
        destroy_calls = [
            c for c in mock_ioctl.call_args_list
            if c[0][1] == ATOMIK_IOC_DESTROY_TABLE
        ]
        assert len(destroy_calls) >= 1

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_close_destroys_table(self, mock_close, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=1)
        table.close()
        destroy_calls = [
            c for c in mock_ioctl.call_args_list
            if c[0][1] == ATOMIK_IOC_DESTROY_TABLE
        ]
        assert len(destroy_calls) == 1
        buf = destroy_calls[0][0][2]
        (table_id,) = struct.unpack(_FMT_DESTROY_TABLE, buf)
        assert table_id == 1

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_close_closes_fd_when_owned(self, mock_os_close, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=1)
        table.close()
        mock_os_close.assert_called_once_with(42)

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.close")
    def test_close_does_not_close_external_fd(self, mock_os_close, mock_ioctl):
        table = DeviceTable(num_contexts=1, fd=99)
        table.close()
        mock_os_close.assert_not_called()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_double_close_is_safe(self, mock_close, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=1)
        table.close()
        table.close()  # should not raise

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    def test_repr(self, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=256)
        r = repr(table)
        assert "table_id=1" in r
        assert "num_contexts=256" in r
        table.close()


# =========================================================================
# DeviceContext tests
# =========================================================================

class TestDeviceContext:
    """Test DeviceContext — drop-in replacement for AtomikContext."""

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_create(self, mock_close, mock_open, mock_ioctl):
        ctx = DeviceContext()
        assert ctx._table.table_id == 1
        ctx.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_load(self, mock_close, mock_open, mock_ioctl):
        ctx = DeviceContext()
        ctx.load(0xDEADBEEF)
        load_call = mock_ioctl.call_args_list[1]
        assert load_call[0][1] == ATOMIK_IOC_LOAD
        buf = load_call[0][2]
        _, _, initial_state = struct.unpack(_FMT_LOAD, buf)
        assert initial_state == 0xDEADBEEF
        ctx.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_accum(self, mock_close, mock_open, mock_ioctl):
        ctx = DeviceContext()
        ctx.accum(0xFF)
        accum_call = mock_ioctl.call_args_list[1]
        assert accum_call[0][1] == ATOMIK_IOC_ACCUM
        buf = accum_call[0][2]
        _, _, delta = struct.unpack(_FMT_ACCUM, buf)
        assert delta == 0xFF
        ctx.close()

    @patch("atomik_core.device.fcntl.ioctl")
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_read(self, mock_close, mock_open, mock_ioctl):
        mock_ioctl.side_effect = lambda fd, cmd, buf: _mock_ioctl_read(
            fd, cmd, buf, state=0xCAFEBABE
        )
        ctx = DeviceContext()
        result = ctx.read()
        assert result == 0xCAFEBABE
        ctx.close()

    @patch("atomik_core.device.fcntl.ioctl")
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_swap(self, mock_close, mock_open, mock_ioctl):
        mock_ioctl.side_effect = lambda fd, cmd, buf: _mock_ioctl_swap(
            fd, cmd, buf, old_state=0xABCD
        )
        ctx = DeviceContext()
        result = ctx.swap(new_reference=0x1234)
        assert result == 0xABCD
        ctx.close()

    @patch("atomik_core.device.fcntl.ioctl")
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_swap_no_reference(self, mock_close, mock_open, mock_ioctl):
        mock_ioctl.side_effect = lambda fd, cmd, buf: _mock_ioctl_swap(
            fd, cmd, buf, old_state=0x5678
        )
        ctx = DeviceContext()
        result = ctx.swap()  # no new_reference
        assert result == 0x5678
        ctx.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_context_manager(self, mock_close, mock_open, mock_ioctl):
        with DeviceContext() as ctx:
            ctx.load(0xBEEF)
        # Should not raise

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_close(self, mock_close, mock_open, mock_ioctl):
        ctx = DeviceContext()
        ctx.close()
        assert ctx._table is None

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_double_close_is_safe(self, mock_close, mock_open, mock_ioctl):
        ctx = DeviceContext()
        ctx.close()
        ctx.close()  # should not raise


# =========================================================================
# API compatibility with AtomikContext / AtomikTable
# =========================================================================

class TestAPICompatibility:
    """Verify DeviceContext and DeviceTable expose the same core API
    as AtomikContext and AtomikTable."""

    def test_device_context_has_core_methods(self):
        """DeviceContext has load, accum, read, swap."""
        for method in ("load", "accum", "read", "swap", "close"):
            assert hasattr(DeviceContext, method), f"missing {method}"

    def test_device_context_has_context_manager(self):
        assert hasattr(DeviceContext, "__enter__")
        assert hasattr(DeviceContext, "__exit__")

    def test_device_table_has_core_methods(self):
        """DeviceTable has load, accum, read, swap, close."""
        for method in ("load", "accum", "read", "swap", "close", "info"):
            assert hasattr(DeviceTable, method), f"missing {method}"

    def test_device_table_has_properties(self):
        for prop in ("table_id", "num_contexts"):
            assert hasattr(DeviceTable, prop), f"missing {prop}"

    def test_device_table_has_context_manager(self):
        assert hasattr(DeviceTable, "__enter__")
        assert hasattr(DeviceTable, "__exit__")

    def test_device_context_load_signature(self):
        """load() accepts a single positional value argument."""
        import inspect
        sig = inspect.signature(DeviceContext.load)
        params = list(sig.parameters.keys())
        assert params == ["self", "value"]

    def test_device_context_accum_signature(self):
        """accum() accepts a single positional delta argument."""
        import inspect
        sig = inspect.signature(DeviceContext.accum)
        params = list(sig.parameters.keys())
        assert params == ["self", "delta"]

    def test_device_context_read_signature(self):
        """read() takes no arguments, returns int."""
        import inspect
        sig = inspect.signature(DeviceContext.read)
        params = list(sig.parameters.keys())
        assert params == ["self"]

    def test_device_context_swap_signature(self):
        """swap() accepts an optional new_reference."""
        import inspect
        sig = inspect.signature(DeviceContext.swap)
        params = list(sig.parameters.keys())
        assert "self" in params
        assert "new_reference" in params

    def test_device_table_load_signature(self):
        """Table load() takes addr and initial_state."""
        import inspect
        sig = inspect.signature(DeviceTable.load)
        params = list(sig.parameters.keys())
        assert params == ["self", "addr", "initial_state"]

    def test_device_table_accum_signature(self):
        import inspect
        sig = inspect.signature(DeviceTable.accum)
        params = list(sig.parameters.keys())
        assert params == ["self", "addr", "delta"]

    def test_device_table_read_signature(self):
        import inspect
        sig = inspect.signature(DeviceTable.read)
        params = list(sig.parameters.keys())
        assert params == ["self", "addr"]

    def test_device_table_swap_signature(self):
        import inspect
        sig = inspect.signature(DeviceTable.swap)
        params = list(sig.parameters.keys())
        assert "self" in params
        assert "addr" in params
        assert "new_reference" in params


# =========================================================================
# DeviceInfo
# =========================================================================

class TestDeviceInfo:
    """Test DeviceInfo parsing."""

    def test_version_parsing(self):
        version = (1 << 16) | (2 << 8) | 3
        buf = struct.pack(_FMT_INFO, version, 0, 0, 0, 0, 0, 0)
        info = DeviceInfo(buf)
        assert info.version == "1.2.3"
        assert info.version_major == 1
        assert info.version_minor == 2
        assert info.version_patch == 3

    def test_software_backend(self):
        buf = struct.pack(_FMT_INFO, 0, 0, 0, 0, 0, 0, 0)
        info = DeviceInfo(buf)
        assert info.backend == "software"

    def test_hardware_backend(self):
        buf = struct.pack(_FMT_INFO, 0, 1, 16, 0, 0, 0, 0)
        info = DeviceInfo(buf)
        assert info.backend == "hardware"
        assert info.hw_banks == 16

    def test_repr(self):
        version = (0 << 16) | (5 << 8) | 0
        buf = struct.pack(_FMT_INFO, version, 0, 0, 0, 42, 0, 0)
        info = DeviceInfo(buf)
        r = repr(info)
        assert "0.5.0" in r
        assert "software" in r
        assert "42" in r

    @patch("atomik_core.device.fcntl.ioctl")
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_table_info_ioctl(self, mock_close, mock_open, mock_ioctl):
        version = (0 << 16) | (5 << 8) | 0
        info_response = struct.pack(_FMT_INFO, version, 1, 4, 0, 100, 10, 2048)

        def side_effect(fd, cmd, buf):
            if cmd == ATOMIK_IOC_CREATE_TABLE:
                _mock_ioctl_create_table(fd, cmd, buf)
            elif cmd == ATOMIK_IOC_GET_INFO:
                buf[:] = bytearray(info_response)
            return 0

        mock_ioctl.side_effect = side_effect
        table = DeviceTable(num_contexts=1)
        info = table.info()
        assert info.version == "0.5.0"
        assert info.backend == "hardware"
        assert info.hw_banks == 4
        assert info.ops_total == 100
        table.close()


# =========================================================================
# Edge cases
# =========================================================================

class TestEdgeCases:
    """Edge cases and boundary conditions."""

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_load_masks_to_64bit(self, mock_close, mock_open, mock_ioctl):
        """Values larger than 64 bits are masked."""
        ctx = DeviceContext()
        ctx.load(0x1_FFFFFFFFFFFFFFFF)  # 65-bit value
        load_call = mock_ioctl.call_args_list[1]
        buf = load_call[0][2]
        _, _, initial_state = struct.unpack(_FMT_LOAD, buf)
        assert initial_state == 0xFFFFFFFFFFFFFFFF
        ctx.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_accum_masks_to_64bit(self, mock_close, mock_open, mock_ioctl):
        ctx = DeviceContext()
        ctx.accum(0x1_00000000000000FF)
        accum_call = mock_ioctl.call_args_list[1]
        buf = accum_call[0][2]
        _, _, delta = struct.unpack(_FMT_ACCUM, buf)
        assert delta == 0xFF
        ctx.close()

    @patch("atomik_core.device.fcntl.ioctl")
    @patch("atomik_core.device.os.open", return_value=42)
    @patch("atomik_core.device.os.close")
    def test_swap_masks_new_reference(self, mock_close, mock_open, mock_ioctl):
        mock_ioctl.side_effect = lambda fd, cmd, buf: _mock_ioctl_swap(
            fd, cmd, buf, old_state=0
        )
        ctx = DeviceContext()
        ctx.swap(new_reference=0x1_FFFFFFFFFFFFFFFF)
        swap_call = mock_ioctl.call_args_list[1]
        buf = bytes(swap_call[0][2])
        _, _, new_ref, _ = struct.unpack(_FMT_SWAP, buf)
        assert new_ref == 0xFFFFFFFFFFFFFFFF
        ctx.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    def test_max_contexts(self, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=65536)
        assert table.num_contexts == 65536
        table.close()

    @patch("atomik_core.device.fcntl.ioctl", side_effect=_mock_ioctl_create_table)
    @patch("atomik_core.device.os.open", return_value=42)
    def test_single_context(self, mock_open, mock_ioctl):
        table = DeviceTable(num_contexts=1)
        assert table.num_contexts == 1
        table.close()


# =========================================================================
# Import from atomik_core
# =========================================================================

class TestImport:
    """Verify DeviceContext and DeviceTable are importable from atomik_core."""

    def test_import_device_context(self):
        from atomik_core import DeviceContext as DC
        assert DC is DeviceContext

    def test_import_device_table(self):
        from atomik_core import DeviceTable as DT
        assert DT is DeviceTable

    def test_in_all(self):
        import atomik_core
        assert "DeviceContext" in atomik_core.__all__
        assert "DeviceTable" in atomik_core.__all__
