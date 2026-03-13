#!/usr/bin/env python3
"""ATOMiK Zynq Fmax Sweep — Clock Frequency & Multi-Bank Characterization

Sweeps ATOMIK_CLK_DIV parameter across synthesis + implementation on
XC7Z020-2CLG400I to find maximum achievable ATOMiK core frequency,
optionally with multi-bank parallel configurations.

VCO is fixed at 1000 MHz (100 MHz FCLK * 10x multiplier).
ATOMIK_CLK_DIV controls core clock: freq = 1000 / div.

Usage:
    python scripts/fmax_sweep.py                    # Full freq sweep, N=1
    python scripts/fmax_sweep.py --banks 1,4,8,16   # Freq x banks sweep
    python scripts/fmax_sweep.py --quick             # Quick: 200,300,400 MHz only
    python scripts/fmax_sweep.py --gen-only          # Generate files, don't build
    python scripts/fmax_sweep.py --freqs 200,250,300 # Custom frequencies

Results written to: sweep/results/sweep_summary.json
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import textwrap
from dataclasses import dataclass, asdict
from pathlib import Path
from datetime import datetime

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

FCLK_MHZ = 100.0
VCO_MULT = 10.0
VCO_MHZ = FCLK_MHZ * VCO_MULT  # 1000 MHz

SCRIPT_DIR = Path(__file__).resolve().parent
ZYNQ_DIR = SCRIPT_DIR.parent

PART = "xc7z020clg400-2"

# Default frequency targets (MHz) — VCO/div must yield integer-ish dividers
DEFAULT_FREQS = [200, 250, 300, 333, 400, 500]
QUICK_FREQS = [200, 300, 400]

DEFAULT_BANKS = [1]

# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class SweepConfig:
    freq_mhz: float
    clk_div: float
    n_banks: int

    @property
    def tag(self) -> str:
        # Replace dots with 'p' for valid Verilog module names
        freq_s = f"{self.freq_mhz:.0f}" if self.freq_mhz == int(self.freq_mhz) else f"{self.freq_mhz:.1f}"
        return f"N{self.n_banks}_F{freq_s.replace('.', 'p')}"

    @property
    def period_ns(self) -> float:
        return 1000.0 / self.freq_mhz


@dataclass
class SweepResult:
    tag: str
    freq_mhz: float
    n_banks: int
    timing_met: bool
    wns_ns: float        # Worst negative slack (positive = met)
    tns_ns: float        # Total negative slack
    fmax_mhz: float      # Achieved Fmax from WNS
    lut_used: int
    lut_total: int
    ff_used: int
    ff_total: int
    bram_used: float
    bram_total: float
    build_time_s: float
    error: str | None = None


# ---------------------------------------------------------------------------
# File generators
# ---------------------------------------------------------------------------

def gen_sweep_top(cfg: SweepConfig, out_dir: Path) -> Path:
    """Generate a thin top-level wrapper with specific parameters.

    Both single-bank and multi-bank use atomik_zynq_top with N_BANKS parameter.
    The top instantiates atomik_core_zynq_parallel which handles 1..N banks.
    """
    path = out_dir / f"sweep_top_{cfg.tag}.v"

    content = textwrap.dedent(f"""\
    `timescale 1ns / 1ps
    module sweep_top_{cfg.tag} (
        input  wire        fclk_clk0,
        input  wire        fclk_reset_n,
        output wire        locked,
        input  wire [5:0]  s_axi_awaddr,
        input  wire        s_axi_awvalid,
        output wire        s_axi_awready,
        input  wire [31:0] s_axi_wdata,
        input  wire [3:0]  s_axi_wstrb,
        input  wire        s_axi_wvalid,
        output wire        s_axi_wready,
        output wire [1:0]  s_axi_bresp,
        output wire        s_axi_bvalid,
        input  wire        s_axi_bready,
        input  wire [5:0]  s_axi_araddr,
        input  wire        s_axi_arvalid,
        output wire        s_axi_arready,
        output wire [31:0] s_axi_rdata,
        output wire [1:0]  s_axi_rresp,
        output wire        s_axi_rvalid,
        input  wire        s_axi_rready
    );
        atomik_zynq_top #(
            .N_BANKS({cfg.n_banks}),
            .ATOMIK_CLK_DIV({cfg.clk_div})
        ) u_top (
            .fclk_clk0     (fclk_clk0),
            .fclk_reset_n  (fclk_reset_n),
            .locked        (locked),
            .s_axi_awaddr  (s_axi_awaddr),
            .s_axi_awvalid (s_axi_awvalid),
            .s_axi_awready (s_axi_awready),
            .s_axi_wdata   (s_axi_wdata),
            .s_axi_wstrb   (s_axi_wstrb),
            .s_axi_wvalid  (s_axi_wvalid),
            .s_axi_wready  (s_axi_wready),
            .s_axi_bresp   (s_axi_bresp),
            .s_axi_bvalid  (s_axi_bvalid),
            .s_axi_bready  (s_axi_bready),
            .s_axi_araddr  (s_axi_araddr),
            .s_axi_arvalid (s_axi_arvalid),
            .s_axi_arready (s_axi_arready),
            .s_axi_rdata   (s_axi_rdata),
            .s_axi_rresp   (s_axi_rresp),
            .s_axi_rvalid  (s_axi_rvalid),
            .s_axi_rready  (s_axi_rready)
        );
    endmodule
    """)

    path.write_text(content)
    return path



def gen_sweep_xdc(cfg: SweepConfig, out_dir: Path) -> Path:
    """Generate constraints with correct clock period for this config."""
    path = out_dir / f"sweep_{cfg.tag}.xdc"

    content = textwrap.dedent(f"""\
    # Sweep config: {cfg.tag} — ATOMiK @ {cfg.freq_mhz:.1f} MHz, N_BANKS={cfg.n_banks}

    # Base clock on top-level port (works for both PL-only and PS+PL builds)
    create_clock -period {FCLK_MHZ / FCLK_MHZ * 10.0:.3f} -name fclk_clk0 [get_ports fclk_clk0]

    # MMCM-generated clocks — use -hier wildcard to handle any wrapper depth
    create_generated_clock -name atomik_clk \\
        -source [get_pins -hier -filter {{NAME =~ */u_mmcm/CLKIN1}}] \\
        -master_clock fclk_clk0 \\
        [get_pins -hier -filter {{NAME =~ */u_mmcm/CLKOUT0}}]

    create_generated_clock -name axi_clk \\
        -source [get_pins -hier -filter {{NAME =~ */u_mmcm/CLKIN1}}] \\
        -master_clock fclk_clk0 \\
        [get_pins -hier -filter {{NAME =~ */u_mmcm/CLKOUT1}}]

    # CDC false paths (toggle-handshake, data stable before toggle)
    set_false_path -from [get_clocks axi_clk] -to [get_clocks atomik_clk]
    set_false_path -from [get_clocks atomik_clk] -to [get_clocks axi_clk]
    """)

    path.write_text(content)
    return path


def gen_sweep_tcl(cfg: SweepConfig, top_path: Path, xdc_path: Path,
                  out_dir: Path, report_dir: Path) -> Path:
    """Generate Vivado TCL build script for this config."""
    path = out_dir / f"sweep_{cfg.tag}.tcl"

    rtl_sources = [
        str(ZYNQ_DIR / "rtl" / "atomik_core_zynq.v"),
        str(ZYNQ_DIR / "rtl" / "atomik_core_zynq_parallel.v"),
        str(ZYNQ_DIR / "rtl" / "atomik_axi4lite_wrapper.v"),
        str(ZYNQ_DIR / "rtl" / "atomik_cdc_bridge.v"),
        str(ZYNQ_DIR / "rtl" / "atomik_zynq_clk.v"),
        str(ZYNQ_DIR / "rtl" / "atomik_zynq_top.v"),
    ]

    rtl_sources.append(str(top_path))

    read_cmds = "\n".join(f"read_verilog {{{s}}}" for s in rtl_sources)
    top_name = f"sweep_top_{cfg.tag}"

    content = textwrap.dedent(f"""\
    # Auto-generated sweep build: {cfg.tag}
    # ATOMiK @ {cfg.freq_mhz} MHz (div={cfg.clk_div}), N_BANKS={cfg.n_banks}

    set_param general.maxThreads 4

    file mkdir {{{report_dir}}}

    {read_cmds}
    read_xdc {{{xdc_path}}}

    auto_detect_xpm
    synth_design -top {top_name} -part {PART}
    report_utilization -file {{{report_dir}/util_{cfg.tag}.rpt}}

    opt_design -directive Explore
    place_design -directive ExtraTimingOpt
    phys_opt_design -directive AggressiveExplore
    route_design -directive AggressiveExplore

    report_timing_summary -file {{{report_dir}/timing_{cfg.tag}.rpt}}
    report_utilization -file {{{report_dir}/impl_util_{cfg.tag}.rpt}}
    report_utilization -hierarchical -hierarchical_depth 2 -file {{{report_dir}/hier_util_{cfg.tag}.rpt}}

    puts ""
    puts "SWEEP_DONE: {cfg.tag}"
    puts ""
    """)

    path.write_text(content)
    return path


# ---------------------------------------------------------------------------
# Report parsers
# ---------------------------------------------------------------------------

def parse_timing_report(path: Path) -> tuple[float, float]:
    """Extract WNS and TNS from Vivado timing_summary report.
    Returns (wns_ns, tns_ns). Positive WNS = timing met.
    Returns (None, None) if no constrained paths (all NA).
    """
    text = path.read_text()

    # Vivado format (whitespace-aligned columns):
    #     WNS(ns)      TNS(ns)  TNS Failing Endpoints ...
    #     -------      -------  --------------------- ...
    #       1.234        0.000                      0 ...
    # Or "NA" if no constrained paths

    # Find the data line after the separator
    m = re.search(
        r'WNS\(ns\)\s+TNS\(ns\)\s+.*?\n\s*[\-]+\s+[\-]+.*?\n\s*(\S+)\s+(\S+)',
        text
    )
    if m:
        wns_s, tns_s = m.group(1), m.group(2)
        if wns_s == "NA" or tns_s == "NA":
            return (None, None)
        return float(wns_s), float(tns_s)
    return (None, None)


def parse_utilization_report(path: Path) -> dict:
    """Extract LUT, FF, BRAM counts from Vivado utilization report."""
    text = path.read_text()
    result = {
        "lut_used": 0, "lut_total": 53200,
        "ff_used": 0, "ff_total": 106400,
        "bram_used": 0.0, "bram_total": 140.0,
    }

    # Slice LUTs
    m = re.search(r'Slice LUTs\s*\|\s*(\d+)\s*\|\s*\d+\s*\|\s*\d+\s*\|\s*(\d+)', text)
    if m:
        result["lut_used"] = int(m.group(1))
        result["lut_total"] = int(m.group(2))

    # Slice Registers (FFs)
    m = re.search(r'Slice Registers\s*\|\s*(\d+)\s*\|\s*\d+\s*\|\s*\d+\s*\|\s*(\d+)', text)
    if m:
        result["ff_used"] = int(m.group(1))
        result["ff_total"] = int(m.group(2))

    # Block RAM
    m = re.search(r'Block RAM Tile\s*\|\s*([\d\.]+)\s*\|\s*\d+\s*\|\s*\d+\s*\|\s*([\d\.]+)', text)
    if m:
        result["bram_used"] = float(m.group(1))
        result["bram_total"] = float(m.group(2))

    return result


# ---------------------------------------------------------------------------
# Main sweep logic
# ---------------------------------------------------------------------------

def freq_to_div(freq_mhz: float) -> float:
    """Convert target frequency to MMCM divider. VCO = 1000 MHz."""
    return VCO_MHZ / freq_mhz


def build_configs(freqs: list[float], banks: list[int]) -> list[SweepConfig]:
    """Build sweep configurations from frequency and bank lists."""
    configs = []
    for n in banks:
        for f in freqs:
            div = freq_to_div(f)
            # MMCME2 constraints: CLKOUT0_DIVIDE_F must be 1.0-128.0 in 0.125 steps
            if div < 1.0 or div > 128.0:
                print(f"  SKIP: {f} MHz (div={div:.3f} out of MMCM range)")
                continue
            # Round to nearest 0.125
            div = round(div * 8) / 8
            actual_freq = VCO_MHZ / div
            configs.append(SweepConfig(freq_mhz=actual_freq, clk_div=div, n_banks=n))
    return configs


def run_sweep(configs: list[SweepConfig], gen_only: bool = False) -> list[SweepResult]:
    """Generate files and optionally run Vivado for each config."""
    sweep_dir = ZYNQ_DIR / "sweep"
    top_dir = sweep_dir / "top"
    xdc_dir = sweep_dir / "xdc"
    tcl_dir = sweep_dir / "tcl"
    report_dir = sweep_dir / "reports"
    results_dir = sweep_dir / "results"

    for d in [top_dir, xdc_dir, tcl_dir, report_dir, results_dir]:
        d.mkdir(parents=True, exist_ok=True)

    results = []

    print(f"\n{'='*70}")
    print(f" ATOMiK Zynq Fmax Sweep — {len(configs)} configurations")
    print(f" Target: {PART}")
    print(f" VCO: {VCO_MHZ:.0f} MHz (FCLK={FCLK_MHZ:.0f} x {VCO_MULT:.0f})")
    print(f"{'='*70}\n")

    for i, cfg in enumerate(configs):
        print(f"[{i+1}/{len(configs)}] {cfg.tag}: {cfg.freq_mhz:.1f} MHz (div={cfg.clk_div}), "
              f"N_BANKS={cfg.n_banks}")

        # Generate files
        top_path = gen_sweep_top(cfg, top_dir)
        xdc_path = gen_sweep_xdc(cfg, xdc_dir)
        tcl_path = gen_sweep_tcl(cfg, top_path, xdc_path, tcl_dir, report_dir)

        if gen_only:
            print(f"  Generated: {tcl_path.name}")
            continue

        # Run Vivado
        import time
        t0 = time.time()
        try:
            proc = subprocess.run(
                ["vivado", "-mode", "batch", "-source", str(tcl_path)],
                capture_output=True, text=True, timeout=1800,
                cwd=str(ZYNQ_DIR)
            )
            build_time = time.time() - t0
            success = proc.returncode == 0 and "SWEEP_DONE" in proc.stdout
        except subprocess.TimeoutExpired:
            build_time = time.time() - t0
            success = False

        if not success:
            print(f"  BUILD FAILED ({build_time:.0f}s)")
            # Try to extract error
            err_lines = [l for l in (proc.stdout + proc.stderr).split("\n")
                         if "ERROR" in l][:3]
            error_msg = "; ".join(err_lines) if err_lines else "Unknown error"
            results.append(SweepResult(
                tag=cfg.tag, freq_mhz=cfg.freq_mhz, n_banks=cfg.n_banks,
                timing_met=False, wns_ns=0, tns_ns=0, fmax_mhz=0,
                lut_used=0, lut_total=53200, ff_used=0, ff_total=106400,
                bram_used=0, bram_total=140, build_time_s=build_time,
                error=error_msg
            ))
            continue

        # Parse reports
        timing_rpt = report_dir / f"timing_{cfg.tag}.rpt"
        util_rpt = report_dir / f"impl_util_{cfg.tag}.rpt"

        wns, tns = parse_timing_report(timing_rpt)
        util = parse_utilization_report(util_rpt)

        if wns is None:
            # No constrained paths — constraints didn't apply
            print(f"  WARNING: No constrained timing paths (constraints not applied)")
            results.append(SweepResult(
                tag=cfg.tag, freq_mhz=cfg.freq_mhz, n_banks=cfg.n_banks,
                timing_met=False, wns_ns=0, tns_ns=0, fmax_mhz=0,
                lut_used=util["lut_used"], lut_total=util["lut_total"],
                ff_used=util["ff_used"], ff_total=util["ff_total"],
                bram_used=util["bram_used"], bram_total=util["bram_total"],
                build_time_s=round(build_time, 1),
                error="No constrained timing paths"
            ))
            continue

        # Compute achieved Fmax from WNS
        # Fmax = 1 / (period - WNS) * 1000 (MHz)
        period = cfg.period_ns
        if wns >= 0:
            # Timing met — Fmax = 1/(period - WNS) * 1000
            fmax = 1000.0 / (period - wns) if (period - wns) > 0 else 9999.0
        else:
            # Timing failed — Fmax = 1/(period - WNS) * 1000 (WNS is negative)
            fmax = 1000.0 / (period - wns)

        timing_met = wns >= 0

        result = SweepResult(
            tag=cfg.tag, freq_mhz=cfg.freq_mhz, n_banks=cfg.n_banks,
            timing_met=timing_met, wns_ns=wns, tns_ns=tns, fmax_mhz=round(fmax, 1),
            lut_used=util["lut_used"], lut_total=util["lut_total"],
            ff_used=util["ff_used"], ff_total=util["ff_total"],
            bram_used=util["bram_used"], bram_total=util["bram_total"],
            build_time_s=round(build_time, 1)
        )
        results.append(result)

        status = "PASS" if timing_met else "FAIL"
        margin = f"+{wns:.3f}ns" if wns >= 0 else f"{wns:.3f}ns"
        print(f"  {status}: Fmax={fmax:.1f} MHz, WNS={margin}, "
              f"LUT={util['lut_used']} ({100*util['lut_used']/util['lut_total']:.1f}%), "
              f"FF={util['ff_used']} ({100*util['ff_used']/util['ff_total']:.1f}%), "
              f"BRAM={util['bram_used']:.0f}, "
              f"({build_time:.0f}s)")

    # Save results
    if results and not gen_only:
        save_results(results, results_dir)

    return results


def save_results(results: list[SweepResult], results_dir: Path):
    """Save results as JSON and print summary table."""
    # JSON
    json_path = results_dir / "sweep_summary.json"
    data = {
        "timestamp": datetime.now().isoformat(),
        "part": PART,
        "vco_mhz": VCO_MHZ,
        "results": [asdict(r) for r in results]
    }
    json_path.write_text(json.dumps(data, indent=2))
    print(f"\nResults saved to: {json_path}")

    # Summary table
    print(f"\n{'='*90}")
    print(f" SWEEP RESULTS SUMMARY")
    print(f"{'='*90}")
    print(f"{'Config':<14} {'Target':>8} {'Fmax':>8} {'WNS':>8} {'TNS':>8} "
          f"{'LUT':>10} {'FF':>10} {'BRAM':>6} {'Status':>8}")
    print(f"{'-'*14} {'-'*8} {'-'*8} {'-'*8} {'-'*8} "
          f"{'-'*10} {'-'*10} {'-'*6} {'-'*8}")

    for r in results:
        if r.error:
            print(f"{r.tag:<14} {r.freq_mhz:>7.1f}M {'---':>8} {'---':>8} {'---':>8} "
                  f"{'---':>10} {'---':>10} {'---':>6} {'ERROR':>8}")
        else:
            lut_pct = f"{r.lut_used}({100*r.lut_used/r.lut_total:.1f}%)"
            ff_pct = f"{r.ff_used}({100*r.ff_used/r.ff_total:.1f}%)"
            status = "PASS" if r.timing_met else "FAIL"
            print(f"{r.tag:<14} {r.freq_mhz:>7.1f}M {r.fmax_mhz:>7.1f}M "
                  f"{r.wns_ns:>7.3f} {r.tns_ns:>7.3f} "
                  f"{lut_pct:>10} {ff_pct:>10} {r.bram_used:>5.0f} {status:>8}")

    # Find Fmax wall
    passing = [r for r in results if r.timing_met and not r.error]
    if passing:
        best = max(passing, key=lambda r: r.freq_mhz)
        print(f"\nHighest passing frequency: {best.freq_mhz:.1f} MHz "
              f"(Fmax={best.fmax_mhz:.1f} MHz, margin={best.wns_ns:.3f}ns)")

    failing = [r for r in results if not r.timing_met and not r.error]
    if failing:
        wall = min(failing, key=lambda r: r.freq_mhz)
        print(f"First failing frequency:  {wall.freq_mhz:.1f} MHz "
              f"(WNS={wall.wns_ns:.3f}ns)")

    print()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="ATOMiK Zynq Fmax Sweep")
    parser.add_argument("--freqs", type=str, default=None,
                        help="Comma-separated target frequencies in MHz (default: 200,250,300,333,400,500)")
    parser.add_argument("--banks", type=str, default=None,
                        help="Comma-separated N_BANKS values (default: 1)")
    parser.add_argument("--quick", action="store_true",
                        help="Quick sweep: 200,300,400 MHz only")
    parser.add_argument("--gen-only", action="store_true",
                        help="Generate files only, don't run Vivado")
    args = parser.parse_args()

    if args.freqs:
        freqs = [float(f) for f in args.freqs.split(",")]
    elif args.quick:
        freqs = QUICK_FREQS
    else:
        freqs = DEFAULT_FREQS

    banks = [int(b) for b in args.banks.split(",")] if args.banks else DEFAULT_BANKS

    configs = build_configs(freqs, banks)
    if not configs:
        print("No valid configurations to sweep.")
        sys.exit(1)

    results = run_sweep(configs, gen_only=args.gen_only)

    if not args.gen_only and results:
        any_fail = any(not r.timing_met for r in results if not r.error)
        sys.exit(1 if any_fail else 0)


if __name__ == "__main__":
    main()
