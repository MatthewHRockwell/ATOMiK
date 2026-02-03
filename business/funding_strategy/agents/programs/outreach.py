"""Outreach program agents — VC email and defense outreach.

Firms with email addresses receive cold emails via SMTP.
Firms without email (web-form only) are opened in the browser
with pitch content displayed in a Rich panel for manual copy-paste.
"""

from __future__ import annotations

import webbrowser
from pathlib import Path
from typing import TYPE_CHECKING

from rich.console import Console
from rich.panel import Panel

from .base import ApplicationContent, BaseApplication, SubmissionMethod

if TYPE_CHECKING:
    from ..config import FundingConfig
    from ..drivers.browser import BrowserDriver
    from ..drivers.email_sender import EmailSender
    from ..materials.content_engine import ContentEngine

# VC firms from the funding playbook (Tier 5)
# Contact info sourced from publicly listed channels only.
# Firms with web-form-only submission have url set; email left blank.
VC_FIRMS = [
    {"firm": "Eclipse Ventures", "email": "", "contact_name": "", "thesis": "Semiconductor, manufacturing, deep tech", "url": "https://eclipse.vc"},
    {"firm": "Lux Capital", "email": "info@luxcapital.com", "contact_name": "", "thesis": "Deep tech — hardware, materials, compute", "url": "https://www.luxcapital.com/contact"},
    {"firm": "DCVC", "email": "", "contact_name": "", "thesis": "Computational approaches, deep tech, AI infrastructure", "url": "https://www.dcvc.com/"},
    {"firm": "Playground Global", "email": "info@pjc.vc", "contact_name": "", "thesis": "Hardware, systems, computing architecture", "url": "https://www.playground.vc/"},
    {"firm": "Intel Capital", "email": "", "contact_name": "", "thesis": "Strategic semiconductor ecosystem investments", "url": "https://www.intelcapital.com/contact/"},
    {"firm": "Qualcomm Ventures", "email": "5GFund@qti.qualcomm.com", "contact_name": "", "thesis": "Edge computing, low-power hardware", "url": "https://www.qualcommventures.com/"},
    {"firm": "Samsung NEXT", "email": "catalyst@samsung.com", "contact_name": "", "thesis": "Hardware innovation, semiconductor ecosystem", "url": "https://www.samsungnext.com"},
    {"firm": "Cantos Ventures", "email": "grant@cantos.vc", "contact_name": "Grant", "thesis": "Deep tech, hardware, small checks", "url": "https://cantos.vc/"},
    {"firm": "Khosla Ventures", "email": "info@khoslaventures.com", "contact_name": "", "thesis": "Breakthrough tech thesis, AI infrastructure", "url": "https://www.khoslaventures.com/entrepreneurs"},
    {"firm": "a16z Speedrun", "email": "", "contact_name": "", "thesis": "Technical founders building foundational tech", "url": "https://speedrun.a16z.com/apply"},
]

DEFENSE_ORGS = [
    {"firm": "In-Q-Tel (IQT)", "email": "", "contact_name": "", "role": "CIA/IC strategic VC", "url": "https://www.iqt.org/submit-a-business-plan"},
    {"firm": "Shield Capital", "email": "media@shieldcap.com", "contact_name": "", "role": "Defense-focused VC", "url": "https://shieldcap.com"},
]


