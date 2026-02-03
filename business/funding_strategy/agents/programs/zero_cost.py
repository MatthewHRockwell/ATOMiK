"""Zero-cost program agents — NVIDIA Inception, CDL, I-Corps, Intel Partner Alliance."""

from __future__ import annotations

from typing import TYPE_CHECKING

from .base import ApplicationContent, BaseApplication, SubmissionMethod

if TYPE_CHECKING:
    from ..config import FundingConfig
    from ..drivers.browser import BrowserDriver
    from ..drivers.email_sender import EmailSender
    from ..materials.content_engine import ContentEngine


class NvidiaInception(BaseApplication):
    """NVIDIA Inception — free startup programme, rolling admission."""

    name = "nvidia_inception"
    display_name = "NVIDIA Inception"
    tier = 2
    phase = 1
    submission_method = SubmissionMethod.BROWSER
    url = "https://www.nvidia.com/en-us/startups/"
    pitch_angle = (
        "Hardware-accelerated state management for AI inference pipelines. "
        "Delta-state accumulation reduces memory bandwidth by 95-100%."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        if not config.company.website:
            missing.append("website")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Company Name": config.company.name,
                "Company Description": engine.get_company_description(500),
                "Industry": "AI Hardware / Semiconductors",
                "Team Size": str(config.company.employee_count),
                "Website": config.company.website,
                "Stage": "Pre-seed",
                "What are you building?": engine.get_technical_summary(1000),
                "How does your product use AI/GPUs?": engine.get_pitch_for_program(self.name),
            },
            notes=(
                "NVIDIA Inception form: navigate to nvidia.com/startups, "
                "click 'Apply Now', fill the form fields. "
                "Selectors may need updating if the page changes."
            ),
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        if browser is None:
            print("[nvidia_inception] No browser driver — cannot submit.")
            return False

        await browser.goto(self.url)
        await browser.wait_for_human(
            "Navigate to the NVIDIA Inception application form, then press Enter."
        )

        # Best-effort form filling — selectors are approximate.
        for field_name, value in content.fields.items():
            try:
                selector = f'input[name*="{field_name.lower().replace(" ", "")}"], textarea[name*="{field_name.lower().replace(" ", "")}"]'
                await browser.fill_field(selector, value)
            except Exception:
                pass  # Field may not match; operator fills manually.

        await browser.wait_for_human(
            "Review the form, complete any missing fields or CAPTCHA, then press Enter to submit."
        )
        await browser.screenshot(
            f"business/funding_strategy/output/{self.name}/submission_screenshot.png"
        )
        return True


class CDL(BaseApplication):
    """Creative Destruction Lab — free, no-equity deep-tech programme."""

    name = "cdl"
    display_name = "Creative Destruction Lab (CDL)"
    tier = 2
    phase = 1
    submission_method = SubmissionMethod.BROWSER
    url = "https://www.creativedestructionlab.com/"
    pitch_angle = (
        "A new computing primitive — delta-state algebra in silicon. "
        "Mathematically novel with 92 formal proofs."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.founder.bio:
            missing.append("founder bio")
        if not config.founder.name:
            missing.append("founder name")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Company Name": config.company.name,
                "Technical Description": engine.get_technical_summary(2000),
                "Founder Background": engine.get_team_description(),
                "Problem": (
                    "Modern computing wastes 60-90% of energy on data movement. "
                    "Every state update requires a full read-modify-write cycle."
                ),
                "Solution": engine.get_company_description(1000),
                "Target Stream": "New Compute",
                "Traction": engine.get_traction(),
            },
            notes=(
                "CDL application: target the 'New Compute' stream at the Berlin "
                "or Toronto site. Applications typically open summer 2026."
            ),
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        if browser is None:
            print("[cdl] No browser driver — cannot submit.")
            return False

        await browser.goto(self.url)
        await browser.wait_for_human(
            "Navigate to the CDL application form (Apply -> Ventures), then press Enter."
        )

        for field_name, value in content.fields.items():
            try:
                selector = f'textarea[name*="{field_name.lower().replace(" ", "_")}"], input[name*="{field_name.lower().replace(" ", "_")}"]'
                await browser.fill_field(selector, value)
            except Exception:
                pass

        await browser.wait_for_human(
            "Review the form and complete any remaining fields, then press Enter."
        )
        await browser.screenshot(
            f"business/funding_strategy/output/{self.name}/submission_screenshot.png"
        )
        return True


