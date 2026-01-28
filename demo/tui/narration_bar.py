"""Narration bar — presentation-mode text display with typing animation."""

from __future__ import annotations

from typing import Any

from textual.widgets import Static


class NarrationBar(Static):
    """Bottom bar for VC presentation narration text."""

    def __init__(self, **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self._full_text = ""

    def set_text(self, text: str) -> None:
        """Update the narration text immediately."""
        self._full_text = text
        self.update(f"  {text}")

    def clear(self) -> None:
        self._full_text = ""
        self.update("")
