"""
ATOMiK SDK - Stateless Delta-Driven Computational Architecture

A Python SDK for working with ATOMiK's delta-based video processing pipeline.

Three API Levels:
    Level 1 (High-Level): Application developers - DeltaStream, Motif
    Level 2 (Mid-Level): Algorithm designers - VoxelEncoder, PatternMatcher
    Level 3 (Low-Level): Hardware engineers - BitstreamGenerator, GenomeCompiler

Example:
    >>> from atomik_sdk import DeltaStream, Motif
    >>> stream = DeltaStream.from_video("input.mp4", voxel=(4,4,4))
    >>> for delta in stream:
    ...     if delta.motif == Motif.HORIZONTAL_MOTION:
    ...         print(f"Motion at frame {delta.frame_index}")

"""

__version__ = "0.1.0"
__author__ = "ATOMiK Project"

# Level 1: High-Level API (Application Developers)
from .delta_stream import DeltaStream, Delta
from .motifs import Motif

# Level 2: Mid-Level API (Algorithm Designers)
from .voxel_encoder import VoxelEncoder
from .pattern_matcher import PatternMatcher

# Level 3: Low-Level API (Hardware Engineers)
from .genome_compiler import GenomeCompiler
from .bitstream_gen import BitstreamGenerator

__all__ = [
    # Level 1
    "DeltaStream",
    "Delta",
    "Motif",
    # Level 2
    "VoxelEncoder",
    "PatternMatcher",
    # Level 3
    "GenomeCompiler",
    "BitstreamGenerator",
]
