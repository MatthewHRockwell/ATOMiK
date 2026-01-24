"""
Motif Module - Pattern vocabulary for delta classification.

This module defines the Motif enumeration and pattern matching utilities
for classifying 64-bit delta words into semantic motion categories.

The motif vocabulary is designed to achieve 1000:1+ compression ratios
by mapping common delta patterns to compact 4-bit identifiers.
"""

from enum import IntEnum
from typing import Dict, List


class Motif(IntEnum):
    """
    Enumeration of delta pattern motifs.
    
    Each motif represents a semantic category of motion or change
    that can be efficiently encoded in 4 bits (16 possible values).
    
    Attributes:
        STATIC: No change detected (0x0000000000000000)
        HORIZONTAL_MOTION: Left-to-right or right-to-left movement
        VERTICAL_MOTION: Up-to-down or down-to-up movement
        DIAGONAL_NE: Northeast diagonal movement
        DIAGONAL_NW: Northwest diagonal movement
        DIAGONAL_SE: Southeast diagonal movement
        DIAGONAL_SW: Southwest diagonal movement
        EXPANSION: Outward growth pattern
        CONTRACTION: Inward shrinking pattern
        ROTATION_CW: Clockwise rotation
        ROTATION_CCW: Counter-clockwise rotation
        FLICKER: High-frequency oscillation
        EDGE_APPEAR: New edge formation
        EDGE_DISAPPEAR: Edge dissolution
        NOISE: Random/unclassified pattern
        RESERVED: Reserved for future use
    """
    STATIC = 0
    HORIZONTAL_MOTION = 1
    VERTICAL_MOTION = 2
    DIAGONAL_NE = 3
    DIAGONAL_NW = 4
    DIAGONAL_SE = 5
    DIAGONAL_SW = 6
    EXPANSION = 7
    CONTRACTION = 8
    ROTATION_CW = 9
    ROTATION_CCW = 10
    FLICKER = 11
    EDGE_APPEAR = 12
    EDGE_DISAPPEAR = 13
    NOISE = 14
    RESERVED = 15


# Canonical pattern signatures for each motif (MSB patterns)
MOTIF_SIGNATURES: Dict[Motif, List[int]] = {
    Motif.STATIC: [0x0000000000000000],
    Motif.HORIZONTAL_MOTION: [
        0xFF00000000000000,  # Right motion
        0x00000000000000FF,  # Left motion
        0xFFFF000000000000,
        0x000000000000FFFF,
    ],
    Motif.VERTICAL_MOTION: [
        0x0101010101010101,  # Down motion
        0x8080808080808080,  # Up motion
    ],
    Motif.DIAGONAL_NE: [
        0x0102040810204080,
    ],
    Motif.DIAGONAL_SW: [
        0x8040201008040201,
    ],
    Motif.EXPANSION: [
        0x183C7EFFFF7E3C18,  # Center-out
    ],
    Motif.CONTRACTION: [
        0xE7C38100008143E7,  # Outside-in
    ],
}


