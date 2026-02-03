"""Abstract base class for all program application agents."""

from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import Enum
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from ..config import FundingConfig
    from ..drivers.browser import BrowserDriver
    from ..drivers.email_sender import EmailSender
    from ..materials.content_engine import ContentEngine
    from ..status import ApplicationStatus, StatusTracker


class SubmissionMethod(Enum):
    BROWSER = "browser"
    EMAIL = "email"
    DOCUMENT = "document"
    OUTREACH = "outreach"


@dataclass
class ApplicationContent:
    """Content generated for a specific program."""

    program_id: str
    program_name: str
    fields: dict[str, str]
    attachments: list[str] = field(default_factory=list)
    email_subject: str = ""
    email_body: str = ""
    documents: list[str] = field(default_factory=list)
    notes: str = ""


class BaseApplication(ABC):
    """Abstract base for all program application agents."""

    name: str
    display_name: str
    tier: int
    phase: int
    submission_method: SubmissionMethod
    url: str
    pitch_angle: str

    @abstractmethod
    def check_prerequisites(
        self, config: FundingConfig
    ) -> tuple[bool, list[str]]:
        """Return (ready, [missing_items])."""

    @abstractmethod
    def generate_content(
        self, config: FundingConfig, engine: ContentEngine
    ) -> ApplicationContent:
        """Generate all application content from existing materials."""

    @abstractmethod
    async def submit(
        self,
        content: ApplicationContent,
        config: FundingConfig,
        browser: BrowserDriver | None,
        emailer: EmailSender | None,
    ) -> bool:
        """Submit the application. Return True on success."""

    def get_status(self, tracker: StatusTracker) -> ApplicationStatus:
        return tracker.get(self.name)
