"""
Unit tests for the Motifs module.
"""

import pytest
from atomik_sdk.motifs import (
    Motif,
    classify_delta,
    motif_to_bytes,
    bytes_to_motif,
    get_motif_name,
    get_compression_stats,
    MOTIF_SIGNATURES
)


class TestMotifEnum:
    """Tests for the Motif enumeration."""
    
    def test_motif_values(self):
        """Test that motif values are in expected range."""
        for motif in Motif:
            assert 0 <= motif.value <= 15
    
    def test_motif_count(self):
        """Test that we have 16 motifs."""
        assert len(Motif) == 16
    
    def test_static_is_zero(self):
        """Test that STATIC motif has value 0."""
        assert Motif.STATIC == 0


class TestClassifyDelta:
    """Tests for the classify_delta function."""
    
    def test_classify_zero_is_static(self):
        """Test that zero delta is classified as STATIC."""
        result = classify_delta(0)
        assert result == Motif.STATIC
    
    def test_classify_horizontal_signatures(self):
        """Test classification of horizontal motion signatures."""
        # Right motion signature
        result = classify_delta(0xFF00000000000000)
        assert result in (Motif.HORIZONTAL_MOTION, Motif.VERTICAL_MOTION, Motif.EXPANSION)
    
    def test_classify_vertical_signatures(self):
        """Test classification of vertical motion signatures."""
        result = classify_delta(0x0101010101010101)
        # Should be recognized as some motion pattern
        assert result != Motif.STATIC
    
    def test_classify_sparse_is_noise(self):
        """Test that sparse deltas are classified as NOISE."""
        # Only 2 bits set
        result = classify_delta(0x0000000000000003)
        assert result == Motif.NOISE
    
    def test_classify_dense_is_expansion(self):
        """Test that dense deltas are classified as EXPANSION or FLICKER."""
        result = classify_delta(0xFFFFFFFFFFFFFFFF)
        assert result in (Motif.EXPANSION, Motif.FLICKER)


class TestMotifSerialization:
    """Tests for motif serialization functions."""
    
    def test_motif_to_bytes(self):
        """Test converting motif to bytes."""
        result = motif_to_bytes(Motif.STATIC)
        assert result == b'\x00'
        
        result = motif_to_bytes(Motif.HORIZONTAL_MOTION)
        assert result == b'\x01'
    
    def test_bytes_to_motif(self):
        """Test converting bytes to motif."""
        result = bytes_to_motif(b'\x00')
        assert result == Motif.STATIC
        
        result = bytes_to_motif(b'\x01')
        assert result == Motif.HORIZONTAL_MOTION
    
    def test_roundtrip_serialization(self):
        """Test that serialization roundtrips correctly."""
        for motif in Motif:
            serialized = motif_to_bytes(motif)
            deserialized = bytes_to_motif(serialized)
            assert deserialized == motif
    
    def test_bytes_to_motif_empty_raises(self):
        """Test that empty bytes raises ValueError."""
        with pytest.raises(ValueError):
            bytes_to_motif(b'')
    
    def test_bytes_to_motif_masks_upper_bits(self):
        """Test that upper 4 bits are masked."""
        # Value 0xF1 should be masked to 0x01 (HORIZONTAL_MOTION)
        result = bytes_to_motif(b'\xF1')
        assert result == Motif.HORIZONTAL_MOTION


class TestGetMotifName:
    """Tests for the get_motif_name function."""
    
    def test_all_motifs_have_names(self):
        """Test that all motifs have human-readable names."""
        for motif in Motif:
            name = get_motif_name(motif)
            assert isinstance(name, str)
            assert len(name) > 0
    
    def test_specific_names(self):
        """Test specific motif names."""
        assert get_motif_name(Motif.STATIC) == "No Motion"
        assert get_motif_name(Motif.HORIZONTAL_MOTION) == "Horizontal Motion"
        assert get_motif_name(Motif.VERTICAL_MOTION) == "Vertical Motion"


class TestCompressionStats:
    """Tests for the get_compression_stats function."""
    
    def test_empty_list(self):
        """Test compression stats for empty list."""
        result = get_compression_stats([])
        assert result["compression_ratio"] == 0.0
        assert result["entropy"] == 0.0
    
    def test_single_motif(self):
        """Test compression stats for single motif."""
        result = get_compression_stats([Motif.STATIC])
        assert result["compression_ratio"] == 16.0  # 64 bits / 4 bits
        assert result["entropy"] == 0.0  # Single symbol has zero entropy
        assert result["unique_motifs"] == 1
    
    def test_uniform_distribution(self):
        """Test compression stats for uniform distribution."""
        motifs = [Motif.STATIC, Motif.HORIZONTAL_MOTION, 
                  Motif.VERTICAL_MOTION, Motif.EXPANSION]
        result = get_compression_stats(motifs)
        assert result["compression_ratio"] == 16.0
        assert result["unique_motifs"] == 4
        # Entropy should be 2 bits for uniform 4-symbol distribution
        assert abs(result["entropy"] - 2.0) < 0.01
    
    def test_dominant_motif(self):
        """Test dominant motif detection."""
        motifs = [Motif.STATIC, Motif.STATIC, Motif.STATIC, Motif.HORIZONTAL_MOTION]
        result = get_compression_stats(motifs)
        assert result["dominant_motif"] == "STATIC"


class TestMotifSignatures:
    """Tests for motif signature constants."""
    
    def test_signatures_exist(self):
        """Test that signature dictionary exists."""
        assert isinstance(MOTIF_SIGNATURES, dict)
    
    def test_static_signature_is_zero(self):
        """Test that STATIC signature is zero."""
        assert 0 in MOTIF_SIGNATURES[Motif.STATIC]
    
    def test_signatures_are_64bit(self):
        """Test that all signatures fit in 64 bits."""
        for motif, signatures in MOTIF_SIGNATURES.items():
            for sig in signatures:
                assert 0 <= sig <= 0xFFFFFFFFFFFFFFFF
