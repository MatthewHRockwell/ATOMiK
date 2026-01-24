"""
Unit tests for the DeltaStream module.
"""

import numpy as np
import pytest
from atomik_sdk.delta_stream import DeltaStream, Delta, MotifType


class TestDelta:
    """Tests for the Delta dataclass."""
    
    def test_delta_creation(self):
        """Test creating a Delta object."""
        delta = Delta(
            frame_index=0,
            voxel_index=(0, 0),
            delta_word=0xFF00FF00,
            motif=MotifType.HORIZONTAL_MOTION,
            magnitude=0.5,
            timestamp_ms=0.0
        )
        assert delta.frame_index == 0
        assert delta.delta_word == 0xFF00FF00
        assert delta.motif == MotifType.HORIZONTAL_MOTION
    
    def test_delta_is_event_true(self):
        """Test is_event property when delta is significant."""
        delta = Delta(
            frame_index=0,
            voxel_index=(0, 0),
            delta_word=0xFFFFFFFF,
            motif=MotifType.EXPANSION,
            magnitude=0.5,
            timestamp_ms=0.0
        )
        assert delta.is_event is True
    
    def test_delta_is_event_false_zero_word(self):
        """Test is_event property when delta_word is zero."""
        delta = Delta(
            frame_index=0,
            voxel_index=(0, 0),
            delta_word=0,
            motif=MotifType.STATIC,
            magnitude=0.0,
            timestamp_ms=0.0
        )
        assert delta.is_event is False
    
    def test_delta_to_bytes(self):
        """Test serialization to bytes."""
        delta = Delta(
            frame_index=0,
            voxel_index=(0, 0),
            delta_word=0x0102030405060708,
            motif=MotifType.STATIC,
            magnitude=0.0,
            timestamp_ms=0.0
        )
        result = delta.to_bytes()
        assert len(result) == 8


class TestDeltaStream:
    """Tests for the DeltaStream class."""
    
    @pytest.fixture
    def sample_frames(self):
        """Create sample frame data for testing."""
        frames = np.zeros((8, 8, 8), dtype=np.uint8)
        frames[0:4, :, :] = 100
        frames[4:8, :, :] = 200
        return frames
    
    def test_stream_creation(self, sample_frames):
        """Test creating a DeltaStream from frames."""
        stream = DeltaStream(sample_frames, voxel_size=(4, 4, 4))
        assert stream.frame_count == 8
        assert stream.voxel_size == (4, 4, 4)
    
    def test_stream_invalid_shape(self):
        """Test that invalid frame shapes raise ValueError."""
        frames = np.zeros((8,), dtype=np.uint8)
        with pytest.raises(ValueError):
            DeltaStream(frames)
    
    def test_stream_iteration(self, sample_frames):
        """Test iterating over the stream."""
        stream = DeltaStream(sample_frames, voxel_size=(4, 4, 4))
        deltas = list(stream)
        assert len(deltas) >= 1
        assert all(isinstance(d, Delta) for d in deltas)


class TestMotifClassification:
    """Tests for motif classification logic."""
    
    def test_classify_static(self):
        """Test classification of static pattern."""
        stream = DeltaStream.__new__(DeltaStream)
        motif = stream._classify_motif(0)
        assert motif == MotifType.STATIC
    
    def test_classify_noise(self):
        """Test classification of noise pattern."""
        stream = DeltaStream.__new__(DeltaStream)
        motif = stream._classify_motif(0x0000000000000003)
        assert motif == MotifType.NOISE
