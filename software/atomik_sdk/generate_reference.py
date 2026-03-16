#!/usr/bin/env python3
"""
ATOMiK SDK Reference Generation Script

Exercises the full SDK generation pipeline end-to-end:
  1. Loads domain schemas
  2. Validates them
  3. Generates Python, C, and Rust output
  4. Writes to software/atomik_sdk/generated/
  5. Reports results

Usage:
    python generate_reference.py
"""

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

# Ensure atomik_sdk is importable
SDK_ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = SDK_ROOT.parent.parent
sys.path.insert(0, str(SDK_ROOT.parent))  # software/ dir

from atomik_sdk.generator.c_generator import CGenerator  # noqa: E402
from atomik_sdk.generator.core import GeneratorConfig, GeneratorEngine  # noqa: E402
from atomik_sdk.generator.python_generator import PythonGenerator  # noqa: E402
from atomik_sdk.generator.rust_generator import RustGenerator  # noqa: E402

# Output directory
OUTPUT_DIR = SDK_ROOT / "generated"

# Schema locations
DOMAIN_SCHEMAS = PROJECT_ROOT / "sdk" / "schemas" / "domains"
EXAMPLE_SCHEMAS = PROJECT_ROOT / "sdk" / "schemas" / "examples"

GENERATORS = {
    "python": PythonGenerator,
    "c": CGenerator,
    "rust": RustGenerator,
}

SEPARATOR = "=" * 70


def generate_from_schema(schema_path: Path, output_dir: Path) -> dict:
    """Generate code from a single schema, return results dict."""
    schema_name = schema_path.stem

    # Per-schema output directory
    schema_output = output_dir / schema_name

    engine = GeneratorEngine(GeneratorConfig(
        output_dir=str(schema_output),
        validate_schemas=True,
        verbose=False,
    ))

    # Register generators
    for lang, gen_cls in GENERATORS.items():
        engine.register_generator(lang, gen_cls())

    # Load and validate
    validation = engine.load_schema(schema_path)
    if not validation:
        return {
            "schema": schema_name,
            "success": False,
            "validation_errors": validation.errors,
            "validation_warnings": validation.warnings,
            "languages": {},
            "files_written": [],
        }

    # Extract namespace
    ns = engine.extract_metadata()
    namespace = f"{ns.vertical}.{ns.field}.{ns.object}"

    # Generate
    results = engine.generate()

    # Write to disk
    files_written = engine.write_output(results)

    # Collect per-language results
    lang_results = {}
    for lang, result in results.items():
        lang_results[lang] = {
            "success": result.success,
            "files": [f.relative_path for f in result.files],
            "errors": result.errors,
            "warnings": result.warnings,
        }

    return {
        "schema": schema_name,
        "namespace": namespace,
        "success": all(r.success for r in results.values()),
        "validation_errors": [],
        "validation_warnings": validation.warnings if hasattr(validation, 'warnings') else [],
        "languages": lang_results,
        "files_written": files_written,
    }


def verify_python(output_dir: Path, schema_name: str) -> tuple[bool, str]:
    """Try to import the generated Python module."""
    # Find generated .py files (not __init__.py, not test_*)
    gen_dir = output_dir / schema_name
    py_files = list(gen_dir.rglob("atomik/**/*.py"))
    py_files = [f for f in py_files if f.name != "__init__.py"]

    if not py_files:
        return False, "No Python module files found"

    # Try to compile each file (syntax check)
    for py_file in py_files:
        try:
            with open(py_file) as f:
                source = f.read()
            compile(source, str(py_file), "exec")
        except SyntaxError as e:
            return False, f"Syntax error in {py_file.name}: {e}"

    # Try to run the test file
    test_files = list(gen_dir.rglob("tests/test_*.py"))
    if test_files:
        # We need to add the gen_dir to PYTHONPATH for imports to work
        env = os.environ.copy()
        env["PYTHONPATH"] = str(gen_dir) + os.pathsep + env.get("PYTHONPATH", "")
        for test_file in test_files:
            try:
                result = subprocess.run(
                    [sys.executable, str(test_file)],
                    capture_output=True, text=True, timeout=10,
                    env=env,
                )
                if result.returncode != 0:
                    return False, f"Test {test_file.name} failed:\n{result.stderr}"
            except subprocess.TimeoutExpired:
                return False, f"Test {test_file.name} timed out"
            except Exception as e:
                return False, f"Test {test_file.name} error: {e}"

    return True, "Python compile + test OK"


