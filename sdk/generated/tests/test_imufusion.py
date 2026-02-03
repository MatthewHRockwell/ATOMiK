"""
Tests for IMUFusion
"""

import unittest

from atomik.Edge.Sensor import IMUFusion


class TestIMUFusion(unittest.TestCase):
    """Test cases for IMUFusion."""

    def test_initialization(self):
        """Test module initialization."""
        module = IMUFusion()
        self.assertIsNotNone(module)

    def test_motion_delta_load_reconstruct(self):
        """Test load and reconstruct for motion_delta."""
        module = IMUFusion()
        initial = 0x1111111111111111
        module.load_motion_delta(initial)
        state = module.reconstruct_motion_delta()
        self.assertEqual(state, initial)

    def test_motion_delta_accumulate(self):
        """Test delta accumulation for motion_delta."""
        module = IMUFusion()
        module.load_motion_delta(0)
        delta1 = 0xFF
        delta2 = 0xAA
        module.accumulate_motion_delta(delta1)
        module.accumulate_motion_delta(delta2)
        state = module.reconstruct_motion_delta()
        self.assertEqual(state, delta1 ^ delta2)

    def test_motion_delta_self_inverse(self):
        """Test delta self-inverse property for motion_delta."""
        module = IMUFusion()
        initial = 0x123456
        module.load_motion_delta(initial)
        delta = 0xABCDEF
        module.accumulate_motion_delta(delta)
        module.accumulate_motion_delta(delta)  # Apply same delta twice
        state = module.reconstruct_motion_delta()
        self.assertEqual(state, initial)  # Should cancel out

    def test_alert_flags_load_reconstruct(self):
        """Test load and reconstruct for alert_flags."""
        module = IMUFusion()
        initial = 0x1111111111111111
        module.load_alert_flags(initial)
        state = module.reconstruct_alert_flags()
        self.assertEqual(state, initial)

    def test_alert_flags_accumulate(self):
        """Test delta accumulation for alert_flags."""
        module = IMUFusion()
        module.load_alert_flags(0)
        delta1 = 0xFF
        delta2 = 0xAA
        module.accumulate_alert_flags(delta1)
        module.accumulate_alert_flags(delta2)
        state = module.reconstruct_alert_flags()
        self.assertEqual(state, delta1 ^ delta2)

    def test_alert_flags_self_inverse(self):
        """Test delta self-inverse property for alert_flags."""
        module = IMUFusion()
        initial = 0x123456
        module.load_alert_flags(initial)
        delta = 0xABCDEF
        module.accumulate_alert_flags(delta)
        module.accumulate_alert_flags(delta)  # Apply same delta twice
        state = module.reconstruct_alert_flags()
        self.assertEqual(state, initial)  # Should cancel out

    def test_accumulator_zero(self):
        """Test accumulator zero detection."""
        module = IMUFusion()
        self.assertTrue(module.is_accumulator_zero())
        module.accumulate_motion_delta(0x100)
        self.assertFalse(module.is_accumulator_zero())

    def test_rollback(self):
        """Test rollback functionality."""
        module = IMUFusion()
        module.load_motion_delta(0)
        module.accumulate_motion_delta(0x10)
        module.accumulate_motion_delta(0x20)
        state_before = module.reconstruct_motion_delta()
        success = module.rollback(1)
        self.assertTrue(success)
        state_after = module.reconstruct_motion_delta()
        self.assertNotEqual(state_before, state_after)


if __name__ == "__main__":
    unittest.main()
