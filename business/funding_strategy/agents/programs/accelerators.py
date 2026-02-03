"""Accelerator program agents — Silicon Catalyst, Alchemist, HAX, YC, Techstars."""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING

import yaml

from .base import ApplicationContent, BaseApplication, SubmissionMethod

if TYPE_CHECKING:
    from ..config import FundingConfig
    from ..drivers.browser import BrowserDriver
    from ..drivers.email_sender import EmailSender
    from ..materials.content_engine import ContentEngine

_QA_PATH = Path(__file__).resolve().parent.parent / "templates" / "common_qa.yaml"


def _load_qa() -> dict:
    if _QA_PATH.exists():
        return yaml.safe_load(_QA_PATH.read_text(encoding="utf-8")) or {}
    return {}


class SiliconCatalyst(BaseApplication):
    """Silicon Catalyst — semiconductor-specific accelerator."""

    name = "silicon_catalyst"
    display_name = "Silicon Catalyst"
    tier = 3
    phase = 3
    submission_method = SubmissionMethod.BROWSER
    url = "https://siliconcatalyst.com/application"
    pitch_angle = (
        "Formally verified XOR accumulator IP targeting ASIC integration. "
        "92 Lean4 proofs, working FPGA prototype, 7% LUT utilization."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        if not config.founder.name:
            missing.append("founder name")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        metrics = engine.get_key_metrics()
        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Company Name": config.company.name,
                "IC Design Details": (
                    "XOR accumulator array — N parallel banks with binary merge tree. "
                    "Single-cycle delta accumulation via pure LUT-based XOR computation. "
                    "Currently on Tang Nano 9K (GW1NR-9) FPGA. Target: ASIC integration "
                    "as a co-processor IP block."
                ),
                "Technology Node": (
                    "FPGA prototype on Gowin GW1NR-9 (Tang Nano 9K). "
                    "ASIC roadmap: 28nm or 65nm initial shuttle."
                ),
                "LUT Utilization": metrics.get("LUT utilization", "7% (single bank)"),
                "Throughput": metrics.get("Throughput", "1,056 Mops/s"),
                "TAM/SAM/SOM": (
                    "TAM: ~$85B (FPGA + edge + AI accelerators). "
                    "SAM: ~$8B. SOM (Year 5): ~$80M."
                ),
                "Team": engine.get_team_description(),
                "Formal Verification": (
                    "92 Lean4 proofs — closure, commutativity, associativity, "
                    "identity, self-inverse. Zero sorry statements."
                ),
                "What do you need from Silicon Catalyst?": (
                    "EDA tool access (Synopsys, Cadence), fab partner introductions "
                    "(TSMC, GlobalFoundries), and mentorship on ASIC tape-out process."
                ),
            },
            notes="Silicon Catalyst is the highest-value semiconductor accelerator.",
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        if browser is None:
            print("[silicon_catalyst] No browser driver.")
            return False
        await browser.goto(self.url)
        await browser.wait_for_human(
            "Fill the Silicon Catalyst application form, then press Enter."
        )
        await browser.screenshot(
            f"business/funding_strategy/output/{self.name}/submission_screenshot.png"
        )
        return True


class AlchemistAccelerator(BaseApplication):
    """Alchemist Accelerator — B2B/enterprise deep-tech programme."""

    name = "alchemist"
    display_name = "Alchemist Accelerator"
    tier = 3
    phase = 3
    submission_method = SubmissionMethod.BROWSER
    url = "https://www.alchemistaccelerator.com/"
    pitch_angle = (
        "B2B IP licensing play — delta-state hardware blocks for chip "
        "designers and system integrators."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        qa = _load_qa()
        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Company Name": config.company.name,
                "What does your company do?": qa.get("what_does_your_company_do", {}).get("long", engine.get_company_description(500)),
                "How far along are you?": qa.get("how_far_along", {}).get("answer", engine.get_traction()),
                "Business Model": qa.get("what_is_the_business_model", {}).get("answer", ""),
                "Target Customer": "Chip designers, FPGA integrators, system-on-chip teams",
                "Team": engine.get_team_description(),
                "Why Alchemist?": (
                    "B2B enterprise focus matches our IP licensing model. "
                    "Deep tech track aligns with foundational hardware technology."
                ),
            },
            notes="Alchemist Deep Tech track (Chicago/UChicago) is best fit.",
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        if browser is None:
            print("[alchemist] No browser driver.")
            return False
        await browser.goto(self.url)
        await browser.wait_for_human(
            "Navigate to the Alchemist application, fill in the form, then press Enter."
        )
        await browser.screenshot(
            f"business/funding_strategy/output/{self.name}/submission_screenshot.png"
        )
        return True