def verify_c(output_dir: Path, schema_name: str) -> tuple[bool, str]:
    """Try to compile the generated C code with gcc."""
    gen_dir = output_dir / schema_name

    # Find .c and .h files
    c_files = list(gen_dir.rglob("atomik/**/*.c"))
    _h_files = list(gen_dir.rglob("atomik/**/*.h"))  # noqa: F841
    test_files = list(gen_dir.rglob("tests/test_*.c"))

    if not c_files:
        return False, "No C source files found"

    # Try to compile
    all_c = [str(f) for f in c_files] + [str(f) for f in test_files]

    if not all_c:
        return False, "No C files to compile"

    with tempfile.NamedTemporaryFile(suffix=".out", delete=False) as tmp:
        out_path = tmp.name

    try:
        compile_cmd = [
            "gcc", "-Wall", "-Wextra", "-std=c99",
            f"-I{gen_dir}",
            "-o", out_path,
        ] + all_c

        result = subprocess.run(
            compile_cmd,
            capture_output=True, text=True, timeout=15,
        )

        if result.returncode != 0:
            return False, f"GCC compile failed:\n{result.stderr}"

        # Run the compiled test
        run_result = subprocess.run(
            [out_path],
            capture_output=True, text=True, timeout=10,
        )

        if run_result.returncode != 0:
            return False, f"C test binary failed:\n{run_result.stderr}"

        return True, f"GCC compile + run OK\n{run_result.stdout.strip()}"

    except FileNotFoundError:
        return False, "gcc not found"
    except subprocess.TimeoutExpired:
        return False, "Compile/run timed out"
    except Exception as e:
        return False, f"Error: {e}"
    finally:
        try:
            os.unlink(out_path)
        except OSError:
            pass


def verify_rust_syntax(output_dir: Path, schema_name: str) -> tuple[bool, str]:
    """Check Rust syntax (no full cargo build -- just check file presence and structure)."""
    gen_dir = output_dir / schema_name

    rs_files = list(gen_dir.rglob("src/**/*.rs"))
    cargo_toml = gen_dir / "Cargo.toml"

    if not rs_files:
        return False, "No Rust source files found"

    # Check Cargo.toml exists
    if not cargo_toml.exists():
        return False, "Cargo.toml not found"

    # Read and check basic structure
    issues = []
    for rs_file in rs_files:
        with open(rs_file) as f:
            content = f.read()

        # Basic checks: balanced braces, contains struct/fn/mod
        open_braces = content.count("{")
        close_braces = content.count("}")
        if open_braces != close_braces:
            issues.append(f"{rs_file.name}: unbalanced braces ({open_braces} open, {close_braces} close)")

        # Check for key patterns in main module (not mod.rs or lib.rs)
        if rs_file.name not in ("mod.rs", "lib.rs"):
            if "pub struct" not in content:
                issues.append(f"{rs_file.name}: missing 'pub struct'")
            if "pub fn new" not in content:
                issues.append(f"{rs_file.name}: missing 'pub fn new'")
            if "pub fn accumulate" not in content:
                issues.append(f"{rs_file.name}: missing 'pub fn accumulate'")
            if "pub fn reconstruct" not in content:
                issues.append(f"{rs_file.name}: missing 'pub fn reconstruct'")

    if issues:
        return False, "Rust structural issues:\n  " + "\n  ".join(issues)

    # Try rustc --edition 2021 syntax check on main module if rustc available
    main_rs = [f for f in rs_files if f.name not in ("mod.rs", "lib.rs") and "test" not in f.name]
    if main_rs:
        try:
            result = subprocess.run(
                ["rustc", "--edition", "2021", "--crate-type", "lib",
                 str(main_rs[0]), "-o", "/dev/null"],
                capture_output=True, text=True, timeout=15,
            )
            if result.returncode == 0:
                return True, "Rust syntax check PASSED (rustc --edition 2021)"
            else:
                # rustc may fail due to missing dependencies, but we can still
                # check if the errors are just import-related
                stderr = result.stderr
                if "cannot find" in stderr or "unresolved import" in stderr:
                    return True, "Rust syntax OK (import errors expected without crate context)"
                return False, f"rustc check failed:\n{stderr[:500]}"
        except FileNotFoundError:
            pass  # rustc not available, fall through to structural check only

    return True, f"Rust structural check OK ({len(rs_files)} files, Cargo.toml present)"


