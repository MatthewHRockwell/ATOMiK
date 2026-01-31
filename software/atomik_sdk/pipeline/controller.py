"""
ATOMiK Pipeline Controller

Orchestrates the autonomous pipeline: schema intake, stage dispatch,
token accounting, metrics aggregation, and failure routing.

Supports both sequential (Phase 4C) and event-driven DAG (Phase 5)
execution modes. The orchestrator is used when use_orchestrator=True
in PipelineConfig.
"""

from __future__ import annotations

import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .stages import BaseStage, StageManifest, StageStatus


@dataclass
class PipelineConfig:
    """Configuration for a pipeline run."""
    output_dir: str = "generated"
    languages: list[str] | None = None
    verbose: bool = False
    report_path: str | None = None
    checkpoint_dir: str = ".atomik"
    metrics_csv: str = ".atomik/metrics.csv"
    com_port: str | None = None
    token_budget: int | None = None
    sim_only: bool = False
    skip_synthesis: bool = False
    dry_run: bool = False
    use_orchestrator: bool = False
    max_workers: int = 1
    fail_on_regression: bool = False
    # Source-mode fields
    source_mode: bool = False
    source_path: str | None = None
    source_language: str = "auto"
    existing_schema_path: str | None = None
    inference_overrides: dict[str, Any] = field(default_factory=dict)


@dataclass
class PipelineResult:
    """Result of a complete pipeline run."""
    schema: str
    success: bool
    stages: list[StageManifest] = field(default_factory=list)
    total_time_ms: float = 0.0
    total_tokens: int = 0
    files_generated: int = 0
    lines_generated: int = 0
    validation_level: str = "none"
    errors: list[str] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return {
            "schema": self.schema,
            "success": self.success,
            "total_time_ms": round(self.total_time_ms, 1),
            "total_tokens": self.total_tokens,
            "files_generated": self.files_generated,
            "lines_generated": self.lines_generated,
            "validation_level": self.validation_level,
            "stages": [s.to_dict() for s in self.stages],
            "errors": self.errors,
        }


