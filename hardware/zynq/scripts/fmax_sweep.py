#!/usr/bin/env python3
"""ATOMiK Zynq Fmax Sweep — Clock Frequency & Multi-Bank Characterization

Sweeps ATOMIK_CLK_DIV parameter across synthesis + implementation on
XC7Z020-2CLG400I to find maximum achievable ATOMiK core frequency,
optionally with multi-bank parallel configurations.

VCO is fixed at 1000 MHz (100 MHz FCLK * 10x multiplier).
ATOMIK_CLK_DIV controls core clock: freq = 1000 / div.

Modes:
    --ceiling       Find the absolute max frequency per N_BANKS by walking
                    up through MMCM frequency steps until timing fails.
                    Uses multiple Vivado strategies at the boundary.

    (default)       Sweep fixed frequencies across bank configs.

Usage:
    python scripts/fmax_sweep.py                          # Fixed freq sweep, N=1
    python scripts/fmax_sweep.py --banks 1,4,16           # Freq x banks sweep
    python scripts/fmax_sweep.py --ceiling --banks 1,4,16 # Find ceiling per N
    python scripts/fmax_sweep.py --ceiling --banks 1      # Ceiling hunt, N=1 only
    python scripts/fmax_sweep.py --strategy maximum        # Use max-effort strategy
    python scripts/fmax_sweep.py --freqs 200,250,300       # Custom frequencies
    python scripts/fmax_sweep.py --gen-only                # Generate files, don't build

Results written to: sweep/results/sweep_summary.json (or ceiling_summary.json)
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import textwrap
import time
from dataclasses import dataclass, asdict, field
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

# MMCME2 divider: 1.000 to 128.000 in 0.125 steps
MMCM_DIV_STEP = 0.125
MMCM_DIV_MIN = 1.0
MMCM_DIV_MAX = 128.0

# ---------------------------------------------------------------------------
# Vivado Strategy Presets
# ---------------------------------------------------------------------------
# Each strategy defines directives for synth, opt, place, phys_opt, route.
# phys_opt can be a list for multi-pass optimization.

STRATEGIES = {
    "baseline": {
        "desc": "Vivado defaults — minimum effort baseline",
        "synth":    "",
        "opt":      "",
        "place":    "",
        "phys_opt": [""],
        "route":    "",
    },
    "aggressive": {
        "desc": "Current production settings — good balance",
        "synth":    "",
        "opt":      "-directive Explore",
        "place":    "-directive ExtraTimingOpt",
        "phys_opt": ["-directive AggressiveExplore"],
        "route":    "-directive AggressiveExplore",
    },
    "maximum": {
        "desc": "Maximum effort — multi-pass phys_opt, performance synth",
        "synth":    "-directive PerformanceOptimized",
        "opt":      "-directive ExploreWithRemap",
        "place":    "-directive ExtraNetDelay_high",
        "phys_opt": [
            "-directive AggressiveExplore",
            "-directive AggressiveFanoutOpt",
            "-directive AlternateFlowWithRetiming",
        ],
        "route":    "-directive AggressiveExplore",
    },
    "area": {
        "desc": "Minimize LUT/FF — find resource floor",
        "synth":    "-directive AreaOptimized_high",
        "opt":      "-directive ExploreArea",
        "place":    "-directive ExtraTimingOpt",
        "phys_opt": ["-directive AggressiveExplore"],
        "route":    "-directive Explore",
    },
}

DEFAULT_STRATEGY = "aggressive"

# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class SweepConfig:
    freq_mhz: float
    clk_div: float
    n_banks: int
    strategy: str = DEFAULT_STRATEGY

    @property
    def tag(self) -> str:
        freq_s = f"{self.freq_mhz:.0f}" if self.freq_mhz == int(self.freq_mhz) else f"{self.freq_mhz:.1f}"
        tag = f"N{self.n_banks}_F{freq_s.replace('.', 'p')}"
        if self.strategy != DEFAULT_STRATEGY:
            tag += f"_{self.strategy}"
        return tag

    @property
    def period_ns(self) -> float:
        return 1000.0 / self.freq_mhz


@dataclass
class SweepResult:
    tag: str
    freq_mhz: float
    n_banks: int
    strategy: str
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
# MMCM frequency enumeration
# ---------------------------------------------------------------------------

def mmcm_frequencies(min_mhz: float = 100.0, max_mhz: float = 1000.0) -> list[tuple[float, float]]:
    """Return all achievable (freq_mhz, div) pairs from the MMCM, sorted ascending by freq."""
    results = []
    div = MMCM_DIV_MIN
    while div <= MMCM_DIV_MAX:
        freq = VCO_MHZ / div
        if min_mhz <= freq <= max_mhz:
            results.append((round(freq, 2), round(div, 3)))
        div = round(div + MMCM_DIV_STEP, 3)
    results.sort(key=lambda x: x[0])
    return results


# ---------------------------------------------------------------------------
# File generators
# ---------------------------------------------------------------------------

def gen_sweep_top(cfg: SweepConfig, out_dir: Path) -> Path:
    """Generate a thin top-level wrapper with specific parameters."""
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
    """Generate Vivado TCL build script for this config with strategy directives."""
    path = out_dir / f"sweep_{cfg.tag}.tcl"

    strat = STRATEGIES[cfg.strategy]

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

    # Build implementation commands with strategy directives
    synth_dir = f" {strat['synth']}" if strat['synth'] else ""
    opt_dir = f" {strat['opt']}" if strat['opt'] else ""
    place_dir = f" {strat['place']}" if strat['place'] else ""
    route_dir = f" {strat['route']}" if strat['route'] else ""

    # Multi-pass phys_opt_design
    phys_opt_cmds = []
    for i, po in enumerate(strat['phys_opt']):
        po_dir = f" {po}" if po else ""
        phys_opt_cmds.append(f"phys_opt_design{po_dir}")
    phys_opt_block = "\n".join(phys_opt_cmds)

    content = textwrap.dedent(f"""\
    # Auto-generated sweep build: {cfg.tag}
    # ATOMiK @ {cfg.freq_mhz} MHz (div={cfg.clk_div}), N_BANKS={cfg.n_banks}
    # Strategy: {cfg.strategy} — {strat['desc']}

    set_param general.maxThreads 4

    file mkdir {{{report_dir}}}

    {read_cmds}
    read_xdc {{{xdc_path}}}

    auto_detect_xpm
    synth_design -top {top_name} -part {PART}{synth_dir}
    report_utilization -file {{{report_dir}/util_{cfg.tag}.rpt}}

    opt_design{opt_dir}
    place_design{place_dir}
    {phys_opt_block}
    route_design{route_dir}

    report_timing_summary -file {{{report_dir}/timing_{cfg.tag}.rpt}}
    report_utilization -file {{{report_dir}/impl_util_{cfg.tag}.rpt}}
    report_utilization -hierarchical -hierarchical_depth 2 -file {{{report_dir}/hier_util_{cfg.tag}.rpt}}

    # Critical path detail for ceiling analysis
    report_timing -nworst 5 -path_type full -file {{{report_dir}/crit_path_{cfg.tag}.rpt}}

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

    m = re.search(r'Slice LUTs\s*\|\s*(\d+)\s*\|\s*\d+\s*\|\s*\d+\s*\|\s*(\d+)', text)
    if m:
        result["lut_used"] = int(m.group(1))
        result["lut_total"] = int(m.group(2))

    m = re.search(r'Slice Registers\s*\|\s*(\d+)\s*\|\s*\d+\s*\|\s*\d+\s*\|\s*(\d+)', text)
    if m:
        result["ff_used"] = int(m.group(1))
        result["ff_total"] = int(m.group(2))

    m = re.search(r'Block RAM Tile\s*\|\s*([\d\.]+)\s*\|\s*\d+\s*\|\s*\d+\s*\|\s*([\d\.]+)', text)
    if m:
        result["bram_used"] = float(m.group(1))
        result["bram_total"] = float(m.group(2))

    return result


