"""Phase 0 agent: Incorporation (CA LLC -> DE C-Corp conversion)."""

from __future__ import annotations

import webbrowser
from typing import TYPE_CHECKING

from .base import ApplicationContent, BaseApplication, SubmissionMethod

if TYPE_CHECKING:
    from ..config import FundingConfig
    from ..drivers.browser import BrowserDriver
    from ..drivers.email_sender import EmailSender
    from ..materials.content_engine import ContentEngine


class Incorporation(BaseApplication):
    """Guide and track CA LLC to DE C-Corp statutory conversion."""

    name = "incorporation"
    display_name = "C-Corp Conversion (DE)"
    tier = 0
    phase = 0
    submission_method = SubmissionMethod.BROWSER
    url = "https://seedlegals.com"
    pitch_angle = "Entity formation for funding eligibility"

    def check_prerequisites(
        self, config: FundingConfig
    ) -> tuple[bool, list[str]]:
        # Incorporation is always actionable — it's the first step
        return True, []

    def generate_content(
        self, config: FundingConfig, engine: ContentEngine
    ) -> ApplicationContent:
        checklist = (
            f"# C-Corp Conversion Checklist\n\n"
            f"**Entity**: {config.company.legal_name}\n"
            f"**EIN**: {config.company.ein}\n"
            f"**Current**: {config.company.legal_name} (CA LLC)\n"
            f"**Target**: Delaware C-Corp\n\n"
            f"## Steps\n\n"
            f"1. [ ] Choose conversion service (SeedLegals recommended)\n"
            f"2. [ ] Draft Plan of Conversion\n"
            f"3. [ ] Member approval (sole member consent)\n"
            f"4. [ ] File with California SoS\n"
            f"5. [ ] File with Delaware SoS\n"
            f"6. [ ] Bylaws, stock agreement, 83(b) election\n"
            f"7. [ ] IP assignment ({config.founder.name} -> company)\n"
            f"8. [ ] EIN update (Form 8822-B if name changes)\n"
            f"9. [ ] Business licence + vendor updates\n"
            f"10. [ ] Delaware registered agent setup\n\n"
            f"See: business/funding_strategy/incorporation_guide.md\n"
        )

        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={"checklist": checklist},
            notes="Review the full guide before starting. Budget $850-2,300.",
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        webbrowser.open(self.url)
        return True
