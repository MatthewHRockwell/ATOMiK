"""Email template engine — generates personalised outreach emails."""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING

from jinja2 import Environment, FileSystemLoader

if TYPE_CHECKING:
    from ..config import FundingConfig
    from .content_engine import ContentEngine

_TEMPLATE_DIR = Path(__file__).resolve().parent.parent / "templates"


class EmailDrafter:
    """Renders Jinja2 email templates into personalised outreach messages."""

    def __init__(self, config: FundingConfig, engine: ContentEngine) -> None:
        self.config = config
        self.engine = engine
        self._env = Environment(
            loader=FileSystemLoader(str(_TEMPLATE_DIR)),
            keep_trailing_newline=True,
        )

    def _base_context(self) -> dict:
        metrics = self.engine.get_key_metrics()
        metrics_block = "\n".join(
            f"- {k}: {v}" for k, v in list(metrics.items())[:6]
        )
        return {
            "company": {
                "name": self.config.company.name,
                "website": self.config.company.website,
            },
            "founder": {
                "name": self.config.founder.name or "Founder",
                "title": self.config.founder.title,
                "email": self.config.founder.email,
                "phone": self.config.founder.phone,
            },
            "one_liner": (
                "a formally verified hardware architecture for delta-state "
                "computing, achieving 1 billion operations per second on a "
                "$10 FPGA"
            ),
            "metrics_block": metrics_block,
            "traction": self.engine.get_traction(),
        }

    def draft_vc_email(
        self,
        firm: str,
        contact_name: str = "",
        thesis_match: str = "",
    ) -> tuple[str, str]:
        """Render a VC cold email.  Returns ``(subject, body)``."""
        ctx = self._base_context()
        ctx.update(
            {
                "contact_name": contact_name or "team",
                "firm": firm,
                "pitch_hook": "Formally verified silicon IP — 1 Gops/s on a $10 chip",
                "opening_line": (
                    f"I'm reaching out because {firm}'s investment thesis "
                    "in deep-tech hardware aligns with what we're building."
                    if not thesis_match
                    else thesis_match
                ),
                "ask": (
                    "Would you be open to a 20-minute call to discuss how "
                    "ATOMiK's verified hardware IP fits your portfolio thesis?"
                ),
                "closing": "Best regards,",
            }
        )
        tpl = self._env.get_template("vc_cold_email.md")
        rendered = tpl.render(**ctx)
        # First line is the subject line (after "Subject: ").
        lines = rendered.strip().splitlines()
        subject = lines[0].replace("Subject: ", "").strip() if lines else ""
        body = "\n".join(lines[1:]).strip()
        return subject, body

    def draft_defense_email(
        self,
        org: str,
        contact_name: str = "",
    ) -> tuple[str, str]:
        """Render a defense outreach email.  Returns ``(subject, body)``."""
        ctx = self._base_context()
        ctx.update(
            {
                "contact_name": contact_name or "team",
                "firm": org,
                "pitch_hook": "Formally verified hardware for assured edge computing",
                "opening_line": (
                    f"I'm reaching out because {org}'s focus on defense and "
                    "national-security technology aligns closely with ATOMiK's "
                    "formally verified hardware for assured state management."
                ),
                "ask": (
                    "Would your team be open to a brief technical briefing on "
                    "ATOMiK's formally verified hardware architecture?"
                ),
                "closing": "Respectfully,",
            }
        )
        tpl = self._env.get_template("vc_cold_email.md")
        rendered = tpl.render(**ctx)
        lines = rendered.strip().splitlines()
        subject = lines[0].replace("Subject: ", "").strip() if lines else ""
        body = "\n".join(lines[1:]).strip()
        return subject, body