# ---------------------------------------------------------------------------
# Build a single config and return result
# ---------------------------------------------------------------------------

def build_one(cfg: SweepConfig, sweep_dir: Path) -> SweepResult:
    """Generate files, run Vivado, parse reports for one configuration."""
    top_dir = sweep_dir / "top"
    xdc_dir = sweep_dir / "xdc"
    tcl_dir = sweep_dir / "tcl"
    report_dir = sweep_dir / "reports"

    for d in [top_dir, xdc_dir, tcl_dir, report_dir]:
        d.mkdir(parents=True, exist_ok=True)

    top_path = gen_sweep_top(cfg, top_dir)
    xdc_path = gen_sweep_xdc(cfg, xdc_dir)
    tcl_path = gen_sweep_tcl(cfg, top_path, xdc_path, tcl_dir, report_dir)

    t0 = time.time()
    try:
        proc = subprocess.run(
            ["vivado", "-mode", "batch", "-source", str(tcl_path)],
            capture_output=True, text=True, timeout=5400,
            cwd=str(ZYNQ_DIR)
        )
        build_time = time.time() - t0
        success = proc.returncode == 0 and "SWEEP_DONE" in proc.stdout
    except subprocess.TimeoutExpired:
        build_time = time.time() - t0
        success = False
        proc = type('obj', (object,), {'stdout': '', 'stderr': 'TIMEOUT'})()

    if not success:
        err_lines = [l for l in (proc.stdout + proc.stderr).split("\n")
                     if "ERROR" in l][:3]
        error_msg = "; ".join(err_lines) if err_lines else "Build failed"
        return SweepResult(
            tag=cfg.tag, freq_mhz=cfg.freq_mhz, n_banks=cfg.n_banks,
            strategy=cfg.strategy,
            timing_met=False, wns_ns=0, tns_ns=0, fmax_mhz=0,
            lut_used=0, lut_total=53200, ff_used=0, ff_total=106400,
            bram_used=0, bram_total=140, build_time_s=round(build_time, 1),
            error=error_msg
        )

    # Parse reports
    timing_rpt = report_dir / f"timing_{cfg.tag}.rpt"
    util_rpt = report_dir / f"impl_util_{cfg.tag}.rpt"

    wns, tns = parse_timing_report(timing_rpt)
    util = parse_utilization_report(util_rpt)

    if wns is None:
        return SweepResult(
            tag=cfg.tag, freq_mhz=cfg.freq_mhz, n_banks=cfg.n_banks,
            strategy=cfg.strategy,
            timing_met=False, wns_ns=0, tns_ns=0, fmax_mhz=0,
            lut_used=util["lut_used"], lut_total=util["lut_total"],
            ff_used=util["ff_used"], ff_total=util["ff_total"],
            bram_used=util["bram_used"], bram_total=util["bram_total"],
            build_time_s=round(build_time, 1),
            error="No constrained timing paths"
        )

    period = cfg.period_ns
    fmax = 1000.0 / (period - wns) if (period - wns) > 0 else 9999.0
    timing_met = wns >= 0

    return SweepResult(
        tag=cfg.tag, freq_mhz=cfg.freq_mhz, n_banks=cfg.n_banks,
        strategy=cfg.strategy,
        timing_met=timing_met, wns_ns=wns, tns_ns=tns,
        fmax_mhz=round(fmax, 1),
        lut_used=util["lut_used"], lut_total=util["lut_total"],
        ff_used=util["ff_used"], ff_total=util["ff_total"],
        bram_used=util["bram_used"], bram_total=util["bram_total"],
        build_time_s=round(build_time, 1)
    )


