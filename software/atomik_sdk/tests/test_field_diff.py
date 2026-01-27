"""Tests for field-level differential analysis."""

from pipeline.analysis.field_diff import FieldDiff, FieldDiffResult


class TestFieldDiff:
    def test_no_changes(self):
        diff = FieldDiff()
        old = {"name": "test", "fields": [{"name": "a", "type": "int"}]}
        new = {"name": "test", "fields": [{"name": "a", "type": "int"}]}
        result = diff.compare(old, new)
        assert isinstance(result, FieldDiffResult)
        assert result.change_count == 0

    def test_field_added(self):
        diff = FieldDiff()
        old = {"name": "test"}
        new = {"name": "test", "version": "1.0"}
        result = diff.compare(old, new)
        assert result.change_count > 0
        added = [c for c in result.changes if c.change_type == "added"]
        assert len(added) >= 1

    def test_field_removed(self):
        diff = FieldDiff()
        old = {"name": "test", "version": "1.0"}
        new = {"name": "test"}
        result = diff.compare(old, new)
        removed = [c for c in result.changes if c.change_type == "removed"]
        assert len(removed) >= 1

    def test_field_modified(self):
        diff = FieldDiff()
        old = {"name": "test", "value": 10}
        new = {"name": "test", "value": 20}
        result = diff.compare(old, new)
        modified = [c for c in result.changes if c.change_type == "modified"]
        assert len(modified) >= 1

    def test_nested_changes(self):
        diff = FieldDiff()
        old = {"config": {"timeout": 30, "retries": 3}}
        new = {"config": {"timeout": 60, "retries": 3}}
        result = diff.compare(old, new)
        assert result.change_count >= 1
        # Should track the nested path
        paths = [c.path for c in result.changes]
        assert any("timeout" in p for p in paths)
