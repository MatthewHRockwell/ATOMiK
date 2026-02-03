"""Content engine — reads existing ATOMiK materials and adapts per-program."""

from __future__ import annotations

import re
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from ..config import FundingConfig

# ---------------------------------------------------------------------------
# Pitch angles keyed by program_id (extracted from the funding playbook).
# ---------------------------------------------------------------------------
_PITCH_ANGLES: dict[str, str] = {
    "nvidia_inception": (
        "Hardware-accelerated state management for AI inference pipelines. "
        "Delta-state accumulation reduces memory bandwidth by 95-100%, "
        "enabling faster model serving at the edge."
    ),
    "cdl": (
        "A new computing primitive — delta-state algebra in silicon. Not an "
        "incremental improvement to existing architectures but a "
        "mathematically novel approach with 92 formal proofs. Fits CDL's "
        "mandate to support science-based ventures."
    ),
    "icorps": (
        "Use $50K to validate which vertical (HFT, IoT, video processing, "
        "database replication) has the highest willingness-to-pay for "
        "delta-state hardware IP. 100+ customer discovery interviews."
    ),
    "nsf_sbir": (
        "Novel semiconductor architecture with 92 formal proofs — advancing "
        "the state of the art in verified hardware design. Hardware-"
        "accelerated delta-state algebra with applications in AI inference, "
        "sensor fusion, and secure state management."
    ),
    "dod_sbir": (
        "Formally verified hardware for assured state management at the "
        "tactical edge. 92 Lean4 proofs guarantee correctness properties "
        "(commutativity, idempotence, self-inverse) that eliminate classes "
        "of hardware bugs. Single-cycle operation at ~20 mW."
    ),
    "doe_sbir": (
        "Delta-state accumulation eliminates 95-100% of memory bus traffic "
        "compared to full-state copy architectures. At ~20 mW on a $10 "
        "FPGA, ATOMiK demonstrates a path to orders-of-magnitude energy "
        "reduction in state-heavy workloads."
    ),
    "nasa_sbir": (
        "XOR-based accumulation is inherently radiation-tolerant (single-bit "
        "upsets are self-correcting via commutative merge). 92 formal proofs "
        "provide flight-system-grade assurance without simulation gaps."
    ),
    "chips_act": (
        "Novel XOR-based computing primitive that achieves linear throughput "
        "scaling in 7% LUT utilization — extreme area efficiency for a fully "
        "formally verified hardware block. Applicable as a co-processor IP "
        "block for next-generation American semiconductor designs."
    ),
    "silicon_catalyst": (
        "Formally verified XOR accumulator IP targeting ASIC integration. 92 "
        "Lean4 proofs, working FPGA prototype on Tang Nano 9K, 7% LUT "
        "utilization (single bank). Seeking EDA tool access and fab partner "
        "introductions to move from FPGA to ASIC."
    ),
    "alchemist": (
        "B2B IP licensing play — delta-state hardware blocks for chip "
        "designers and system integrators. Proven architecture (1 Gops/s on "
        "$10 FPGA), 5-language SDK for customer integration, patent pending."
    ),
    "hax": (
        "Working hardware on $10 FPGA — not a slide deck, not a simulation. "
        "1 Gops/s throughput, 92 formal proofs, $225 total development cost. "
        "Ready for HAX's hardware-to-market pipeline."
    ),
    "yc": (
        "Hardware that works, math that's proven, software that ships. 92 "
        "formal proofs, 1 Gops/s on a $10 chip, 314 passing tests, $225 "
        "total spend. The IP licensing business model scales without "
        "manufacturing risk."
    ),
    "techstars": (
        "Novel computing IP with working hardware and multi-language SDK. "
        "Seeking commercial pilot customers in HFT, IoT, or database "
        "replication verticals."
    ),
    "vc_outreach": (
        "IP licensing in $600B+ semiconductor market — irreproducible formal "
        "verification moat, working silicon, patent pending."
    ),
    "defense_outreach": (
        "Assured hardware for intelligence/defense community — formally "
        "verified, self-inverse (instant rollback), commutative (lock-free "
        "merge). Dual-use hardware IP."
    ),
}