def print_result(result: SweepResult):
    """Print a single result line."""
    if result.error:
        print(f"  ERROR: {result.error} ({result.build_time_s:.0f}s)")
        return
    status = "PASS" if result.timing_met else "FAIL"
    margin = f"+{result.wns_ns:.3f}ns" if result.wns_ns >= 0 else f"{result.wns_ns:.3f}ns"
    lut_pct = 100 * result.lut_used / result.lut_total if result.lut_total else 0
    print(f"  {status}: Fmax={result.fmax_mhz:.1f} MHz, WNS={margin}, "
          f"LUT={result.lut_used} ({lut_pct:.1f}%), "
          f"FF={result.ff_used}, BRAM={result.bram_used:.0f}, "
          f"({result.build_time_s:.0f}s)")


# ---------------------------------------------------------------------------
# Ceiling search — find maximum achievable MMCM frequency per N_BANKS
# ---------------------------------------------------------------------------

def run_ceiling_search(banks: list[int], strategies: list[str]) -> list[SweepResult]:
    """For each N_BANKS, walk up through MMCM frequencies to find the ceiling.

    Algorithm per N:
      1. Estimate starting frequency from previous 400 MHz baseline data
      2. Start 2 MMCM steps below the estimate (should pass)
      3. Walk up through MMCM steps with primary strategy until 2 consecutive fails
      4. At the ceiling step (last pass), try all requested strategies
      5. Report the best result per N

    Returns all results (for analysis), with ceiling winners marked.
    """
    sweep_dir = ZYNQ_DIR / "sweep"
    results_dir = sweep_dir / "results"
    results_dir.mkdir(parents=True, exist_ok=True)

    all_results: list[SweepResult] = []
    ceiling_results: list[SweepResult] = []

    # Known Fmax estimates from 400 MHz baseline sweep (conservative starting points)
    # These are the Fmax values computed from WNS at 400 MHz target
    baseline_fmax = {
        1: 413, 2: 396, 4: 340, 8: 311, 16: 270,
        32: 235, 64: 211, 128: 176, 256: 144,
        512: 125, 768: 110, 1024: 100,
    }

    # All MMCM frequency steps (ascending)
    all_steps = mmcm_frequencies(min_mhz=100.0, max_mhz=550.0)

    primary_strategy = strategies[0]  # Use first strategy for the walk

    print(f"\n{'='*78}")
    print(f" ATOMiK Zynq CEILING SEARCH — {len(banks)} bank config(s)")
    print(f" Target: {PART}, VCO: {VCO_MHZ:.0f} MHz")
    print(f" Primary strategy: {primary_strategy} ({STRATEGIES[primary_strategy]['desc']})")
    if len(strategies) > 1:
        print(f" Boundary strategies: {', '.join(strategies)}")
    print(f" MMCM steps available: {len(all_steps)} ({all_steps[0][0]:.1f}–{all_steps[-1][0]:.1f} MHz)")
    print(f"{'='*78}\n")

    for n_banks in banks:
        est_fmax = baseline_fmax.get(n_banks, 200)
        print(f"\n{'─'*78}")
        print(f" N_BANKS={n_banks} — estimated Fmax ~{est_fmax} MHz")
        print(f"{'─'*78}")

        # Find starting MMCM step: 2 steps below estimated Fmax
        start_idx = 0
        for i, (freq, div) in enumerate(all_steps):
            if freq >= est_fmax:
                start_idx = max(0, i - 2)
                break

        # Walk up from start
        consecutive_fails = 0
        last_pass_result = None
        last_pass_freq = None
        first_fail_freq = None
        walk_results = []

        for freq, div in all_steps[start_idx:]:
            if consecutive_fails >= 2:
                break

            cfg = SweepConfig(freq_mhz=freq, clk_div=div,
                              n_banks=n_banks, strategy=primary_strategy)
            print(f"\n  [{primary_strategy}] N={n_banks} @ {freq:.1f} MHz (div={div})...")
            result = build_one(cfg, sweep_dir)
            print_result(result)
            all_results.append(result)
            walk_results.append(result)

            if result.timing_met and not result.error:
                last_pass_result = result
                last_pass_freq = freq
                consecutive_fails = 0
            else:
                if first_fail_freq is None:
                    first_fail_freq = freq
                consecutive_fails += 1

        if last_pass_result is None:
            print(f"\n  !! N={n_banks}: No passing frequency in upward walk from {all_steps[start_idx][0]:.1f} MHz")
            print(f"     Walking DOWNWARD to find actual ceiling...")

            # Walk downward from start_idx - 1
            for freq, div in reversed(all_steps[:start_idx]):
                cfg = SweepConfig(freq_mhz=freq, clk_div=div,
                                  n_banks=n_banks, strategy=primary_strategy)
                print(f"\n  [{primary_strategy}] N={n_banks} @ {freq:.1f} MHz (div={div})...")
                result = build_one(cfg, sweep_dir)
                print_result(result)
                all_results.append(result)
                walk_results.append(result)

                if result.timing_met and not result.error:
                    last_pass_result = result
                    last_pass_freq = freq
                    # The first_fail_freq is the step above this pass
                    idx_here = next(i for i, (f, _) in enumerate(all_steps) if abs(f - freq) < 0.01)
                    if idx_here + 1 < len(all_steps):
                        first_fail_freq = all_steps[idx_here + 1][0]
                    break

        if last_pass_result is None:
            print(f"\n  !! N={n_banks}: No passing frequency found at any MMCM step!")
            continue

        print(f"\n  >> N={n_banks} ceiling with '{primary_strategy}': "
              f"{last_pass_freq:.1f} MHz (Fmax={last_pass_result.fmax_mhz:.1f} MHz)")

        # Try additional strategies at the ceiling and one step above
        best_for_n = last_pass_result

        if len(strategies) > 1 and first_fail_freq is not None:
            print(f"\n  Trying {len(strategies)-1} additional strategies at boundary...")

            # Try all strategies at the first failing frequency
            for strat_name in strategies[1:]:
                fail_div = VCO_MHZ / first_fail_freq
                fail_div = round(fail_div * 8) / 8
                actual_freq = VCO_MHZ / fail_div

                cfg = SweepConfig(freq_mhz=actual_freq, clk_div=fail_div,
                                  n_banks=n_banks, strategy=strat_name)
                print(f"\n  [{strat_name}] N={n_banks} @ {actual_freq:.1f} MHz (div={fail_div})...")
                result = build_one(cfg, sweep_dir)
                print_result(result)
                all_results.append(result)

                if result.timing_met and not result.error:
                    print(f"  !! Strategy '{strat_name}' CLOSES TIMING at {actual_freq:.1f} MHz!")
                    if result.freq_mhz > best_for_n.freq_mhz:
                        best_for_n = result

            # Also try additional strategies at the ceiling freq for resource comparison
            for strat_name in strategies[1:]:
                ceil_div = VCO_MHZ / last_pass_freq
                ceil_div = round(ceil_div * 8) / 8
                actual_freq = VCO_MHZ / ceil_div

                cfg = SweepConfig(freq_mhz=actual_freq, clk_div=ceil_div,
                                  n_banks=n_banks, strategy=strat_name)
                print(f"\n  [{strat_name}] N={n_banks} @ {actual_freq:.1f} MHz (resource comparison)...")
                result = build_one(cfg, sweep_dir)
                print_result(result)
                all_results.append(result)

                if result.timing_met and not result.error:
                    # Prefer higher Fmax, then lower LUT
                    if (result.fmax_mhz > best_for_n.fmax_mhz or
                        (result.freq_mhz == best_for_n.freq_mhz and
                         result.lut_used < best_for_n.lut_used)):
                        best_for_n = result

        ceiling_results.append(best_for_n)
        print(f"\n  ★ N={n_banks} CEILING: {best_for_n.freq_mhz:.1f} MHz "
              f"(Fmax={best_for_n.fmax_mhz:.1f} MHz, "
              f"LUT={best_for_n.lut_used}, strategy={best_for_n.strategy})")

    # Save and print summary
    save_ceiling_results(ceiling_results, all_results, results_dir)
    return all_results


