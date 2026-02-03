"""Phase 0 agent: SAM.gov registration."""

from __future__ import annotations

import webbrowser
from typing import TYPE_CHECKING

from .base import ApplicationContent, BaseApplication, SubmissionMethod

if TYPE_CHECKING:
    from ..config import FundingConfig
    from ..drivers.browser import BrowserDriver
    from ..drivers.email_sender import EmailSender
    from ..materials.content_engine import ContentEngine


class SamGov(BaseApplication):
    """Guide and track SAM.gov entity registration."""

    name = "sam_gov"
    display_name = "SAM.gov Registration"
    tier = 0
    phase = 0
    submission_method = SubmissionMethod.BROWSER
    url = "https://sam.gov"
    pitch_angle = "Federal grants eligibility (SBIR, CHIPS Act)"

    def check_prerequisites(
        self, config: FundingConfig
    ) -> tuple[bool, list[str]]:
        missing = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        if not config.company.ein:
            missing.append("EIN")
        return len(missing) == 0, missing

    def generate_content(
        self, config: FundingConfig, engine: ContentEngine
    ) -> ApplicationContent:
        checklist = (
            f"# SAM.gov Registration Checklist\n\n"
            f"**Entity**: {config.company.legal_name}\n"
            f"**EIN**: {config.company.ein}\n"
            f"**Address**: {config.company.address}\n\n"
            f"## NAICS Codes\n\n"
            f"- 334413 — Semiconductor and Related Device Manufacturing\n"
            f"- 541715 — R&D in Physical, Engineering, and Life Sciences\n\n"
            f"## Steps\n\n"
            f"1. [ ] Create Login.gov account with MFA\n"
            f"2. [ ] Request UEI (1-3 business days)\n"
            f"3. [ ] Gather info: EIN, NAICS, banking details\n"
            f"4. [ ] Complete Core Data section\n"
            f"5. [ ] Representations & Certifications\n"
            f"6. [ ] Submit for validation\n"
            f"7. [ ] CAGE code assignment (1-7 days)\n"
            f"8. [ ] Final approval (10-15 days)\n\n"
            f"See: business/funding_strategy/sam_gov_guide.md\n"
        )

        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={"checklist": checklist},
            notes="Total timeline: 2-4 weeks. Required for SBIR/CHIPS Act.",
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
