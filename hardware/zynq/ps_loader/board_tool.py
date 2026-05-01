#!/usr/bin/env python3
"""
ATOMiK Board Tool — structured board introspection for Claude.

Provides named commands that query the Zynq board and return structured
results. Built on board_cmd.py's ~command protocol.

Usage:
    python3 board_tool.py status          # full system status
    python3 board_tool.py health          # quick health check
    python3 board_tool.py cpu             # CPU info
    python3 board_tool.py memory          # memory usage
    python3 board_tool.py processes       # running processes
    python3 board_tool.py files [path]    # list files
    python3 board_tool.py read <path>     # read file from board
    python3 board_tool.py write <path>    # write stdin to board file
    python3 board_tool.py mmio <addr>     # read MMIO register
    python3 board_tool.py atomik          # ATOMiK hardware state
    python3 board_tool.py demo            # current demo state
    python3 board_tool.py exec <cmd>      # raw command execution
    python3 board_tool.py compile <src>   # cross-compile + transfer + run
    python3 board_tool.py deploy          # rebuild + transfer atomik_live
"""

import json
import os
import sys
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)

from board_cmd import send_command

PORT = "/dev/ttyUSB2"
BAUD = 921600


def cmd(command, timeout=15):
    """Run a command on the board, return output string."""
    out, rc = send_command(command, PORT, BAUD, timeout)
    return out


def cmd_status():
    """Full system status — one-shot snapshot of the board."""
    lines = []
    lines.append("=== ATOMiK Board Status ===")
    lines.append("")

    # System
    lines.append(f"Kernel: {cmd('uname -r')}")
    lines.append(f"Uptime: {cmd('cat /proc/uptime').split()[0]}s")
    lines.append(f"Date:   {cmd('date')}")
    lines.append("")

    # CPU
    cpu = cmd("grep -E 'isa|hart' /proc/cpuinfo")
    for l in cpu.split('\n'):
        if l.strip():
            lines.append(f"CPU: {l.strip()}")
    lines.append("")

    # Memory
    mem = cmd("free -h | grep Mem")
    if mem:
        parts = mem.split()
        lines.append(f"Memory: {parts[2]} free / {parts[1]} total")

    # Disk
    disk = cmd("df -h / | tail -1")
    if disk:
        parts = disk.split()
        lines.append(f"Disk:   {parts[3]} free / {parts[1]} total ({parts[4]} used)")
    lines.append("")

    # Processes
    nproc = cmd("ps aux | wc -l")
    lines.append(f"Processes: {nproc}")

    # atomik_live
    al = cmd("ps aux | grep atomik_live | grep -v grep | head -1")
    lines.append(f"atomik_live: {'RUNNING' if 'atomik_live' in al else 'NOT RUNNING'}")
    lines.append("")

    # Files in /tmp
    files = cmd("ls -la /tmp/ 2>/dev/null | tail -5")
    lines.append("Recent /tmp files:")
    for l in files.split('\n'):
        if l.strip():
            lines.append(f"  {l.strip()}")

    return "\n".join(lines)


def cmd_health():
    """Quick health check — is everything working?"""
    checks = []

    # Board alive
    ping = cmd("echo OK")
    checks.append(f"Board:       {'OK' if 'OK' in ping else 'FAIL'}")

    # Kernel
    kern = cmd("uname -r")
    checks.append(f"Kernel:      {kern if kern else 'FAIL'}")

    # Memory
    mem = cmd("awk '/MemAvailable/{print $2}' /proc/meminfo")
    if mem:
        mb = int(mem) // 1024
        checks.append(f"Free RAM:    {mb} MB {'OK' if mb > 100 else 'LOW'}")
    else:
        checks.append("Free RAM:    UNKNOWN")

    # atomik_live
    al = cmd("pgrep -c atomik_live 2>/dev/null || echo 0")
    checks.append(f"atomik_live: {'RUNNING' if al.strip() != '0' else 'NOT RUNNING'}")

    # HDMI (check if fb device exists)
    fb = cmd("ls /dev/fb0 2>/dev/null || echo NONE")
    checks.append(f"Framebuffer: {'OK' if '/dev/fb0' in fb else 'via /dev/mem'}")

    return "\n".join(checks)


def cmd_cpu():
    """CPU information."""
    return cmd("cat /proc/cpuinfo")


def cmd_memory():
    """Memory information."""
    return cmd("free -h") + "\n\n" + cmd("head -10 /proc/meminfo")


def cmd_processes():
    """Running processes."""
    return cmd("ps aux")


def cmd_files(path="/tmp"):
    """List files at path."""
    return cmd(f"ls -la {path}")


def cmd_read(path):
    """Read a file from the board."""
    return cmd(f"cat {path}")


def cmd_write(path, content):
    """Write content to a file on the board."""
    # Escape for shell, write via heredoc-style echo
    # For safety, base64 encode
    import base64
    b64 = base64.b64encode(content.encode()).decode()
    return cmd(f"echo '{b64}' | base64 -d > {path}")


