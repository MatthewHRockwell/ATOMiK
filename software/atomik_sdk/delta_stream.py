"""
Delta Stream Module - High-level API for video-to-delta processing.

This module provides the primary interface for processing video files
as streams of delta objects, enabling efficient motion detection and
pattern recognition using ATOMiK's stateless architecture.

Example:
    >>> stream = DeltaStream.from_video("input.mp4", voxel=(4,4,4))
    >>> for delta in stream:
    ...     print(f"Frame {delta.frame_index}: motif={delta.motif}")

"""

from __future__ import annotations

from collections.abc import Iterator
from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path

import numpy as np


class MotifType(Enum):
    """Classification of delta patterns into semantic motifs."""

    STATIC = auto()
    HORIZONTAL_MOTION = auto()
    VERTICAL_MOTION = auto()
    DIAGONAL_MOTION = auto()
    EXPANSION = auto()
    CONTRACTION = auto()
    ROTATION = auto()
    NOISE = auto()


@dataclass
class Delta:
    """
    Represents a single delta computation result.

    Attributes:
        frame_index: The frame number in the video sequence.
        voxel_index: The spatial position (x, y) of this voxel.
        delta_word: The 64-bit XOR result between consecutive voxels.
        motif: The classified motion pattern.
        magnitude: The strength of the detected change (0.0 to 1.0).
        timestamp_ms: The timestamp in milliseconds.
    """

    frame_index: int
    voxel_index: tuple[int, int]
    delta_word: int
    motif: MotifType
    magnitude: float
    timestamp_ms: float

    @property
    def is_event(self) -> bool:
        """Returns True if this delta represents a significant event."""
        return self.delta_word != 0 and self.magnitude > 0.1

    def to_bytes(self) -> bytes:
        """Serialize the delta to bytes for hardware transmission."""
        return self.delta_word.to_bytes(8, byteorder="little")


