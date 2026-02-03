"""
Tests for PriceTick
"""

import unittest

from atomik.Finance.Trading import PriceTick


class TestPriceTick(unittest.TestCase):
    """Test cases for PriceTick."""

    def test_initialization(self):
        """Test module initialization."""
        module = PriceTick()
        self.assertIsNotNone(module)

    def test_price_delta_load_reconstruct(self):
        """Test load and reconstruct for price_delta."""
        module = PriceTick()
        initial = 0x1111111111111111
        module.load_price_delta(initial)
        state = module.reconstruct_price_delta()
        self.assertEqual(state, initial)

    def test_price_delta_accumulate(self):
        """Test delta accumulation for price_delta."""
        module = PriceTick()
        module.load_price_delta(0)
        delta1 = 0xFF
        delta2 = 0xAA
        module.accumulate_price_delta(delta1)
        module.accumulate_price_delta(delta2)
        state = module.reconstruct_price_delta()
        self.assertEqual(state, delta1 ^ delta2)

    def test_price_delta_self_inverse(self):
        """Test delta self-inverse property for price_delta."""
        module = PriceTick()
        initial = 0x123456
        module.load_price_delta(initial)
        delta = 0xABCDEF
        module.accumulate_price_delta(delta)
        module.accumulate_price_delta(delta)  # Apply same delta twice
        state = module.reconstruct_price_delta()
        self.assertEqual(state, initial)  # Should cancel out

    def test_volume_delta_load_reconstruct(self):
        """Test load and reconstruct for volume_delta."""
        module = PriceTick()
        initial = 0x1111111111111111
        module.load_volume_delta(initial)
        state = module.reconstruct_volume_delta()
        self.assertEqual(state, initial)

    def test_volume_delta_accumulate(self):
        """Test delta accumulation for volume_delta."""
        module = PriceTick()
        module.load_volume_delta(0)
        delta1 = 0xFF
        delta2 = 0xAA
        module.accumulate_volume_delta(delta1)
        module.accumulate_volume_delta(delta2)
        state = module.reconstruct_volume_delta()
        self.assertEqual(state, delta1 ^ delta2)

    def test_volume_delta_self_inverse(self):
        """Test delta self-inverse property for volume_delta."""
        module = PriceTick()
        initial = 0x123456
        module.load_volume_delta(initial)
        delta = 0xABCDEF
        module.accumulate_volume_delta(delta)
        module.accumulate_volume_delta(delta)  # Apply same delta twice
        state = module.reconstruct_volume_delta()
        self.assertEqual(state, initial)  # Should cancel out

    def test_trade_flags_load_reconstruct(self):
        """Test load and reconstruct for trade_flags."""
        module = PriceTick()
        initial = 0x1111111111111111
        module.load_trade_flags(initial)
        state = module.reconstruct_trade_flags()
        self.assertEqual(state, initial)

    def test_trade_flags_accumulate(self):
        """Test delta accumulation for trade_flags."""
        module = PriceTick()
        module.load_trade_flags(0)
        delta1 = 0xFF
        delta2 = 0xAA
        module.accumulate_trade_flags(delta1)
        module.accumulate_trade_flags(delta2)
        state = module.reconstruct_trade_flags()
        self.assertEqual(state, delta1 ^ delta2)

    def test_trade_flags_self_inverse(self):
        """Test delta self-inverse property for trade_flags."""
        module = PriceTick()
        initial = 0x123456
        module.load_trade_flags(initial)
        delta = 0xABCDEF
        module.accumulate_trade_flags(delta)
        module.accumulate_trade_flags(delta)  # Apply same delta twice
        state = module.reconstruct_trade_flags()
        self.assertEqual(state, initial)  # Should cancel out

    def test_accumulator_zero(self):
        """Test accumulator zero detection."""
        module = PriceTick()
        self.assertTrue(module.is_accumulator_zero())
        module.accumulate_price_delta(0x100)
        self.assertFalse(module.is_accumulator_zero())

    def test_rollback(self):
        """Test rollback functionality."""
        module = PriceTick()
        module.load_price_delta(0)
        module.accumulate_price_delta(0x10)
        module.accumulate_price_delta(0x20)
        state_before = module.reconstruct_price_delta()
        success = module.rollback(1)
        self.assertTrue(success)
        state_after = module.reconstruct_price_delta()
        self.assertNotEqual(state_before, state_after)


if __name__ == "__main__":
    unittest.main()
