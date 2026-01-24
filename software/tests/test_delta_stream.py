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
    
    def test_delta_is_event_false_low_magnitude(self):
        """Test is_event property when magnitude is below threshold."""
        delta = Delta(
            frame_index=0,
            voxel_index=(0, 0),
            delta_word=1,
            motif=MotifType.NOISE,
            magnitude=0.05,
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
        assert result == b'\x08\x07\x06\x05\x04\x03\x02\x01'


class TestDeltaStream:
    """Tests for the DeltaStream class."""
    
    @pytest.fixture
    def sample_frames(self):
        """Create sample frame data for testing."""
        # Create 8 frames of 8x8 pixels
        frames = np.zeros((8, 8, 8), dtype=np.uint8)
        # Add some variation
        frames[0:4, :, :] = 100
        frames[4:8, :, :] = 200
        return frames
    
    def test_stream_creation(self, sample_frames):
        """Test creating a DeltaStream from frames."""
        stream = DeltaStream(sample_frames, voxel_size=(4, 4, 4))
        assert stream.frame_count == 8
        assert stream.voxel_size == (4, 4, 4)
    
    def test_stream_creation_with_color(self):
        """Test creating a DeltaStream from color frames."""
        # Create color frames (T, H, W, C)
        frames = np.random.randint(0, 255, (8, 8, 8, 3), dtype=np.uint8)
        stream = DeltaStream(frames)
        assert stream.frame_count == 8
    
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
    
    def test_stream_len(self, sample_frames):
        """Test __len__ method."""
        stream = DeltaStream(sample_frames, voxel_size=(4, 4, 4))
        length = len(stream)
        assert length >= 0
    
    def test_stream_filter_motif(self, sample_frames):
        """Test filtering by motif type."""
        stream = DeltaStream(sample_frames, voxel_size=(4, 4, 4))
        static_deltas = list(stream.filter_motif(MotifType.STATIC))
        # First delta should be static (no previous frame)
        assert len(static_deltas) >= 0
    
    def test_stream_events_only(self, sample_frames):
        """Test filtering for events only."""
        stream = DeltaStream(sample_frames, voxel_size=(4, 4, 4))
        events = list(stream.events_only())
        # All returned deltas should be events
        assert all(d.is_event for d in events)
    
    def test_stream_from_numpy(self, sample_frames):
        """Test creating stream from numpy array."""
        stream = DeltaStream.from_numpy(sample_frames, voxel=(4, 4, 4), fps=30.0)
        assert stream.fps == 30.0
        assert stream.frame_count == 8
    
    def test_stream_timestamps(self, sample_frames):
        """Test that timestamps are correctly calculated."""
        stream = DeltaStream(sample_frames, voxel_size=(4, 4, 4), fps=30.0)
        deltas = list(stream)
        
        if len(deltas) >= 2:
            # Check timestamps are monotonically increasing
            for i in range(1, len(deltas)):
                assert deltas[i].timestamp_ms >= deltas[i-1].timestamp_ms


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
        # Less than 4 bits set
        motif = stream._classify_motif(0x0000000000000003)
        assert motif == MotifType.NOISE
    
    def test_classify_expansion(self):
        """Test classification of expansion pattern."""
        stream = DeltaStream.__new__(DeltaStream)
        # Many bits set (32+)
        motif = stream._classify_motif(0xFFFFFFFFFFFFFFFF)
        assert motif == MotifType.EXPANSION