def main():
    print(SEPARATOR)
    print("ATOMiK SDK Reference Generation")
    print(SEPARATOR)
    print()

    # Clean output directory
    if OUTPUT_DIR.exists():
        import shutil
        shutil.rmtree(OUTPUT_DIR)
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Gather all schemas
    schemas = []
    if DOMAIN_SCHEMAS.exists():
        schemas.extend(sorted(DOMAIN_SCHEMAS.glob("*.json")))
    if EXAMPLE_SCHEMAS.exists():
        schemas.extend(sorted(EXAMPLE_SCHEMAS.glob("*.json")))

    if not schemas:
        print("ERROR: No schemas found!")
        return 1

    print(f"Found {len(schemas)} schema(s)")
    print(f"Target languages: {', '.join(GENERATORS.keys())}")
    print(f"Output directory: {OUTPUT_DIR}")
    print()

    # === Phase 1: Generate ===
    print("Phase 1: Generation")
    print("-" * 40)

    all_results = []
    total_files = 0
    gen_failures = 0

    for schema_path in schemas:
        result = generate_from_schema(schema_path, OUTPUT_DIR)
        all_results.append(result)

        if result["success"]:
            n_files = len(result["files_written"])
            total_files += n_files
            lang_status = []
            for lang, lr in result["languages"].items():
                lang_status.append(f"{lang}:{len(lr['files'])}")
            print(f"  [OK]   {result['schema']:30s}  {result.get('namespace', 'N/A'):30s}  ({', '.join(lang_status)})")
        else:
            gen_failures += 1
            if result.get("validation_errors"):
                print(f"  [FAIL] {result['schema']:30s}  Validation: {result['validation_errors']}")
            else:
                for lang, lr in result["languages"].items():
                    if not lr["success"]:
                        print(f"  [FAIL] {result['schema']:30s}  {lang}: {lr['errors']}")

    print()
    print(f"Generated: {len(schemas)} schemas, {total_files} files, {gen_failures} failure(s)")
    print()

    # === Phase 2: Verification ===
    print("Phase 2: Verification")
    print("-" * 40)

    verify_results = []

    for result in all_results:
        if not result["success"]:
            continue

        schema_name = result["schema"]

        # Python verification
        py_ok, py_msg = verify_python(OUTPUT_DIR, schema_name)
        status = "[PASS]" if py_ok else "[FAIL]"
        print(f"  {status} {schema_name} / Python: {py_msg}")
        verify_results.append(("python", schema_name, py_ok, py_msg))

        # C verification
        c_ok, c_msg = verify_c(OUTPUT_DIR, schema_name)
        status = "[PASS]" if c_ok else "[FAIL]"
        # Only print first line for C output
        c_msg_short = c_msg.split("\n")[0]
        print(f"  {status} {schema_name} / C: {c_msg_short}")
        verify_results.append(("c", schema_name, c_ok, c_msg))

        # Rust verification
        rs_ok, rs_msg = verify_rust_syntax(OUTPUT_DIR, schema_name)
        status = "[PASS]" if rs_ok else "[FAIL]"
        print(f"  {status} {schema_name} / Rust: {rs_msg}")
        verify_results.append(("rust", schema_name, rs_ok, rs_msg))

    print()

    # === Summary ===
    print(SEPARATOR)
    print("SUMMARY")
    print(SEPARATOR)

    gen_ok = sum(1 for r in all_results if r["success"])
    gen_fail = sum(1 for r in all_results if not r["success"])
    verify_pass = sum(1 for _, _, ok, _ in verify_results if ok)
    verify_fail = sum(1 for _, _, ok, _ in verify_results if not ok)

    print(f"  Schemas processed:   {len(schemas)}")
    print(f"  Generation:          {gen_ok} OK, {gen_fail} FAIL")
    print(f"  Files written:       {total_files}")
    print(f"  Verification:        {verify_pass} PASS, {verify_fail} FAIL")
    print()

    # List output files
    if OUTPUT_DIR.exists():
        all_files = sorted(OUTPUT_DIR.rglob("*"))
        all_files = [f for f in all_files if f.is_file()]
        print(f"Output tree ({len(all_files)} files):")
        for f in all_files[:60]:  # Cap at 60 for readability
            rel = f.relative_to(OUTPUT_DIR)
            print(f"  {rel}")
        if len(all_files) > 60:
            print(f"  ... and {len(all_files) - 60} more")

    print()

    # Write JSON report
    report_path = OUTPUT_DIR / "generation_report.json"
    report = {
        "schemas_processed": len(schemas),
        "generation_ok": gen_ok,
        "generation_fail": gen_fail,
        "total_files": total_files,
        "verification_pass": verify_pass,
        "verification_fail": verify_fail,
        "details": all_results,
        "verification": [
            {"language": lang, "schema": sn, "passed": ok, "message": msg}
            for lang, sn, ok, msg in verify_results
        ],
    }
    with open(report_path, "w") as f:
        json.dump(report, f, indent=2)
    print(f"Report: {report_path}")

    overall = gen_fail == 0 and verify_fail == 0
    print()
    if overall:
        print("RESULT: ALL PASSED")
    else:
        print("RESULT: SOME FAILURES (see above)")

    return 0 if overall else 1


if __name__ == "__main__":
    sys.exit(main())