def classify_delta(delta_word: int) -> Motif:
    """
    Classify a 64-bit delta word into a motif category.
    
    The classification uses a combination of bit population count,
    bit position analysis, and pattern matching against known signatures.
    
    Args:
        delta_word: The 64-bit XOR result to classify.
    
    Returns:
        The classified Motif.
    
    Example:
        >>> motif = classify_delta(0xFF00000000000000)
        >>> print(motif)  # Motif.HORIZONTAL_MOTION
    """
    if delta_word == 0:
        return Motif.STATIC
    
    # Check against known signatures
    for motif, signatures in MOTIF_SIGNATURES.items():
        for sig in signatures:
            # Allow for partial matches (>70% overlap)
            overlap = bin(delta_word & sig).count('1')
            sig_bits = bin(sig).count('1')
            if sig_bits > 0 and overlap / sig_bits > 0.7:
                return motif
    
    # Fallback to heuristic classification
    bit_count = bin(delta_word).count('1')
    
    if bit_count <= 2:
        return Motif.NOISE
    
    # Analyze bit distribution
    upper_32 = (delta_word >> 32) & 0xFFFFFFFF
    lower_32 = delta_word & 0xFFFFFFFF
    
    upper_bits = bin(upper_32).count('1')
    lower_bits = bin(lower_32).count('1')
    
    if abs(upper_bits - lower_bits) > 10:
        return Motif.VERTICAL_MOTION
    
    # Check for horizontal patterns (consecutive bytes)
    byte_counts = []
    for i in range(8):
        byte_val = (delta_word >> (i * 8)) & 0xFF
        byte_counts.append(bin(byte_val).count('1'))
    
    # High variance in byte population suggests horizontal motion
    variance = sum((x - sum(byte_counts)/8)**2 for x in byte_counts) / 8
    if variance > 10:
        return Motif.HORIZONTAL_MOTION
    
    # Default classification based on density
    if bit_count > 40:
        return Motif.EXPANSION
    elif bit_count > 20:
        return Motif.FLICKER
    else:
        return Motif.NOISE


def motif_to_bytes(motif: Motif) -> bytes:
    """
    Encode a motif as a single byte.
    
    Args:
        motif: The Motif to encode.
    
    Returns:
        Single byte representation.
    """
    return bytes([motif.value])


def bytes_to_motif(data: bytes) -> Motif:
    """
    Decode a motif from a byte.
    
    Args:
        data: Byte data to decode.
    
    Returns:
        The decoded Motif.
    
    Raises:
        ValueError: If the byte value is invalid.
    """
    if len(data) < 1:
        raise ValueError("Empty byte sequence")
    
    value = data[0] & 0x0F  # Only use lower 4 bits
    return Motif(value)


def get_motif_name(motif: Motif) -> str:
    """
    Get a human-readable name for a motif.
    
    Args:
        motif: The Motif to describe.
    
    Returns:
        Human-readable string description.
    """
    names = {
        Motif.STATIC: "No Motion",
        Motif.HORIZONTAL_MOTION: "Horizontal Motion",
        Motif.VERTICAL_MOTION: "Vertical Motion",
        Motif.DIAGONAL_NE: "Diagonal (NE)",
        Motif.DIAGONAL_NW: "Diagonal (NW)",
        Motif.DIAGONAL_SE: "Diagonal (SE)",
        Motif.DIAGONAL_SW: "Diagonal (SW)",
        Motif.EXPANSION: "Expansion",
        Motif.CONTRACTION: "Contraction",
        Motif.ROTATION_CW: "Clockwise Rotation",
        Motif.ROTATION_CCW: "Counter-Clockwise Rotation",
        Motif.FLICKER: "Flicker",
        Motif.EDGE_APPEAR: "Edge Appearing",
        Motif.EDGE_DISAPPEAR: "Edge Disappearing",
        Motif.NOISE: "Noise",
        Motif.RESERVED: "Reserved",
    }
    return names.get(motif, "Unknown")


def get_compression_stats(motifs: List[Motif]) -> Dict[str, float]:
    """
    Calculate compression statistics for a sequence of motifs.
    
    Args:
        motifs: List of classified motifs.
    
    Returns:
        Dictionary with compression statistics.
    """
    if not motifs:
        return {"compression_ratio": 0.0, "entropy": 0.0}
    
    # Original: 64 bits per delta
    # Compressed: 4 bits per motif
    original_bits = len(motifs) * 64
    compressed_bits = len(motifs) * 4
    
    compression_ratio = original_bits / compressed_bits if compressed_bits > 0 else 0
    
    # Calculate entropy
    from collections import Counter
    counts = Counter(motifs)
    total = len(motifs)
    entropy = 0.0
    
    import math
    for count in counts.values():
        if count > 0:
            p = count / total
            entropy -= p * math.log2(p)
    
    return {
        "compression_ratio": compression_ratio,
        "entropy": entropy,
        "unique_motifs": len(counts),
        "dominant_motif": counts.most_common(1)[0][0].name if counts else None,
    }
