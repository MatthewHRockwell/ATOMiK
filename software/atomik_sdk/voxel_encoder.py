"""
Voxel Encoder Module - Mid-level API for spatiotemporal encoding.

This module provides the VoxelEncoder class for converting video frames
into ATOMiK's 4x4x4 voxel representation with configurable binarization.
"""

from __future__ import annotations

from collections.abc import Iterator
from dataclasses import dataclass
from enum import Enum, auto

import numpy as np


class TileMethod(Enum):
    """Binarization methods for tile processing."""

    OTSU = auto()
    MEAN = auto()
    ADAPTIVE = auto()
    FIXED = auto()


@dataclass
class Voxel:
    """
    A 4x4x4 spatiotemporal voxel encoded as a 64-bit word.

    Attributes:
        word: The 64-bit encoded voxel data.
        position: The (t, y, x) position in the video grid.
        tile_stats: Statistics from the binarization process.
    """

    word: int
    position: tuple[int, int, int]
    tile_stats: dict | None = None

    def __int__(self) -> int:
        return self.word

    def to_bytes(self) -> bytes:
        return self.word.to_bytes(8, byteorder="little")

    @classmethod
    def from_bytes(
        cls, data: bytes, position: tuple[int, int, int] = (0, 0, 0)
    ) -> Voxel:
        word = int.from_bytes(data[:8], byteorder="little")
        return cls(word=word, position=position)


class VoxelEncoder:
    """
    Encoder for converting video frames to 64-bit voxel words.

    The VoxelEncoder implements ATOMiK's 4x4x4 spatiotemporal encoding:
    - 4 temporal frames
    - 4x4 spatial tile
    - 64 binary values packed into a 64-bit word

    Attributes:
        voxel_size: The (temporal, height, width) voxel dimensions.
        tile_method: The binarization method used.
        threshold: Fixed threshold value (only for FIXED method).

    Example:
        >>> encoder = VoxelEncoder(tile_method="otsu", voxel_size=(4,4,4))
        >>> voxels = encoder.encode_frames(frames)
        >>> for v in voxels:
        ...     print(f"Position {v.position}: {v.word:#018x}")
    """

    def __init__(
        self,
        tile_method: str | TileMethod = TileMethod.OTSU,
        voxel_size: tuple[int, int, int] = (4, 4, 4),
        threshold: float | None = None,
    ):
        """
        Initialize the VoxelEncoder.

        Args:
            tile_method: Binarization method ('otsu', 'mean', 'adaptive', 'fixed').
            voxel_size: Dimensions (temporal, height, width) for voxels.
            threshold: Fixed threshold value (required if tile_method='fixed').

        Raises:
            ValueError: If fixed method specified without threshold.
        """
        if isinstance(tile_method, str):
            tile_method = TileMethod[tile_method.upper()]

        self.tile_method = tile_method
        self.voxel_size = voxel_size
        self.threshold = threshold

        if self.tile_method == TileMethod.FIXED and threshold is None:
            raise ValueError("Fixed method requires threshold parameter")

        self._cache: dict = {}

    def encode_frames(self, frames: np.ndarray) -> list[Voxel]:
        """
        Encode a sequence of frames into voxels.

        Args:
            frames: NumPy array of shape (T, H, W) containing grayscale frames.

        Returns:
            List of Voxel objects covering the entire video.

        Raises:
            ValueError: If frame dimensions are incompatible with voxel size.
        """
        t, h, w = self.voxel_size
        frames_t, frames_h, frames_w = frames.shape

        if frames_t < t or frames_h < h or frames_w < w:
            raise ValueError(
                f"Frame dimensions {frames.shape} too small for voxel size {self.voxel_size}"
            )

        voxels = []

        for ti in range(0, frames_t - t + 1, t):
            for yi in range(0, frames_h - h + 1, h):
                for xi in range(0, frames_w - w + 1, w):
                    # Extract voxel region
                    region = frames[ti : ti + t, yi : yi + h, xi : xi + w]

                    # Binarize and encode
                    word, stats = self._encode_region(region)

                    voxel = Voxel(word=word, position=(ti, yi, xi), tile_stats=stats)
                    voxels.append(voxel)

        return voxels

    def encode_video(self, path: str) -> list[Voxel]:
        """
        Encode a video file into voxels.

        Args:
            path: Path to the video file.

        Returns:
            List of Voxel objects.
        """
        try:
            import cv2
        except ImportError:
            raise ImportError("OpenCV required for video encoding")

        cap = cv2.VideoCapture(path)
        frames = []

        while True:
            ret, frame = cap.read()
            if not ret:
                break
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            frames.append(gray)

        cap.release()

        return self.encode_frames(np.array(frames))

    def _encode_region(self, region: np.ndarray) -> tuple[int, dict]:
        """
        Encode a single voxel region into a 64-bit word.

        Args:
            region: Array of shape (t, h, w) containing pixel values.

        Returns:
            Tuple of (64-bit word, statistics dictionary).
        """
        # Compute threshold based on method
        if self.tile_method == TileMethod.FIXED:
            threshold = self.threshold
        elif self.tile_method == TileMethod.MEAN:
            threshold = np.mean(region)
        elif self.tile_method == TileMethod.OTSU:
            threshold = self._otsu_threshold(region)
        elif self.tile_method == TileMethod.ADAPTIVE:
            threshold = self._adaptive_threshold(region)
        else:
            threshold = np.mean(region)

        # Binarize
        binary = (region > threshold).astype(np.uint8)

        # Pack into 64-bit word
        flat = binary.flatten()[:64]  # Take first 64 bits

        # Pad if necessary
        if len(flat) < 64:
            flat = np.pad(flat, (0, 64 - len(flat)))

        word = 0
        for i, bit in enumerate(flat):
            if bit:
                word |= 1 << i

        stats = {
            "threshold": float(threshold),
            "mean": float(np.mean(region)),
            "std": float(np.std(region)),
            "ones_count": int(np.sum(binary)),
        }

        return word, stats

    def _otsu_threshold(self, region: np.ndarray) -> float:
        """Compute Otsu's threshold for the region."""
        hist, _ = np.histogram(region.flatten(), bins=256, range=(0, 256))
        total = region.size

        sum_total = np.sum(np.arange(256) * hist)
        sum_bg = 0
        weight_bg = 0

        max_variance = 0
        threshold = 0

        for t in range(256):
            weight_bg += hist[t]
            if weight_bg == 0:
                continue

            weight_fg = total - weight_bg
            if weight_fg == 0:
                break

            sum_bg += t * hist[t]

            mean_bg = sum_bg / weight_bg
            mean_fg = (sum_total - sum_bg) / weight_fg

            variance = weight_bg * weight_fg * (mean_bg - mean_fg) ** 2

            if variance > max_variance:
                max_variance = variance
                threshold = t

        return float(threshold)

    def _adaptive_threshold(self, region: np.ndarray) -> float:
        """Compute adaptive threshold based on local statistics."""
        mean = np.mean(region)
        std = np.std(region)
        return mean - 0.5 * std

    def deltas(self, voxels: list[Voxel]) -> Iterator[int]:
        """
        Compute deltas between consecutive voxels.

        Args:
            voxels: List of voxels in temporal order.

        Yields:
            64-bit XOR delta values.
        """
        prev = None
        for voxel in voxels:
            if prev is not None:
                yield voxel.word ^ prev.word
            prev = voxel
