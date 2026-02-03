#!/usr/bin/env python3
"""
ATOMiK Phase 4B: Domain SDK Generator

Generates multi-language SDK code from domain schema definitions.
Runs the full pipeline: validate -> namespace -> generate -> write.

Usage:
    python scripts/generate_domain_sdks.py
    python scripts/generate_domain_sdks.py --output-dir custom_output
    python scripts/generate_domain_sdks.py --schemas-dir sdk/schemas/domains
    python scripts/generate_domain_sdks.py --languages python rust
"""

import argparse
import json
import sys
from pathlib import Path

# Add SDK to path
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root / "software" / "atomik_sdk"))

from generator.core import GeneratorConfig, GeneratorEngine
from generator.python_generator import PythonGenerator
from generator.rust_generator import RustGenerator
from generator.c_generator import CGenerator
from generator.javascript_generator import JavaScriptGenerator
from generator.verilog_generator import VerilogGenerator


GENERATORS = {
    "python": PythonGenerator,
    "rust": RustGenerator,
    "c": CGenerator,
    "javascript": JavaScriptGenerator,
    "verilog": VerilogGenerator,
}


def generate_from_schema(
    schema_path: Path,
    output_dir: Path,
    languages: list[str] | None = None,
) -> dict:
    """Generate SDK code from a single schema file.

    Returns dict with schema name, files generated, and any errors.
    """
    target_languages = languages or list(GENERATORS.keys())

    engine = GeneratorEngine(GeneratorConfig(
        output_dir=output_dir,
        validate_schemas=True,
        verbose=False,
    ))

    for lang in target_languages:
        if lang in GENERATORS:
            engine.register_generator(lang, GENERATORS[lang]())

    # Load and validate
    validation = engine.load_schema(schema_path)
    if not validation:
        return {
            "schema": schema_path.name,
            "success": False,
            "errors": validation.errors,
            "files": [],
        }

    # Extract namespace for reporting
    ns = engine.extract_metadata()
    name = f"{ns.vertical}.{ns.field}.{ns.object}"

    # Generate
    results = engine.generate(target_languages)

    # Write
    files = engine.write_output(results)

    # Collect per-language results
    lang_results = {}
    errors = []
    for lang, result in results.items():
        lang_results[lang] = {
            "success": result.success,
            "files": len(result.files),
        }
        if not result.success:
            errors.extend(result.errors)

    return {
        "schema": schema_path.name,
        "namespace": name,
        "success": len(errors) == 0,
        "languages": lang_results,
        "total_files": len(files),
        "errors": errors,
    }


def main():
    parser = argparse.ArgumentParser(description="Generate domain SDK code")
    parser.add_argument(
        "--schemas-dir",
        default="sdk/schemas/domains",
        help="Directory containing domain schema JSON files",
    )
    parser.add_argument(
        "--output-dir",
        default="sdk/generated",
        help="Output directory for generated code",
    )
    parser.add_argument(
        "--languages",
        nargs="+",
        choices=list(GENERATORS.keys()),
        help="Target languages (default: all)",
    )
    parser.add_argument(
        "--report",
        help="Write JSON report to file",
    )
    args = parser.parse_args()

    schemas_dir = project_root / args.schemas_dir
    output_dir = project_root / args.output_dir

    if not schemas_dir.exists():
        print(f"ERROR: Schemas directory not found: {schemas_dir}")
        return 1

    schemas = sorted(schemas_dir.glob("*.json"))
    if not schemas:
        print(f"ERROR: No JSON schemas found in {schemas_dir}")
        return 1

    print("=" * 60)
    print("ATOMiK Phase 4B: Domain SDK Generation")
    print("=" * 60)
    print(f"Schemas:  {schemas_dir}")
    print(f"Output:   {output_dir}")
    print(f"Languages: {args.languages or 'all'}")
    print(f"Found {len(schemas)} schema(s)")
    print()

    all_results = []
    total_files = 0
    failures = 0

    for schema_path in schemas:
        print(f"--- {schema_path.name} ---")
        result = generate_from_schema(schema_path, output_dir, args.languages)
        all_results.append(result)

        if result["success"]:
            print(f"  Namespace: {result['namespace']}")
            for lang, lr in result.get("languages", {}).items():
                status = "OK" if lr["success"] else "FAIL"
                print(f"    {lang}: {status} ({lr['files']} files)")
            print(f"  Total: {result['total_files']} files written")
            total_files += result["total_files"]
        else:
            print(f"  FAILED:")
            for err in result["errors"]:
                print(f"    - {err}")
            failures += 1
        print()

    # Summary
    print("=" * 60)
    print("Generation Summary")
    print("=" * 60)
    print(f"  Schemas processed: {len(schemas)}")
    print(f"  Total files generated: {total_files}")
    print(f"  Failures: {failures}")
    print()

    if failures == 0:
        print("*** ALL DOMAIN SDKS GENERATED SUCCESSFULLY ***")
    else:
        print("*** SOME SCHEMAS FAILED ***")

    # Write JSON report if requested
    if args.report:
        report_path = project_root / args.report
        report = {
            "schemas_dir": str(schemas_dir),
            "output_dir": str(output_dir),
            "total_schemas": len(schemas),
            "total_files": total_files,
            "failures": failures,
            "results": all_results,
        }
        with open(report_path, "w") as f:
            json.dump(report, f, indent=2)
        print(f"\nReport written to {report_path}")

    return 1 if failures > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