class VCOutreach(BaseApplication):
    """VC email outreach — 10 semiconductor/hardware-focused firms."""

    name = "vc_outreach"
    display_name = "VC Outreach (Tier 5)"
    tier = 5
    phase = 4
    submission_method = SubmissionMethod.OUTREACH
    url = ""
    pitch_angle = (
        "IP licensing in $600B+ semiconductor market — irreproducible "
        "formal verification moat, working silicon, patent pending."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        if not config.email.smtp_host:
            missing.append("SMTP email configuration")
        if not config.founder.email:
            missing.append("founder email")
        # Check that at least some firms have email addresses populated
        configured = sum(1 for f in VC_FIRMS if f.get("email"))
        if configured == 0:
            missing.append("VC contact emails (populate VC_FIRMS in outreach.py)")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        from ..materials.email_drafter import EmailDrafter

        drafter = EmailDrafter(config, engine)

        # Generate a sample email for review (first firm with an email)
        target = next((f for f in VC_FIRMS if f.get("email")), VC_FIRMS[0])
        subject, body = drafter.draft_vc_email(
            firm=target["firm"],
            contact_name=target.get("contact_name", ""),
            thesis_match=target.get("thesis", ""),
        )

        # Save all drafts to output
        output_dir = Path(f"business/funding_strategy/output/{self.name}")
        output_dir.mkdir(parents=True, exist_ok=True)

        firms_with_email = [f for f in VC_FIRMS if f.get("email")]
        draft_paths: list[str] = []
        for firm_info in firms_with_email:
            s, b = drafter.draft_vc_email(
                firm=firm_info["firm"],
                contact_name=firm_info.get("contact_name", ""),
                thesis_match=firm_info.get("thesis", ""),
            )
            draft_path = output_dir / f"{firm_info['firm'].lower().replace(' ', '_')}_email.txt"
            draft_path.write_text(f"Subject: {s}\n\n{b}", encoding="utf-8")
            draft_paths.append(str(draft_path))

        one_pager = config.materials.get("one_pager", "")

        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Target Firms": ", ".join(f["firm"] for f in VC_FIRMS),
                "Firms with Email": str(len(firms_with_email)),
                "Email Drafts": str(len(draft_paths)),
            },
            email_subject=subject,
            email_body=body,
            attachments=[one_pager] if one_pager else [],
            documents=draft_paths,
            notes=(
                f"Batch send to {len(firms_with_email)} VC firms with 30s delay. "
                "One-pager attached to each email. "
                "Populate email addresses in outreach.py before sending."
            ),
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        from ..materials.email_drafter import EmailDrafter
        from ..materials.content_engine import ContentEngine

        engine = ContentEngine(config)
        drafter = EmailDrafter(config, engine)
        console = Console()

        one_pager = config.materials.get("one_pager", "")
        attachments = [one_pager] if one_pager and Path(one_pager).exists() else []
        results: list[tuple[str, bool]] = []

        # --- Email-based firms ---
        firms_with_email = [f for f in VC_FIRMS if f.get("email")]
        if firms_with_email and emailer:
            for firm_info in firms_with_email:
                subject, body = drafter.draft_vc_email(
                    firm=firm_info["firm"],
                    contact_name=firm_info.get("contact_name", ""),
                    thesis_match=firm_info.get("thesis", ""),
                )
                ok = emailer.send(
                    to=firm_info["email"],
                    subject=subject,
                    body=body,
                    attachments=attachments,
                )
                results.append((firm_info["firm"], ok))
                import time
                time.sleep(30)

        # --- Web-form fallback for firms without email ---
        firms_web_only = [f for f in VC_FIRMS if not f.get("email") and f.get("url")]
        for firm_info in firms_web_only:
            pitch = (
                f"Firm: {firm_info['firm']}\n"
                f"Thesis match: {firm_info.get('thesis', '')}\n\n"
                f"{self.pitch_angle}\n\n"
                f"ATOMiK: 1 Gops/s on $10 FPGA, 92 formal proofs, "
                f"patent pending, $225 total dev cost.\n\n"
                f"Contact: {config.founder.name} ({config.founder.email})"
            )
            console.print(Panel(
                pitch,
                title=f"[bold]{firm_info['firm']}[/bold] — Copy to web form",
                border_style="yellow",
            ))
            webbrowser.open(firm_info["url"])
            results.append((firm_info["firm"], True))

        sent = sum(1 for _, ok in results if ok)
        console.print(f"[vc_outreach] Processed {sent}/{len(results)} firms.")
        return sent > 0


