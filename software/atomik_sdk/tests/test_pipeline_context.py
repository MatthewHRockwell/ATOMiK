"""
Tests for ATOMiK Pipeline Context Management

Tests cover:
- Pipeline manifest creation and serialization
- Artifact cache put/get/invalidate
- Checkpoint save/load/restore
- Content hashing
- Cross-session state persistence
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.context.cache import ArtifactCache
from pipeline.context.checkpoint import Checkpoint
from pipeline.context.manifest import PipelineManifest


class TestPipelineManifest:
    def test_create_manifest(self):
        manifest = PipelineManifest()
        assert manifest.version == "2.1"
        assert manifest.phase == "5"
        assert manifest.schemas_registered == 0

    def test_register_schema(self):
        manifest = PipelineManifest()
        manifest.register_schema("test", "abc123", "/path/test.json", "V.F.O")
        assert manifest.schemas_registered == 1
        assert "test" in manifest.schemas
        assert manifest.schemas["test"].sha256 == "abc123"

    def test_update_existing_schema(self):
        manifest = PipelineManifest()
        manifest.register_schema("test", "abc", "/path/test.json")
        manifest.register_schema("test", "def", "/path/test.json")
        assert manifest.schemas_registered == 1
        assert manifest.schemas["test"].sha256 == "def"

    def test_record_run(self):
        manifest = PipelineManifest()
        manifest.record_run(tokens_consumed=100)
        assert manifest.last_pipeline_run != ""
        assert manifest.token_ledger["session_total"] == 100

    def test_to_dict(self):
        manifest = PipelineManifest()
        manifest.register_schema("test", "abc", "/path")
        d = manifest.to_dict()
        assert "project_state" in d
        assert "artifact_index" in d
        assert d["project_state"]["schemas_registered"] == 1

    def test_save_and_load(self, tmp_path):
        manifest = PipelineManifest()
        manifest.register_schema("video", "abc123", "/video.json", "V.S.H264")
        manifest.record_run(tokens_consumed=500)

        path = tmp_path / "manifest.json"
        manifest.save(path)

        loaded = PipelineManifest.load(path)
        assert loaded.schemas_registered == 1
        assert "video" in loaded.schemas
        assert loaded.token_ledger["session_total"] == 500


class TestArtifactCache:
    def test_create_cache(self, tmp_path):
        ArtifactCache(tmp_path / "cache")
        assert (tmp_path / "cache").exists()

    def test_content_hash(self):
        h1 = ArtifactCache.content_hash("hello")
        h2 = ArtifactCache.content_hash("hello")
        h3 = ArtifactCache.content_hash("world")
        assert h1 == h2
        assert h1 != h3

    def test_put_and_get(self, tmp_path):
        cache = ArtifactCache(tmp_path / "cache")
        data = {"key": "value", "count": 42}
        cache.put("schema1", "result", data, schema_hash="abc")

        retrieved = cache.get("schema1", "result")
        assert retrieved is not None
        assert retrieved["key"] == "value"
        assert retrieved["count"] == 42

    def test_get_nonexistent(self, tmp_path):
        cache = ArtifactCache(tmp_path / "cache")
        assert cache.get("nonexistent", "key") is None

    def test_invalidate(self, tmp_path):
        cache = ArtifactCache(tmp_path / "cache")
        cache.put("schema1", "result", {"data": 1})
        cache.invalidate("schema1")
        assert cache.get("schema1", "result") is None

    def test_is_valid(self, tmp_path):
        cache = ArtifactCache(tmp_path / "cache")
        cache.put("schema1", "result", {"data": 1}, schema_hash="hash1")
        assert cache.is_valid("schema1", "hash1")
        assert not cache.is_valid("schema1", "hash2")

    def test_clear(self, tmp_path):
        cache = ArtifactCache(tmp_path / "cache")
        cache.put("s1", "r1", {"a": 1})
        cache.put("s2", "r2", {"b": 2})
        cache.clear()
        assert cache.get("s1", "r1") is None
        assert cache.get("s2", "r2") is None


class TestCheckpoint:
    def test_create_checkpoint(self, tmp_path):
        cp = Checkpoint(tmp_path)
        assert cp.to_dict()["version"] == "2.0"

    def test_update_and_query_schema(self, tmp_path):
        cp = Checkpoint(tmp_path)
        cp.update_schema("test", "hash123", {"python": "abc"})

        assert cp.get_schema_hash("test") == "hash123"
        assert cp.is_current("test", "hash123")
        assert not cp.is_current("test", "different")

    def test_save_and_reload(self, tmp_path):
        cp1 = Checkpoint(tmp_path)
        cp1.update_schema("video", "h1", metrics={"files": 19})

        # Reload from same directory
        cp2 = Checkpoint(tmp_path)
        assert cp2.get_schema_hash("video") == "h1"

    def test_metrics_history(self, tmp_path):
        cp = Checkpoint(tmp_path)
        cp.append_metrics("video", {"tokens": 0, "files": 19})
        cp.append_metrics("sensor", {"tokens": 0, "files": 19})
        cp.save()

        all_history = cp.get_metrics_history()
        assert len(all_history) == 2

        video_history = cp.get_metrics_history("video")
        assert len(video_history) == 1
        assert video_history[0]["tokens"] == 0

    def test_nonexistent_schema(self, tmp_path):
        cp = Checkpoint(tmp_path)
        assert cp.get_schema_hash("nonexistent") is None
        assert not cp.is_current("nonexistent", "any")

    def test_backup_on_save(self, tmp_path):
        cp = Checkpoint(tmp_path)
        cp.update_schema("test", "h1")  # First save
        cp.update_schema("test", "h2")  # Second save creates backup

        backup = tmp_path / "checkpoint.json.bak"
        assert backup.exists()
