#!/usr/bin/env python3
"""
ATOMiK FPGA Automation Pipeline

Orchestrates RTL simulation, FPGA programming via openFPGALoader,
and hardware-in-the-loop validation for the Tang Nano 9K.

Exit codes:
    0: All steps passed
    1: Simulation failure
    2: Board not detected / programming failure
    3: Hardware validation failure
    4: Configuration error (missing tools, bad arguments)

Usage:
    python scripts/fpga_pipeline.py                          # Full pipeline
    python scripts/fpga_pipeline.py --sim-only               # RTL simulation only
    python scripts/fpga_pipeline.py --program-only           # Program FPGA only
    python scripts/fpga_pipeline.py --validate-only --port COM6  # Hardware tests only
    python scripts/fpga_pipeline.py --flash                  # Program to flash
    python scripts/fpga_pipeline.py --report results.json    # Save JSON report
    python scripts/fpga_pipeline.py --skip-sim               # Skip simulation
    python scripts/fpga_pipeline.py --check-drivers          # Diagnose USB drivers
    python scripts/fpga_pipeline.py --setup-drivers          # Fix USB drivers (admin)
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Resolve project root (scripts/ is one level below root)
# ---------------------------------------------------------------------------
PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent

# ---------------------------------------------------------------------------
# Default configuration
# ---------------------------------------------------------------------------
DEFAULTS = {
    "openFPGALoader": "openFPGALoader",
    "iverilog": "iverilog",
    "vvp": "vvp",
    "board": "tangnano9k",
    "bitstream": "impl/pnr/ATOMiK.fs",
    "uart_baudrate": 115200,
    "rtl_files": [
        "hardware/rtl/atomik_delta_acc.v",
        "hardware/rtl/atomik_state_rec.v",
        "hardware/rtl/atomik_core_v2.v",
    ],
    "testbench": "hardware/sim/tb_core_v2.v",
    "post_program_delay_s": 3.0,
    "usb_vid_pid": ["0403:6010"],
}

sys.path.insert(0, str(PROJECT_ROOT / "software"))
from atomik_sdk.hardware_discovery import find_tool as _hd_find_tool


# ===================================================================
# Utility helpers
# ===================================================================

def _find_tool(name: str, config_path: str | None = None) -> str | None:
    """Return an absolute path (or bare command name) for *name*.

    Priority:
      1. Explicit path from config / CLI
      2. Shared hardware_discovery lookup (PATH + Gowin + fallbacks)
    """
    return _hd_find_tool(name, config_path=config_path)


def _load_config(config_path: str | None) -> dict:
    """Merge fpga_config.json (if present) with built-in defaults."""
    cfg = dict(DEFAULTS)
    path = None
    if config_path:
        path = Path(config_path)
    else:
        default_cfg = PROJECT_ROOT / "hardware" / "scripts" / "fpga_config.json"
        if default_cfg.exists():
            path = default_cfg
    if path and path.exists():
        with open(path) as f:
            cfg.update(json.load(f))
    return cfg


def _detect_com_port(vid_pid_list: list[str]) -> str | None:
    """Auto-detect a serial port matching known USB VID:PID pairs."""
    try:
        import serial.tools.list_ports
    except ImportError:
        return None

    for port_info in serial.tools.list_ports.comports():
        if port_info.vid is not None and port_info.pid is not None:
            device_id = f"{port_info.vid:04X}:{port_info.pid:04X}".upper()
            for pattern in vid_pid_list:
                if pattern.upper() == device_id:
                    return port_info.device
        # Also match on description keywords
        desc = (port_info.description or "").lower()
        if "tang" in desc or "gowin" in desc or "ft2232" in desc:
            return port_info.device
    return None


# ===================================================================
# USB Driver Diagnostics (Windows)
# ===================================================================

# Expected driver configuration for Tang Nano 9K (FT2232 dual-channel):
#   Interface 0 (MI_00): JTAG  -> WinUSB  (for openFPGALoader / libusb)
#   Interface 1 (MI_01): UART  -> FTDIBUS (for COM port / pyserial)

FTDI_VID_PID = "VID_0403&PID_6010"

_DRIVER_LABELS = {
    "WinUSB": "WinUSB (libusb-compatible)",
    "FTDIBUS": "FTDI Serial (COM port)",
    "usbccgp": "USB Composite (parent)",
    "usbser": "Windows CDC Serial",
}


def _query_ftdi_devices() -> list[dict]:
    """Query Windows PnP for all active FTDI FT2232 device nodes.

    Returns a list of dicts with keys: name, instance_id, class, service, mi.
    """
    if sys.platform != "win32":
        return []

    ps_script = (
        "Get-PnpDevice -Status OK "
        '| Where-Object { $_.InstanceId -match "VID_0403&PID_6010" } '
        "| ForEach-Object { "
        '  $svc = (Get-PnpDeviceProperty -InstanceId $_.InstanceId '
        '    | Where-Object { $_.KeyName -eq "DEVPKEY_Device_Service" }).Data; '
        "  [PSCustomObject]@{ "
        "    Name=$_.FriendlyName; "
        "    Id=$_.InstanceId; "
        "    Class=$_.Class; "
        "    Service=$svc "
        "  } "
        "} | ConvertTo-Json -Compress"
    )

    try:
        r = subprocess.run(
            ["powershell.exe", "-NoProfile", "-Command", ps_script],
            capture_output=True, text=True, timeout=15,
        )
        if r.returncode != 0 or not r.stdout.strip():
            return []
        data = json.loads(r.stdout)
        # PowerShell returns a single object (not array) when there's one result
        if isinstance(data, dict):
            data = [data]
        # Extract MI (multi-interface) index from InstanceId
        for d in data:
            mi_match = re.search(r"MI_(\d+)", d.get("Id", ""))
            d["mi"] = int(mi_match.group(1)) if mi_match else None
        return data
    except Exception:
        return []


def diagnose_drivers() -> dict:
    """Check FT2232 USB driver state and return a diagnostic report.

    Returns dict with keys:
      - found: bool -- FT2232 device detected at USB level
      - interfaces: list of {mi, name, service, ok, needed}
      - jtag_ready: bool -- Interface 0 has WinUSB
      - uart_ready: bool -- Interface 1 has FTDI serial / COM port available
      - actions: list of human-readable fix instructions
    """
    report = {
        "found": False,
        "interfaces": [],
        "jtag_ready": False,
        "uart_ready": False,
        "actions": [],
    }

    if sys.platform != "win32":
        report["actions"].append("Driver diagnostics only supported on Windows")
        return report

    devices = _query_ftdi_devices()
    if not devices:
        report["actions"].append(
            "No FTDI FT2232 device found. Is the Tang Nano 9K plugged in via USB?"
        )
        return report

    report["found"] = True

    for dev in devices:
        mi = dev.get("mi")
        svc = (dev.get("Service") or "").strip()
        name = dev.get("Name", "")

        if mi == 0:
            needed = "WinUSB"
            ok = svc.lower() == "winusb"
            report["jtag_ready"] = ok
            report["interfaces"].append({
                "mi": 0, "role": "JTAG", "name": name,
                "service": svc, "ok": ok, "needed": needed,
            })
        elif mi == 1:
            needed = "FTDIBUS"
            ok = svc.lower() in ("ftdibus", "ftser2k", "usbser")
            report["uart_ready"] = ok
            report["interfaces"].append({
                "mi": 1, "role": "UART", "name": name,
                "service": svc, "ok": ok, "needed": needed,
            })
        # Skip parent composite device node

    # Build action list
    jtag_iface = next((i for i in report["interfaces"] if i["mi"] == 0), None)
    uart_iface = next((i for i in report["interfaces"] if i["mi"] == 1), None)

    if jtag_iface and not jtag_iface["ok"]:
        report["actions"].append(
            f'Interface 0 (JTAG): has "{jtag_iface["service"]}", needs WinUSB.\n'
            "  Fix: run  python scripts/fpga_pipeline.py --setup-drivers\n"
            "       or use Zadig: select 'USB Serial Converter A' -> WinUSB -> Replace"
        )
    if uart_iface and not uart_iface["ok"]:
        report["actions"].append(
            f'Interface 1 (UART): has "{uart_iface["service"]}", needs FTDI Serial.\n'
            "  Fix: Device Manager -> 'JTAG Debugger' -> Update driver ->\n"
            "       Browse -> Let me pick -> Ports (COM & LPT) -> FTDI -> USB Serial Port"
        )
    if not jtag_iface:
        report["actions"].append(
            "Interface 0 (JTAG) not found. The FT2232 may not be enumerating properly.\n"
            "  Try: unplug/replug the USB cable, or use a different USB port."
        )

    return report


def print_driver_report(report: dict):
    """Pretty-print the driver diagnostic report."""
    print("=" * 60)
    print("USB Driver Diagnostics -- Tang Nano 9K (FT2232)")
    print("=" * 60)
    print()

    if not report["found"]:
        print("  [FAIL] No FTDI FT2232 device detected")
        for action in report["actions"]:
            print(f"  {action}")
        return

    print("  [INFO] FTDI FT2232 device found")
    print()

    for iface in report["interfaces"]:
        svc = iface["service"] or "(none)"
        label = _DRIVER_LABELS.get(svc, svc)
        status = "OK" if iface["ok"] else "WRONG"
        needed_label = _DRIVER_LABELS.get(iface["needed"], iface["needed"])
        print(f"  Interface {iface['mi']} ({iface['role']}): {iface['name']}")
        print(f"    Driver:  {svc} -- {label}")
        print(f"    Status:  [{status}]", end="")
        if not iface["ok"]:
            print(f"  (needs: {needed_label})")
        else:
            print()
        print()

    jtag = "READY" if report["jtag_ready"] else "NOT READY"
    uart = "READY" if report["uart_ready"] else "NOT READY"
    print(f"  JTAG programming: [{jtag}]")
    print(f"  UART serial port: [{uart}]")
    print()

    if report["actions"]:
        print("-" * 60)
        print("  Required actions:")
        print()
        for i, action in enumerate(report["actions"], 1):
            for j, line in enumerate(action.splitlines()):
                prefix = f"  {i}. " if j == 0 else "     "
                print(f"{prefix}{line}")
            print()
    else:
        print("  All drivers configured correctly.")


# ===================================================================
# Automated Driver Setup (Windows, requires admin)
# ===================================================================

_WINUSB_INF_TEMPLATE = """\
; ATOMiK FPGA Pipeline -- WinUSB driver for FT2232 JTAG interface
; Auto-generated. Install with: pnputil /add-driver atomik_jtag.inf /install

