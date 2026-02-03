"""Playwright browser automation driver for web-form submissions."""

from __future__ import annotations

from pathlib import Path

from ..config import BrowserConfig


class BrowserDriver:
    """Wraps Playwright for filling and submitting web application forms.

    Runs in headed mode by default so the operator can solve CAPTCHAs and
    visually verify form content before submission.
    """

    def __init__(self, config: BrowserConfig) -> None:
        self.config = config
        self._playwright = None
        self._browser = None
        self._page = None

    async def launch(self) -> None:
        from playwright.async_api import async_playwright

        self._playwright = await async_playwright().start()
        self._browser = await self._playwright.chromium.launch(
            headless=self.config.headless,
            slow_mo=self.config.slow_mo,
        )
        self._page = await self._browser.new_page()

    async def _ensure_page(self) -> None:
        if self._page is None:
            await self.launch()

    async def goto(self, url: str) -> None:
        await self._ensure_page()
        assert self._page is not None
        await self._page.goto(url, wait_until="domcontentloaded")

    async def fill_field(self, selector: str, value: str) -> None:
        assert self._page is not None
        await self._page.fill(selector, value)

    async def select_option(self, selector: str, value: str) -> None:
        assert self._page is not None
        await self._page.select_option(selector, value)

    async def upload_file(self, selector: str, file_path: str) -> None:
        assert self._page is not None
        await self._page.set_input_files(selector, file_path)

    async def click(self, selector: str) -> None:
        assert self._page is not None
        await self._page.click(selector)

    async def wait_for_human(
        self, prompt: str = "Complete CAPTCHA then press Enter"
    ) -> None:
        """Pause for human intervention (CAPTCHA, review, etc.)."""
        print(f"\n>>> {prompt}")
        input("    Press Enter to continue...")

    async def submit_form(self, selector: str = "form") -> None:
        assert self._page is not None
        await self._page.eval_on_selector(
            selector, "form => form.submit()"
        )

    async def screenshot(self, path: str) -> None:
        assert self._page is not None
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        await self._page.screenshot(path=path)

    async def close(self) -> None:
        if self._browser:
            await self._browser.close()
            self._browser = None
        if self._playwright:
            await self._playwright.stop()
            self._playwright = None
        self._page = None
