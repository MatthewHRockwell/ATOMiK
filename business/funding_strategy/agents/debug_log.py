"""Structured JSON issue log — single source of truth for debugging."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path

from rich.console import Console
from rich.table import Table


class DebugLog:
    """Append-only structured issue log persisted to JSON."""

    def __init__(
        self, path: str = "business/funding_strategy/debug_log.json"
    ) -> None:
        self.path = Path(path)
        self._entries: list[dict] = []
        if self.path.exists():
            try:
                self._entries = json.loads(
                    self.path.read_text(encoding="utf-8")
                )
            except (json.JSONDecodeError, ValueError):
                self._entries = []

    def _save(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_text(
            json.dumps(self._entries, indent=2, ensure_ascii=False),
            encoding="utf-8",
        )

    def log(
        self,
        workstream: str,
        program: str,
        severity: str,
        summary: str,
        details: str = "",
    ) -> int:
        """Append an issue entry. Returns its index."""
        entry = {
            "index": len(self._entries),
            "timestamp": datetime.now(timezone.utc).isoformat(
                timespec="seconds"
            ),
            "workstream": workstream,
            "program": program,
            "severity": severity,
            "summary": summary,
            "details": details,
            "resolved": False,
            "resolution": "",
        }
        self._entries.append(entry)
        self._save()
        return entry["index"]

    def resolve(self, index: int, resolution: str) -> None:
        """Mark an issue as resolved."""
        if 0 <= index < len(self._entries):
            self._entries[index]["resolved"] = True
            self._entries[index]["resolution"] = resolution
            self._save()

    def get_unresolved(self) -> list[dict]:
        """Return all unresolved issues."""
        return [e for e in self._entries if not e.get("resolved")]

    def show(self) -> None:
        """Display issue log as a Rich table."""
        console = Console()
        table = Table(title="Debug Log", show_lines=True)
        table.add_column("#", justify="right", style="dim")
        table.add_column("Time", style="dim")
        table.add_column("Workstream")
        table.add_column("Program")
        table.add_column("Severity")
        table.add_column("Summary", style="bold")
        table.add_column("Resolved?")

        for entry in self._entries:
            sev_colour = {
                "error": "red",
                "warning": "yellow",
                "info": "blue",
            }.get(entry.get("severity", ""), "white")
            resolved = (
                "[green]Yes[/green]"
                if entry.get("resolved")
                else "[red]No[/red]"
            )
            table.add_row(
                str(entry.get("index", "")),
                entry.get("timestamp", "")[:16],
                entry.get("workstream", ""),
                entry.get("program", ""),
                f"[{sev_colour}]{entry.get('severity', '')}[/{sev_colour}]",
                entry.get("summary", ""),
                resolved,
            )

        console.print(table)

        unresolved = self.get_unresolved()
        console.print(
            f"\n[bold]{len(unresolved)}[/bold] unresolved issue(s) "
            f"out of {len(self._entries)} total.\n"
        )