class HAXAccelerator(BaseApplication):
    """HAX (SOSV) — premier hardware accelerator."""

    name = "hax"
    display_name = "HAX (SOSV)"
    tier = 3
    phase = 3
    submission_method = SubmissionMethod.BROWSER
    url = "https://hax.co/"
    pitch_angle = (
        "Working hardware on $10 FPGA — 1 Gops/s, 92 formal proofs, "
        "$225 total cost."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        qa = _load_qa()
        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Company Name": config.company.name,
                "Product Description": qa.get("what_does_your_company_do", {}).get("long", ""),
                "Hardware Details": (
                    "Tang Nano 9K FPGA ($10). XOR accumulator array with binary "
                    "merge tree. 7% LUT utilization per bank, 20% at 16 banks. "
                    "1,056 Mops/s throughput. 80/80 hardware tests passing."
                ),
                "Traction": qa.get("how_far_along", {}).get("answer", engine.get_traction()),
                "Market": qa.get("what_is_the_market", {}).get("answer", ""),
                "Team": engine.get_team_description(),
                "Why HAX?": (
                    "HAX's hardware-to-market pipeline is exactly what ATOMiK needs "
                    "to move from FPGA prototype to production-ready IP. "
                    "Prototyping resources and supply chain support."
                ),
            },
            attachments=[
                config.materials.get("one_pager", ""),
                config.materials.get("pitch_deck_pptx", ""),
            ],
            notes="HAX has rolling applications. Upload demo video when available.",
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        if browser is None:
            print("[hax] No browser driver.")
            return False
        await browser.goto(self.url)
        await browser.wait_for_human(
            "Navigate to the HAX application form, fill it in, then press Enter."
        )
        await browser.screenshot(
            f"business/funding_strategy/output/{self.name}/submission_screenshot.png"
        )
        return True


class YCombinator(BaseApplication):
    """Y Combinator — $500K SAFE, 3-month batch."""

    name = "yc"
    display_name = "Y Combinator"
    tier = 3
    phase = 3
    submission_method = SubmissionMethod.BROWSER
    url = "https://www.ycombinator.com/apply/"
    pitch_angle = (
        "Hardware that works, math that's proven, software that ships. "
        "92 proofs, 1 Gops/s, $225 total spend."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        if not config.founder.name:
            missing.append("founder name")
        if not config.founder.email:
            missing.append("founder email")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        qa = _load_qa()
        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Company name": config.company.name,
                "Company url": config.company.website,
                "Describe what your company does in 50 characters or less": (
                    "Delta-state computing in silicon"
                ),
                "What is your company going to make?": qa.get("what_does_your_company_do", {}).get("long", ""),
                "Why did you pick this idea to work on?": qa.get("why_now", {}).get("answer", ""),
                "What is your progress?": qa.get("how_far_along", {}).get("answer", engine.get_traction()),
                "How will you make money?": qa.get("what_is_the_business_model", {}).get("answer", ""),
                "How is this different from existing solutions?": qa.get("how_is_this_different", {}).get("answer", ""),
                "Who are your competitors?": (
                    "Event sourcing systems, CRDTs, GPU accelerators, "
                    "near-memory computing. None combine formal verification "
                    "with hardware acceleration of algebraic state management."
                ),
                "How long have the founders known each other?": "Solo founder",
                "Please tell us about the time you most successfully hacked some system": (
                    "Built a complete formally verified hardware architecture — "
                    "92 Lean4 proofs, FPGA prototype, and 5-language SDK — for "
                    "$225 total development cost. That's a 4-million-to-one ratio "
                    "of performance ($1B ops/s) to cost."
                ),
            },
            notes=(
                "YC application requires a 1-minute video. Upload separately. "
                "Apply for the Summer batch (deadline ~March)."
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
            print("[yc] No browser driver.")
            return False
        await browser.goto(self.url)
        await browser.wait_for_human(
            "Log in to YC and navigate to the application form, then press Enter."
        )

        # YC uses a structured multi-page form. Best-effort auto-fill.
        for field_name, value in content.fields.items():
            try:
                # YC form fields are typically textareas with label text
                selector = f'textarea, input[type="text"]'
                await browser.fill_field(selector, value)
            except Exception:
                pass

        await browser.wait_for_human(
            "Review all YC application fields, upload your video, then press Enter to submit."
        )
        await browser.screenshot(
            f"business/funding_strategy/output/{self.name}/submission_screenshot.png"
        )
        return True


class Techstars(BaseApplication):
    """Techstars — $120K for 6% equity."""

    name = "techstars"
    display_name = "Techstars"
    tier = 3
    phase = 3
    submission_method = SubmissionMethod.BROWSER
    url = "https://www.techstars.com/"
    pitch_angle = (
        "Novel computing IP with working hardware and multi-language SDK."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        qa = _load_qa()
        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Company Name": config.company.name,
                "What does your company do?": qa.get("what_does_your_company_do", {}).get("long", ""),
                "What stage are you at?": "Pre-seed — working prototype, seeking first customers",
                "Traction": qa.get("how_far_along", {}).get("answer", engine.get_traction()),
                "Business Model": qa.get("what_is_the_business_model", {}).get("answer", ""),
                "Team": engine.get_team_description(),
                "Why Techstars?": (
                    "Seeking commercial traction support — specifically pilot "
                    "customers in HFT, IoT, or database replication verticals."
                ),
            },
            notes="Apply for a Spring 2026 Techstars programme.",
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        if browser is None:
            print("[techstars] No browser driver.")
            return False
        await browser.goto(self.url)
        await browser.wait_for_human(
            "Navigate to the Techstars application, fill it in, then press Enter."
        )
        await browser.screenshot(
            f"business/funding_strategy/output/{self.name}/submission_screenshot.png"
        )
        return True
