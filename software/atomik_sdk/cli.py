"""
ATOMiK CLI Tool (atomik-gen)

Command-line interface for ATOMiK schema validation and multi-language
SDK code generation. Wraps the GeneratorEngine API as a pip-installable
entry point.

Usage:
    atomik-gen generate <schema> [options]
    atomik-gen validate <schema>
    atomik-gen info <schema>
    atomik-gen batch <directory> [options]
    atomik-gen list
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

    return parser


def main() -> int:
    """Entry point for the atomik-gen CLI."""
    parser = build_parser()
    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        return EXIT_SUCCESS

    commands = {
        "generate": cmd_generate,
        "validate": cmd_validate,
        "info": cmd_info,
        "batch": cmd_batch,
        "list": cmd_list,
    }

    try:
        return commands[args.command](args)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return EXIT_FILE_ERROR


if __name__ == "__main__":
    sys.exit(main())
