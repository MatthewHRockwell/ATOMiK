#!/usr/bin/env python3
"""
atomik-keygen.py — Generate ATOMiK license keys

This tool generates cryptographically signed license keys that are
verified offline by the ATOMiK kernel module using HMAC-SHA256.

Key format:
    ATOMIK-XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX  (32 hex chars)

    Bytes 0-3:   payload (version=0x01, tier, reserved[2])
    Bytes 4-19:  HMAC-SHA256(secret, payload) truncated to 128 bits

Usage:
    python3 atomik-keygen.py --tier professional
    python3 atomik-keygen.py --tier enterprise
    python3 atomik-keygen.py --tier professional --count 10
    python3 atomik-keygen.py --verify ATOMIK-01010000-A73C198E-42F5D16B-902DE854

IMPORTANT: This tool contains the HMAC secret key. It must NEVER be
distributed to customers. Keep it on the license generation server only.
"""

import argparse
import hashlib
import hmac
import os
import sys

# Default development secret: "ATOMiK_DEV_ONLY" (ASCII) + zero padding.
# For production, set ATOMIK_HMAC_SECRET env var to 64-char hex string.
_DEV_SECRET = bytes([
    0x41, 0x54, 0x4f, 0x4d, 0x69, 0x4b, 0x5f, 0x44,
    0x45, 0x56, 0x5f, 0x4f, 0x4e, 0x4c, 0x59, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
])

_env_secret = os.environ.get('ATOMIK_HMAC_SECRET')
if _env_secret:
    try:
        HMAC_SECRET = bytes.fromhex(_env_secret)
        if len(HMAC_SECRET) != 32:
            print("WARNING: ATOMIK_HMAC_SECRET must be exactly 32 bytes (64 hex chars), "
                  f"got {len(HMAC_SECRET)} bytes — falling back to dev key",
                  file=sys.stderr)
            HMAC_SECRET = _DEV_SECRET
    except ValueError:
        print("WARNING: ATOMIK_HMAC_SECRET is not valid hex — falling back to dev key",
              file=sys.stderr)
        HMAC_SECRET = _DEV_SECRET
else:
    print("WARNING: ATOMIK_HMAC_SECRET not set — using development key. "
          "Keys generated with this secret will NOT work in production builds.",
          file=sys.stderr)
    HMAC_SECRET = _DEV_SECRET

TIERS = {
    'professional': 0x01,
    'enterprise': 0x02,
}

VERSION = 0x01


def generate_key(tier: str) -> str:
    """Generate a signed ATOMiK license key."""
    tier_byte = TIERS[tier]

    # Build 4-byte payload: [version, tier, reserved, reserved]
    payload = bytes([VERSION, tier_byte, 0x00, 0x00])

    # Compute HMAC-SHA256 and truncate to 128 bits (16 bytes)
    mac = hmac.new(HMAC_SECRET, payload, hashlib.sha256).digest()[:16]

    # Combine payload + truncated HMAC
    key_bytes = payload + mac

    # Format as ATOMIK-XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX
    hex_str = key_bytes.hex().upper()
    formatted = f"ATOMIK-{hex_str[0:8]}-{hex_str[8:16]}-{hex_str[16:24]}-{hex_str[24:32]}-{hex_str[32:40]}"

    return formatted


def verify_key(key_str: str) -> bool:
    """Verify a license key's HMAC signature."""
    if not key_str.startswith("ATOMIK-") or len(key_str) != 51:
        print(f"ERROR: Invalid key format (expected 51 chars, got {len(key_str)})")
        return False

    # Strip prefix and dashes
    hex_chars = key_str[7:].replace("-", "")
    if len(hex_chars) != 40:
        print("ERROR: Invalid hex length")
        return False

    try:
        key_bytes = bytes.fromhex(hex_chars)
    except ValueError:
        print("ERROR: Invalid hex characters")
        return False

    payload = key_bytes[:4]
    provided_hmac = key_bytes[4:20]

    # Check version
    if payload[0] != VERSION:
        print(f"ERROR: Unknown key version 0x{payload[0]:02x}")
        return False

    # Check tier
    tier_name = {v: k for k, v in TIERS.items()}.get(payload[1])
    if tier_name is None:
        print(f"ERROR: Unknown tier 0x{payload[1]:02x}")
        return False

    # Verify HMAC
    expected_hmac = hmac.new(HMAC_SECRET, payload, hashlib.sha256).digest()[:16]

    if hmac.compare_digest(provided_hmac, expected_hmac):
        print(f"VALID: {tier_name} license")
        return True
    else:
        print("INVALID: HMAC signature mismatch — forged or corrupted key")
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Generate or verify ATOMiK license keys"
    )
    parser.add_argument(
        "--tier",
        choices=["professional", "enterprise"],
        help="License tier to generate"
    )
    parser.add_argument(
        "--count", type=int, default=1,
        help="Number of keys to generate (default: 1)"
    )
    parser.add_argument(
        "--verify",
        help="Verify a license key instead of generating"
    )

    args = parser.parse_args()

    if args.verify:
        valid = verify_key(args.verify)
        sys.exit(0 if valid else 1)

    if not args.tier:
        parser.error("--tier is required when generating keys")

    for _ in range(args.count):
        key = generate_key(args.tier)
        print(key)


if __name__ == "__main__":
    main()
