"""FundingPipeline orchestrator — drives the full application workflow."""

from __future__ import annotations

import json
from pathlib import Path
from typing import TYPE_CHECKING

from .config import FundingConfig
from .debug_log import DebugLog
from .docs_sync import DocsSync
from .drivers.browser import BrowserDriver
from .drivers.email_sender import EmailSender
from .git_ops import GitOps
from .materials.content_engine import ContentEngine
from .programs import get_all_programs
from .review import ReviewUI
from .status import ApplicationStatus, StatusTracker

if TYPE_CHECKING:
    from .programs.base import ApplicationContent, BaseApplication


class FundingPipeline:
    """Top-level orchestrator for the ATOMiK funding agent system."""

    PHASES = {
        0: "Entity Formation",
        1: "Zero-Cost Programs",
        2: "Grant Preparation",
        3: "Accelerator Applications",
        4: "VC Outreach",
    }

    def __init__(
        self, config_path: str = "business/funding_strategy/config.yaml"
    ) -> None:
        self.config = FundingConfig.from_yaml(config_path)
        self.tracker = StatusTracker("business/funding_strategy/status.json")
        self.engine = ContentEngine(self.config)
        self.review = ReviewUI()
        self.debug_log = DebugLog()
        self.docs_sync = DocsSync()
        self.git_ops = GitOps()
        self.programs = self._register_programs()

    def _register_programs(self) -> list[BaseApplication]:
        """Instantiate all programme agents and register them in the tracker."""
        programs = get_all_programs()
        for prog in programs:
            self.tracker.ensure_program(
                program_id=prog.name,
                name=prog.display_name,
                tier=prog.tier,
                phase=prog.phase,
                url=prog.url,
            )
        return programs

    def plan(self) -> None:
        """Display the execution plan with readiness status."""
        self.review.show_plan(self.programs, self.config, self.tracker)

    async def begin(self, phase: int | None = None) -> None:
        """Execute applications, optionally filtering to a single phase."""
        browser = BrowserDriver(self.config.browser)
        emailer = (
            EmailSender(self.config.email)
            if self.config.email.smtp_host
            else None
        )

        if not self.review.confirm_begin():
            return

        try:
            for program in self.programs:
                if phase is not None and program.phase != phase:
                    continue

                current = self.tracker.get(program.name)
                if current == ApplicationStatus.SUBMITTED:
                    continue

                # Check prerequisites
                ready, missing = program.check_prerequisites(self.config)
                if not ready:
                    self.tracker.update(
                        program.name,
                        ApplicationStatus.PREREQUISITES_MISSING,
                        f"Missing: {', '.join(missing)}",
                    )
                    continue

                # Generate content
                content = program.generate_content(self.config, self.engine)
                self.tracker.update(
                    program.name, ApplicationStatus.CONTENT_GENERATED
                )

                # Save generated content
                self._save_content(program, content)

                # Human review
                self.tracker.update(
                    program.name, ApplicationStatus.UNDER_REVIEW
                )
                decision = self.review.review_content(content)
                if decision == "quit":
                    break
                if decision == "skip":
                    self.tracker.update(
                        program.name, ApplicationStatus.SKIPPED
                    )
                    continue
                if decision == "edit":
                    self.tracker.update(
                        program.name,
                        ApplicationStatus.APPROVED,
                        "User chose to edit — re-run after editing output files.",
                    )
                    continue

                # Approved — submit
                self.tracker.update(
                    program.name, ApplicationStatus.APPROVED
                )
                self.tracker.update(
                    program.name, ApplicationStatus.SUBMITTING
                )
                try:
                    success = await program.submit(
                        content, self.config, browser, emailer
                    )
                    status = (
                        ApplicationStatus.SUBMITTED
                        if success
                        else ApplicationStatus.APPROVED
                    )
                    self.tracker.update(program.name, status)
                    self.review.show_submission_result(
                        program.display_name, success, ""
                    )
                except Exception as exc:
                    self.tracker.update(
                        program.name,
                        ApplicationStatus.APPROVED,
                        str(exc),
                    )
                    self.debug_log.log(
                        workstream=f"phase-{program.phase}",
                        program=program.name,
                        severity="error",
                        summary=f"Submission failed: {exc}",
                        details=str(exc),
                    )
                    self.review.show_submission_result(
                        program.display_name, False, str(exc)
                    )
                # --- Post-program upkeep ---
                self._run_upkeep(program.name)

        finally:
            await browser.close()

        # Final sync after pipeline completes
        self._run_upkeep("pipeline_complete")
        self.review.show_status(self.tracker)

    def status(self) -> None:
        """Display progress dashboard (read-only, no auto-commit)."""
        self.docs_sync.sync(self.tracker, self.config)
        self.review.show_status(self.tracker)

    def _run_upkeep(self, context: str = "") -> None:
        """Post-action upkeep: sync docs, refresh data room, auto-commit."""
        try:
            # 1. Update all documentation
            written = self.docs_sync.sync(self.tracker, self.config)

            # 2. Regenerate data room if generator exists
            gen_script = Path("business/data_room/_generate.py")
            if gen_script.exists():
                import runpy
                try:
                    runpy.run_path(str(gen_script), run_name="__main__")
                except Exception as exc:
                    self.debug_log.log(
                        workstream="upkeep",
                        program=context,
                        severity="warning",
                        summary=f"Data room generation failed: {exc}",
                    )
        except Exception as exc:
            self.debug_log.log(
                workstream="upkeep",
                program=context,
                severity="warning",
                summary=f"Upkeep sync failed: {exc}",
            )

    @staticmethod
    def _save_content(
        program: BaseApplication, content: ApplicationContent
    ) -> None:
        """Persist generated content to the programme's output directory."""
        output_dir = Path(
            f"business/funding_strategy/output/{program.name}"
        )
        output_dir.mkdir(parents=True, exist_ok=True)

        # Save fields as JSON
        fields_path = output_dir / "fields.json"
        fields_path.write_text(
            json.dumps(content.fields, indent=2, ensure_ascii=False),
            encoding="utf-8",
        )

        # Save email content if present
        if content.email_subject or content.email_body:
            email_path = output_dir / "email_draft.txt"
            email_path.write_text(
                f"Subject: {content.email_subject}\n\n{content.email_body}",
                encoding="utf-8",
            )

        # Save notes
        if content.notes:
            notes_path = output_dir / "notes.txt"
            notes_path.write_text(content.notes, encoding="utf-8")
