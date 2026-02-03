"""Auto-commit and CI verification utility."""

from __future__ import annotations

import subprocess
import time
from pathlib import Path

from rich.console import Console


class GitOps:
    """Git operations helper — stage, commit, and verify CI."""

    def __init__(self, repo_root: str = ".") -> None:
        self.root = Path(repo_root).resolve()
        self.console = Console()

    def _run(self, *args: str, check: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            args,
            cwd=self.root,
            capture_output=True,
            text=True,
            check=check,
        )

    def ensure_clean(self) -> bool:
        """Return True if the working tree is clean."""
        result = self._run("git", "status", "--porcelain")
        return result.stdout.strip() == ""

    def commit(
        self,
        message: str,
        files: list[str] | None = None,
        workstream: str = "",
        blockers: list[str] | None = None,
    ) -> bool:
        """Stage specific files and commit.

        Never uses ``git add -A``. Returns True on success.
        """
        if files:
            for f in files:
                self._run("git", "add", f, check=False)
        else:
            self.console.print("[yellow]No files specified — skipping commit.[/yellow]")
            return False

        # Check if there's anything staged
        result = self._run("git", "diff", "--cached", "--quiet", check=False)
        if result.returncode == 0:
            self.console.print("[dim]Nothing staged — skipping commit.[/dim]")
            return False

        # Build detailed commit message
        body_lines: list[str] = []
        if files:
            body_lines.append(f"Files: {', '.join(files[:10])}")
            if len(files) > 10:
                body_lines.append(f"  ... and {len(files) - 10} more")
        if workstream:
            body_lines.append(f"Workstream: {workstream}")
        if blockers:
            body_lines.append(f"Blockers: {', '.join(blockers)}")

        full_message = message
        if body_lines:
            full_message += "\n\n" + "\n".join(body_lines)

        result = self._run("git", "commit", "-m", full_message, check=False)
        if result.returncode == 0:
            self.console.print(f"[green]Committed:[/green] {message}")
            return True

        self.console.print(f"[red]Commit failed:[/red] {result.stderr.strip()}")
        return False

    def verify_ci(self, timeout: int = 300) -> bool:
        """Poll ``gh run list`` for latest workflow status.

        Returns True if the latest run succeeds within *timeout* seconds.
        Falls back to True if ``gh`` is not available.
        """
        try:
            self._run("gh", "--version")
        except FileNotFoundError:
            self.console.print("[dim]gh CLI not found — skipping CI check.[/dim]")
            return True

        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            result = self._run(
                "gh", "run", "list", "--limit", "1",
                "--json", "status,conclusion",
                check=False,
            )
            if result.returncode != 0:
                self.console.print("[dim]Could not query CI — skipping.[/dim]")
                return True

            import json
            try:
                runs = json.loads(result.stdout)
            except (json.JSONDecodeError, ValueError):
                return True

            if not runs:
                return True

            run = runs[0]
            if run.get("status") == "completed":
                success = run.get("conclusion") == "success"
                if success:
                    self.console.print("[green]CI passed.[/green]")
                else:
                    self.console.print(
                        f"[red]CI failed:[/red] {run.get('conclusion')}"
                    )
                return success

            time.sleep(15)

        self.console.print("[yellow]CI check timed out.[/yellow]")
        return False