def save_ceiling_results(ceiling: list[SweepResult], all_results: list[SweepResult],
                         results_dir: Path):
    """Save ceiling results and print the definitive table."""
    json_path = results_dir / "ceiling_summary.json"
    data = {
        "timestamp": datetime.now().isoformat(),
        "part": PART,
        "vco_mhz": VCO_MHZ,
        "mode": "ceiling",
        "ceiling": [asdict(r) for r in ceiling],
        "all_results": [asdict(r) for r in all_results],
    }
    json_path.write_text(json.dumps(data, indent=2))
    print(f"\nResults saved to: {json_path}")

    # Definitive ceiling table
    print(f"\n{'='*100}")
    print(f" ATOMiK ZYNQ CEILING — Maximum Achievable Frequency per N_BANKS")
    print(f" Part: {PART}, VCO: {VCO_MHZ:.0f} MHz")
    print(f"{'='*100}")
    print(f"{'N':>5}  {'Strategy':<12} {'MMCM':>8} {'Fmax':>8} {'WNS':>8} "
          f"{'LUT':>6} {'LUT%':>6} {'FF':>6} {'BRAM':>5} "
          f"{'N×Fmax':>9} {'Build':>7}")
    print(f"{'-'*5}  {'-'*12} {'-'*8} {'-'*8} {'-'*8} "
          f"{'-'*6} {'-'*6} {'-'*6} {'-'*5} "
          f"{'-'*9} {'-'*7}")

    for r in ceiling:
        lut_pct = 100 * r.lut_used / r.lut_total if r.lut_total else 0
        throughput = r.n_banks * r.fmax_mhz
        margin = f"+{r.wns_ns:.3f}" if r.wns_ns >= 0 else f"{r.wns_ns:.3f}"
        print(f"{r.n_banks:>5}  {r.strategy:<12} {r.freq_mhz:>7.1f}M {r.fmax_mhz:>7.1f}M "
              f"{margin:>8} {r.lut_used:>6} {lut_pct:>5.1f}% {r.ff_used:>6} "
              f"{r.bram_used:>5.0f} {throughput:>8.0f}M {r.build_time_s:>6.0f}s")

    # Efficiency analysis
    print(f"\n{'─'*100}")
    print(f" Efficiency Analysis")
    print(f"{'─'*100}")
    if len(ceiling) >= 2:
        base = ceiling[0]
        for r in ceiling[1:]:
            if base.lut_used > 0 and base.fmax_mhz > 0:
                lut_ratio = r.lut_used / base.lut_used
                tput_ratio = (r.n_banks * r.fmax_mhz) / (base.n_banks * base.fmax_mhz)
                eff = tput_ratio / lut_ratio if lut_ratio > 0 else 0
                marginal_lut = (r.lut_used - base.lut_used) / max(1, r.n_banks - base.n_banks)
                print(f"  N={r.n_banks:>3} vs N={base.n_banks}: "
                      f"{lut_ratio:.1f}x LUT, {tput_ratio:.1f}x throughput, "
                      f"efficiency={eff:.2f}, ~{marginal_lut:.0f} LUT/bank")

    # Peak throughput
    if ceiling:
        peak = max(ceiling, key=lambda r: r.n_banks * r.fmax_mhz)
        print(f"\n  ★ PEAK THROUGHPUT: N={peak.n_banks} @ {peak.freq_mhz:.1f} MHz → "
              f"{peak.n_banks * peak.fmax_mhz:,.0f} Mops/s "
              f"({peak.lut_used} LUT, {100*peak.lut_used/peak.lut_total:.1f}%)")

    print()


