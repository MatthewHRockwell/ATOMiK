"""Application status tracking with JSON persistence."""

from __future__ import annotations

import json
from dataclasses import asdict, dataclass, field
from datetime import datetime, timezone
from enum import Enum
from pathlib import Path


class ApplicationStatus(Enum):
    NOT_STARTED = "not_started"
    PREREQUISITES_MISSING = "prerequisites_missing"
    CONTENT_GENERATED = "content_generated"
    UNDER_REVIEW = "under_review"
    APPROVED = "approved"
    SUBMITTING = "submitting"
    SUBMITTED = "submitted"
    BLOCKED = "blocked"
    SKIPPED = "skipped"


@dataclass
class ProgramStatus:
    program_id: str
    name: str
    tier: int
    phase: int
    status: ApplicationStatus
    last_updated: str
    notes: str = ""
    submission_url: str = ""
    output_dir: str = ""


class StatusTracker:
    """Persists per-program application status to a JSON file."""

    def __init__(self, path: str) -> None:
        self.path = Path(path)
        self._programs: dict[str, ProgramStatus] = {}
        if self.path.exists():
            self.load()

    def load(self) -> dict[str, ProgramStatus]:
        raw = json.loads(self.path.read_text(encoding="utf-8"))
        self._programs = {}
        for pid, data in raw.items():
            data["status"] = ApplicationStatus(data["status"])
            self._programs[pid] = ProgramStatus(**data)
        return self._programs

    def save(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        out: dict[str, dict] = {}
        for pid, ps in self._programs.items():
            d = asdict(ps)
            d["status"] = ps.status.value
            out[pid] = d
        self.path.write_text(
            json.dumps(out, indent=2, ensure_ascii=False), encoding="utf-8"
        )

    def ensure_program(
        self,
        program_id: str,
        name: str,
        tier: int,
        phase: int,
        url: str = "",
    ) -> None:
        """Register a program if not already tracked."""
        if program_id not in self._programs:
            self._programs[program_id] = ProgramStatus(
                program_id=program_id,
                name=name,
                tier=tier,
                phase=phase,
                status=ApplicationStatus.NOT_STARTED,
                last_updated=_now(),
                submission_url=url,
                output_dir=f"business/funding_strategy/output/{program_id}",
            )
            self.save()

    def get(self, program_id: str) -> ApplicationStatus:
        ps = self._programs.get(program_id)
        return ps.status if ps else ApplicationStatus.NOT_STARTED

    def update(
        self,
        program_id: str,
        status: ApplicationStatus,
        notes: str = "",
    ) -> None:
        if program_id in self._programs:
            ps = self._programs[program_id]
            ps.status = status
            ps.last_updated = _now()
            if notes:
                ps.notes = notes
            self.save()

    @property
    def programs(self) -> dict[str, ProgramStatus]:
        return dict(self._programs)

    def get_dashboard_data(self) -> list[ProgramStatus]:
        """Return all tracked programs sorted by phase then tier."""
        return sorted(
            self._programs.values(), key=lambda p: (p.phase, p.tier)
        )


def _now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")
