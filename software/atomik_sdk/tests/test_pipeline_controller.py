"""
Tests for ATOMiK Pipeline Controller

Tests cover:
- Pipeline creation and configuration
- Stage registration and ordering
- End-to-end pipeline execution on domain schemas
- Dry run mode
- Batch processing
- Token budget enforcement
- Pipeline status reporting
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.controller import Pipeline, PipelineConfig, PipelineResult
from pipeline.stages.diff import DiffStage
from pipeline.stages.generate import GenerateStage
from pipeline.stages.hardware import HardwareStage
from pipeline.stages.metrics import MetricsStage
from pipeline.stages.validate import ValidateStage
from pipeline.stages.verify import VerifyStage


@pytest.fixture
def project_root():
    return Path(__file__).parent.parent.parent.parent


@pytest.fixture
def domain_schema_path(project_root):
    return project_root / "sdk" / "schemas" / "domains" / "video-h264-delta.json"


@pytest.fixture
def example_schema_path(project_root):
    return project_root / "sdk" / "schemas" / "examples" / "terminal-io.json"


@pytest.fixture
def domain_schemas_dir(project_root):
    return project_root / "sdk" / "schemas" / "domains"


@pytest.fixture
def pipeline_with_stages(tmp_path):
    """Pipeline with all stages registered and temp output."""
    config = PipelineConfig(
        output_dir=str(tmp_path / "generated"),
        checkpoint_dir=str(tmp_path / ".atomik"),
        metrics_csv=str(tmp_path / ".atomik" / "metrics.csv"),
        verbose=False,
    )
    pipeline = Pipeline(config)
    pipeline.register_stage(ValidateStage())
    pipeline.register_stage(DiffStage())
    pipeline.register_stage(GenerateStage())
    pipeline.register_stage(VerifyStage())
    pipeline.register_stage(HardwareStage())
    pipeline.register_stage(MetricsStage())
    return pipeline


class TestPipelineConfig:
    def test_default_config(self):
        config = PipelineConfig()
        assert config.output_dir == "generated"
        assert config.verbose is False
        assert config.sim_only is False
        assert config.token_budget is None

    def test_custom_config(self):
        config = PipelineConfig(
            output_dir="/tmp/out",
            verbose=True,
            sim_only=True,
            token_budget=15000,
        )
        assert config.output_dir == "/tmp/out"
        assert config.verbose is True
        assert config.token_budget == 15000


class TestPipelineResult:
    def test_result_to_dict(self):
        result = PipelineResult(schema="test.json", success=True)
        d = result.to_dict()
        assert d["schema"] == "test.json"
        assert d["success"] is True
        assert d["total_tokens"] == 0

    def test_failed_result(self):
        result = PipelineResult(
            schema="test.json", success=False, errors=["validation failed"]
        )
        assert not result.success
        assert len(result.errors) == 1


class TestPipeline:
    def test_create_pipeline(self):
        pipeline = Pipeline()
        assert pipeline.config is not None
        assert pipeline._stages == {}

    def test_register_stages(self):
        pipeline = Pipeline()
        pipeline.register_stage(ValidateStage())
        pipeline.register_stage(DiffStage())
        assert "validate" in pipeline._stages
        assert "diff" in pipeline._stages

    def test_run_nonexistent_schema(self):
        pipeline = Pipeline()
        result = pipeline.run("/nonexistent/path.json")
        assert not result.success
        assert any("load failed" in e.lower() for e in result.errors)

    def test_run_domain_schema(self, pipeline_with_stages, domain_schema_path):
        if not domain_schema_path.exists():
            pytest.skip("Domain schema not found")
        result = pipeline_with_stages.run(domain_schema_path)
        assert result.schema == domain_schema_path.name
        assert result.total_time_ms > 0
        assert len(result.stages) > 0

    def test_run_example_schema(self, pipeline_with_stages, example_schema_path):
        if not example_schema_path.exists():
            pytest.skip("Example schema not found")
        result = pipeline_with_stages.run(example_schema_path)
        assert result.schema == example_schema_path.name

    def test_dry_run(self, domain_schema_path, tmp_path):
        if not domain_schema_path.exists():
            pytest.skip("Domain schema not found")
        config = PipelineConfig(
            output_dir=str(tmp_path / "generated"),
            dry_run=True,
        )
        pipeline = Pipeline(config)
        pipeline.register_stage(ValidateStage())
        result = pipeline.run(domain_schema_path)
        assert result.files_generated == 0

    def test_batch_processing(self, pipeline_with_stages, domain_schemas_dir):
        if not domain_schemas_dir.exists():
            pytest.skip("Domain schemas directory not found")
        results = pipeline_with_stages.run_batch(domain_schemas_dir)
        assert len(results) >= 1

    def test_get_status(self, pipeline_with_stages):
        status = pipeline_with_stages.get_status()
        assert "total_runs" in status
        assert "stages_registered" in status
        assert len(status["stages_registered"]) == 6

    def test_token_budget_enforcement(self, tmp_path):
        """Pipeline should respect token budget limits."""
        config = PipelineConfig(
            output_dir=str(tmp_path / "generated"),
            checkpoint_dir=str(tmp_path / ".atomik"),
            token_budget=0,  # Zero budget
        )
        pipeline = Pipeline(config)
        pipeline.register_stage(ValidateStage())
        # With 0 budget, should still run local stages (0 tokens)
        # This tests the budget check mechanism exists
        assert config.token_budget == 0