# ---------------------------------------------------------------------------
# Fixed-frequency sweep (original mode)
# ---------------------------------------------------------------------------

def freq_to_div(freq_mhz: float) -> float:
    """Convert target frequency to MMCM divider. VCO = 1000 MHz."""
    return VCO_MHZ / freq_mhz


def build_configs(freqs: list[float], banks: list[int],
                  strategy: str = DEFAULT_STRATEGY) -> list[SweepConfig]:
    """Build sweep configurations from frequency and bank lists."""
    configs = []
    for n in banks:
        for f in freqs:
            div = freq_to_div(f)
            if div < MMCM_DIV_MIN or div > MMCM_DIV_MAX:
                print(f"  SKIP: {f} MHz (div={div:.3f} out of MMCM range)")
                continue
            div = round(div * 8) / 8
            actual_freq = VCO_MHZ / div
            configs.append(SweepConfig(freq_mhz=actual_freq, clk_div=div,
                                       n_banks=n, strategy=strategy))
    return configs


def run_sweep(configs: list[SweepConfig], gen_only: bool = False) -> list[SweepResult]:
    """Generate files and optionally run Vivado for each config."""
    sweep_dir = ZYNQ_DIR / "sweep"
    results_dir = sweep_dir / "results"
    results_dir.mkdir(parents=True, exist_ok=True)

    results = []

    print(f"\n{'='*70}")
    print(f" ATOMiK Zynq Fmax Sweep — {len(configs)} configurations")
    print(f" Target: {PART}")
    print(f" VCO: {VCO_MHZ:.0f} MHz (FCLK={FCLK_MHZ:.0f} x {VCO_MULT:.0f})")
    print(f"{'='*70}\n")

    for i, cfg in enumerate(configs):
        print(f"[{i+1}/{len(configs)}] {cfg.tag}: {cfg.freq_mhz:.1f} MHz (div={cfg.clk_div}), "
              f"N_BANKS={cfg.n_banks}, strategy={cfg.strategy}")

        if gen_only:
            top_dir = sweep_dir / "top"
            xdc_dir = sweep_dir / "xdc"
            tcl_dir = sweep_dir / "tcl"
            report_dir = sweep_dir / "reports"
            for d in [top_dir, xdc_dir, tcl_dir, report_dir]:
                d.mkdir(parents=True, exist_ok=True)
            top_path = gen_sweep_top(cfg, top_dir)
            xdc_path = gen_sweep_xdc(cfg, xdc_dir)
            tcl_path = gen_sweep_tcl(cfg, top_path, xdc_path, tcl_dir, report_dir)
            print(f"  Generated: {tcl_path.name}")
            continue

        result = build_one(cfg, sweep_dir)
        print_result(result)
        results.append(result)

    if results and not gen_only:
        save_results(results, results_dir)

    return results