[Version]
Signature   = "$Windows NT$"
Class       = USBDevice
ClassGUID   = {{88BAE032-5A81-49f0-BC3D-A4FF138216D6}}
Provider    = %ProviderName%
CatalogFile = atomik_jtag.cat
DriverVer   = 01/01/2025,1.0.0.0

[Manufacturer]
%MfgName% = DeviceList,NTamd64

[DeviceList.NTamd64]
%DeviceName% = USB_Install, USB\\VID_0403&PID_6010&MI_00

[USB_Install]
Include = winusb.inf
Needs   = WINUSB.NT

[USB_Install.Services]
Include = winusb.inf
AddService = WinUSB,0x00000002,WinUSB_ServiceInstall

[WinUSB_ServiceInstall]
DisplayName   = %ServiceName%
ServiceType   = 1
StartType     = 3
ErrorControl  = 1
ServiceBinary = %12%\\WinUSB.sys

[USB_Install.HW]
AddReg = Dev_AddReg

[Dev_AddReg]
HKR,,DeviceInterfaceGUIDs,0x10000,"{{a5dcbf10-6530-11d2-901f-00c04fb951ed}}"

[Strings]
ProviderName = "ATOMiK Project"
MfgName      = "FTDI"
DeviceName   = "Tang Nano 9K JTAG (WinUSB)"
ServiceName  = "WinUSB"
"""


def setup_drivers() -> bool:
    """Fix FT2232 USB driver configuration for both interfaces.

    Interface 0 (JTAG) -> WinUSB  (for openFPGALoader)
    Interface 1 (UART) -> FTDI Serial (for COM port)

    Requires running as Administrator. Returns True on success.
    """
    print("=" * 60)
    print("Automated Driver Setup")
    print("=" * 60)
    print()

    if sys.platform != "win32":
        print("  [SKIP] Only applicable on Windows")
        return False

    # Check admin
    try:
        import ctypes
        is_admin = ctypes.windll.shell32.IsUserAnAdmin() != 0
    except Exception:
        is_admin = False

    if not is_admin:
        print("  [FAIL] Administrator privileges required.")
        print()
        print("  Re-run from an elevated prompt:")
        print("    1. Open PowerShell or CMD as Administrator")
        print("    2. python scripts/fpga_pipeline.py --setup-drivers")
        return False

    report = diagnose_drivers()

    if not report["found"]:
        print("  [FAIL] No FT2232 device found. Connect the Tang Nano 9K first.")
        return False

    if report["jtag_ready"] and report["uart_ready"]:
        print("  [OK] Both interfaces already configured correctly.")
        return True

    success = True

    # --- Fix Interface 0 (JTAG -> WinUSB) ---
    jtag_iface = next((i for i in report["interfaces"] if i["mi"] == 0), None)
    if jtag_iface and not jtag_iface["ok"]:
        print("  [Step 1] Installing WinUSB on Interface 0 (JTAG)...")
        print()
        if _install_winusb_on_jtag():
            print("  [PASS] WinUSB driver staged for Interface 0")
        else:
            print("  [FAIL] Could not install WinUSB automatically")
            _print_manual_jtag_instructions()
            success = False
        print()
    elif jtag_iface:
        print("  [OK] Interface 0 (JTAG) already has WinUSB")
    else:
        print("  [WARN] Interface 0 (JTAG) not found")
        success = False

    # --- Fix Interface 1 (UART -> FTDI Serial) ---
    uart_iface = next((i for i in report["interfaces"] if i["mi"] == 1), None)
    if uart_iface and not uart_iface["ok"]:
        print("  [Step 2] Restoring FTDI Serial on Interface 1 (UART)...")
        print()
        if _restore_ftdi_on_uart():
            print("  [PASS] FTDI serial driver restored for Interface 1")
        else:
            print("  [FAIL] Could not restore FTDI driver automatically")
            _print_manual_uart_instructions()
            success = False
        print()
    elif uart_iface:
        print("  [OK] Interface 1 (UART) already has FTDI Serial")

    if success:
        print()
        print("  Unplug and replug the USB cable for changes to take effect.")
        print("  Then verify with: python scripts/fpga_pipeline.py --check-drivers")

    return success


def _install_winusb_on_jtag() -> bool:
    """Stage a WinUSB .inf for FT2232 Interface 0 via pnputil."""
    with tempfile.TemporaryDirectory() as tmpdir:
        inf_path = os.path.join(tmpdir, "atomik_jtag.inf")
        cat_path = os.path.join(tmpdir, "atomik_jtag.cat")

        with open(inf_path, "w") as f:
            f.write(_WINUSB_INF_TEMPLATE)
        with open(cat_path, "w") as f:
            f.write("")

        r = subprocess.run(
            ["pnputil", "/add-driver", inf_path, "/install"],
            capture_output=True, text=True, timeout=30,
        )
        output = r.stdout + r.stderr
        for line in output.strip().splitlines():
            print(f"    {line}")

        return r.returncode == 0


def _restore_ftdi_on_uart() -> bool:
    """Try to switch Interface 1 back to the FTDI serial driver.

    Uses PowerShell to disable/enable the device, prompting Windows
    to re-evaluate driver ranking and pick the FTDI driver.
    """
    # Find Interface 1 instance ID
    devices = _query_ftdi_devices()
    instance_id = ""
    for dev in devices:
        if dev.get("mi") == 1:
            instance_id = dev.get("Id", "")
            break

    if not instance_id:
        print("    Interface 1 not found")
        return False

    print(f"    Device: {instance_id}")

    # Disable and re-enable, which triggers driver re-evaluation
    ps_cmd = (
        f"Disable-PnpDevice -InstanceId '{instance_id}' -Confirm:$false; "
        "Start-Sleep -Seconds 2; "
        f"Enable-PnpDevice -InstanceId '{instance_id}' -Confirm:$false"
    )
    try:
        r = subprocess.run(
            ["powershell.exe", "-NoProfile", "-Command", ps_cmd],
            capture_output=True, text=True, timeout=20,
        )
        output = (r.stdout + r.stderr).strip()
        if output:
            for line in output.splitlines():
                print(f"    {line}")

        # Wait for driver re-enumeration and check result
        time.sleep(2)
        new_report = diagnose_drivers()
        return new_report["uart_ready"]

    except Exception as e:
        print(f"    Error: {e}")
        return False


def _print_manual_jtag_instructions():
    """Print manual fix instructions for Interface 0."""
    print()
    print("  Manual fix for Interface 0 (JTAG -> WinUSB):")
    print()
    print("  Option A -- Zadig (recommended):")
    print("    1. Download Zadig from https://zadig.akeo.ie/")
    print("    2. Options -> List All Devices")
    print("    3. Select 'USB Serial Converter A' (Interface 0)")
    print("    4. Set target driver to 'WinUSB'")
    print("    5. Click 'Replace Driver'")
    print()
    print("  Option B -- Device Manager:")
    print("    1. Find 'USB Serial Converter A' under USB controllers")
    print("    2. Right-click -> Update driver -> Browse -> Let me pick")
    print("    3. Select 'Universal Serial Bus devices' -> WinUSB Device")


def _print_manual_uart_instructions():
    """Print manual fix instructions for Interface 1."""
    print()
    print("  Manual fix for Interface 1 (UART -> FTDI Serial):")
    print()
    print("  Device Manager:")
    print("    1. Find 'JTAG Debugger' under 'Universal Serial Bus devices'")
    print("    2. Right-click -> Update driver")
    print("    3. Browse my computer -> Let me pick from a list")
    print("    4. Select 'USB Serial Converter' (FTDI driver)")
    print("    5. After install, a new COM port should appear")


# ===================================================================
# Pipeline steps
# ===================================================================

class PipelineResult:
    """Accumulates step outcomes for the final report."""

    def __init__(self):
        self.steps: list[dict] = []
        self.start_time = datetime.now(timezone.utc)

    def record(self, name: str, passed: bool, detail: str = "", duration_s: float = 0.0):
        self.steps.append({
            "step": name,
            "passed": passed,
            "detail": detail,
            "duration_s": round(duration_s, 3),
        })

    @property
    def all_passed(self) -> bool:
        return all(s["passed"] for s in self.steps)

    def to_dict(self) -> dict:
        return {
            "timestamp": self.start_time.isoformat(),
            "all_passed": self.all_passed,
            "steps": self.steps,
        }


# -- Step 1: RTL Simulation -------------------------------------------

def step_simulate(cfg: dict, result: PipelineResult) -> bool:
    """Run iverilog + vvp on the project testbench."""
    print("=" * 60)
    print("Step 1: RTL Simulation")
    print("=" * 60)

    iverilog = _find_tool("iverilog", cfg.get("iverilog"))
    vvp = _find_tool("vvp", cfg.get("vvp"))

    if not iverilog or not vvp:
        msg = "iverilog/vvp not found in PATH"
        print(f"  [FAIL] {msg}")
        result.record("rtl_simulation", False, msg)
        return False

    rtl_files = [str(PROJECT_ROOT / f) for f in cfg["rtl_files"]]
    testbench = str(PROJECT_ROOT / cfg["testbench"])

    # Verify source files exist
    for src in rtl_files + [testbench]:
        if not os.path.isfile(src):
            msg = f"Source file not found: {src}"
            print(f"  [FAIL] {msg}")
            result.record("rtl_simulation", False, msg)
            return False

    sim_output = str(PROJECT_ROOT / "hardware" / "sim" / "pipeline_sim.vvp")
    compile_cmd = [iverilog, "-o", sim_output] + rtl_files + [testbench]

    print(f"  Compiling: iverilog -> {Path(sim_output).name}")
    t0 = time.monotonic()

    try:
        comp = subprocess.run(
            compile_cmd, capture_output=True, text=True, timeout=30
        )
        if comp.returncode != 0:
            msg = comp.stderr.strip() or "iverilog compilation failed"
            print(f"  [FAIL] {msg}")
            result.record("rtl_simulation", False, msg, time.monotonic() - t0)
            return False
        print("  [PASS] Compilation succeeded")

        print(f"  Running: vvp {Path(sim_output).name}")
        sim = subprocess.run(
            [vvp, sim_output], capture_output=True, text=True, timeout=60
        )
        elapsed = time.monotonic() - t0

        # Parse pass/fail from simulation output
        output = sim.stdout
        lines = output.strip().splitlines()
        for line in lines:
            print(f"    {line}")

        if sim.returncode != 0:
            msg = "Simulation returned non-zero exit code"
            print(f"  [FAIL] {msg}")
            result.record("rtl_simulation", False, msg, elapsed)
            return False

        # Look for common pass/fail markers in simulation output
        combined = output.lower()
        if "all tests passed" in combined:
            pass  # Explicit pass marker from testbench
        elif "fail" in combined:
            # Check it's not just a "Failed: 0" summary line
            import re
            real_failures = re.search(r'failed\s*:\s*[1-9]', combined)
            has_fail_tag = "fail [" in combined or "[fail]" in combined
            if real_failures or has_fail_tag:
                msg = "Simulation output contains failures"
                print(f"  [WARN] {msg}")
                result.record("rtl_simulation", False, msg, elapsed)
                return False

        print(f"  [PASS] Simulation completed ({elapsed:.1f}s)")
        result.record("rtl_simulation", True, f"{len(lines)} output lines", elapsed)
        return True

    except subprocess.TimeoutExpired:
        msg = "Simulation timed out"
        print(f"  [FAIL] {msg}")
        result.record("rtl_simulation", False, msg, time.monotonic() - t0)
        return False
    except FileNotFoundError as e:
        msg = f"Tool not found: {e}"
        print(f"  [FAIL] {msg}")
        result.record("rtl_simulation", False, msg)
        return False
    finally:
        # Cleanup temp vvp file
        if os.path.isfile(sim_output):
            os.remove(sim_output)


# -- Step 2: Board Detection ------------------------------------------

def step_detect_board(cfg: dict, result: PipelineResult) -> bool:
    """Detect the FPGA board via openFPGALoader --detect."""
    print()
    print("=" * 60)
    print("Step 2: Board Detection")
    print("=" * 60)

    loader = _find_tool("openFPGALoader", cfg.get("openFPGALoader"))
    if not loader:
        msg = "openFPGALoader not found in PATH or fallback locations"
        print(f"  [FAIL] {msg}")
        result.record("board_detection", False, msg)
        return False

    print(f"  Using: {loader}")
    t0 = time.monotonic()

    try:
        det = subprocess.run(
            [loader, "--detect"], capture_output=True, text=True, timeout=15
        )
        elapsed = time.monotonic() - t0
        output = det.stdout + det.stderr  # openFPGALoader may use stderr

        for line in output.strip().splitlines():
            print(f"    {line}")

        board = cfg["board"]
        # Check for successful detection indicators
        out_lower = output.lower()
        if det.returncode == 0 and ("idcode" in out_lower or "detected" in out_lower
                                     or "gw1n" in out_lower):
            print(f"  [PASS] Board detected ({board})")
            result.record("board_detection", True, board, elapsed)
            return True
        else:
            # Provide actionable diagnostics on failure
            msg = "No FPGA board detected"
            if "unable to claim usb device" in out_lower or "ftdi" in out_lower:
                msg = "FTDI USB driver conflict -- wrong driver on JTAG interface"
            print(f"  [FAIL] {msg}")
            result.record("board_detection", False, msg, elapsed)

            # Run driver diagnostics automatically
            if sys.platform == "win32":
                print()
                report = diagnose_drivers()
                print_driver_report(report)

            return False

    except subprocess.TimeoutExpired:
        msg = "Board detection timed out"
        print(f"  [FAIL] {msg}")
        result.record("board_detection", False, msg, time.monotonic() - t0)
        return False
    except FileNotFoundError:
        msg = f"openFPGALoader executable not found at {loader}"
        print(f"  [FAIL] {msg}")
        result.record("board_detection", False, msg)
        return False


# -- Step 3: Program FPGA ---------------------------------------------

def step_program(cfg: dict, result: PipelineResult, flash: bool = False) -> bool:
    """Program the FPGA bitstream via openFPGALoader."""
    print()
    print("=" * 60)
    mode = "Flash" if flash else "SRAM"
    print(f"Step 3: Program FPGA ({mode})")
    print("=" * 60)

    loader = _find_tool("openFPGALoader", cfg.get("openFPGALoader"))
    if not loader:
        msg = "openFPGALoader not found"
        print(f"  [FAIL] {msg}")
        result.record("program_fpga", False, msg)
        return False

    bitstream = str(PROJECT_ROOT / cfg["bitstream"])
    if not os.path.isfile(bitstream):
        msg = f"Bitstream not found: {bitstream}"
        print(f"  [FAIL] {msg}")
        result.record("program_fpga", False, msg)
        return False

    board = cfg["board"]
    cmd = [loader, "-b", board]
    if flash:
        cmd.append("-f")
    cmd.append(bitstream)

    print(f"  Board: {board}")
    print(f"  Bitstream: {bitstream}")
    print(f"  Mode: {mode}")

    t0 = time.monotonic()
    try:
        prog = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        elapsed = time.monotonic() - t0
        output = prog.stdout + prog.stderr

        for line in output.strip().splitlines():
            print(f"    {line}")

        if prog.returncode == 0:
            print(f"  [PASS] FPGA programmed successfully ({elapsed:.1f}s)")
            result.record("program_fpga", True, f"{mode} mode", elapsed)
            return True
        else:
            msg = "Programming failed"
            print(f"  [FAIL] {msg}")
            result.record("program_fpga", False, msg, elapsed)
            return False

    except subprocess.TimeoutExpired:
        msg = "Programming timed out"
        print(f"  [FAIL] {msg}")
        result.record("program_fpga", False, msg, time.monotonic() - t0)
        return False
    except FileNotFoundError:
        msg = f"openFPGALoader not found at {loader}"
        print(f"  [FAIL] {msg}")
        result.record("program_fpga", False, msg)
        return False


# -- Step 4: COM Port Detection ---------------------------------------

def step_detect_port(cfg: dict, result: PipelineResult,
                     explicit_port: str | None = None,
                     delay: float | None = None) -> str | None:
    """Detect the serial port for UART communication."""
    print()
    print("=" * 60)
    print("Step 4: COM Port Detection")
    print("=" * 60)

    if explicit_port:
        print(f"  Using explicit port: {explicit_port}")
        result.record("port_detection", True, explicit_port)
        return explicit_port

    # Wait for USB re-enumeration after programming
    wait = delay if delay is not None else cfg.get("post_program_delay_s", 3.0)
    if wait > 0:
        print(f"  Waiting {wait}s for USB re-enumeration...")
        time.sleep(wait)

    vid_pids = cfg.get("usb_vid_pid", [])
    port = _detect_com_port(vid_pids)

    if port:
        print(f"  [PASS] Detected port: {port}")
        result.record("port_detection", True, port)
        return port

    # No port found -- diagnose
    msg = "No serial port detected"
    print(f"  [FAIL] {msg}")

    try:
        import serial.tools.list_ports
        all_ports = list(serial.tools.list_ports.comports())
        if all_ports:
            print("  [INFO] Available ports (none matched FTDI VID:PID):")
            for p in all_ports:
                print(f"    {p.device}: {p.description}")
        else:
            print("  [INFO] No COM ports visible at all")
            if sys.platform == "win32":
                report = diagnose_drivers()
                uart_iface = next(
                    (i for i in report["interfaces"] if i["mi"] == 1), None
                )
                if uart_iface and not uart_iface["ok"]:
                    print()
                    print(f'  Interface 1 (UART) driver: "{uart_iface["service"]}"')
                    print("  Needs FTDI Serial driver for COM port.")
                    print("  Fix: Device Manager -> right-click 'JTAG Debugger' ->")
                    print("       Update driver -> Browse -> Let me pick ->")
                    print("       Ports (COM & LPT) -> FTDI -> USB Serial Port")
    except ImportError:
        print("  [WARN] pyserial not installed (pip install pyserial)")

    result.record("port_detection", False, msg)
    return None


# -- Step 5: Hardware Validation ---------------------------------------

def step_validate(cfg: dict, result: PipelineResult, port: str) -> bool:
    """Run hardware validation tests via UART."""
    print()
    print("=" * 60)
    print("Step 5: Hardware Validation")
    print("=" * 60)

    # Import the existing test_hardware module
    try:
        sys.path.insert(0, str(PROJECT_ROOT / "hardware" / "scripts"))
        from test_hardware import run_tests
    except ImportError as e:
        msg = f"Cannot import test_hardware: {e}"
        print(f"  [FAIL] {msg}")
        result.record("hardware_validation", False, msg)
        return False

    print(f"  Port: {port}")
    print(f"  Baudrate: {cfg.get('uart_baudrate', 115200)}")
    print()

    t0 = time.monotonic()
    try:
        passed = run_tests(port)
        elapsed = time.monotonic() - t0

        if passed:
            print(f"\n  [PASS] Hardware validation passed ({elapsed:.1f}s)")
            result.record("hardware_validation", True, "All tests passed", elapsed)
            return True
        else:
            msg = "Some hardware tests failed"
            print(f"\n  [FAIL] {msg}")
            result.record("hardware_validation", False, msg, elapsed)
            return False

    except Exception as e:
        elapsed = time.monotonic() - t0
        msg = f"Hardware validation error: {e}"
        print(f"  [FAIL] {msg}")
        result.record("hardware_validation", False, msg, elapsed)
        return False


# -- Step 6: Report Generation ----------------------------------------

def step_report(result: PipelineResult, report_path: str | None = None):
    """Print summary and optionally write JSON report."""
    print()
    print("=" * 60)
    print("Pipeline Summary")
    print("=" * 60)

    for step in result.steps:
        status = "PASS" if step["passed"] else "FAIL"
        detail = f" ({step['detail']})" if step["detail"] else ""
        duration = f" [{step['duration_s']}s]" if step["duration_s"] else ""
        print(f"  [{status}] {step['step']}{detail}{duration}")

    print()
    if result.all_passed:
        print("*** ALL PIPELINE STEPS PASSED ***")
    else:
        failed = [s["step"] for s in result.steps if not s["passed"]]
        print(f"*** PIPELINE FAILED: {', '.join(failed)} ***")

    if report_path:
        report_data = result.to_dict()
        with open(report_path, "w") as f:
            json.dump(report_data, f, indent=2)
        print(f"\n  Report written to: {report_path}")


# ===================================================================
# Main orchestrator
# ===================================================================

def main() -> int:
    parser = argparse.ArgumentParser(
        description="ATOMiK FPGA Automation Pipeline",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  %(prog)s                          Full pipeline\n"
            "  %(prog)s --sim-only               RTL simulation only\n"
            "  %(prog)s --program-only           Program FPGA only\n"
            "  %(prog)s --validate-only --port COM6  Hardware tests only\n"
            "  %(prog)s --flash                  Program to flash memory\n"
            "  %(prog)s --skip-sim               Skip simulation step\n"
            "  %(prog)s --check-drivers          Diagnose USB driver state\n"
            "  %(prog)s --setup-drivers          Install correct drivers (admin)\n"
        ),
    )
    parser.add_argument("--sim-only", action="store_true",
                        help="Run RTL simulation only")
    parser.add_argument("--program-only", action="store_true",
                        help="Program FPGA only")
    parser.add_argument("--validate-only", action="store_true",
                        help="Run hardware validation only")
    parser.add_argument("--skip-sim", action="store_true",
                        help="Skip RTL simulation step")
    parser.add_argument("--flash", action="store_true",
                        help="Program to flash (persistent) instead of SRAM")
    parser.add_argument("--check-drivers", action="store_true",
                        help="Diagnose USB driver configuration")
    parser.add_argument("--setup-drivers", action="store_true",
                        help="Install correct USB drivers (requires admin)")
    parser.add_argument("--board", type=str, default=None,
                        help="Board name (default: tangnano9k)")
    parser.add_argument("--bitstream", type=str, default=None,
                        help="Path to bitstream file")
    parser.add_argument("--port", type=str, default=None,
                        help="Serial port (e.g. COM6, /dev/ttyUSB0)")
    parser.add_argument("--report", type=str, default=None,
                        help="Path to write JSON report")
    parser.add_argument("--config", type=str, default=None,
                        help="Path to fpga_config.json")

    args = parser.parse_args()

    # Load configuration
    cfg = _load_config(args.config)
    if args.board:
        cfg["board"] = args.board
    if args.bitstream:
        cfg["bitstream"] = args.bitstream

    result = PipelineResult()

    # --- Driver management modes (no pipeline banner) ---
    if args.check_drivers:
        report = diagnose_drivers()
        print_driver_report(report)
        if report["jtag_ready"] and report["uart_ready"]:
            return 0
        return 4

    if args.setup_drivers:
        ok = setup_drivers()
        return 0 if ok else 4

    print()
    print("*" * 60)
    print("  ATOMiK FPGA Automation Pipeline")
    print(f"  Started: {result.start_time.strftime('%Y-%m-%d %H:%M:%S UTC')}")
    print("*" * 60)
    print()

    # --- Sim-only mode ---
    if args.sim_only:
        ok = step_simulate(cfg, result)
        step_report(result, args.report)
        return 0 if ok else 1

    # --- Program-only mode ---
    if args.program_only:
        ok = step_detect_board(cfg, result)
        if ok:
            ok = step_program(cfg, result, flash=args.flash)
        step_report(result, args.report)
        return 0 if ok else 2

    # --- Validate-only mode ---
    if args.validate_only:
        port = step_detect_port(cfg, result, explicit_port=args.port, delay=0)
        if not port:
            step_report(result, args.report)
            return 4
        ok = step_validate(cfg, result, port)
        step_report(result, args.report)
        return 0 if ok else 3

    # --- Full pipeline ---
    # Step 1: RTL Simulation
    if not args.skip_sim:
        sim_ok = step_simulate(cfg, result)
        if not sim_ok:
            step_report(result, args.report)
            return 1

    # Step 2: Board Detection
    board_ok = step_detect_board(cfg, result)
    if not board_ok:
        step_report(result, args.report)
        return 2

    # Step 3: Program FPGA
    prog_ok = step_program(cfg, result, flash=args.flash)
    if not prog_ok:
        step_report(result, args.report)
        return 2

    # Step 4: COM Port Detection
    port = step_detect_port(cfg, result, explicit_port=args.port)
    if not port:
        step_report(result, args.report)
        return 2

    # Step 5: Hardware Validation
    hw_ok = step_validate(cfg, result, port)

    # Step 6: Report
    step_report(result, args.report)
    return 0 if hw_ok else 3


if __name__ == "__main__":
    sys.exit(main())
