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

from .bitstream_gen import BitstreamGenerator
from .delta_stream import Delta, DeltaStream
from .genome_compiler import GenomeCompiler
from .motifs import Motif
from .pattern_matcher import PatternMatcher
from .voxel_encoder import VoxelEncoder

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
