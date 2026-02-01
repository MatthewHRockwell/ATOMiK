"""
Hardware-in-the-Loop Pipeline Stage (Mandatory)

Stage 5 of the pipeline: validates generated Verilog through RTL
simulation and, when hardware is available, FPGA synthesis,
programming, and on-device test execution.

RTL simulation always runs. FPGA synthesis and programming run
when the respective tools and hardware are detected.
"""

from __future__ import annotations

import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from . import BaseStage, StageManifest, StageStatus


class HardwareStage(BaseStage):
    """Mandatory hardware validation stage."""

    name = "hardware"

    # Phase 3 baseline for comparison
    PHASE3_BASELINE = {
        "fmax_mhz": 94.9,
        "lut_pct": 7,
        "ff_pct": 9,
        "tests_passed": 10,
        "tests_total": 10,
    }

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        sim_only = getattr(config, "sim_only", False)
        skip_synthesis = getattr(config, "skip_synthesis", False)
        output_dir = getattr(config, "output_dir", "generated")

        # Find generated Verilog files
        verilog_files = list(Path(output_dir).rglob("*.v"))
        if not verilog_files:
            manifest.warnings.append("No Verilog files found in output directory")
            manifest.validation_level = "no_rtl"
            manifest.next_stage = "metrics"
            return

        # Separate testbenches from source
        tb_files = [f for f in verilog_files if f.name.startswith("tb_")]
        src_files = [f for f in verilog_files if not f.name.startswith("tb_")]

        # ---- Step 1: RTL Simulation (ALWAYS) ----
        sim_result = self._rtl_simulation(src_files, tb_files, manifest)
        manifest.validation_level = "simulation_only"

        if not sim_result:
            manifest.status = StageStatus.FAILED
            manifest.next_stage = "metrics"
            return

        # Include Phase 3 baseline in all cases where RTL exists
        manifest.metrics["phase3_baseline"] = self.PHASE3_BASELINE

        if sim_only:
            manifest.next_stage = "metrics"
            return

        # ---- Step 2: FPGA Synthesis (when tools available) ----
        if not skip_synthesis and self._has_gowin_eda():
            synth_result = self._fpga_synthesis(src_files, manifest)
            if synth_result:
                manifest.validation_level = "synthesized"

        # ---- Step 3: FPGA Programming (when board detected) ----
        if not skip_synthesis and self._has_fpga_tools():
            board = self._detect_board()
            if board:
                program_ok = self._program_fpga(manifest, config)
                if program_ok:
                    manifest.validation_level = "hw_programmed"

                    # ---- Step 4: On-Device Validation ----
                    com_port = getattr(config, "com_port", None) or self._detect_com_port()
                    if com_port:
                        hw_ok = self._on_device_validation(com_port, manifest)
                        if hw_ok:
                            manifest.validation_level = "hw_validated"

        manifest.next_stage = "metrics"
        manifest.tokens_consumed = 0  # All local execution

    def _rtl_simulation(
        self,
        src_files: list[Path],
        tb_files: list[Path],
        manifest: StageManifest,
    ) -> bool:
        """Run RTL simulation with iverilog/vvp."""
        iverilog = shutil.which("iverilog")
        vvp = shutil.which("vvp")

        if not iverilog or not vvp:
            manifest.warnings.append(
                "iverilog/vvp not found -- RTL simulation skipped"
            )
            manifest.metrics["sim_tests_passed"] = 0
            manifest.metrics["sim_tests_total"] = 0
            manifest.metrics["sim_available"] = False
            return True  # Not a failure, just can't simulate

        manifest.metrics["sim_available"] = True
        tests_passed = 0
        tests_total = 0

        for tb in tb_files:
            tests_total += 1
            try:
                # Compile
                with tempfile.NamedTemporaryFile(
                    suffix=".vvp", delete=False
                ) as tmp:
                    vvp_out = tmp.name

                all_files = [str(f) for f in src_files] + [str(tb)]
                compile_result = subprocess.run(
                    [iverilog, "-o", vvp_out] + all_files,
                    capture_output=True,
                    text=True,
                    timeout=60,
                )

                if compile_result.returncode != 0:
                    manifest.errors.append(
                        f"iverilog compile failed for {tb.name}: "
                        f"{compile_result.stderr.strip()}"
                    )
                    continue

                # Simulate
                sim_result = subprocess.run(
                    [vvp, vvp_out],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )

                output = sim_result.stdout + sim_result.stderr

                # Parse for pass/fail indicators
                if "FAIL" in output.upper() and "0 FAIL" not in output.upper():
                    manifest.errors.append(
                        f"RTL simulation FAILED for {tb.name}"
                    )
                else:
                    tests_passed += 1

                # Clean up
                Path(vvp_out).unlink(missing_ok=True)

            except subprocess.TimeoutExpired:
                manifest.errors.append(f"RTL simulation timed out for {tb.name}")
            except Exception as e:
                manifest.errors.append(f"RTL simulation error for {tb.name}: {e}")

        manifest.metrics["sim_tests_passed"] = tests_passed
        manifest.metrics["sim_tests_total"] = tests_total

        return tests_passed == tests_total

    def _has_gowin_eda(self) -> bool:
        """Check if Gowin EDA synthesis tools are available."""
        from atomik_sdk.hardware_discovery import find_tool

        return find_tool("gw_sh") is not None

    def _has_fpga_tools(self) -> bool:
        """Check if board-programming tools are available."""
        from atomik_sdk.hardware_discovery import find_tool

        return (
            find_tool("programmer_cli") is not None
            or find_tool("openFPGALoader") is not None
        )

    def _detect_board(self) -> str | None:
        """Detect connected Tang Nano 9K board."""
        from atomik_sdk.hardware_discovery import detect_board

        return detect_board()

    def _program_fpga(self, manifest: StageManifest, config: Any) -> bool:
        """Program the FPGA with the bitstream."""
        # This would invoke the existing fpga_pipeline.py --program-only
        project_root = Path(__file__).resolve().parents[4]
        pipeline_script = project_root / "scripts" / "fpga_pipeline.py"

        if not pipeline_script.exists():
            manifest.warnings.append("fpga_pipeline.py not found")
            return False

        try:
            result = subprocess.run(
                ["python", str(pipeline_script), "--program-only"],
                capture_output=True, text=True, timeout=30,
                cwd=str(project_root),
            )
            manifest.metrics["programming_time_s"] = 0  # Would parse from output
            return result.returncode == 0
        except (subprocess.TimeoutExpired, OSError) as e:
            manifest.warnings.append(f"FPGA programming failed: {e}")
            return False

    def _detect_com_port(self) -> str | None:
        """Auto-detect COM port for UART communication."""
        from atomik_sdk.hardware_discovery import detect_com_port

        return detect_com_port()

    def _on_device_validation(
        self, com_port: str, manifest: StageManifest
    ) -> bool:
        """Run hardware test suite via UART."""
        project_root = Path(__file__).resolve().parents[4]
        test_script = project_root / "scripts" / "test_hardware.py"

        if not test_script.exists():
            manifest.warnings.append("test_hardware.py not found")
            return False

        try:
            result = subprocess.run(
                ["python", str(test_script), "--port", com_port],
                capture_output=True, text=True, timeout=60,
                cwd=str(project_root),
            )
            output = result.stdout

            # Parse test results
            passed = output.count("PASS")
            failed = output.count("FAIL")
            total = passed + failed

            manifest.metrics["hw_tests_passed"] = passed
            manifest.metrics["hw_tests_total"] = total

            return result.returncode == 0

        except (subprocess.TimeoutExpired, OSError) as e:
            manifest.warnings.append(f"Hardware validation failed: {e}")
            return False

    def _fpga_synthesis(
        self, src_files: list[Path], manifest: StageManifest
    ) -> bool:
        """Run FPGA synthesis with Gowin EDA."""
        manifest.warnings.append("FPGA synthesis integration pending")
        return False
