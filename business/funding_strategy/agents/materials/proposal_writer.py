"""SBIR proposal generator — builds 15-page proposals from templates."""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING

from jinja2 import Environment, FileSystemLoader

if TYPE_CHECKING:
    from ..config import FundingConfig
    from .content_engine import ContentEngine

_TEMPLATE_DIR = Path(__file__).resolve().parent.parent / "templates"


class ProposalWriter:
    """Generates SBIR/grant proposal documents from Jinja2 templates."""

    def __init__(self, config: FundingConfig, engine: ContentEngine) -> None:
        self.config = config
        self.engine = engine
        self._env = Environment(
            loader=FileSystemLoader(str(_TEMPLATE_DIR)),
            keep_trailing_newline=True,
        )

    def _build_context(self, program_id: str) -> dict:
        """Assemble template variables from config + content engine."""
        metrics = self.engine.get_key_metrics()
        return {
            "company": {
                "name": self.config.company.name,
                "legal_name": self.config.company.legal_name
                or self.config.company.name,
                "ein": self.config.company.ein,
                "uei": self.config.company.uei,
                "address": self.config.company.address,
                "website": self.config.company.website,
                "employee_count": self.config.company.employee_count,
                "us_owned_pct": self.config.company.us_owned_pct,
            },
            "founder": {
                "name": self.config.founder.name,
                "title": self.config.founder.title,
                "email": self.config.founder.email,
                "phone": self.config.founder.phone,
                "bio": self.config.founder.bio,
            },
            "program_id": program_id,
            "pitch_angle": self.engine.get_pitch_for_program(program_id),
            "technical_summary": self.engine.get_technical_summary(),
            "company_description": self.engine.get_company_description(1500),
            "traction": self.engine.get_traction(),
            "market_applications": self.engine.get_market_applications(),
            "competitive_moat": self.engine.get_competitive_moat(),
            "team_description": self.engine.get_team_description(),
            "metrics": metrics,
        }

    def write_sbir_proposal(
        self, program_id: str, output_dir: str
    ) -> str:
        """Render the SBIR proposal template and write to *output_dir*.

        Returns the path to the generated Markdown file.
        """
        ctx = self._build_context(program_id)
        template = self._env.get_template("sbir_proposal.md")
        rendered = template.render(**ctx)

        out = Path(output_dir)
        out.mkdir(parents=True, exist_ok=True)
        dest = out / f"{program_id}_proposal.md"
        dest.write_text(rendered, encoding="utf-8")
        return str(dest)

    def write_chips_concept(self, output_dir: str) -> str:
        """Render the CHIPS Act concept plan."""
        ctx = self._build_context("chips_act")
        template = self._env.get_template("chips_concept.md")
        rendered = template.render(**ctx)

        out = Path(output_dir)
        out.mkdir(parents=True, exist_ok=True)
        dest = out / "chips_act_concept_plan.md"
        dest.write_text(rendered, encoding="utf-8")
        return str(dest)
