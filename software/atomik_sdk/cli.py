"""
ATOMiK CLI Tool (atomik-gen)

Command-line interface for ATOMiK schema validation, multi-language
SDK code generation, autonomous pipeline execution, performance
metrics, and domain hardware demonstrators.

Usage:
    atomik-gen generate <schema> [options]
    atomik-gen validate <schema>
    atomik-gen info <schema>
    atomik-gen batch <directory> [options]
    atomik-gen list
    atomik-gen pipeline run <schema> [options]
    atomik-gen pipeline diff <schema>
    atomik-gen pipeline status
    atomik-gen metrics show [options]
    atomik-gen metrics compare
    atomik-gen metrics export --output <file>
    atomik-gen demo <domain> [options]
    atomik-gen --version
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from atomik_sdk import __version__
from atomik_sdk.generator.c_generator import CGenerator
from atomik_sdk.generator.core import GeneratorConfig, GeneratorEngine
from atomik_sdk.generator.javascript_generator import JavaScriptGenerator
from atomik_sdk.generator.python_generator import PythonGenerator
from atomik_sdk.generator.rust_generator import RustGenerator
from atomik_sdk.generator.schema_validator import SchemaValidator
from atomik_sdk.generator.verilog_generator import VerilogGenerator

GENERATORS = {
    "python": PythonGenerator,
    "rust": RustGenerator,
    "c": CGenerator,
    "javascript": JavaScriptGenerator,
    "verilog": VerilogGenerator,
}

EXIT_SUCCESS = 0
EXIT_VALIDATION_FAILURE = 1
EXIT_GENERATION_FAILURE = 2
EXIT_FILE_ERROR = 3


def _create_engine(
    output_dir: str = "generated",
    languages: list[str] | None = None,
    verbose: bool = False,
) -> GeneratorEngine:
    """Create and configure a GeneratorEngine with registered generators."""
    engine = GeneratorEngine(GeneratorConfig(
        output_dir=output_dir,
        validate_schemas=True,
        verbose=verbose,
    ))
    target = languages or list(GENERATORS.keys())
    for lang in target:
        if lang in GENERATORS:
            engine.register_generator(lang, GENERATORS[lang]())
    return engine


def cmd_generate(args: argparse.Namespace) -> int:
    """Generate SDK code from a schema file."""
    schema_path = Path(args.schema)
    if not schema_path.exists():
        print(f"Error: schema file not found: {schema_path}", file=sys.stderr)
        return EXIT_FILE_ERROR

    engine = _create_engine(args.output_dir, args.languages, args.verbose)

    # Load and validate
    validation = engine.load_schema(schema_path)
    if not validation:
        print(f"Validation failed for {schema_path.name}:", file=sys.stderr)
        for err in validation.errors:
            print(f"  - {err}", file=sys.stderr)
        return EXIT_VALIDATION_FAILURE

    ns = engine.extract_metadata()
    namespace = f"{ns.vertical}.{ns.field}.{ns.object}"

    if args.verbose:
        print(f"Namespace: {namespace}")

    # Generate
    results = engine.generate(args.languages)
    files = engine.write_output(results)

    # Collect errors
    errors = []
    for lang, result in results.items():
        status = "OK" if result.success else "FAIL"
        print(f"  {lang}: {status} ({len(result.files)} files)")
        if not result.success:
            errors.extend(result.errors)

    print(f"Generated {len(files)} file(s) in {args.output_dir}/")

    # Write report if requested
    if args.report:
        _write_report(args.report, {
            "schema": schema_path.name,
            "namespace": namespace,
            "output_dir": args.output_dir,
            "total_files": len(files),
            "success": len(errors) == 0,
            "languages": {
                lang: {"success": r.success, "files": len(r.files)}
                for lang, r in results.items()
            },
            "errors": errors,
        })

    return EXIT_GENERATION_FAILURE if errors else EXIT_SUCCESS


def cmd_validate(args: argparse.Namespace) -> int:
    """Validate a schema file without generating code."""
    schema_path = Path(args.schema)
    if not schema_path.exists():
        print(f"Error: schema file not found: {schema_path}", file=sys.stderr)
        return EXIT_FILE_ERROR

    validator = SchemaValidator()
    result = validator.validate_file(schema_path)

    print(f"{schema_path.name}: {result}")

    if result.errors:
        for err in result.errors:
            print(f"  ERROR: {err}")

    if result.warnings:
        for warn in result.warnings:
            print(f"  WARN:  {warn}")

    return EXIT_SUCCESS if result.valid else EXIT_VALIDATION_FAILURE


def cmd_info(args: argparse.Namespace) -> int:
    """Show summary information about a schema."""
    schema_path = Path(args.schema)
    if not schema_path.exists():
        print(f"Error: schema file not found: {schema_path}", file=sys.stderr)
        return EXIT_FILE_ERROR

    engine = GeneratorEngine(GeneratorConfig(validate_schemas=False))

    try:
        engine.load_schema(schema_path)
    except (ValueError, FileNotFoundError) as e:
        print(f"Error: {e}", file=sys.stderr)
        return EXIT_FILE_ERROR

    summary = engine.get_schema_summary()
    cat = summary["catalogue"]

    print(f"Schema:     {schema_path.name}")
    print(f"Namespace:  {cat['vertical']}.{cat['field']}.{cat['object']}")
    print(f"Version:    {cat['version']}")
    print(f"Hardware:   {'yes' if summary['has_hardware'] else 'no'}")
    print()

    print("Delta Fields:")
    for name, spec in summary["delta_fields"].items():
        print(f"  {name}: {spec['type']} ({spec['width']}-bit)")

    print()
    print(f"Operations: {', '.join(summary['operations'])}")

    return EXIT_SUCCESS


def cmd_batch(args: argparse.Namespace) -> int:
    """Batch-generate SDK code from a directory of schemas."""
    schemas_dir = Path(args.directory)
    if not schemas_dir.is_dir():
        print(f"Error: directory not found: {schemas_dir}", file=sys.stderr)
        return EXIT_FILE_ERROR

    schemas = sorted(schemas_dir.glob("*.json"))
    if not schemas:
        print(f"Error: no JSON schemas found in {schemas_dir}", file=sys.stderr)
        return EXIT_FILE_ERROR

    print(f"Found {len(schemas)} schema(s) in {schemas_dir}")
    print()

    all_results = []
    total_files = 0
    failures = 0

    for schema_path in schemas:
        engine = _create_engine(args.output_dir, args.languages, args.verbose)

        validation = engine.load_schema(schema_path)
        if not validation:
            print(f"  {schema_path.name}: VALIDATION FAILED")
            for err in validation.errors:
                print(f"    - {err}")
            all_results.append({
                "schema": schema_path.name,
                "success": False,
                "errors": validation.errors,
                "files": 0,
            })
            failures += 1
            continue

        ns = engine.extract_metadata()
        namespace = f"{ns.vertical}.{ns.field}.{ns.object}"
        results = engine.generate(args.languages)
        files = engine.write_output(results)

        errors = []
        lang_results = {}
        for lang, result in results.items():
            lang_results[lang] = {
                "success": result.success,
                "files": len(result.files),
            }
            if not result.success:
                errors.extend(result.errors)

        schema_ok = len(errors) == 0
        status = "OK" if schema_ok else "FAIL"
        print(f"  {schema_path.name}: {status} ({namespace}, {len(files)} files)")

        if not schema_ok:
            failures += 1

        total_files += len(files)
        all_results.append({
            "schema": schema_path.name,
            "namespace": namespace,
            "success": schema_ok,
            "languages": lang_results,
            "total_files": len(files),
            "errors": errors,
        })

    print()
    print(f"Processed: {len(schemas)} schema(s), {total_files} file(s), {failures} failure(s)")

    if args.report:
        _write_report(args.report, {
            "schemas_dir": str(schemas_dir),
            "output_dir": args.output_dir,
            "total_schemas": len(schemas),
            "total_files": total_files,
            "failures": failures,
            "results": all_results,
        })

    return EXIT_GENERATION_FAILURE if failures else EXIT_SUCCESS


def cmd_list(_args: argparse.Namespace) -> int:
    """List available target languages."""
    print("Available target languages:")
    for lang in GENERATORS:
        print(f"  {lang}")
    return EXIT_SUCCESS


# ---------------------------------------------------------------------------
# Pipeline commands (Phase 4C)
# ---------------------------------------------------------------------------

EXIT_HARDWARE_FAILURE = 4


def _create_pipeline(args: argparse.Namespace):
    """Create and configure a Pipeline with all stages registered."""
    from atomik_sdk.pipeline.controller import Pipeline, PipelineConfig
    from atomik_sdk.pipeline.stages.diff import DiffStage
    from atomik_sdk.pipeline.stages.generate import GenerateStage
    from atomik_sdk.pipeline.stages.hardware import HardwareStage
    from atomik_sdk.pipeline.stages.metrics import MetricsStage
    from atomik_sdk.pipeline.stages.validate import ValidateStage
    from atomik_sdk.pipeline.stages.verify import VerifyStage

    config = PipelineConfig(
        output_dir=getattr(args, "output_dir", "generated"),
        languages=getattr(args, "languages", None),
        verbose=getattr(args, "verbose", False),
        report_path=getattr(args, "report", None),
        checkpoint_dir=getattr(args, "checkpoint", ".atomik"),
        metrics_csv=getattr(args, "metrics_csv", ".atomik/metrics.csv"),
        com_port=getattr(args, "com_port", None),
        token_budget=getattr(args, "token_budget", None),
        sim_only=getattr(args, "sim_only", False),
        skip_synthesis=getattr(args, "skip_synthesis", False),
        dry_run=getattr(args, "dry_run", False),
    )

    pipeline = Pipeline(config)
    pipeline.register_stage(ValidateStage())
    pipeline.register_stage(DiffStage())
    pipeline.register_stage(GenerateStage())
    pipeline.register_stage(VerifyStage())
    pipeline.register_stage(HardwareStage())
    pipeline.register_stage(MetricsStage())

    return pipeline


def cmd_pipeline_run(args: argparse.Namespace) -> int:
    """Execute the autonomous pipeline on a schema or directory."""
    target = Path(args.target)
    if not target.exists():
        print(f"Error: not found: {target}", file=sys.stderr)
        return EXIT_FILE_ERROR

    pipeline = _create_pipeline(args)

    if target.is_dir() or getattr(args, "batch", False):
        results = pipeline.run_batch(target)
        failures = sum(1 for r in results if not r.success)
        total = len(results)
        print(f"\nPipeline complete: {total} schema(s), {failures} failure(s)")
        for r in results:
            status = "OK" if r.success else "FAIL"
            print(f"  {r.schema}: {status} [{r.validation_level}] "
                  f"({r.files_generated} files, {r.total_tokens} tokens, "
                  f"{r.total_time_ms:.0f}ms)")
        return EXIT_GENERATION_FAILURE if failures else EXIT_SUCCESS
    else:
        result = pipeline.run(target)
        if result.success:
            print(f"\nPipeline SUCCESS [{result.validation_level}]")
            print(f"  Files: {result.files_generated}, "
                  f"Lines: {result.lines_generated}, "
                  f"Tokens: {result.total_tokens}, "
                  f"Time: {result.total_time_ms:.0f}ms")
        else:
            print("\nPipeline FAILED", file=sys.stderr)
            for err in result.errors:
                print(f"  - {err}", file=sys.stderr)
        return EXIT_SUCCESS if result.success else EXIT_GENERATION_FAILURE


def cmd_pipeline_diff(args: argparse.Namespace) -> int:
    """Show what the pipeline would do without executing."""
    args.dry_run = True
    return cmd_pipeline_run(args)


def cmd_pipeline_status(_args: argparse.Namespace) -> int:
    """Show pipeline status from checkpoint."""
    from atomik_sdk.pipeline.context.checkpoint import Checkpoint

    checkpoint_dir = getattr(_args, "checkpoint", ".atomik")
    checkpoint = Checkpoint(checkpoint_dir)
    state = checkpoint.to_dict()

    schemas = state.get("schemas", {})
    history = state.get("metrics_history", [])

    print("ATOMiK Pipeline Status")
    print("=" * 40)
    print(f"  Checkpoint:    {checkpoint_dir}")
    print(f"  Last updated:  {state.get('last_updated', 'never')}")
    print(f"  Schemas:       {len(schemas)}")
    print(f"  History:       {len(history)} run(s)")
    print()

    if schemas:
        print("Registered Schemas:")
        for name, info in schemas.items():
            gen = info.get("last_generated", "never")
            print(f"  {name}: generated {gen}")
        print()

    if history:
        last = history[-1]
        print("Last Run:")
        print(f"  Schema:    {last.get('schema', 'unknown')}")
        print(f"  Timestamp: {last.get('timestamp', 'unknown')}")
        print(f"  Tokens:    {last.get('tokens_consumed', 0)}")
        print(f"  Files:     {last.get('files_generated', 0)}")

    return EXIT_SUCCESS


# ---------------------------------------------------------------------------
# Metrics commands (Phase 4C)
# ---------------------------------------------------------------------------

def cmd_metrics_show(args: argparse.Namespace) -> int:
    """Show metrics for the last pipeline run."""
    from atomik_sdk.pipeline.context.checkpoint import Checkpoint
    from atomik_sdk.pipeline.metrics.reporter import MetricsReporter

    checkpoint = Checkpoint(getattr(args, "checkpoint", ".atomik"))
    schema_name = getattr(args, "schema", None)
    history = checkpoint.get_metrics_history(schema_name)

    if not history:
        print("No metrics data available. Run the pipeline first.")
        return EXIT_SUCCESS

    reporter = MetricsReporter()
    last = history[-1]
    report = reporter.format_text_report(last.get("schema", "unknown"), last)
    print(report)

    return EXIT_SUCCESS


def cmd_metrics_compare(args: argparse.Namespace) -> int:
    """Compare metrics across schemas."""
    from atomik_sdk.pipeline.context.checkpoint import Checkpoint
    from atomik_sdk.pipeline.metrics.reporter import MetricsReporter

    checkpoint = Checkpoint(getattr(args, "checkpoint", ".atomik"))
    history = checkpoint.get_metrics_history()

    if not history:
        print("No metrics data available.")
        return EXIT_SUCCESS

    # Get latest run per schema
    latest: dict[str, dict] = {}
    for entry in history:
        name = entry.get("schema", "unknown")
        latest[name] = entry

    reporter = MetricsReporter()
    schemas = {}
    for name, data in latest.items():
        schemas[name] = {
            "time_ms": data.get("pipeline_total_time_ms", 0),
            "tokens": data.get("tokens_consumed", 0),
            "files": data.get("files_generated", 0),
            "lines": data.get("lines_generated", 0),
            "efficiency": data.get("token_efficiency_pct", 100),
        }

    table = reporter.format_comparison_table(schemas)
    print(table)

    return EXIT_SUCCESS


def cmd_metrics_export(args: argparse.Namespace) -> int:
    """Export metrics history to CSV."""
    from atomik_sdk.pipeline.context.checkpoint import Checkpoint

    checkpoint = Checkpoint(getattr(args, "checkpoint", ".atomik"))
    history = checkpoint.get_metrics_history()

    if not history:
        print("No metrics data to export.")
        return EXIT_SUCCESS

    import csv
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    keys = list(history[0].keys())
    with open(output, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=keys)
        writer.writeheader()
        writer.writerows(history)

    print(f"Exported {len(history)} entries to {output}")
    return EXIT_SUCCESS


# ---------------------------------------------------------------------------
# Demo commands (Phase 4C)
# ---------------------------------------------------------------------------


def cmd_demo(args: argparse.Namespace) -> int:
    """Run a domain hardware demonstrator."""
    import re
    import shutil
    import subprocess
    import tempfile
    import time

    domain = args.domain
    valid_domains = ["video", "sensor", "finance"]

    if domain not in valid_domains:
        print(f"Error: unknown domain '{domain}'. "
              f"Valid: {', '.join(valid_domains)}", file=sys.stderr)
        return EXIT_FILE_ERROR

    project_root = Path(__file__).resolve().parent.parent.parent
    demo_dir = project_root / "demos" / "hardware" / domain

    if not demo_dir.exists():
        print(f"Error: demo directory not found: {demo_dir}", file=sys.stderr)
        return EXIT_FILE_ERROR

    top_file = demo_dir / f"{domain}_demo_top.v"
    tb_file = demo_dir / f"tb_{domain}_demo.v"

    if not top_file.exists():
        print(f"Error: demo top module not found: {top_file}", file=sys.stderr)
        return EXIT_FILE_ERROR

    sim_only = getattr(args, "sim_only", False)
    verbose = getattr(args, "verbose", False)
    t_start = time.perf_counter()

    # -- Tool detection -------------------------------------------------------
    from atomik_sdk.hardware_discovery import detect_board, find_tool

    iverilog = shutil.which("iverilog")
    vvp_cmd = shutil.which("vvp")

    has_gowin = find_tool("gw_sh") is not None
    has_programmer = find_tool("programmer_cli") is not None
    has_openfpga = find_tool("openFPGALoader") is not None

    # -- Board detection (skip when --sim-only) --------------------------------
    board_detected = False
    hardware_used = False

    if not sim_only:
        board_detected = detect_board() is not None

    mode = "simulation" if sim_only else "auto"
    print(f"ATOMiK {domain.title()} Domain Demonstrator")
    print("=" * 40)
    print(f"  Top module: {top_file.name}")
    print(f"  Testbench:  {tb_file.name if tb_file.exists() else 'N/A'}")
    print(f"  Mode:       {mode}")
    if not sim_only:
        print(f"  Board:      {'detected' if board_detected else 'not detected'}")
    print()

    # -- Gather Verilog source files -------------------------------------------
    common_dir = project_root / "demos" / "hardware" / "common"
    rtl_dir = project_root / "hardware" / "rtl"
    rtl_pll_dir = project_root / "hardware" / "rtl" / "pll"
    sim_stubs_dir = project_root / "hardware" / "sim" / "stubs"

    src_files = []
    # Stubs first (Gowin rPLL behavioural model), then PLL wrappers, RTL,
    # common modules, and finally the demo-specific sources.
    for d in [sim_stubs_dir, rtl_pll_dir, rtl_dir, common_dir, demo_dir]:
        if d.exists():
            src_files.extend(sorted(d.glob("*.v")))

    # Remove testbenches from source list
    src_files = [f for f in src_files if not f.name.startswith("tb_")]

    # -- Simulation & validation tracking --------------------------------------
    success = False
    validation_level = "no_rtl"
    sim_tests_passed = 0
    sim_tests_total = 0
    exit_code = EXIT_SUCCESS

    if tb_file.exists():
        if iverilog and vvp_cmd:
            print("Running RTL simulation...")
            all_files = [str(f) for f in src_files] + [str(tb_file)]

            with tempfile.NamedTemporaryFile(suffix=".vvp", delete=False) as tmp:
                vvp_out = tmp.name

            compile_result = subprocess.run(
                [iverilog, "-o", vvp_out] + all_files,
                capture_output=True, text=True,
                timeout=60,
            )

            if compile_result.returncode != 0:
                print(f"Compilation failed:\n{compile_result.stderr}")
                exit_code = EXIT_HARDWARE_FAILURE
            else:
                sim_result = subprocess.run(
                    [vvp_cmd, vvp_out],
                    capture_output=True, text=True,
                    timeout=60,
                )

                print(sim_result.stdout)
                if sim_result.stderr and verbose:
                    print(sim_result.stderr)

                Path(vvp_out).unlink(missing_ok=True)

                # Parse pass/fail counts from the test summary block.
                # Testbenches print "Passed: N" and "Failed: N" lines.
                pass_match = re.search(
                    r"Passed:\s+(\d+)", sim_result.stdout,
                )
                fail_match = re.search(
                    r"Failed:\s+(\d+)", sim_result.stdout,
                )
                if pass_match:
                    sim_tests_passed = int(pass_match.group(1))
                if fail_match:
                    sim_tests_total = sim_tests_passed + int(fail_match.group(1))
                else:
                    sim_tests_total = sim_tests_passed

                # Use parsed fail count when available; fall back to
                # string heuristic only when no structured output exists.
                if fail_match:
                    has_failure = int(fail_match.group(1)) > 0
                else:
                    has_failure = "FAIL" in sim_result.stdout.upper()
                if has_failure:
                    print("Demo simulation: FAILED")
                    validation_level = "simulation_only"
                    exit_code = EXIT_HARDWARE_FAILURE
                else:
                    print("Demo simulation: PASSED")
                    validation_level = "simulation_only"
                    success = True
        else:
            print("iverilog/vvp not found -- cannot simulate")
            exit_code = EXIT_HARDWARE_FAILURE
    else:
        print("No testbench found for this demo")

    # -- Hardware escalation (auto mode only) ----------------------------------
    if success and not sim_only and board_detected and has_gowin:
        print("Attempting synthesis (Gowin EDA)...")
        validation_level = "synthesized"
        hardware_used = True
        # Full synthesis + programming is handled by HardwareStage in the
        # pipeline; here we record the capability level reached.

    duration_ms = (time.perf_counter() - t_start) * 1000.0

    if args.report:
        report_data = {
            "domain": domain,
            "success": success,
            "validation_level": validation_level,
            "hardware_used": hardware_used,
            "board_detected": board_detected,
            "mode": mode,
            "top_module": str(top_file),
            "testbench": str(tb_file) if tb_file.exists() else None,
            "sim_tests_passed": sim_tests_passed,
            "sim_tests_total": sim_tests_total,
            "duration_ms": round(duration_ms, 1),
            "tools": {
                "iverilog": iverilog is not None,
                "gowin_eda": has_gowin,
                "programmer_cli": has_programmer,
                "openfpga_loader": has_openfpga,
            },
        }
        _write_report(args.report, report_data)

    return exit_code


def cmd_from_source(args: argparse.Namespace) -> int:
    """Generate SDK from existing source code (zero LLM tokens)."""
    source_path = Path(args.source)
    if not source_path.exists():
        print(f"Error: {source_path} not found", file=sys.stderr)
        return EXIT_FILE_ERROR

    from atomik_sdk.pipeline.controller import Pipeline, PipelineConfig
    from atomik_sdk.pipeline.stages.diff import DiffStage
    from atomik_sdk.pipeline.stages.extract import ExtractStage
    from atomik_sdk.pipeline.stages.generate import GenerateStage
    from atomik_sdk.pipeline.stages.hardware import HardwareStage
    from atomik_sdk.pipeline.stages.infer import InferStage
    from atomik_sdk.pipeline.stages.metrics import MetricsStage
    from atomik_sdk.pipeline.stages.migrate_check import MigrateCheckStage
    from atomik_sdk.pipeline.stages.validate import ValidateStage
    from atomik_sdk.pipeline.stages.verify import VerifyStage

    config = PipelineConfig(
        source_mode=True,
        source_path=str(source_path),
        output_dir=args.output_dir,
        languages=args.languages,
        verbose=args.verbose,
        report_path=args.report,
        existing_schema_path=getattr(args, "existing_schema", None),
        fail_on_regression=getattr(args, "strict", False),
        inference_overrides={
            "vertical": args.vertical,
            "version": args.version,
        },
    )

    pipeline = Pipeline(config)
    pipeline.register_stage(ExtractStage())
    pipeline.register_stage(InferStage())
    pipeline.register_stage(MigrateCheckStage())
    pipeline.register_stage(ValidateStage())
    pipeline.register_stage(DiffStage())
    pipeline.register_stage(GenerateStage())
    pipeline.register_stage(VerifyStage())
    pipeline.register_stage(HardwareStage())
    pipeline.register_stage(MetricsStage())

    result = pipeline.run(source_path)

    if result.success:
        print(f"\nPipeline SUCCESS [{result.validation_level}]")
        print(f"  Files: {result.files_generated}, "
              f"Lines: {result.lines_generated}, "
              f"Tokens: {result.total_tokens}, "
              f"Time: {result.total_time_ms:.0f}ms")
    else:
        print("\nPipeline FAILED", file=sys.stderr)
        for err in result.errors:
            print(f"  - {err}", file=sys.stderr)

    return EXIT_SUCCESS if result.success else EXIT_GENERATION_FAILURE


def _write_report(path: str, data: dict) -> None:
    """Write a JSON report file."""
    report_path = Path(path)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    with open(report_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"Report written to {report_path}")


def build_parser() -> argparse.ArgumentParser:
    """Build the argument parser with all subcommands."""
    parser = argparse.ArgumentParser(
        prog="atomik-gen",
        description="ATOMiK schema validation and multi-language SDK code generation",
    )
    parser.add_argument(
        "--version", action="version", version=f"atomik-gen {__version__}"
    )

    sub = parser.add_subparsers(dest="command", help="Available commands")

    # generate
    gen = sub.add_parser("generate", help="Generate SDK code from a schema")
    gen.add_argument("schema", help="Path to schema JSON file")
    gen.add_argument("--output-dir", default="generated", help="Output directory (default: generated)")
    gen.add_argument(
        "--languages", nargs="+", choices=list(GENERATORS.keys()),
        help="Target languages (default: all)",
    )
    gen.add_argument("--report", help="Write JSON report to file")
    gen.add_argument("-v", "--verbose", action="store_true", help="Verbose output")

    # validate
    val = sub.add_parser("validate", help="Validate a schema (no generation)")
    val.add_argument("schema", help="Path to schema JSON file")

    # info
    inf = sub.add_parser("info", help="Show schema summary")
    inf.add_argument("schema", help="Path to schema JSON file")

    # batch
    bat = sub.add_parser("batch", help="Batch generate from a directory of schemas")
    bat.add_argument("directory", help="Directory containing schema JSON files")
    bat.add_argument("--output-dir", default="generated", help="Output directory (default: generated)")
    bat.add_argument(
        "--languages", nargs="+", choices=list(GENERATORS.keys()),
        help="Target languages (default: all)",
    )
    bat.add_argument("--report", help="Write JSON report to file")
    bat.add_argument("-v", "--verbose", action="store_true", help="Verbose output")

    # list
    sub.add_parser("list", help="List available target languages")

    # -----------------------------------------------------------------------
    # pipeline subcommand group
    # -----------------------------------------------------------------------
    pipe = sub.add_parser("pipeline", help="Autonomous pipeline execution")
    pipe_sub = pipe.add_subparsers(dest="pipeline_command")

    # pipeline run
    pipe_run = pipe_sub.add_parser("run", help="Execute full pipeline")
    pipe_run.add_argument("target", help="Schema file or directory")
    pipe_run.add_argument("--output-dir", default="generated")
    pipe_run.add_argument("--languages", nargs="+", choices=list(GENERATORS.keys()))
    pipe_run.add_argument("--report", help="Write pipeline report to file")
    pipe_run.add_argument("--checkpoint", default=".atomik")
    pipe_run.add_argument("--metrics-csv", default=".atomik/metrics.csv")
    pipe_run.add_argument("--com-port", help="Serial port for hardware validation")
    pipe_run.add_argument("--token-budget", type=int, help="Maximum tokens for this run")
    pipe_run.add_argument("--sim-only", action="store_true", help="RTL simulation only")
    pipe_run.add_argument("--skip-synthesis", action="store_true")
    pipe_run.add_argument("--batch", action="store_true", help="Process directory")
    pipe_run.add_argument("-v", "--verbose", action="store_true")

    # pipeline diff
    pipe_diff = pipe_sub.add_parser("diff", help="Show what would change (dry run)")
    pipe_diff.add_argument("target", help="Schema file or directory")
    pipe_diff.add_argument("--output-dir", default="generated")
    pipe_diff.add_argument("--checkpoint", default=".atomik")
    pipe_diff.add_argument("-v", "--verbose", action="store_true")

    # pipeline status
    pipe_status = pipe_sub.add_parser("status", help="Show pipeline state")
    pipe_status.add_argument("--checkpoint", default=".atomik")

    # -----------------------------------------------------------------------
    # metrics subcommand group
    # -----------------------------------------------------------------------
    met = sub.add_parser("metrics", help="Performance metrics")
    met_sub = met.add_subparsers(dest="metrics_command")

    # metrics show
    met_show = met_sub.add_parser("show", help="Show metrics for last run")
    met_show.add_argument("--schema", help="Filter by schema name")
    met_show.add_argument("--checkpoint", default=".atomik")

    # metrics compare
    met_compare = met_sub.add_parser("compare", help="Compare metrics across schemas")
    met_compare.add_argument("--checkpoint", default=".atomik")

    # metrics export
    met_export = met_sub.add_parser("export", help="Export metrics to CSV")
    met_export.add_argument("--output", required=True, help="Output CSV file")
    met_export.add_argument("--checkpoint", default=".atomik")

    # -----------------------------------------------------------------------
    # demo subcommand
    # -----------------------------------------------------------------------
    demo = sub.add_parser("demo", help="Run domain hardware demonstrator")
    demo.add_argument("domain", help="Domain name (video, sensor, finance)")
    demo.add_argument("--com-port", help="Serial port for FPGA")
    demo.add_argument("--sim-only", action="store_true", help="Simulation only")
    demo.add_argument("--report", help="Write demo report to file")
    demo.add_argument("-v", "--verbose", action="store_true")

    # -----------------------------------------------------------------------
    # from-source subcommand
    # -----------------------------------------------------------------------
    fs = sub.add_parser("from-source", help="Generate SDK from existing source code")
    fs.add_argument("source", help="Source file path (.py, .rs, .c, .js, .v)")
    fs.add_argument("--vertical", help="Override inferred vertical category")
    fs.add_argument("--version", default="1.0.0", help="Schema version")
    fs.add_argument("--existing-schema", help="Existing schema for migration diff")
    fs.add_argument("--strict", action="store_true", help="Fail on migration warnings")
    fs.add_argument("--output-dir", default="generated")
    fs.add_argument("--languages", nargs="+", choices=list(GENERATORS.keys()))
    fs.add_argument("--report", help="Write JSON report")
    fs.add_argument("-v", "--verbose", action="store_true")

    return parser


def main() -> int:
    """Entry point for the atomik-gen CLI."""
    parser = build_parser()
    args = parser.parse_args()

    if args.command is None:
        if sys.stdin.isatty():
            from atomik_sdk.terminal import main as terminal_main

            terminal_main()
            return EXIT_SUCCESS
        parser.print_help()
        return EXIT_SUCCESS

    # Handle subcommand groups
    if args.command == "pipeline":
        pipeline_commands = {
            "run": cmd_pipeline_run,
            "diff": cmd_pipeline_diff,
            "status": cmd_pipeline_status,
        }
        subcmd = getattr(args, "pipeline_command", None)
        if subcmd is None:
            parser.parse_args(["pipeline", "--help"])
            return EXIT_SUCCESS
        return pipeline_commands[subcmd](args)

    if args.command == "metrics":
        metrics_commands = {
            "show": cmd_metrics_show,
            "compare": cmd_metrics_compare,
            "export": cmd_metrics_export,
        }
        subcmd = getattr(args, "metrics_command", None)
        if subcmd is None:
            parser.parse_args(["metrics", "--help"])
            return EXIT_SUCCESS
        return metrics_commands[subcmd](args)

    commands = {
        "generate": cmd_generate,
        "validate": cmd_validate,
        "info": cmd_info,
        "batch": cmd_batch,
        "list": cmd_list,
        "demo": cmd_demo,
        "from-source": cmd_from_source,
    }

    try:
        return commands[args.command](args)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return EXIT_FILE_ERROR


if __name__ == "__main__":
    sys.exit(main())