class ICorp(BaseApplication):
    """NSF I-Corps — $50K for customer discovery."""

    name = "icorps"
    display_name = "NSF I-Corps"
    tier = 2
    phase = 1
    submission_method = SubmissionMethod.DOCUMENT
    url = "https://new.nsf.gov/funding/initiatives/i-corps"
    pitch_angle = (
        "Validate which vertical has the highest willingness-to-pay "
        "for delta-state hardware IP."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.founder.name:
            missing.append("founder name")
        # I-Corps requires a 3-person team.
        missing.append("3-person team (mentor + entrepreneurial lead)")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Project Description": engine.get_technical_summary(2000),
                "Customer Discovery Plan": (
                    "100+ customer discovery interviews across four verticals: "
                    "high-frequency trading, IoT sensor fusion, video processing, "
                    "and database replication. Goal: identify which segment has "
                    "the highest willingness-to-pay for delta-state hardware IP."
                ),
                "Team - Technical Lead": engine.get_team_description(),
                "Team - Mentor": "TBD — seek through NSF I-Corps Hub network",
                "Team - Entrepreneurial Lead": "TBD — recruit from local university",
            },
            notes=(
                "I-Corps requires a 3-person team. Generate document package "
                "and guide through manual upload at the I-Corps Hub portal."
            ),
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        if browser:
            await browser.goto(self.url)
        print(
            f"\n[icorps] Document package generated. Open {self.url} to apply.\n"
            "I-Corps requires a 3-person team and connection to an I-Corps Hub.\n"
            "Upload the generated materials manually."
        )
        if browser:
            await browser.wait_for_human("Press Enter when done.")
        return True


class IntelPartnerAlliance(BaseApplication):
    """Intel Partner Alliance — free partner programme for FPGA IP providers."""

    name = "intel_partner_alliance"
    display_name = "Intel Partner Alliance"
    tier = 2
    phase = 1
    submission_method = SubmissionMethod.BROWSER
    url = "https://www.intel.com/content/www/us/en/partner-alliance/overview.html"
    pitch_angle = (
        "Delta-state computing IP for Intel Agilex FPGAs. "
        "Projected 9.6 Gops/s on Agilex with 16 parallel banks — "
        "validated architecture on Gowin, ready to port to Intel."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.company.website:
            missing.append("website")
        if not config.founder.email:
            missing.append("founder email")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Company Name": config.company.name,
                "Company Description": engine.get_company_description(500),
                "Industry": "Semiconductor IP / FPGA Design",
                "Team Size": str(config.company.employee_count),
                "Website": config.company.website,
                "Contact Email": config.founder.email,
                "Partner Type": "IP Provider / Design Ecosystem",
                "Intel Products Used": (
                    "Targeting Intel Agilex FPGAs for delta-state "
                    "computing IP cores. Architecture validated on Gowin "
                    "GW1NR-9 at 1 Gops/s, projected ~9.6 Gops/s on Agilex."
                ),
                "Value Proposition": engine.get_pitch_for_program(self.name),
            },
            notes=(
                "Intel Partner Alliance: navigate to the 'Join Now' portal, "
                "create account with company email, complete enrollment form. "
                "Applications reviewed within 3 business days. "
                "Request access to Quartus Prime tools and Agilex evaluation boards."
            ),
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        if browser is None:
            print("[intel_partner_alliance] No browser driver — cannot submit.")
            return False

        await browser.goto(self.url)
        await browser.wait_for_human(
            "Navigate to the Intel Partner Alliance 'Join Now' page, then press Enter."
        )

        for field_name, value in content.fields.items():
            try:
                selector = f'input[name*="{field_name.lower().replace(" ", "")}"], textarea[name*="{field_name.lower().replace(" ", "")}"]'
                await browser.fill_field(selector, value)
            except Exception:
                pass

        await browser.wait_for_human(
            "Review the form, complete any remaining fields, then press Enter to submit."
        )
        await browser.screenshot(
            f"business/funding_strategy/output/{self.name}/submission_screenshot.png"
        )
        return True