class Pipeline:
    """
    Autonomous pipeline controller.

    Dispatches work through a sequence of stages, tracks progress,
    manages token budgets, and produces structured reports.

    Example:
        >>> pipeline = Pipeline()
        >>> result = pipeline.run("sdk/schemas/domains/video-h264-delta.json")
        >>> print(f"Success: {result.success}, Files: {result.files_generated}")
    """

    # Default stage ordering
    STAGE_ORDER = [
        "validate",
        "diff",
        "generate",
        "verify",
        "hardware",
        "metrics",
    ]

    # Stage ordering for source-mode (extract → infer → migrate_check → ...)
    SOURCE_STAGE_ORDER = [
        "extract",
        "infer",
        "migrate_check",
        "validate",
        "diff",
        "generate",
        "verify",
        "hardware",
        "metrics",
    ]

    def __init__(self, config: PipelineConfig | None = None):
        self.config = config or PipelineConfig()
        self._stages: dict[str, BaseStage] = {}
        self._stage_deps: dict[str, list[str]] = {}
        self._results: list[PipelineResult] = []

    def register_stage(
        self, stage: BaseStage, dependencies: list[str] | None = None
    ) -> None:
        """Register a pipeline stage with optional dependencies."""
        self._stages[stage.name] = stage
        if dependencies is not None:
            self._stage_deps[stage.name] = dependencies

    def run(self, schema_path: str | Path) -> PipelineResult:
        """
        Execute the full pipeline on a single schema.

        Args:
            schema_path: Path to schema JSON file, or source file in source mode.

        Returns:
            PipelineResult with per-stage results and aggregate metrics.
        """
        schema_path = Path(schema_path)
        result = PipelineResult(schema=schema_path.name, success=True)
        start = time.perf_counter()

        # Source mode: schema starts empty, populated by infer stage
        if self.config.source_mode:
            schema: dict[str, Any] = {}
            stage_order = self.SOURCE_STAGE_ORDER
            if self.config.verbose:
                print(f"Pipeline: source mode — {schema_path.name}")
        else:
            # Load schema from JSON
            try:
                with open(schema_path, encoding="utf-8") as f:
                    schema = json.load(f)
            except (FileNotFoundError, json.JSONDecodeError) as e:
                result.success = False
                result.errors.append(f"Schema load failed: {e}")
                return result
            stage_order = self.STAGE_ORDER
            if self.config.verbose:
                print(f"Pipeline: processing {schema_path.name}")

        if self.config.dry_run:
            return self._dry_run(schema, str(schema_path), result)

        # Delegate to orchestrator when enabled
        if self.config.use_orchestrator:
            return self._run_orchestrated(schema, str(schema_path), result, start)

        # Execute stages in order
        previous_manifest = None
        for stage_name in stage_order:
            stage = self._stages.get(stage_name)
            if stage is None:
                continue

            # Check token budget before each stage
            if self.config.token_budget is not None:
                spent = sum(s.tokens_consumed for s in result.stages)
                if spent >= self.config.token_budget:
                    result.errors.append(
                        f"Token budget exhausted ({spent}/{self.config.token_budget})"
                    )
                    result.success = False
                    break

            if self.config.verbose:
                print(f"  Stage: {stage_name}...")

            manifest = stage.execute(
                schema, str(schema_path), previous_manifest, self.config
            )
            result.stages.append(manifest)

            if self.config.verbose:
                status = manifest.status.value.upper()
                ms = round(manifest.duration_ms, 1)
                print(f"  Stage: {stage_name} -> {status} ({ms}ms)")

            # Abort pipeline on failure (unless skipped)
            if manifest.status == StageStatus.FAILED:
                result.success = False
                result.errors.extend(manifest.errors)
                break

            # Propagate short-circuit from diff stage
            if manifest.status == StageStatus.SKIPPED:
                if self.config.verbose:
                    reason = manifest.metrics.get("skip_reason", "up-to-date")
                    print(f"  Pipeline short-circuited: {reason}")
                break

            previous_manifest = manifest

        # Aggregate metrics
        result.total_time_ms = (time.perf_counter() - start) * 1000
        result.total_tokens = sum(s.tokens_consumed for s in result.stages)

        for stage in result.stages:
            result.files_generated += stage.metrics.get("files_generated", 0)
            result.lines_generated += stage.metrics.get("lines_generated", 0)

        # Determine validation level from hardware stage
        hw_stages = [s for s in result.stages if s.stage == "hardware"]
        if hw_stages:
            result.validation_level = hw_stages[0].validation_level
        else:
            verify_stages = [s for s in result.stages if s.stage == "verify"]
            if verify_stages and verify_stages[0].success:
                result.validation_level = "sw_verified"

        self._results.append(result)

        # Write report if requested
        if self.config.report_path:
            self._write_report(result)

        return result

    def run_batch(self, directory: str | Path) -> list[PipelineResult]:
        """
        Execute the pipeline on all schemas in a directory.

        Args:
            directory: Path to directory containing schema JSON files.

        Returns:
            List of PipelineResult for each schema.
        """
        directory = Path(directory)
        schemas = sorted(directory.glob("*.json"))

        if not schemas:
            return []

        if self.config.verbose:
            print(f"Pipeline: batch processing {len(schemas)} schema(s)")

        results = []
        for schema_path in schemas:
            result = self.run(schema_path)
            results.append(result)

        return results

    def _dry_run(
        self, schema: dict[str, Any], schema_path: str, result: PipelineResult
    ) -> PipelineResult:
        """Show what would be done without executing."""
        catalogue = schema.get("catalogue", {})
        ns = f"{catalogue.get('vertical')}.{catalogue.get('field')}.{catalogue.get('object')}"

        print(f"Dry run for: {result.schema}")
        print(f"  Namespace: {ns}")
        print(f"  Stages: {', '.join(self.STAGE_ORDER)}")
        print(f"  Languages: {self.config.languages or 'all'}")
        print(f"  Output: {self.config.output_dir}")

        registered = list(self._stages.keys())
        missing = [s for s in self.STAGE_ORDER if s not in registered]
        if missing:
            print(f"  Missing stages: {', '.join(missing)}")

        return result

    def _write_report(self, result: PipelineResult) -> None:
        """Write pipeline report to JSON file."""
        path = Path(self.config.report_path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(result.to_dict(), f, indent=2)

    def _run_orchestrated(
        self,
        schema: dict[str, Any],
        schema_path: str,
        result: PipelineResult,
        start: float,
    ) -> PipelineResult:
        """Execute pipeline via the event-driven orchestrator."""
        from .orchestrator import Orchestrator

        stage_order = (
            self.SOURCE_STAGE_ORDER if self.config.source_mode
            else self.STAGE_ORDER
        )

        orch = Orchestrator(max_workers=self.config.max_workers)

        # Build default linear dependencies if none declared
        if not self._stage_deps:
            prev = None
            for name in stage_order:
                if name in self._stages:
                    deps = [prev] if prev else []
                    self._stage_deps[name] = deps
                    prev = name

        for name, stage in self._stages.items():
            deps = self._stage_deps.get(name, [])
            orch.register_stage(stage, dependencies=deps)

        manifests = orch.execute(schema, schema_path, self.config)

        # Convert orchestrator output to PipelineResult
        for stage_name in stage_order:
            if stage_name in manifests:
                result.stages.append(manifests[stage_name])

        # Check for failures
        for manifest in result.stages:
            if manifest.status == StageStatus.FAILED:
                result.success = False
                result.errors.extend(manifest.errors)

        # Aggregate metrics
        result.total_time_ms = (time.perf_counter() - start) * 1000
        result.total_tokens = sum(s.tokens_consumed for s in result.stages)

        for stage in result.stages:
            result.files_generated += stage.metrics.get("files_generated", 0)
            result.lines_generated += stage.metrics.get("lines_generated", 0)

        # Determine validation level
        hw_stages = [s for s in result.stages if s.stage == "hardware"]
        if hw_stages:
            result.validation_level = hw_stages[0].validation_level
        else:
            verify_stages = [s for s in result.stages if s.stage == "verify"]
            if verify_stages and verify_stages[0].success:
                result.validation_level = "sw_verified"

        self._results.append(result)

        if self.config.report_path:
            self._write_report(result)

        return result

    def get_status(self) -> dict[str, Any]:
        """Get current pipeline status summary."""
        return {
            "total_runs": len(self._results),
            "successful": sum(1 for r in self._results if r.success),
            "failed": sum(1 for r in self._results if not r.success),
            "total_tokens": sum(r.total_tokens for r in self._results),
            "total_files": sum(r.files_generated for r in self._results),
            "stages_registered": list(self._stages.keys()),
        }