def save_results(results: list[SweepResult], results_dir: Path):
    """Save results as JSON and print summary table."""
    json_path = results_dir / "sweep_summary.json"
    data = {
        "timestamp": datetime.now().isoformat(),
        "part": PART,
        "vco_mhz": VCO_MHZ,
        "results": [asdict(r) for r in results]
    }
    json_path.write_text(json.dumps(data, indent=2))
    print(f"\nResults saved to: {json_path}")

    print(f"\n{'='*100}")
    print(f" SWEEP RESULTS SUMMARY")
    print(f"{'='*100}")
    print(f"{'Config':<20} {'Target':>8} {'Fmax':>8} {'WNS':>8} "
          f"{'LUT':>10} {'FF':>10} {'BRAM':>6} {'N×Fmax':>9} {'Status':>8}")
    print(f"{'-'*20} {'-'*8} {'-'*8} {'-'*8} "
          f"{'-'*10} {'-'*10} {'-'*6} {'-'*9} {'-'*8}")

    for r in results:
        if r.error:
            print(f"{r.tag:<20} {r.freq_mhz:>7.1f}M {'---':>8} {'---':>8} "
                  f"{'---':>10} {'---':>10} {'---':>6} {'---':>9} {'ERROR':>8}")
        else:
            lut_pct = f"{r.lut_used}({100*r.lut_used/r.lut_total:.1f}%)"
            ff_pct = f"{r.ff_used}({100*r.ff_used/r.ff_total:.1f}%)"
            throughput = r.n_banks * r.fmax_mhz
            status = "PASS" if r.timing_met else "FAIL"
            print(f"{r.tag:<20} {r.freq_mhz:>7.1f}M {r.fmax_mhz:>7.1f}M "
                  f"{r.wns_ns:>7.3f} "
                  f"{lut_pct:>10} {ff_pct:>10} {r.bram_used:>5.0f} "
                  f"{throughput:>8.0f}M {status:>8}")

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
    parser = argparse.ArgumentParser(
        description="ATOMiK Zynq Fmax Sweep — frequency characterization & ceiling search",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
        Strategies:
          baseline    Vivado defaults — minimum effort
          aggressive  Production settings — Explore + AggressiveExplore (default)
          maximum     Multi-pass phys_opt, PerformanceOptimized synth
          area        AreaOptimized_high — minimize resource usage

        Examples:
          %(prog)s --ceiling --banks 1,4,16,64,256        # Full ceiling hunt
          %(prog)s --ceiling --banks 1 --strategy maximum  # Single-bank max effort
          %(prog)s --freqs 300,350,400 --strategy area     # Area optimization study
        """))

    parser.add_argument("--freqs", type=str, default=None,
                        help="Comma-separated target frequencies in MHz")
    parser.add_argument("--banks", type=str, default=None,
                        help="Comma-separated N_BANKS values (default: 1)")
    parser.add_argument("--quick", action="store_true",
                        help="Quick sweep: 200,300,400 MHz only")
    parser.add_argument("--gen-only", action="store_true",
                        help="Generate files only, don't run Vivado")
    parser.add_argument("--strategy", type=str, default=DEFAULT_STRATEGY,
                        choices=list(STRATEGIES.keys()),
                        help=f"Vivado strategy preset (default: {DEFAULT_STRATEGY})")
    parser.add_argument("--ceiling", action="store_true",
                        help="Find maximum frequency per N_BANKS (ascending walk)")
    parser.add_argument("--all-strategies", action="store_true",
                        help="With --ceiling: try all strategies at the boundary")
    args = parser.parse_args()

    banks = [int(b) for b in args.banks.split(",")] if args.banks else DEFAULT_BANKS

    if args.ceiling:
        if args.all_strategies:
            strategies = [args.strategy] + [s for s in STRATEGIES if s != args.strategy]
        else:
            strategies = [args.strategy]

        run_ceiling_search(banks, strategies)
    else:
        if args.freqs:
            freqs = [float(f) for f in args.freqs.split(",")]
        elif args.quick:
            freqs = QUICK_FREQS
        else:
            freqs = DEFAULT_FREQS

        configs = build_configs(freqs, banks, strategy=args.strategy)
        if not configs:
            print("No valid configurations to sweep.")
            sys.exit(1)

        results = run_sweep(configs, gen_only=args.gen_only)

        if not args.gen_only and results:
            any_fail = any(not r.timing_met for r in results if not r.error)
            sys.exit(1 if any_fail else 0)


if __name__ == "__main__":
    main()