class DeltaStream:
    """
    Iterator over video frames producing Delta objects.

    The DeltaStream processes video input using ATOMiK's 4x4x4 voxel
    encoding scheme, computing XOR-based deltas between consecutive
    temporal windows and classifying the resulting patterns.

    Attributes:
        voxel_size: The (temporal, height, width) dimensions of each voxel.
        tile_method: The binarization method ('otsu', 'mean', 'adaptive').
        frame_count: Total number of frames in the video.
        fps: Frames per second of the source video.

    Example:
        >>> stream = DeltaStream.from_video("input.mp4")
        >>> motions = [d for d in stream if d.motif == MotifType.HORIZONTAL_MOTION]
        >>> print(f"Found {len(motions)} horizontal motion events")
    """

    def __init__(
        self,
        frames: np.ndarray,
        voxel_size: tuple[int, int, int] = (4, 4, 4),
        tile_method: str = "otsu",
        fps: float = 30.0,
    ):
        """
        Initialize a DeltaStream from frame data.

        Args:
            frames: NumPy array of shape (T, H, W) or (T, H, W, C).
            voxel_size: Temporal and spatial dimensions for voxel encoding.
            tile_method: Binarization method ('otsu', 'mean', 'adaptive').
            fps: Frames per second for timestamp calculation.

        Raises:
            ValueError: If frames array has invalid shape.
        """
        if frames.ndim == 4:
            # Convert to grayscale if color
            frames = np.mean(frames, axis=-1).astype(np.uint8)

        if frames.ndim != 3:
            raise ValueError(f"Expected 3D array (T, H, W), got shape {frames.shape}")

        self._frames = frames
        self.voxel_size = voxel_size
        self.tile_method = tile_method
        self.fps = fps
        self.frame_count = frames.shape[0]

        self._current_index = 0
        self._prev_voxels: np.ndarray | None = None

    @classmethod
    def from_video(
        cls,
        path: str | Path,
        voxel: tuple[int, int, int] = (4, 4, 4),
        tile_method: str = "otsu",
    ) -> DeltaStream:
        """
        Create a DeltaStream from a video file.

        Args:
            path: Path to the video file.
            voxel: Voxel dimensions (temporal, height, width).
            tile_method: Binarization method.

        Returns:
            A configured DeltaStream instance.

        Raises:
            FileNotFoundError: If the video file doesn't exist.
            ImportError: If cv2 is not available.

        Example:
            >>> stream = DeltaStream.from_video("surveillance.mp4", voxel=(4,4,4))
        """
        try:
            import cv2
        except ImportError:
            raise ImportError("OpenCV (cv2) is required for video loading")

        path = Path(path)
        if not path.exists():
            raise FileNotFoundError(f"Video file not found: {path}")

        cap = cv2.VideoCapture(str(path))
        fps = cap.get(cv2.CAP_PROP_FPS)

        frames = []
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            frames.append(gray)

        cap.release()

        return cls(np.array(frames), voxel_size=voxel, tile_method=tile_method, fps=fps)

    @classmethod
    def from_numpy(
        cls,
        frames: np.ndarray,
        voxel: tuple[int, int, int] = (4, 4, 4),
        fps: float = 30.0,
    ) -> DeltaStream:
        """
        Create a DeltaStream from a NumPy array.

        Args:
            frames: Array of shape (T, H, W) containing grayscale frames.
            voxel: Voxel dimensions (temporal, height, width).
            fps: Frames per second for timestamp calculation.

        Returns:
            A configured DeltaStream instance.
        """
        return cls(frames, voxel_size=voxel, fps=fps)

    def __iter__(self) -> Iterator[Delta]:
        """Reset iterator and return self."""
        self._current_index = 0
        self._prev_voxels = None
        return self

    def __next__(self) -> Delta:
        """
        Compute and return the next Delta.

        Returns:
            The next Delta object in the stream.

        Raises:
            StopIteration: When all frames have been processed.
        """
        t, h, w = self.voxel_size

        if self._current_index + t > self.frame_count:
            raise StopIteration

        # Extract current voxel window
        window = self._frames[self._current_index : self._current_index + t]

        # Binarize and encode
        curr_voxels = self._encode_voxels(window)

        if self._prev_voxels is None:
            self._prev_voxels = curr_voxels
            self._current_index += t
            # Return initial frame with zero delta
            return Delta(
                frame_index=self._current_index - t,
                voxel_index=(0, 0),
                delta_word=0,
                motif=MotifType.STATIC,
                magnitude=0.0,
                timestamp_ms=(self._current_index - t) / self.fps * 1000,
            )

        # Compute XOR delta
        delta_word = int(curr_voxels[0, 0] ^ self._prev_voxels[0, 0])

        # Classify motif
        motif = self._classify_motif(delta_word)
        magnitude = bin(delta_word).count("1") / 64.0

        # Create Delta object
        delta = Delta(
            frame_index=self._current_index,
            voxel_index=(0, 0),
            delta_word=delta_word,
            motif=motif,
            magnitude=magnitude,
            timestamp_ms=self._current_index / self.fps * 1000,
        )

        self._prev_voxels = curr_voxels
        self._current_index += t

        return delta

    def __len__(self) -> int:
        """Return the number of delta computations available."""
        return max(0, (self.frame_count - self.voxel_size[0]) // self.voxel_size[0])

    def _encode_voxels(self, window: np.ndarray) -> np.ndarray:
        """
        Encode a temporal window into 64-bit voxel words.

        Args:
            window: Array of shape (T, H, W) containing frames.

        Returns:
            Array of 64-bit encoded voxel words.
        """
        # Simplified encoding: binarize and pack into 64-bit words
        threshold = np.mean(window)
        binary = (window > threshold).astype(np.uint8)

        # Pack into 64-bit word (simplified)
        packed = np.packbits(binary.flatten()[:64])
        word = int.from_bytes(packed, byteorder="little")

        return np.array([[word]], dtype=np.uint64)

    def _classify_motif(self, delta_word: int) -> MotifType:
        """
        Classify a delta word into a semantic motif.

        Args:
            delta_word: The 64-bit XOR delta.

        Returns:
            The classified MotifType.
        """
        if delta_word == 0:
            return MotifType.STATIC

        bit_count = bin(delta_word).count("1")

        # Simple heuristic classification
        if bit_count < 4:
            return MotifType.NOISE
        elif bit_count < 16:
            # Check pattern for direction
            upper_half = (delta_word >> 32) & 0xFFFFFFFF
            lower_half = delta_word & 0xFFFFFFFF

            if upper_half > lower_half:
                return MotifType.VERTICAL_MOTION
            else:
                return MotifType.HORIZONTAL_MOTION
        elif bit_count < 32:
            return MotifType.DIAGONAL_MOTION
        else:
            return MotifType.EXPANSION

    def filter_motif(self, motif: MotifType) -> Iterator[Delta]:
        """
        Iterate over deltas matching a specific motif.

        Args:
            motif: The motif type to filter for.

        Yields:
            Delta objects matching the specified motif.

        Example:
            >>> for d in stream.filter_motif(MotifType.HORIZONTAL_MOTION):
            ...     print(f"Motion at frame {d.frame_index}")
        """
        for delta in self:
            if delta.motif == motif:
                yield delta

    def events_only(self) -> Iterator[Delta]:
        """
        Iterate over only significant event deltas.

        Yields:
            Delta objects where is_event is True.
        """
        for delta in self:
            if delta.is_event:
                yield delta
