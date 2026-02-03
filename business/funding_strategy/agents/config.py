"""Funding configuration — dataclasses + YAML loader."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

import yaml


@dataclass
class CompanyInfo:
    name: str
    legal_name: str
    ein: str
    uei: str
    address: str
    website: str
    founding_date: str
    employee_count: int
    us_owned_pct: int


@dataclass
class FounderInfo:
    name: str
    email: str
    title: str
    bio: str
    phone: str


@dataclass
class EmailConfig:
    smtp_host: str
    smtp_port: int
    smtp_user: str
    smtp_password: str
    from_name: str
    from_email: str


@dataclass
class BrowserConfig:
    headless: bool = False
    slow_mo: int = 500


@dataclass
class FundingConfig:
    company: CompanyInfo
    founder: FounderInfo
    email: EmailConfig
    browser: BrowserConfig
    materials: dict[str, str]
    incorporation_completed: bool
    sam_gov_registered: bool

    @classmethod
    def from_yaml(cls, path: str) -> FundingConfig:
        """Load configuration from a YAML file."""
        raw = yaml.safe_load(Path(path).read_text(encoding="utf-8"))

        company = CompanyInfo(
            name=raw["company"]["name"],
            legal_name=raw["company"].get("legal_name", ""),
            ein=raw["company"].get("ein", ""),
            uei=raw["company"].get("uei", ""),
            address=raw["company"].get("address", ""),
            website=raw["company"].get("website", ""),
            founding_date=raw["company"].get("founding_date", ""),
            employee_count=raw["company"].get("employee_count", 1),
            us_owned_pct=raw["company"].get("us_owned_pct", 100),
        )

        founder = FounderInfo(
            name=raw["founder"].get("name", ""),
            email=raw["founder"].get("email", ""),
            title=raw["founder"].get("title", "Founder & CEO"),
            bio=raw["founder"].get("bio", ""),
            phone=raw["founder"].get("phone", ""),
        )

        email_raw = raw.get("email", {})
        email_cfg = EmailConfig(
            smtp_host=email_raw.get("smtp_host", ""),
            smtp_port=email_raw.get("smtp_port", 587),
            smtp_user=email_raw.get("smtp_user", ""),
            smtp_password=email_raw.get("smtp_password", ""),
            from_name=email_raw.get("from_name", ""),
            from_email=email_raw.get("from_email", ""),
        )

        browser_raw = raw.get("browser", {})
        browser_cfg = BrowserConfig(
            headless=browser_raw.get("headless", False),
            slow_mo=browser_raw.get("slow_mo", 500),
        )

        materials = raw.get("materials", {})

        incorporation = raw.get("incorporation", {})
        sam_gov = raw.get("sam_gov", {})

        return cls(
            company=company,
            founder=founder,
            email=email_cfg,
            browser=browser_cfg,
            materials=materials,
            incorporation_completed=incorporation.get("completed", False),
            sam_gov_registered=sam_gov.get("registered", False),
        )
