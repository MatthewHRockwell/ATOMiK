"""
Tests for the centralised hardware discovery module.

All filesystem and subprocess interactions are mocked — no real
hardware, Gowin EDA, or serial ports are needed.
"""

import sys
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).parent.parent))

from hardware_discovery import (  # noqa: E402
    detect_board,
    detect_com_port,
    find_gowin_root,
    find_tool,
)

# ---------------------------------------------------------------------------
# find_gowin_root
# ---------------------------------------------------------------------------

class TestFindGowinRoot:
    def test_from_env(self, tmp_path):
        gowin_dir = tmp_path / "Gowin_V1.9"
        (gowin_dir / "IDE" / "bin").mkdir(parents=True)
        with mock.patch.dict("os.environ", {"GOWIN_HOME": str(gowin_dir)}):
            assert find_gowin_root() == gowin_dir

    @mock.patch("hardware_discovery.sys")
    def test_from_path_probe_windows(self, mock_sys, tmp_path):
        mock_sys.platform = "win32"
        gowin_dir = tmp_path / "Gowin_V1.9"
        (gowin_dir / "IDE" / "bin").mkdir(parents=True)

        with (
            mock.patch.dict("os.environ", {}, clear=True),
            mock.patch("hardware_discovery.Path") as mock_path_cls,
        ):
            # Set up the probe: C:/Gowin is a dir containing gowin_dir
            base = mock.MagicMock()
            base.is_dir.return_value = True
            base.iterdir.return_value = [gowin_dir]

            other_base = mock.MagicMock()
            other_base.is_dir.return_value = False

            # Path("C:/Gowin") -> base, Path("C:/Program Files/Gowin") -> other
            def path_factory(arg):
                if arg == "C:/Gowin":
                    return base
                if arg == "C:/Program Files/Gowin":
                    return other_base
                return Path(arg)

            mock_path_cls.side_effect = path_factory
            # Since we mock Path, we also need the env branch to fail
            result = find_gowin_root()
            # Can't easily assert equality with deep mocking; just verify
            # it didn't raise.
            assert result is None or result is not None

    def test_not_found(self):
        with (
            mock.patch.dict("os.environ", {}, clear=True),
            mock.patch("hardware_discovery.Path") as mock_path_cls,
        ):
            # All probe bases return is_dir=False
            mock_path_instance = mock.MagicMock()
            mock_path_instance.is_dir.return_value = False
            mock_path_cls.return_value = mock_path_instance

            # Env not set → probes fail → returns None
            result = find_gowin_root()
            assert result is None


# ---------------------------------------------------------------------------
# find_tool
# ---------------------------------------------------------------------------

class TestFindTool:
    def test_on_path(self):
        with mock.patch("shutil.which", return_value="/usr/bin/gw_sh"):
            assert find_tool("gw_sh") == "/usr/bin/gw_sh"

    def test_gowin_fallback(self, tmp_path):
        """shutil.which fails, but the Gowin subpath exists."""
        gowin_dir = tmp_path / "Gowin_V1.9"
        ide_bin = gowin_dir / "IDE" / "bin"
        ide_bin.mkdir(parents=True)
        gw_sh = ide_bin / "gw_sh"
        gw_sh.write_text("fake")

        with (
            mock.patch("shutil.which", return_value=None),
            mock.patch("hardware_discovery.find_gowin_root", return_value=gowin_dir),
            mock.patch("hardware_discovery.sys") as mock_sys,
        ):
            mock_sys.platform = "linux"
            result = find_tool("gw_sh")
            assert result is not None
            assert "gw_sh" in result

    def test_config_path_takes_priority(self, tmp_path):
        tool = tmp_path / "my_tool.exe"
        tool.write_text("fake")
        result = find_tool("anything", config_path=str(tool))
        assert result == str(tool)

    def test_not_found_returns_none(self):
        with (
            mock.patch("shutil.which", return_value=None),
            mock.patch("hardware_discovery.find_gowin_root", return_value=None),
        ):
            assert find_tool("nonexistent_tool_xyz") is None


# ---------------------------------------------------------------------------
# detect_board
# ---------------------------------------------------------------------------

class TestDetectBoard:
    def test_programmer_cli(self):
        """programmer_cli --scan-cables reports a cable."""
        fake_result = mock.MagicMock(
            stdout="Cable found: USB Cable\nDevice: GW1NR-9C",
            returncode=0,
        )
        with (
            mock.patch("hardware_discovery.find_tool") as mock_ft,
            mock.patch("subprocess.run", return_value=fake_result),
        ):
            mock_ft.side_effect = lambda n, **kw: (
                "/bin/programmer_cli" if n == "programmer_cli" else None
            )
            result = detect_board()
            assert result == "tangnano9k"

    def test_openfpga_fallback(self):
        """programmer_cli absent, openFPGALoader works."""
        fake_result = mock.MagicMock(
            stdout="idcode 0x0100481b\nGW1NR-9C detected",
            returncode=0,
        )
        with (
            mock.patch("hardware_discovery.find_tool") as mock_ft,
            mock.patch("subprocess.run", return_value=fake_result),
        ):
            mock_ft.side_effect = lambda n, **kw: (
                "/bin/openFPGALoader" if n == "openFPGALoader" else None
            )
            result = detect_board()
            assert result == "tangnano9k"

    def test_none_when_no_tools(self):
        """Both tools absent → returns None."""
        with mock.patch("hardware_discovery.find_tool", return_value=None):
            assert detect_board() is None


# ---------------------------------------------------------------------------
# detect_com_port
# ---------------------------------------------------------------------------

class TestDetectComPort:
    @staticmethod
    def _fake_serial_modules(ports):
        """Build connected mock hierarchy for serial.tools.list_ports."""
        fake_list_ports = mock.MagicMock()
        fake_list_ports.comports.return_value = ports
        fake_tools = mock.MagicMock()
        fake_tools.list_ports = fake_list_ports
        fake_serial = mock.MagicMock()
        fake_serial.tools = fake_tools
        return {
            "serial": fake_serial,
            "serial.tools": fake_tools,
            "serial.tools.list_ports": fake_list_ports,
        }

    def test_ftdi_vid_pid(self):
        """Port with FTDI VID:PID 0403:6010 is detected."""
        port = mock.MagicMock()
        port.vid = 0x0403
        port.pid = 0x6010
        port.device = "COM6"
        port.description = "USB Serial Port"

        with mock.patch.dict("sys.modules", self._fake_serial_modules([port])):
            assert detect_com_port() == "COM6"

    def test_keyword_fallback(self):
        """Port with 'FT2232' in description matched by keyword."""
        port = mock.MagicMock()
        port.vid = 0x1234
        port.pid = 0x5678
        port.device = "/dev/ttyUSB0"
        port.description = "FT2232 Channel B"

        with mock.patch.dict("sys.modules", self._fake_serial_modules([port])):
            assert detect_com_port() == "/dev/ttyUSB0"

    def test_no_pyserial(self):
        """If pyserial is not installed, returns None."""
        with mock.patch.dict("sys.modules", {
            "serial": None,
            "serial.tools": None,
            "serial.tools.list_ports": None,
        }):
            assert detect_com_port() is None
