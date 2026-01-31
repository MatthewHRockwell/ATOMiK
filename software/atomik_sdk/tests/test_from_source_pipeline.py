"""Integration tests for the from-source pipeline."""

import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.controller import Pipeline, PipelineConfig
from pipeline.stages.extract import ExtractStage
from pipeline.stages.infer import InferStage
from pipeline.stages.migrate_check import MigrateCheckStage
from pipeline.stages.validate import ValidateStage

# Minimal Python source for testing
SAMPLE_PYTHON_SOURCE = '''\
HISTORY_DEPTH = 256

class SensorFusion:
    """A sensor fusion class for IMU data."""
    accel_x: float
    accel_y: float
    gyro_z: float
    alert_flags: int

    def accumulate(self, delta):
        pass

    def get_state(self):
        pass

    def rollback(self, steps):
        pass

    def calibrate(self):
        pass
'''

SAMPLE_FINANCE_SOURCE = '''\
class PriceTick:
    price_delta: float
    volume: int
    trade_flags: int

    def accumulate(self, delta):
        pass

    def snapshot(self):
        pass
'''


def _run_source_pipeline(source_code: str, suffix: str = ".py", **config_kwargs):
    """Helper: write source to temp file and run extract+infer+validate."""
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=suffix, delete=False, encoding="utf-8"
    ) as f:
        f.write(source_code)
        tmp_path = f.name

    try:
        config = PipelineConfig(
            source_mode=True,
            source_path=tmp_path,
            **config_kwargs,
        )

        pipeline = Pipeline(config)
        pipeline.register_stage(ExtractStage())
        pipeline.register_stage(InferStage())
        pipeline.register_stage(MigrateCheckStage())
        pipeline.register_stage(ValidateStage())

        return pipeline.run(tmp_path)
    finally:
        Path(tmp_path).unlink(missing_ok=True)


class TestExtractInferPipeline:
    def test_sensor_source_to_schema(self):
        """Extract+infer from a sensor Python class produces valid schema."""
        result = _run_source_pipeline(SAMPLE_PYTHON_SOURCE)

        # Extract and infer should succeed
        extract_stage = next(s for s in result.stages if s.stage == "extract")
        infer_stage = next(s for s in result.stages if s.stage == "infer")

        assert extract_stage.success
        assert infer_stage.success
        assert infer_stage.metrics["vertical"] == "Edge"
        assert infer_stage.metrics["object"] == "SensorFusion"
        assert infer_stage.metrics["delta_field_count"] >= 3

    def test_finance_source_to_schema(self):
        """Extract+infer from a finance Python class."""
        result = _run_source_pipeline(SAMPLE_FINANCE_SOURCE)

        extract_stage = next(s for s in result.stages if s.stage == "extract")
        infer_stage = next(s for s in result.stages if s.stage == "infer")

        assert extract_stage.success
        assert infer_stage.success
        assert infer_stage.metrics["vertical"] == "Finance"

    def test_vertical_override(self):
        """--vertical override takes precedence over heuristics."""
        result = _run_source_pipeline(
            SAMPLE_PYTHON_SOURCE,
            inference_overrides={"vertical": "Video", "version": "1.0.0"},
        )
        infer_stage = next(s for s in result.stages if s.stage == "infer")
        assert infer_stage.metrics["vertical"] == "Video"

    def test_operations_inferred(self):
        """Operations should be inferred from method names."""
        result = _run_source_pipeline(SAMPLE_PYTHON_SOURCE)
        infer_stage = next(s for s in result.stages if s.stage == "infer")

        ops = infer_stage.metrics["operations"]
        assert ops.get("accumulate") is True
        assert ops.get("reconstruct") is True  # from get_state
        assert ops.get("rollback") is True


class TestMigrateCheck:
    def test_skipped_without_existing_schema(self):
        """migrate_check should skip when no existing schema provided."""
        result = _run_source_pipeline(SAMPLE_PYTHON_SOURCE)

        mc_stage = next(s for s in result.stages if s.stage == "migrate_check")
        assert mc_stage.success  # SKIPPED counts as success
        assert mc_stage.metrics.get("skip_reason") == "no existing schema"

    def test_diff_against_existing_schema(self):
        """migrate_check should produce a diff when existing schema provided."""
        # First, create a schema JSON to compare against
        existing_schema = {
            "catalogue": {
                "vertical": "Edge",
                "field": "Sensor",
                "object": "SensorFusion",
                "version": "1.0.0",
            },
            "schema": {
                "delta_fields": {
                    "accel_x": {
                        "type": "parameter_delta",
                        "width": 64,
                        "encoding": "raw",
                        "compression": "none",
                        "default_value": 0,
                    },
                },
                "operations": {
                    "accumulate": {"enabled": True, "latency_cycles": 1},
                },
                "constraints": {
                    "target_frequency_mhz": 94.5,
                    "max_memory_mb": 64,
                },
            },
        }

        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".json", delete=False, encoding="utf-8"
        ) as f:
            json.dump(existing_schema, f)
            schema_path = f.name

        try:
            result = _run_source_pipeline(
                SAMPLE_PYTHON_SOURCE,
                existing_schema_path=schema_path,
            )
            mc_stage = next(s for s in result.stages if s.stage == "migrate_check")
            assert mc_stage.success
            assert "change_count" in mc_stage.metrics
        finally:
            Path(schema_path).unlink(missing_ok=True)

    def test_strict_mode_fails_on_breaking(self):
        """strict mode should fail when breaking changes detected."""
        # Existing schema with different fields triggers breaking changes
        existing_schema = {
            "catalogue": {
                "vertical": "Edge",
                "field": "Sensor",
                "object": "SensorFusion",
                "version": "1.0.0",
            },
            "schema": {
                "delta_fields": {
                    "removed_field": {
                        "type": "parameter_delta",
                        "width": 64,
                        "encoding": "raw",
                        "compression": "none",
                        "default_value": 0,
                    },
                },
                "operations": {
                    "accumulate": {"enabled": True, "latency_cycles": 1},
                },
                "constraints": {
                    "target_frequency_mhz": 94.5,
                    "max_memory_mb": 64,
                },
            },
        }

        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".json", delete=False, encoding="utf-8"
        ) as f:
            json.dump(existing_schema, f)
            schema_path = f.name

        try:
            result = _run_source_pipeline(
                SAMPLE_PYTHON_SOURCE,
                existing_schema_path=schema_path,
                fail_on_regression=True,
            )
            mc_stage = next(s for s in result.stages if s.stage == "migrate_check")
            # Should fail because the existing field was "removed"
            assert not mc_stage.success
        finally:
            Path(schema_path).unlink(missing_ok=True)


class TestExtractStageEdgeCases:
    def test_nonexistent_source_file(self):
        """Extract should fail gracefully for missing files."""
        config = PipelineConfig(
            source_mode=True,
            source_path="/nonexistent/file.py",
        )
        pipeline = Pipeline(config)
        pipeline.register_stage(ExtractStage())
        result = pipeline.run("/nonexistent/file.py")
        assert not result.success

    def test_unsupported_extension(self):
        """Extract should fail for unsupported file extensions."""
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".go", delete=False, encoding="utf-8"
        ) as f:
            f.write("package main\n")
            tmp_path = f.name

        try:
            config = PipelineConfig(
                source_mode=True,
                source_path=tmp_path,
            )
            pipeline = Pipeline(config)
            pipeline.register_stage(ExtractStage())
            result = pipeline.run(tmp_path)
            assert not result.success
        finally:
            Path(tmp_path).unlink(missing_ok=True)
