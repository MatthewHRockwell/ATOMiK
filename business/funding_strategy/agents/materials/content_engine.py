"""Content engine — reads existing ATOMiK materials and adapts per-program.

PUBLICATION STATUS: INTERNAL FUNDING GENERATOR / REVIEW REQUIRED.
Generated material must be reviewed against docs/evidence-labels.md and
results/claims_registry.yaml before submission or external publication.
"""

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
        "Evaluate delta-state accumulation against memory-bandwidth pressure "
        "with claims tied to measured workload artifacts."
    ),
    "cdl": (
        "A new computing primitive — delta-state algebra in silicon. Not an "
        "incremental improvement to existing architectures but a "
        "mathematically novel approach with 108 formal proofs. Fits CDL's "
        "mandate to support science-based ventures."
    ),
    "icorps": (
        "Use customer discovery to validate which state-heavy vertical has "
        "real willingness to evaluate delta-state hardware IP."
    ),
    "nsf_sbir": (
        "Novel semiconductor architecture with 108 formal proofs — advancing "
        "the state of the art in verified hardware design. Hardware-"
        "accelerated delta-state algebra with applications in AI inference, "
        "sensor fusion, and secure state management."
    ),
    "dod_sbir": (
        "Formally specified hardware direction for assured state management "
        "at the tactical edge. Proof and hardware claims must stay scoped to "
        "the current artifact set."
    ),
    "doe_sbir": (
        "Delta-state accumulation may reduce memory traffic in state-heavy "
        "workloads. Energy and bandwidth claims require workload-specific "
        "measurement artifacts."
    ),
    "nasa_sbir": (
        "XOR-based accumulation and formal methods may be relevant to "
        "flight-system review, but radiation-tolerance and assurance claims "
        "require scoped validation."
    ),
    "chips_act": (
        "Novel XOR-based computing primitive with synthesis and prototype "
        "artifacts. Co-processor IP claims should be separated into measured, "
        "synthesis-validated, and roadmap tiers."
    ),
    "silicon_catalyst": (
        "Formally verified XOR accumulator IP targeting ASIC integration. 108 "
        "Lean4 proofs, working FPGA prototype on Tang Nano 9K, 7% LUT "
        "utilization (single bank). Seeking EDA tool access and fab partner "
        "introductions to move from FPGA to ASIC."
    ),
    "alchemist": (
        "B2B IP licensing hypothesis around delta-state hardware blocks for "
        "chip designers and system integrators. Use artifact-linked proof "
        "claims only."
    ),
    "hax": (
        "Working FPGA prototype and formal proof artifacts, with commercial "
        "hardware-to-market work still to validate."
    ),
    "yc": (
        "Prototype hardware, formal proof work, and software artifacts. The "
        "IP licensing business model remains a commercial validation path."
    ),
    "techstars": (
        "Novel computing IP with prototype hardware and multi-language SDK "
        "artifacts. Seeking design-partner discovery in state-heavy verticals."
    ),
    "vc_outreach": (
        "IP licensing in $600B+ semiconductor market — irreproducible formal "
        "verification moat, working silicon, patent pending."
    ),
    "defense_outreach": (
        "Assured-state hardware direction for intelligence/defense review. "
        "Rollback and merge claims must be tied to proof scope and workload "
        "assumptions."
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
            "ATOMiK is a state-aware compute architecture for systems that "
            "spend too much work rediscovering what changed. Public claims "
            "are separated by evidence label and artifact.",
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
                "Throughput": "Use only with linked measured or synthesis artifact",
                "Operation latency": "Artifact required",
                "Memory reduction": "Workload-specific; artifact required",
                "Formal proofs": "See current proof artifacts",
                "Hardware tests": "See current hardware-validation docs",
                "LUT utilization": "Use only with linked synthesis artifact",
                "SDK languages": "5 (Python, Rust, C, JavaScript, Verilog)",
                "SDK tests": "See current test output",
                "Device cost": "Use only with current board BOM/source",
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
            "Rather than relying only on full-state movement, ATOMiK "
            "accumulates deltas via XOR operations: "
            "State = S0 XOR d1 XOR d2 ... XOR dn. "
            "Commutativity and self-inverse properties are proof-scoped; "
            "latency, scaling, and area claims require linked hardware or "
            "synthesis artifacts.",
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
                "High-Frequency Trading: deterministic state-tracking evaluation",
                "IoT/Sensor Fusion: multi-stream delta evaluation",
                "Video Processing: delta-oriented workload experiments",
                "Database Replication: state reconstruction evaluation",
                "Digital Twins: distributed-state model review",
                "Gaming: rollback and state-sync experiments",
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
            "Formal Verification: 108 Lean4 proofs — machine-verified.\n"
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
            "from formal proof artifacts to FPGA synthesis work to a "
            "multi-language SDK. Use current artifact links for exact "
            "proof counts, test counts, and cost claims."
        )
        return " ".join(parts)

    def get_traction(self) -> str:
        """Build traction narrative from known deliverables."""
        return (
            "formal proof artifacts, SDK and software tests, working FPGA "
            "prototype artifacts, hardware-validation notes, live ATOMiK Desk "
            "screenshots, patent-pending status, and evidence labels. Exact "
            "performance, cost, and test-count claims require current links."
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
