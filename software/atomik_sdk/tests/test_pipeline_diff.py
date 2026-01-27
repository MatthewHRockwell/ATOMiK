"""
Tests for ATOMiK Pipeline Differential Change Detection

Tests cover:
- No-change detection (short-circuit)
- Schema structural diff classification
- Change impact mapping to affected generators
- Checkpoint integration for previous state
"""

import json
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.context.checkpoint import Checkpoint
from pipeline.stages import StageManifest, StageStatus
from pipeline.stages.diff import DiffStage


@pytest.fixture
def project_root():
    return Path(__file__).parent.parent.parent.parent


@pytest.fixture
def domain_schema_path(project_root):
    return project_root / "sdk" / "schemas" / "domains" / "video-h264-delta.json"


@pytest.fixture
def diff_stage():
    return DiffStage()


class TestDiffStage:
    def test_stage_name(self, diff_stage):
        assert diff_stage.name == "diff"

    def test_new_schema_full_diff(self, diff_stage, domain_schema_path, tmp_path):
        """A new schema should trigger full regeneration."""
        if not domain_schema_path.exists():
            pytest.skip("Domain schema not found")

        with open(domain_schema_path, encoding="utf-8") as f:
            schema = json.load(f)

        # Create a validation manifest with a content hash
        prev_manifest = StageManifest(stage="validate")
        prev_manifest.metrics["content_hash"] = "abc123"

        config = type("Config", (), {"checkpoint_dir": str(tmp_path), "languages": None})()

        manifest = diff_stage.execute(schema, str(domain_schema_path), prev_manifest, config)

        assert manifest.status == StageStatus.SUCCESS
        assert manifest.metrics.get("diff_type") == "full"
        assert len(manifest.metrics.get("affected_generators", [])) > 0

    def test_unchanged_schema_short_circuits(self, diff_stage, domain_schema_path, tmp_path):
        """An unchanged schema should short-circuit."""
        if not domain_schema_path.exists():
            pytest.skip("Domain schema not found")

        with open(domain_schema_path, encoding="utf-8") as f:
            schema = json.load(f)

        # Compute actual hash
        from pipeline.context.cache import ArtifactCache
        content_hash = ArtifactCache.file_hash(domain_schema_path)

        # Store in checkpoint
        checkpoint = Checkpoint(str(tmp_path))
        checkpoint.update_schema(domain_schema_path.stem, content_hash)

        # Run diff stage with matching hash
        prev_manifest = StageManifest(stage="validate")
        prev_manifest.metrics["content_hash"] = content_hash

        config = type("Config", (), {"checkpoint_dir": str(tmp_path), "languages": None})()

        manifest = diff_stage.execute(schema, str(domain_schema_path), prev_manifest, config)

        assert manifest.status == StageStatus.SKIPPED
        assert manifest.metrics.get("diff_type") == "none"
        assert manifest.tokens_consumed == 0

    def test_change_impact_mapping(self):
        """Verify change types map to correct generators."""
        assert "verilog" in DiffStage.CHANGE_IMPACT["hardware"]
        assert len(DiffStage.CHANGE_IMPACT["hardware"]) == 1  # Only Verilog

        # delta_fields affects all generators
        assert len(DiffStage.CHANGE_IMPACT["delta_fields"]) == 5

        # metadata only affects Python and JavaScript
        assert DiffStage.CHANGE_IMPACT["metadata"] == {"python", "javascript"}

    def test_language_filtering(self, diff_stage, domain_schema_path, tmp_path):
        """Affected generators should be filtered by requested languages."""
        if not domain_schema_path.exists():
            pytest.skip("Domain schema not found")

        with open(domain_schema_path, encoding="utf-8") as f:
            schema = json.load(f)

        prev_manifest = StageManifest(stage="validate")
        prev_manifest.metrics["content_hash"] = "different_hash"

        config = type("Config", (), {
            "checkpoint_dir": str(tmp_path),
            "languages": ["python", "rust"],
        })()

        manifest = diff_stage.execute(schema, str(domain_schema_path), prev_manifest, config)

        affected = manifest.metrics.get("affected_generators", [])
        # Should only include requested languages
        for lang in affected:
            assert lang in ["python", "rust"]

    def test_zero_tokens_consumed(self, diff_stage, domain_schema_path, tmp_path):
        """Diff stage should always consume 0 tokens."""
        if not domain_schema_path.exists():
            pytest.skip("Domain schema not found")

        with open(domain_schema_path, encoding="utf-8") as f:
            schema = json.load(f)

        prev_manifest = StageManifest(stage="validate")
        prev_manifest.metrics["content_hash"] = "abc"
        config = type("Config", (), {"checkpoint_dir": str(tmp_path), "languages": None})()

        manifest = diff_stage.execute(schema, str(domain_schema_path), prev_manifest, config)

        assert manifest.tokens_consumed == 0