def cmd_mmio(addr):
    """Read an MMIO register via devmem."""
    return cmd(f"devmem {addr} 32 2>/dev/null || echo 'devmem not available'")


def cmd_atomik():
    """Read ATOMiK hardware registers."""
    lines = ["=== ATOMiK Hardware State ==="]
    # Adapter registers at 0xF0020000
    regs = {
        "CMD":  "0xF0020000",
        "RS1":  "0xF0020004",
        "RS2":  "0xF0020008",
        "RD":   "0xF002000C",
    }
    for name, addr in regs.items():
        val = cmd(f"devmem {addr} 32 2>/dev/null || echo N/A")
        lines.append(f"  {name} ({addr}): {val}")

    # VTG status
    vtg = cmd("devmem 0xF0002800 32 2>/dev/null || echo N/A")
    lines.append(f"  VTG_EN (0xF0002800): {vtg}")

    dma = cmd("devmem 0xF0002004 32 2>/dev/null || echo N/A")
    lines.append(f"  DMA_EN (0xF0002004): {dma}")

    return "\n".join(lines)


def cmd_demo():
    """Current demo state — parse recent ##EVENT from atomik_live."""
    # The demo state is in atomik_live's stdout which goes to UART
    # We can't read it directly, but we can trigger a detect cycle
    # For now, return what we can observe
    lines = ["=== Demo State ==="]
    al = cmd("pgrep -a atomik_live 2>/dev/null || echo NOT_RUNNING")
    lines.append(f"Process: {al}")
    up = cmd("cat /proc/uptime")
    lines.append(f"Uptime: {up}")
    return "\n".join(lines)


def cmd_exec(command):
    """Raw command execution on the board."""
    return cmd(command)


def cmd_compile(source_path):
    """Cross-compile a C file on the laptop, transfer to board, run it."""
    if not os.path.exists(source_path):
        return f"Error: {source_path} not found"

    basename = os.path.splitext(os.path.basename(source_path))[0]
    local_bin = f"/tmp/{basename}_board"
    remote_bin = f"/tmp/{basename}"

    # Cross-compile
    ret = os.system(f"riscv64-linux-gnu-gcc -O2 -o {local_bin} {source_path} 2>&1")
    if ret != 0:
        return "Compile failed"

    os.system(f"riscv64-linux-gnu-strip {local_bin}")
    size = os.path.getsize(local_bin)

    # Transfer via base64
    import base64, gzip
    with open(local_bin, 'rb') as f:
        raw = f.read()
    compressed = gzip.compress(raw, compresslevel=9)
    b64 = base64.b64encode(compressed).decode()

    # Write b64 to board
    CHUNK = 128
    cmd(f"rm -f /tmp/xfer.b64 /tmp/xfer.gz {remote_bin}")
    chunks = [b64[i:i+CHUNK] for i in range(0, len(b64), CHUNK)]
    for chunk in chunks:
        cmd(f"printf '%s' '{chunk}' >> /tmp/xfer.b64")

    cmd("base64 -d /tmp/xfer.b64 > /tmp/xfer.gz")
    cmd(f"gunzip -c /tmp/xfer.gz > {remote_bin}")
    cmd(f"chmod +x {remote_bin}")

    # Verify size
    remote_size = cmd(f"stat -c%s {remote_bin} 2>/dev/null || echo 0")
    if str(size) not in remote_size:
        return f"Transfer failed: expected {size}, got {remote_size}"

    # Run it
    output = cmd(remote_bin, timeout=30)
    return f"Compiled {source_path} ({size}B), transferred, output:\n{output}"


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    command = sys.argv[1]
    args = sys.argv[2:]

    dispatch = {
        "status":    lambda: cmd_status(),
        "health":    lambda: cmd_health(),
        "cpu":       lambda: cmd_cpu(),
        "memory":    lambda: cmd_memory(),
        "processes": lambda: cmd_processes(),
        "files":     lambda: cmd_files(args[0] if args else "/tmp"),
        "read":      lambda: cmd_read(args[0]) if args else "Usage: read <path>",
        "mmio":      lambda: cmd_mmio(args[0]) if args else "Usage: mmio <addr>",
        "atomik":    lambda: cmd_atomik(),
        "demo":      lambda: cmd_demo(),
        "exec":      lambda: cmd_exec(" ".join(args)) if args else "Usage: exec <cmd>",
        "compile":   lambda: cmd_compile(args[0]) if args else "Usage: compile <file.c>",
    }

    if command in dispatch:
        result = dispatch[command]()
        if result:
            print(result)
    else:
        print(f"Unknown command: {command}")
        print(f"Available: {', '.join(sorted(dispatch.keys()))}")
        sys.exit(1)


if __name__ == "__main__":
    main()
