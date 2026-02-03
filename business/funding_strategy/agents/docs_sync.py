"""Auto-update documentation after every pipeline action."""

from __future__ import annotations

import re
from datetime import datetime, timezone
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .config import FundingConfig
    from .status import StatusTracker


class DocsSync:
    """Synchronise pipeline state into living documentation files."""

    def __init__(self, repo_root: str = ".") -> None:
        self.root = Path(repo_root).resolve()

    def sync(self, tracker: StatusTracker, config: FundingConfig) -> list[str]:
        """Run all sync operations. Returns list of files written."""
        written: list[str] = []
        written += self._update_playbook_checklist(tracker)
        written += self._update_progress_report(tracker, config)
        written += self._update_context_file(tracker, config)
        return written

    # ------------------------------------------------------------------
    # Playbook checklist
    # ------------------------------------------------------------------

    def _update_playbook_checklist(
        self, tracker: StatusTracker
    ) -> list[str]:
        """Toggle ``[ ]``/``[x]`` in funding_playbook.md based on status."""
        playbook = self.root / "business" / "funding_strategy" / "funding_playbook.md"
        if not playbook.exists():
            return []

        text = playbook.read_text(encoding="utf-8")
        programs = tracker.get_dashboard_data()
        changed = False

        for ps in programs:
            # Match lines containing the program name and a checkbox
            pattern = re.compile(
                rf"(- \[)([xX ]?)(\] .*{re.escape(ps.name)})",
                re.IGNORECASE,
            )
            submitted = ps.status.value == "submitted"
            new_mark = "x" if submitted else " "
            new_text = pattern.sub(rf"\g<1>{new_mark}\3", text)
            if new_text != text:
                text = new_text
                changed = True

        if changed:
            playbook.write_text(text, encoding="utf-8")
            return [str(playbook.relative_to(self.root))]
        return []

    # ------------------------------------------------------------------
    # Progress report
    # ------------------------------------------------------------------

    def _update_progress_report(
        self, tracker: StatusTracker, config: FundingConfig
    ) -> list[str]:
        """Generate ``progress_report.md`` from current tracker state."""
        programs = tracker.get_dashboard_data()
        submitted = sum(1 for p in programs if p.status.value == "submitted")
        ready = sum(
            1 for p in programs if p.status.value not in ("blocked", "prerequisites_missing")
        )
        blocked = sum(
            1 for p in programs if p.status.value in ("blocked", "prerequisites_missing")
        )

        lines = [
            "# ATOMiK Funding Pipeline — Progress Report",
            "",
            f"*Generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}*",
            "",
            "## Summary",
            "",
            f"- **Entity**: {config.company.legal_name}",
            f"- **Programs tracked**: {len(programs)}",
            f"- **Submitted**: {submitted}",
            f"- **Ready**: {ready}",
            f"- **Blocked**: {blocked}",
            "",
            "## Per-Program Status",
            "",
            "| Program | Phase | Status | Last Updated |",
            "|---------|-------|--------|-------------|",
        ]

        for ps in programs:
            lines.append(
                f"| {ps.name} | {ps.phase} | {ps.status.value} "
                f"| {ps.last_updated[:10] if ps.last_updated else ''} |"
            )

        lines.append("")
        out = self.root / "business" / "funding_strategy" / "progress_report.md"
        out.write_text("\n".join(lines), encoding="utf-8")
        return [str(out.relative_to(self.root))]

    # ------------------------------------------------------------------
    # FUNDING_CONTEXT.md
    # ------------------------------------------------------------------

    def _update_context_file(
        self, tracker: StatusTracker, config: FundingConfig
    ) -> list[str]:
        """Regenerate the living ``FUNDING_CONTEXT.md`` at project root."""
        programs = tracker.get_dashboard_data()
        submitted = sum(1 for p in programs if p.status.value == "submitted")
        ready = sum(
            1 for p in programs if p.status.value not in ("blocked", "prerequisites_missing")
        )
        blocked = sum(
            1 for p in programs if p.status.value in ("blocked", "prerequisites_missing")
        )

        ctx_path = self.root / "FUNDING_CONTEXT.md"

        # Preserve human-managed sections if the file already exists
        decisions_section = ""
        dropped_section = ""
        if ctx_path.exists():
            old = ctx_path.read_text(encoding="utf-8")
            decisions_section = _extract_section(old, "Decisions")
            dropped_section = _extract_section(old, "Dropped Items")

        if not decisions_section:
            decisions_section = (
                "- Demo: state sync benchmarks, NOT LLM training\n"
                "- AI video: requires disclosure on every video\n"
                "- Entity: LLC now, C-Corp conversion pending\n"
            )

        if not dropped_section:
            dropped_section = "- (none yet)\n"

        inc_status = "completed" if config.incorporation_completed else "pending"
        sam_status = "registered" if config.sam_gov_registered else "not started"

        lines = [
            "# ATOMiK Funding Pipeline — Current Context",
            "",
            "## Status Snapshot",
            "",
            f"- Entity: {config.company.legal_name}",
            f"- Incorporation: {inc_status}",
            f"- SAM.gov: {sam_status}",
            f"- Programs: {submitted}/{len(programs)} submitted, "
            f"{ready}/{len(programs)} actionable, "
            f"{blocked}/{len(programs)} blocked",
            "",
            "## Key Files",
            "",
            "- Config: `business/funding_strategy/config.yaml`",
            "- Status: `business/funding_strategy/status.json`",
            "- Debug: `business/funding_strategy/debug_log.json`",
            "- Data room: `business/data_room/`",
            "- Playbook: `business/funding_strategy/funding_playbook.md`",
            "",
            "## Decisions",
            "",
            decisions_section.rstrip(),
            "",
            "## Dropped Items",
            "",
            dropped_section.rstrip(),
            "",
        ]

        ctx_path.write_text("\n".join(lines), encoding="utf-8")
        return [str(ctx_path.relative_to(self.root))]


def _extract_section(text: str, heading: str) -> str:
    """Pull content between ``## heading`` and the next ``##``."""
    pattern = re.compile(
        rf"^## {re.escape(heading)}\s*\n(.*?)(?=^## |\Z)",
        re.MULTILINE | re.DOTALL,
    )
    m = pattern.search(text)
    return m.group(1).strip() + "\n" if m else ""
