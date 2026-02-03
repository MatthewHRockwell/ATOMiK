"""
Tests for H264Delta
"""

import unittest

from atomik.Video.Streaming import H264Delta


class TestH264Delta(unittest.TestCase):
    """Test cases for H264Delta."""

    def test_initialization(self):
        """Test module initialization."""
        module = H264Delta()
        self.assertIsNotNone(module)

    def test_frame_delta_load_reconstruct(self):
        """Test load and reconstruct for frame_delta."""
        module = H264Delta()
        initial = 0x1111111111111111111111111111111111111111111111111111111111111111
        module.load_frame_delta(initial)
        state = module.reconstruct_frame_delta()
        self.assertEqual(state, initial)

    def test_frame_delta_accumulate(self):
        """Test delta accumulation for frame_delta."""
        module = H264Delta()
        module.load_frame_delta(0)
        delta1 = 0xFF
        delta2 = 0xAA
        module.accumulate_frame_delta(delta1)
        module.accumulate_frame_delta(delta2)
        state = module.reconstruct_frame_delta()
        self.assertEqual(state, delta1 ^ delta2)

    def test_frame_delta_self_inverse(self):
        """Test delta self-inverse property for frame_delta."""
        module = H264Delta()
        initial = 0x123456
        module.load_frame_delta(initial)
        delta = 0xABCDEF
        module.accumulate_frame_delta(delta)
        module.accumulate_frame_delta(delta)  # Apply same delta twice
        state = module.reconstruct_frame_delta()
        self.assertEqual(state, initial)  # Should cancel out

    def test_motion_vector_load_reconstruct(self):
        """Test load and reconstruct for motion_vector."""
        module = H264Delta()
        initial = 0x1111111111111111111111111111111111111111111111111111111111111111
        module.load_motion_vector(initial)
        state = module.reconstruct_motion_vector()
        self.assertEqual(state, initial)

    def test_motion_vector_accumulate(self):
        """Test delta accumulation for motion_vector."""
        module = H264Delta()
        module.load_motion_vector(0)
        delta1 = 0xFF
        delta2 = 0xAA
        module.accumulate_motion_vector(delta1)
        module.accumulate_motion_vector(delta2)
        state = module.reconstruct_motion_vector()
        self.assertEqual(state, delta1 ^ delta2)

    def test_motion_vector_self_inverse(self):
        """Test delta self-inverse property for motion_vector."""
        module = H264Delta()
        initial = 0x123456
        module.load_motion_vector(initial)
        delta = 0xABCDEF
        module.accumulate_motion_vector(delta)
        module.accumulate_motion_vector(delta)  # Apply same delta twice
        state = module.reconstruct_motion_vector()
        self.assertEqual(state, initial)  # Should cancel out

    def test_accumulator_zero(self):
        """Test accumulator zero detection."""
        module = H264Delta()
        self.assertTrue(module.is_accumulator_zero())
        module.accumulate_frame_delta(0x100)
        self.assertFalse(module.is_accumulator_zero())

    def test_rollback(self):
        """Test rollback functionality."""
        module = H264Delta()
        module.load_frame_delta(0)
        module.accumulate_frame_delta(0x10)
        module.accumulate_frame_delta(0x20)
        state_before = module.reconstruct_frame_delta()
        success = module.rollback(1)
        self.assertTrue(success)
        state_after = module.reconstruct_frame_delta()
        self.assertNotEqual(state_before, state_after)


if __name__ == "__main__":
    unittest.main()
