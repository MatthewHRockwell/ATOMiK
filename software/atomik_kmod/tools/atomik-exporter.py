#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""
atomik-exporter — Prometheus exporter for ATOMiK kernel module metrics

Reads /proc/atomik and /sys/class/atomik/atomik0/ to expose metrics
in Prometheus exposition format on HTTP port 9471 (configurable).

No external dependencies — uses only Python stdlib.

Usage:
    ./atomik-exporter.py [--port PORT]
    ./atomik-exporter.py --help
"""

import argparse
import json
import logging
import os
import re
import signal
import sys
import threading
import time
from http.server import HTTPServer, BaseHTTPRequestHandler

PROC_ATOMIK = "/proc/atomik"
PROC_ATOMIK_COW = "/proc/atomik/cow"
PROC_ATOMIK_NET = "/proc/atomik/net"
PROC_ATOMIK_SUMMARY = "/proc/atomik/summary"
SYSFS_BASE = "/sys/class/atomik/atomik0/"

DEFAULT_PORT = 9471

log = logging.getLogger("atomik-exporter")


# -- Health checks ---------------------------------------------------------

def check_health():
    """Return (ok: bool, reason: str) based on /proc/atomik readability."""
    if not os.path.exists(PROC_ATOMIK):
        return False, "/proc/atomik does not exist — is the module loaded?"
    content = read_file(PROC_ATOMIK)
    if content is None:
        return False, "/proc/atomik exists but is unreadable"
    return True, ""


# -- File reading helpers --------------------------------------------------

def read_file(path):
    """Read entire file contents, return None on failure."""
    try:
        with open(path, "r") as f:
            return f.read()
    except (OSError, IOError):
        return None


def read_sysfs(attr):
    """Read a sysfs attribute, stripping trailing newline."""
    content = read_file(os.path.join(SYSFS_BASE, attr))
    if content is not None:
        content = content.strip()
    return content


def parse_kv(text):
    """Parse key=value lines into a dict."""
    result = {}
    if text is None:
        return result
    for line in text.strip().splitlines():
        if "=" in line:
            key, _, val = line.partition("=")
            result[key.strip()] = val.strip()
    return result


def kv_u64(kv, key, default=0):
    """Extract an unsigned 64-bit integer from a kv dict."""
    try:
        return int(kv.get(key, default))
    except (ValueError, TypeError):
        return default


def parse_redundancy_rate(rate_str):
    """Parse '42.3%' into a float 42.3, return 0.0 on failure."""
    if not rate_str:
        return 0.0
    m = re.match(r"([\d.]+)", rate_str.replace("%", ""))
    if m:
        return float(m.group(1))
    return 0.0


# -- Metric collection ----------------------------------------------------

def collect_metrics():
    """Read all procfs/sysfs sources and return Prometheus exposition text."""

    lines = []

    # Read /proc/atomik (main key=value file)
    proc_all = parse_kv(read_file(PROC_ATOMIK))

    # Read /proc/atomik/cow
    proc_cow = parse_kv(read_file(PROC_ATOMIK_COW))

    # Read /proc/atomik/net
    proc_net = parse_kv(read_file(PROC_ATOMIK_NET))

    # -- Info metric (gauge with labels) --
    version = proc_all.get("version", "unknown")
    backend = proc_all.get("backend", "unknown")
    license_val = proc_all.get("license", "unknown")

    lines.append("# HELP atomik_info ATOMiK module information.")
    lines.append("# TYPE atomik_info gauge")
    lines.append(
        'atomik_info{{version="{v}",backend="{b}",license="{l}"}} 1'.format(
            v=version, b=backend, l=license_val
        )
    )

    # -- Core operations (counters) --
    ops_load = kv_u64(proc_all, "ops_load")
    ops_accum = kv_u64(proc_all, "ops_accum")
    ops_read = kv_u64(proc_all, "ops_read")
    ops_swap = kv_u64(proc_all, "ops_swap")
    ops_total = kv_u64(proc_all, "ops_total",
                       ops_load + ops_accum + ops_read + ops_swap)

    lines.append("")
    lines.append("# HELP atomik_ops_total Total ATOMiK operations by type.")
    lines.append("# TYPE atomik_ops_total counter")
    lines.append('atomik_ops_total{{op="load"}} {}'.format(ops_load))
    lines.append('atomik_ops_total{{op="accum"}} {}'.format(ops_accum))
    lines.append('atomik_ops_total{{op="read"}} {}'.format(ops_read))
    lines.append('atomik_ops_total{{op="swap"}} {}'.format(ops_swap))

    lines.append("")
    lines.append("# HELP atomik_ops_load_total Total LOAD operations.")
    lines.append("# TYPE atomik_ops_load_total counter")
    lines.append("atomik_ops_load_total {}".format(ops_load))

    lines.append("")
    lines.append("# HELP atomik_ops_accum_total Total ACCUM operations.")
    lines.append("# TYPE atomik_ops_accum_total counter")
    lines.append("atomik_ops_accum_total {}".format(ops_accum))

    lines.append("")
    lines.append("# HELP atomik_ops_read_total Total READ operations.")
    lines.append("# TYPE atomik_ops_read_total counter")
    lines.append("atomik_ops_read_total {}".format(ops_read))

    lines.append("")
    lines.append("# HELP atomik_ops_swap_total Total SWAP operations.")
    lines.append("# TYPE atomik_ops_swap_total counter")
    lines.append("atomik_ops_swap_total {}".format(ops_swap))

    # -- Fingerprinting --
    fp_checks = kv_u64(proc_all, "fp_checks")
    fp_unchanged = kv_u64(proc_all, "fp_unchanged")

    lines.append("")
    lines.append(
        "# HELP atomik_fp_checks_total "
        "Total fingerprint checks performed."
    )
    lines.append("# TYPE atomik_fp_checks_total counter")
    lines.append("atomik_fp_checks_total {}".format(fp_checks))

    lines.append("")
    lines.append(
        "# HELP atomik_fp_unchanged_total "
        "Fingerprint checks that found no change."
    )
    lines.append("# TYPE atomik_fp_unchanged_total counter")
    lines.append("atomik_fp_unchanged_total {}".format(fp_unchanged))

    # -- COW metrics --
    cow_faults = kv_u64(proc_all, "cow_faults")
    cow_redundant = kv_u64(proc_all, "cow_redundant")
    cow_bytes_copied = kv_u64(proc_all, "cow_bytes_copied")
    cow_bytes_saved = kv_u64(proc_all, "cow_bytes_saved")
    cow_rate = parse_redundancy_rate(proc_cow.get("redundancy_rate", "0"))

    lines.append("")
    lines.append("# HELP atomik_cow_faults_total Total COW faults observed.")
    lines.append("# TYPE atomik_cow_faults_total counter")
    lines.append("atomik_cow_faults_total {}".format(cow_faults))

    lines.append("")
    lines.append(
        "# HELP atomik_cow_redundant_total "
        "COW copies that were redundant (no actual change)."
    )
    lines.append("# TYPE atomik_cow_redundant_total counter")
    lines.append("atomik_cow_redundant_total {}".format(cow_redundant))

    lines.append("")
    lines.append(
        "# HELP atomik_cow_bytes_copied_total "
        "Total bytes copied during COW faults."
    )
    lines.append("# TYPE atomik_cow_bytes_copied_total counter")
    lines.append("atomik_cow_bytes_copied_total {}".format(cow_bytes_copied))

    lines.append("")
    lines.append(
        "# HELP atomik_cow_bytes_saved_total "
        "Bytes saved by eliminating redundant COW copies."
    )
    lines.append("# TYPE atomik_cow_bytes_saved_total counter")
    lines.append("atomik_cow_bytes_saved_total {}".format(cow_bytes_saved))

    lines.append("")
    lines.append(
        "# HELP atomik_cow_redundancy_rate "
        "Percentage of COW copies that were redundant (0-100)."
    )
    lines.append("# TYPE atomik_cow_redundancy_rate gauge")
    lines.append("atomik_cow_redundancy_rate {}".format(cow_rate))

    # -- Network metrics --
    net_sends = kv_u64(proc_all, "net_sends")
    net_redundant = kv_u64(proc_all, "net_redundant")
    net_bytes = kv_u64(proc_all, "net_bytes")
    net_bytes_redundant = kv_u64(proc_all, "net_bytes_redundant")
    net_rate = parse_redundancy_rate(proc_net.get("redundancy_rate", "0"))

    lines.append("")
    lines.append(
        "# HELP atomik_net_sends_total Total network sends tracked."
    )
    lines.append("# TYPE atomik_net_sends_total counter")
    lines.append("atomik_net_sends_total {}".format(net_sends))

    lines.append("")
    lines.append(
        "# HELP atomik_net_redundant_total "
        "Network sends that were redundant."
    )
    lines.append("# TYPE atomik_net_redundant_total counter")
    lines.append("atomik_net_redundant_total {}".format(net_redundant))

    lines.append("")
    lines.append(
        "# HELP atomik_net_bytes_total Total network bytes tracked."
    )
    lines.append("# TYPE atomik_net_bytes_total counter")
    lines.append("atomik_net_bytes_total {}".format(net_bytes))

    lines.append("")
    lines.append(
        "# HELP atomik_net_bytes_redundant_total "
        "Network bytes that were redundant."
    )
    lines.append("# TYPE atomik_net_bytes_redundant_total counter")
    lines.append(
        "atomik_net_bytes_redundant_total {}".format(net_bytes_redundant)
    )

    lines.append("")
    lines.append(
        "# HELP atomik_net_redundancy_rate "
        "Percentage of network sends that were redundant (0-100)."
    )
    lines.append("# TYPE atomik_net_redundancy_rate gauge")
    lines.append("atomik_net_redundancy_rate {}".format(net_rate))

    lines.append("")
    return "\n".join(lines) + "\n"


# -- HTTP handler ----------------------------------------------------------

class MetricsHandler(BaseHTTPRequestHandler):
    """Serve Prometheus metrics, health, and readiness endpoints."""

    def _send_json(self, status, obj):
        body = json.dumps(obj).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _handle_health(self):
        ok, reason = check_health()
        if ok:
            self._send_json(200, {"status": "ok"})
        else:
            log.warning("health check failed: %s", reason)
            self._send_json(503, {"status": "error", "reason": reason})

    def do_GET(self):
        if self.path == "/metrics":
            try:
                body = collect_metrics()
                self.send_response(200)
                self.send_header(
                    "Content-Type",
                    "text/plain; version=0.0.4; charset=utf-8",
                )
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body.encode("utf-8"))
            except Exception as e:
                log.error("error collecting metrics: %s", e)
                msg = "Error collecting metrics: {}\n".format(e)
                self.send_response(500)
                self.send_header("Content-Type", "text/plain")
                self.send_header("Content-Length", str(len(msg)))
                self.end_headers()
                self.wfile.write(msg.encode("utf-8"))
        elif self.path == "/healthz":
            self._handle_health()
        elif self.path == "/ready":
            self._handle_health()
        elif self.path == "/":
            body = (
                "atomik-exporter\n"
                "Metrics available at /metrics\n"
            )
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body.encode("utf-8"))
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, format, *args):
        """Route HTTP access logs through the logging module."""
        log.debug(format, *args)


# -- Main ------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Prometheus exporter for ATOMiK kernel module metrics",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help="HTTP port to listen on (default: {})".format(DEFAULT_PORT),
    )
    parser.add_argument(
        "--log-level",
        choices=["debug", "info", "warning", "error"],
        default="info",
        help="Logging verbosity (default: info)",
    )
    args = parser.parse_args()

    # Configure logging
    level = getattr(logging, args.log_level.upper())
    logging.basicConfig(
        level=level,
        format="%(asctime)s %(name)s [%(levelname)s] %(message)s",
        datefmt="%Y-%m-%dT%H:%M:%S",
    )

    # Verify /proc/atomik is accessible before starting
    if not os.path.exists(PROC_ATOMIK):
        log.error(
            "%s not found. Is the atomik kernel module loaded?",
            PROC_ATOMIK,
        )
        sys.exit(1)

    server = HTTPServer(("", args.port), MetricsHandler)

    # SIGTERM handler for graceful shutdown (e.g. K8s pod termination)
    shutdown_event = threading.Event()

    def _sigterm_handler(signum, frame):
        log.info("received SIGTERM — shutting down gracefully")
        shutdown_event.set()
        # shutdown() must be called from a separate thread to avoid deadlock
        threading.Thread(target=server.shutdown, daemon=True).start()

    signal.signal(signal.SIGTERM, _sigterm_handler)

    log.info("atomik-exporter: serving Prometheus metrics")
    log.info("  Port:     %d", args.port)
    log.info("  Endpoint: http://0.0.0.0:%d/metrics", args.port)
    log.info("  Health:   http://0.0.0.0:%d/healthz", args.port)
    log.info("  Ready:    http://0.0.0.0:%d/ready", args.port)
    log.info("  Source:   %s", PROC_ATOMIK)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        log.info("received SIGINT — shutting down")
    finally:
        server.server_close()
        log.info("server stopped")


if __name__ == "__main__":
    main()
