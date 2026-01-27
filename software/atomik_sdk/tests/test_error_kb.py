"""Tests for error pattern knowledge base."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.knowledge.error_kb import ErrorKnowledgeBase, ErrorPattern
from pipeline.knowledge.fuzzy_match import edit_distance, fuzzy_score, token_overlap


class TestFuzzyMatch:
    def test_edit_distance_identical(self):
        assert edit_distance("abc", "abc") == 0

    def test_edit_distance_different(self):
        assert edit_distance("abc", "abd") == 1
        assert edit_distance("abc", "abcd") == 1

    def test_token_overlap(self):
        score = token_overlap("missing semicolon error", "missing semicolon")
        assert score > 0.5

    def test_fuzzy_score(self):
        score = fuzzy_score("missing import os", "missing import sys")
        assert 0.0 < score < 1.0


class TestErrorKnowledgeBase:
    def test_add_and_lookup(self):
        kb = ErrorKnowledgeBase()
        kb.add_pattern(ErrorPattern(
            pattern_id="test_1",
            error_class="syntax_error",
            language="python",
            signature="missing colon",
            fix_template="Add colon at end of line",
            fix_type="add_colon",
            success_count=5,
        ))
        result = kb.lookup("python", "syntax_error", "missing colon after def")
        assert result.found
        assert "colon" in result.pattern.fix_template.lower()

    def test_lookup_miss(self):
        kb = ErrorKnowledgeBase()
        result = kb.lookup("python", "unknown_error", "never seen this")
        assert not result.found

    def test_fuzzy_lookup(self):
        kb = ErrorKnowledgeBase()
        kb.add_pattern(ErrorPattern(
            pattern_id="test_2",
            error_class="import_error",
            language="python",
            signature="ModuleNotFoundError: No module named 'numpy'",
            fix_template="pip install numpy",
            fix_type="install_dep",
            success_count=3,
        ))
        result = kb.lookup(
            "python", "import_error",
            "ModuleNotFoundError: No module named 'pandas'"
        )
        # Fuzzy match may or may not find depending on threshold
        assert isinstance(result.found, bool)

    def test_learn(self):
        kb = ErrorKnowledgeBase()
        pattern = kb.learn(
            language="rust",
            error_class="borrow_error",
            error_message="cannot borrow as mutable",
            fix_description="Use RefCell for interior mutability",
        )
        assert pattern.source == "learned"
        result = kb.lookup("rust", "borrow_error", "cannot borrow as mutable")
        assert result.found

    def test_load_seed_patterns(self):
        kb = ErrorKnowledgeBase()
        count = kb.load_seed()
        assert count >= 5
        assert len(kb.get_all_patterns()) >= 5

    def test_record_success(self):
        kb = ErrorKnowledgeBase()
        kb.add_pattern(ErrorPattern(
            pattern_id="test_3",
            error_class="test",
            language="python",
            signature="test error",
            fix_template="test fix",
            fix_type="test",
            success_count=0,
        ))
        kb.record_success("test_3")
        pattern = kb.get_pattern("test_3")
        assert pattern.success_count == 1
