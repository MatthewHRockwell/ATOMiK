"""Grant program agents — NSF/DoD/DOE/NASA SBIR + CHIPS Act."""

from __future__ import annotations

from typing import TYPE_CHECKING

from .base import ApplicationContent, BaseApplication, SubmissionMethod

if TYPE_CHECKING:
    from ..config import FundingConfig
    from ..drivers.browser import BrowserDriver
    from ..drivers.email_sender import EmailSender
    from ..materials.content_engine import ContentEngine


class _SbirBase(BaseApplication):
    """Shared logic for SBIR/STTR programmes."""

    submission_method = SubmissionMethod.DOCUMENT

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation (Delaware C-Corp)")
        if not config.sam_gov_registered:
            missing.append("SAM.gov registration / UEI")
        if not config.company.uei:
            missing.append("UEI number")
        if not config.founder.name:
            missing.append("founder name (PI)")
        return (len(missing) == 0, missing)

    def _generate_sbir_content(
        self,
        config: FundingConfig,
        engine: ContentEngine,
        agency_notes: str,
    ) -> ApplicationContent:
        from ..materials.proposal_writer import ProposalWriter

        writer = ProposalWriter(config, engine)
        output_dir = f"business/funding_strategy/output/{self.name}"
        proposal_path = writer.write_sbir_proposal(self.name, output_dir)

        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Agency": self.display_name,
                "Pitch Angle": engine.get_pitch_for_program(self.name),
                "Topic Area": agency_notes,
            },
            documents=[proposal_path],
            notes=(
                f"15-page proposal draft generated at {proposal_path}. "
                "Review and refine before submission to the agency portal. "
                "SBIR reauthorisation status must be confirmed before submitting."
            ),
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        # SBIR portals require MFA — guide user through manual upload.
        print(f"\n[{self.name}] SBIR proposal generated:")
        for doc in content.documents:
            print(f"  {doc}")
        print(f"\nSubmit at: {self.url}")
        print("Government portals require MFA. Upload documents manually.")
        if browser:
            await browser.goto(self.url)
            await browser.wait_for_human("Press Enter when upload is complete.")
        return True


class NsfSbir(_SbirBase):
    """NSF SBIR Phase I — $275K."""

    name = "nsf_sbir"
    display_name = "NSF SBIR Phase I"
    tier = 1
    phase = 2
    url = "https://seedfund.nsf.gov/"
    pitch_angle = (
        "Novel semiconductor architecture with 92 formal proofs — "
        "advancing the state of the art in verified hardware design."
    )

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        return self._generate_sbir_content(
            config,
            engine,
            "Semiconductors and Photonics / Novel AI Hardware",
        )


class DodSbir(_SbirBase):
    """DoD SBIR/STTR — $50K-$250K Phase I."""

    name = "dod_sbir"
    display_name = "DoD SBIR/STTR"
    tier = 1
    phase = 2
    url = "https://www.dodsbirsttr.mil/"
    pitch_angle = (
        "Formally verified hardware for assured state management at the "
        "tactical edge."
    )

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        return self._generate_sbir_content(
            config,
            engine,
            "Army: Open Source High Assurance Hardware / DARPA PROVERS",
        )


class DoeSbir(_SbirBase):
    """DOE SBIR Phase I — $200K-$250K."""

    name = "doe_sbir"
    display_name = "DOE SBIR Phase I"
    tier = 1
    phase = 2
    url = "https://science.osti.gov/sbir"
    pitch_angle = (
        "95-100% memory traffic reduction at ~20 mW — "
        "orders-of-magnitude energy reduction."
    )

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        return self._generate_sbir_content(
            config,
            engine,
            "Energy-efficient computing / Advanced microelectronics",
        )


class NasaSbir(_SbirBase):
    """NASA SBIR — $150K Phase I."""

    name = "nasa_sbir"
    display_name = "NASA SBIR"
    tier = 1
    phase = 2
    url = "https://sbir.nasa.gov/"
    pitch_angle = (
        "Radiation-tolerant state management with flight-system-grade "
        "formal verification."
    )

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        return self._generate_sbir_content(
            config,
            engine,
            "Radiation-tolerant computing / Formal verification for flight systems",
        )


class ChipsAct(BaseApplication):
    """CHIPS Act R&D BAA — concept plan submission."""

    name = "chips_act"
    display_name = "CHIPS Act R&D BAA"
    tier = 1
    phase = 2
    submission_method = SubmissionMethod.EMAIL
    url = "mailto:apply@chips.gov"
    pitch_angle = (
        "Novel XOR-based computing primitive — extreme area efficiency "
        "for a fully formally verified hardware block."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        if not config.email.smtp_host:
            missing.append("SMTP email configuration")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        from ..materials.proposal_writer import ProposalWriter

        writer = ProposalWriter(config, engine)
        output_dir = f"business/funding_strategy/output/{self.name}"
        concept_path = writer.write_chips_concept(output_dir)

        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Submission Target": "apply@chips.gov",
                "Pitch Angle": engine.get_pitch_for_program(self.name),
            },
            email_subject=(
                f"{config.company.name} — CHIPS Act R&D BAA Concept Plan: "
                "Formally Verified Delta-State Computing IP"
            ),
            email_body=(
                f"Dear CHIPS Program Office,\n\n"
                f"Please find attached a concept plan from {config.company.name} "
                f"for consideration under the CHIPS Act R&D BAA.\n\n"
                f"{config.company.name} has developed a formally verified hardware "
                f"architecture for delta-state computing that achieves 1 billion "
                f"operations per second on a $10 FPGA at 7% LUT utilization. "
                f"The architecture is backed by 92 Lean4 formal proofs and a "
                f"5-language SDK.\n\n"
                f"We believe this technology is directly relevant to CHIPS Act "
                f"objectives for advancing domestic semiconductor IP.\n\n"
                f"Best regards,\n"
                f"{config.founder.name or 'Founder'}\n"
                f"{config.founder.title}\n"
                f"{config.company.name}\n"
                f"{config.company.website}"
            ),
            documents=[concept_path],
            attachments=[
                concept_path,
                config.materials.get("one_pager", ""),
            ],
            notes="Concept plans accepted through Nov 2026 at apply@chips.gov.",
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        if emailer is None:
            print("[chips_act] No email sender configured.")
            return False
        ok = emailer.send(
            to="apply@chips.gov",
            subject=content.email_subject,
            body=content.email_body,
            attachments=[a for a in content.attachments if a],
        )
        return ok