class DefenseOutreach(BaseApplication):
    """Defense/strategic outreach — In-Q-Tel + Shield Capital."""

    name = "defense_outreach"
    display_name = "Defense Outreach (Tier 4)"
    tier = 4
    phase = 4
    submission_method = SubmissionMethod.OUTREACH
    url = ""
    pitch_angle = (
        "Formally verified hardware for assured edge computing. "
        "Self-inverse + commutative = instant rollback + lock-free merge."
    )

    def check_prerequisites(self, config: FundingConfig) -> tuple[bool, list[str]]:
        missing: list[str] = []
        if not config.incorporation_completed:
            missing.append("incorporation")
        if not config.email.smtp_host:
            missing.append("SMTP email configuration")
        configured = sum(1 for o in DEFENSE_ORGS if o.get("email"))
        if configured == 0:
            missing.append("defense org contact emails (populate DEFENSE_ORGS in outreach.py)")
        return (len(missing) == 0, missing)

    def generate_content(self, config: FundingConfig, engine: ContentEngine) -> ApplicationContent:
        from ..materials.email_drafter import EmailDrafter

        drafter = EmailDrafter(config, engine)

        target = next((o for o in DEFENSE_ORGS if o.get("email")), DEFENSE_ORGS[0])
        subject, body = drafter.draft_defense_email(
            org=target["firm"],
            contact_name=target.get("contact_name", ""),
        )

        output_dir = Path(f"business/funding_strategy/output/{self.name}")
        output_dir.mkdir(parents=True, exist_ok=True)

        orgs_with_email = [o for o in DEFENSE_ORGS if o.get("email")]
        draft_paths: list[str] = []
        for org_info in orgs_with_email:
            s, b = drafter.draft_defense_email(
                org=org_info["firm"],
                contact_name=org_info.get("contact_name", ""),
            )
            draft_path = output_dir / f"{org_info['firm'].lower().replace(' ', '_').replace('(', '').replace(')', '')}_email.txt"
            draft_path.write_text(f"Subject: {s}\n\n{b}", encoding="utf-8")
            draft_paths.append(str(draft_path))

        one_pager = config.materials.get("one_pager", "")

        return ApplicationContent(
            program_id=self.name,
            program_name=self.display_name,
            fields={
                "Target Organisations": ", ".join(o["firm"] for o in DEFENSE_ORGS),
                "Orgs with Email": str(len(orgs_with_email)),
            },
            email_subject=subject,
            email_body=body,
            attachments=[one_pager] if one_pager else [],
            documents=draft_paths,
            notes=(
                "Defense outreach — formal, technical tone. "
                "IQT does not have a public application; cold outreach or "
                "introduction via accelerator network is the path."
            ),
        )

    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        from ..materials.email_drafter import EmailDrafter
        from ..materials.content_engine import ContentEngine

        engine = ContentEngine(config)
        drafter = EmailDrafter(config, engine)
        console = Console()

        one_pager = config.materials.get("one_pager", "")
        attachments = [one_pager] if one_pager and Path(one_pager).exists() else []
        results: list[tuple[str, bool]] = []

        # --- Email-based orgs ---
        orgs_with_email = [o for o in DEFENSE_ORGS if o.get("email")]
        if orgs_with_email and emailer:
            for org_info in orgs_with_email:
                subject, body = drafter.draft_defense_email(
                    org=org_info["firm"],
                    contact_name=org_info.get("contact_name", ""),
                )
                ok = emailer.send(
                    to=org_info["email"],
                    subject=subject,
                    body=body,
                    attachments=attachments,
                )
                results.append((org_info["firm"], ok))
                import time
                time.sleep(30)

        # --- Web-form fallback for orgs without email ---
        orgs_web_only = [o for o in DEFENSE_ORGS if not o.get("email") and o.get("url")]
        for org_info in orgs_web_only:
            pitch = (
                f"Organisation: {org_info['firm']}\n"
                f"Role: {org_info.get('role', '')}\n\n"
                f"{self.pitch_angle}\n\n"
                f"ATOMiK: formally verified delta-state computing, "
                f"80/80 FPGA tests, 1 Gops/s, patent pending.\n\n"
                f"Contact: {config.founder.name} ({config.founder.email})"
            )
            console.print(Panel(
                pitch,
                title=f"[bold]{org_info['firm']}[/bold] — Copy to web form",
                border_style="yellow",
            ))
            webbrowser.open(org_info["url"])
            results.append((org_info["firm"], True))

        sent = sum(1 for _, ok in results if ok)
        console.print(f"[defense_outreach] Processed {sent}/{len(results)} orgs.")
        return sent > 0
