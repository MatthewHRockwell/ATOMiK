"""SMTP email sender for outreach and document-based submissions."""

from __future__ import annotations

import smtplib
import time
from email import encoders
from email.mime.base import MIMEBase
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from pathlib import Path

from ..config import EmailConfig


class EmailSender:
    """Sends emails via SMTP with optional file attachments."""

    def __init__(self, config: EmailConfig) -> None:
        self.config = config

    def _build_message(
        self,
        to: str,
        subject: str,
        body: str,
        attachments: list[str] | None = None,
    ) -> MIMEMultipart:
        msg = MIMEMultipart()
        msg["From"] = f"{self.config.from_name} <{self.config.from_email}>"
        msg["To"] = to
        msg["Subject"] = subject
        msg.attach(MIMEText(body, "plain", "utf-8"))

        for file_path in attachments or []:
            p = Path(file_path)
            if not p.exists():
                continue
            part = MIMEBase("application", "octet-stream")
            part.set_payload(p.read_bytes())
            encoders.encode_base64(part)
            part.add_header(
                "Content-Disposition",
                f'attachment; filename="{p.name}"',
            )
            msg.attach(part)

        return msg

    def send(
        self,
        to: str,
        subject: str,
        body: str,
        attachments: list[str] | None = None,
    ) -> bool:
        """Send a single email. Return True on success."""
        msg = self._build_message(to, subject, body, attachments)
        try:
            with smtplib.SMTP(
                self.config.smtp_host, self.config.smtp_port
            ) as srv:
                srv.ehlo()
                srv.starttls()
                srv.login(self.config.smtp_user, self.config.smtp_password)
                srv.send_message(msg)
            return True
        except Exception as exc:
            print(f"[email] Failed to send to {to}: {exc}")
            return False

    def send_batch(
        self,
        recipients: list[dict],
        subject_template: str,
        body_template: str,
        attachments: list[str] | None = None,
        delay_seconds: int = 30,
    ) -> list[tuple[str, bool]]:
        """Send personalised emails to a list of recipients.

        Each entry in *recipients* is a dict with at least ``email`` and
        optionally ``name``, ``firm``, and ``contact_name`` keys that are
        used for simple ``str.format_map`` substitution in the templates.

        Returns a list of ``(email, success)`` tuples.
        """
        results: list[tuple[str, bool]] = []
        for i, recip in enumerate(recipients):
            addr = recip["email"]
            subj = subject_template.format_map(recip)
            body = body_template.format_map(recip)
            ok = self.send(addr, subj, body, attachments)
            results.append((addr, ok))
            if i < len(recipients) - 1:
                time.sleep(delay_seconds)
        return results