class ContentEngine:
    """Reads existing ATOMiK materials and provides adapted content."""

    def __init__(self, config: FundingConfig) -> None:
        self.config = config
        self._one_pager = self._load(config.materials.get("one_pager", ""))
        self._pitch_deck = self._load(config.materials.get("pitch_deck_md", ""))
        self._playbook = self._load(config.materials.get("playbook", ""))

    @staticmethod
    def _load(path: str) -> str:
        if not path:
            return ""
        p = Path(path)
        if p.exists():
            return p.read_text(encoding="utf-8")
        return ""

    # ------------------------------------------------------------------
    # Public extraction helpers
    # ------------------------------------------------------------------

    def get_company_description(self, max_chars: int = 500) -> str:
        """Extract a company description from the one-pager."""
        # First paragraph after the title heading.
        for block in self._one_pager.split("\n\n"):
            text = block.strip()
            if text and not text.startswith("#") and not text.startswith("|"):
                return _truncate(text, max_chars)
        return _truncate(
            "ATOMiK is a formally verified hardware architecture for "
            "delta-state computing, achieving 1 billion operations per "
            "second on a $10 FPGA.",
            max_chars,
        )

    def get_key_metrics(self) -> dict[str, str]:
        """Extract the metrics table from the one-pager."""
        metrics: dict[str, str] = {}
        in_table = False
        for line in self._one_pager.splitlines():
            if "|" in line and "Metric" in line:
                in_table = True
                continue
            if in_table and line.startswith("|"):
                if "---" in line:
                    continue
                cols = [c.strip() for c in line.split("|") if c.strip()]
                if len(cols) >= 2:
                    metrics[cols[0]] = cols[1]
            elif in_table and not line.strip():
                break
        if not metrics:
            # Fallback hard-coded from known one-pager content.
            metrics = {
                "Throughput": "1,056 Mops/s (16 parallel banks)",
                "Operation latency": "10.6 ns (single cycle)",
                "Memory reduction": "95-100%",
                "Formal proofs": "92 (Lean4 verified)",
                "Hardware tests": "80/80 passing",
                "LUT utilization": "7% (single bank)",
                "SDK languages": "5 (Python, Rust, C, JavaScript, Verilog)",
                "SDK tests": "314 passing",
                "Device cost": "$10 (Tang Nano 9K FPGA)",
            }
        return metrics

    def get_pitch_for_program(self, program_id: str) -> str:
        """Return the program-specific pitch angle."""
        return _PITCH_ANGLES.get(program_id, _PITCH_ANGLES["vc_outreach"])

    def get_technical_summary(self, max_chars: int = 2000) -> str:
        """Extract a technical summary from the pitch deck."""
        # Look for the "Solution" or "Technical" slide content.
        section = _extract_section(
            self._pitch_deck,
            r"(?:Solution|Technical|Delta-State)",
        )
        if section:
            return _truncate(section, max_chars)
        return _truncate(
            "ATOMiK implements a delta-state XOR algebra in hardware. "
            "Rather than the traditional Load-Modify-Store cycle, ATOMiK "
            "accumulates deltas via single-cycle XOR operations: "
            "State = S0 XOR d1 XOR d2 ... XOR dn. "
            "This yields commutativity (lock-free parallelism), "
            "self-inverse (instant undo), and single-cycle latency "
            "(no carry propagation). The architecture scales linearly "
            "via parallel accumulator banks — proven to 16x on a "
            "Tang Nano 9K FPGA at 7% LUT utilization.",
            max_chars,
        )

    def get_market_applications(self) -> list[str]:
        """Extract market application bullet points."""
        apps: list[str] = []
        in_section = False
        for line in self._one_pager.splitlines():
            if "Market Application" in line or "Use Case" in line:
                in_section = True
                continue
            if in_section and line.startswith("- "):
                apps.append(line.lstrip("- ").strip())
            elif in_section and line.startswith("#"):
                break
        if not apps:
            apps = [
                "High-Frequency Trading: Single-cycle tick processing",
                "IoT/Sensor Fusion: Lock-free multi-stream merge",
                "Video Processing: 95% memory reduction",
                "Database Replication: O(1) state reconstruction",
                "Digital Twins: Commutative distributed sync",
                "Gaming: Order-independent multiplayer state",
            ]
        return apps

    def get_competitive_moat(self) -> str:
        """Extract the competitive moat section."""
        section = _extract_section(self._one_pager, r"Competitive Moat|Moat")
        if section:
            return section
        return (
            "Patent Pending: Architecture and execution model under IP "
            "protection.\n"
            "Formal Verification: 92 Lean4 proofs — machine-verified.\n"
            "Hardware Validated: Real FPGA silicon, not simulation.\n"
            "Full Stack: Math proofs + RTL + SDK + agentic pipeline.\n"
            "Linear Scaling: Proven to 16x, extends to 64x+."
        )

    def get_team_description(self) -> str:
        """Build team description from founder info + materials."""
        f = self.config.founder
        name = f.name or "the founder"
        title = f.title or "Founder & CEO"
        bio = f.bio or ""
        parts = [f"{name}, {title}."]
        if bio:
            parts.append(bio)
        parts.append(
            "Solo technical founder who built the full ATOMiK stack — "
            "from 92 Lean4 formal proofs to FPGA synthesis to a "
            "5-language SDK — for $225 total development cost."
        )
        return " ".join(parts)

    def get_traction(self) -> str:
        """Build traction narrative from known deliverables."""
        return (
            "92 formal proofs (Lean4, zero sorry statements), "
            "314 passing tests across 5 SDK languages, "
            "working FPGA prototype on Tang Nano 9K ($10), "
            "80/80 hardware tests passing, "
            "1,056 Mops/s throughput (16 parallel banks), "
            "$225 total development cost, "
            "patent pending, "
            "6 development phases complete."
        )

    def adapt_for_form(
        self, program_id: str, field_name: str, max_chars: int
    ) -> str:
        """Adapt content to fit a specific form field's character limit."""
        key = field_name.lower().replace(" ", "_")
        if "company" in key and "desc" in key:
            return self.get_company_description(max_chars)
        if "technical" in key or "approach" in key:
            return self.get_technical_summary(max_chars)
        if "traction" in key or "progress" in key:
            return _truncate(self.get_traction(), max_chars)
        if "team" in key:
            return _truncate(self.get_team_description(), max_chars)
        if "market" in key:
            return _truncate(
                "; ".join(self.get_market_applications()), max_chars
            )
        if "moat" in key or "competi" in key:
            return _truncate(self.get_competitive_moat(), max_chars)
        # Default: pitch angle for the program.
        return _truncate(self.get_pitch_for_program(program_id), max_chars)


# ------------------------------------------------------------------
# Helpers
# ------------------------------------------------------------------


def _truncate(text: str, max_chars: int) -> str:
    if len(text) <= max_chars:
        return text
    return text[: max_chars - 3].rsplit(" ", 1)[0] + "..."


def _extract_section(text: str, heading_pattern: str) -> str:
    """Extract text between a heading matching *heading_pattern* and the next heading."""
    lines = text.splitlines()
    collecting = False
    result: list[str] = []
    for line in lines:
        if re.search(heading_pattern, line, re.IGNORECASE) and line.lstrip().startswith("#"):
            collecting = True
            continue
        if collecting:
            if line.lstrip().startswith("#"):
                break
            result.append(line)
    return "\n".join(result).strip()
