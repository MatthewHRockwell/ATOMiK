"""
Lookup tables and classification helpers for deterministic schema inference.

All inference is rule-based — zero LLM tokens consumed.
"""

from __future__ import annotations

# Vertical keyword mapping: vertical → keywords found in class/field names
VERTICAL_KEYWORDS: dict[str, list[str]] = {
    "Video": [
        "frame", "pixel", "motion", "h264", "h265", "codec", "yuv",
        "rgb", "video", "stream", "render", "display",
    ],
    "Finance": [
        "price", "tick", "trade", "order", "bid", "ask", "spread",
        "volume", "market", "portfolio", "position", "ledger",
    ],
    "Edge": [
        "sensor", "imu", "accel", "gyro", "temperature", "humidity",
        "pressure", "gps", "adc", "gpio", "embedded", "iot",
    ],
    "Audio": [
        "audio", "sample", "pcm", "frequency", "waveform", "channel",
        "mixer", "filter", "fft", "spectrum",
    ],
    "Network": [
        "packet", "socket", "tcp", "udp", "latency", "bandwidth",
        "throughput", "route", "flow", "connection",
    ],
    "Robotics": [
        "joint", "actuator", "servo", "motor", "pose", "trajectory",
        "odometry", "lidar", "slam", "waypoint",
    ],
    "Medical": [
        "ecg", "eeg", "heart", "pulse", "oxygen", "blood",
        "patient", "vital", "dosage", "imaging",
    ],
    "Compute": [
        "matrix", "tensor", "compute", "transform", "buffer",
        "accumulator", "register", "pipeline", "cache",
    ],
}

# Field names that should be excluded from delta_fields inference
DELTA_FIELD_EXCLUDES: set[str] = {
    "name", "id", "config", "logger", "log", "debug",
    "path", "filepath", "file_path", "filename",
    "version", "type", "kind", "mode", "state",
    "lock", "mutex", "semaphore",
    "callback", "handler", "listener",
    "parent", "children", "child", "root",
    "metadata", "meta", "info", "description",
    "created_at", "updated_at", "timestamp",
    "_internal", "__dict__",
}

# Valid bit widths for snapping
WIDTH_SNAP_TABLE: list[int] = [8, 16, 32, 64, 128, 256]

# Keywords indicating bitmask delta fields
BITMASK_KEYWORDS: list[str] = [
    "flag", "flags", "mask", "bitmask", "status", "bit", "bits",
    "enable", "enabled", "active", "control",
]

# Keywords indicating delta stream fields
STREAM_KEYWORDS: list[str] = [
    "stream", "frame", "buffer", "data", "payload",
    "sample", "samples", "block", "chunk", "packet",
    "raw", "bulk", "batch",
]

# Keywords in method names that indicate reconstruct capability
RECONSTRUCT_KEYWORDS: list[str] = [
    "reconstruct", "read", "get_state", "snapshot",
    "restore", "recover", "decode", "deserialize",
    "get_current", "current_state",
]

# Keywords in method names that indicate rollback capability
ROLLBACK_KEYWORDS: list[str] = [
    "rollback", "undo", "revert", "history",
    "checkpoint", "savepoint", "backup", "previous",
]


def classify_vertical(class_name: str, field_names: list[str]) -> str:
    """
    Classify the vertical category from class and field names.

    Uses keyword matching against known vertical domains.
    Falls back to "Compute" if no strong match is found.
    """
    # Build a single searchable string from all names
    tokens = class_name.lower().split("_")
    tokens.extend(class_name.lower().replace("_", " ").split())
    for name in field_names:
        tokens.extend(name.lower().split("_"))

    # Score each vertical by keyword matches
    best_vertical = "Compute"
    best_score = 0

    for vertical, keywords in VERTICAL_KEYWORDS.items():
        score = sum(1 for kw in keywords if kw in tokens)
        if score > best_score:
            best_score = score
            best_vertical = vertical

    return best_vertical


def classify_delta_type(field_name: str, type_name: str) -> str:
    """
    Classify a field as bitmask_delta, delta_stream, or parameter_delta.

    Rules:
    - Names containing bitmask keywords → bitmask_delta
    - Names containing stream keywords → delta_stream
    - Everything else → parameter_delta
    """
    name_lower = field_name.lower()
    type_lower = type_name.lower()

    for kw in BITMASK_KEYWORDS:
        if kw in name_lower or kw in type_lower:
            return "bitmask_delta"

    for kw in STREAM_KEYWORDS:
        if kw in name_lower or kw in type_lower:
            return "delta_stream"

    return "parameter_delta"


def snap_width(bit_width: int) -> int:
    """
    Snap a bit width to the nearest power-of-2 in WIDTH_SNAP_TABLE.

    - 0 or negative → 64 (default)
    - Values above 256 → 256 (max)
    """
    if bit_width <= 0:
        return 64

    # Find nearest value in snap table
    best = WIDTH_SNAP_TABLE[-1]
    best_dist = abs(bit_width - best)

    for w in WIDTH_SNAP_TABLE:
        dist = abs(bit_width - w)
        if dist < best_dist:
            best = w
            best_dist = dist

    return best


def is_delta_candidate(field_name: str) -> bool:
    """
    Check if a field name is a plausible delta field.

    Returns False for common non-state field patterns
    (config, logger, metadata, etc.).
    """
    name_lower = field_name.lower().strip("_")
    return name_lower not in DELTA_FIELD_EXCLUDES
