"""Rich terminal review/approval UI for funding applications."""

from __future__ import annotations

from typing import TYPE_CHECKING

from rich.console import Console
from rich.panel import Panel
from rich.prompt import Prompt
from rich.table import Table

if TYPE_CHECKING:
    from .config import FundingConfig
    from .programs.base import ApplicationContent, BaseApplication
    from .status import StatusTracker

_STATUS_COLOURS = {
    "not_started": "dim",
    "prerequisites_missing": "yellow",
    "content_generated": "cyan",
    "under_review": "blue",
    "approved": "green",
    "submitting": "bold magenta",
    "submitted": "bold green",
    "blocked": "red",
    "skipped": "dim red",
}


class ReviewUI:
    """Rich-powered terminal UI for reviewing and approving applications."""

    def __init__(self) -> None:
        self.console = Console()

    # ------------------------------------------------------------------
    # Plan view
    # ------------------------------------------------------------------

    def show_plan(
        self,
        programs: list[BaseApplication],
        config: FundingConfig,
        tracker: StatusTracker,
    ) -> None:
        """Display the full execution plan as a Rich table."""
        self.console.print()
        self.console.rule("[bold]ATOMiK Funding Pipeline — Execution Plan")
        self.console.print()

        # Prerequisites summary
        prereq_table = Table(title="Prerequisites", show_lines=True)
        prereq_table.add_column("Item", style="bold")
        prereq_table.add_column("Status")
        prereq_table.add_row(
            "Incorporation (Delaware C-Corp)",
            _yes_no(config.incorporation_completed),
        )
        prereq_table.add_row(
            "SAM.gov Registration",
            _yes_no(config.sam_gov_registered),
        )
        prereq_table.add_row(
            "Founder Name",
            _yes_no(bool(config.founder.name)),
        )
        prereq_table.add_row(
            "SMTP Email Configured",
            _yes_no(bool(config.email.smtp_host)),
        )
        self.console.print(prereq_table)
        self.console.print()

        # Programs table
        table = Table(title="Programs (25 targets)", show_lines=True)
        table.add_column("#", justify="right", style="dim")
        table.add_column("Phase", justify="center")
        table.add_column("Tier", justify="center")
        table.add_column("Program", style="bold")
        table.add_column("Method")
        table.add_column("Status")
        table.add_column("Ready?")

        for i, prog in enumerate(programs, 1):
            status = tracker.get(prog.name)
            ready, missing = prog.check_prerequisites(config)
            colour = _STATUS_COLOURS.get(status.value, "white")
            ready_text = (
                "[green]Yes[/green]"
                if ready
                else f"[yellow]No[/yellow] ({', '.join(missing[:2])})"
            )
            table.add_row(
                str(i),
                str(prog.phase),
                str(prog.tier),
                prog.display_name,
                prog.submission_method.value,
                f"[{colour}]{status.value}[/{colour}]",
                ready_text,
            )

        self.console.print(table)
        self.console.print()

    # ------------------------------------------------------------------
    # Status dashboard
    # ------------------------------------------------------------------

    def show_status(self, tracker: StatusTracker) -> None:
        """Display the status dashboard."""
        self.console.print()
        self.console.rule("[bold]ATOMiK Funding Pipeline — Status Dashboard")
        self.console.print()

        data = tracker.get_dashboard_data()
        if not data:
            self.console.print("[dim]No programs tracked yet. Run 'plan' first.[/dim]")
            return

        table = Table(show_lines=True)
        table.add_column("Program", style="bold")
        table.add_column("Phase", justify="center")
        table.add_column("Tier", justify="center")
        table.add_column("Status")
        table.add_column("Last Updated", style="dim")
        table.add_column("Notes", style="dim")

        submitted = 0
        for ps in data:
            colour = _STATUS_COLOURS.get(ps.status.value, "white")
            table.add_row(
                ps.name,
                str(ps.phase),
                str(ps.tier),
                f"[{colour}]{ps.status.value}[/{colour}]",
                ps.last_updated[:10] if ps.last_updated else "",
                ps.notes[:60] if ps.notes else "",
            )
            if ps.status.value == "submitted":
                submitted += 1

        self.console.print(table)
        self.console.print(
            f"\n[bold]{submitted}[/bold] / {len(data)} programs submitted.\n"
        )

    # ------------------------------------------------------------------
    # Content review
    # ------------------------------------------------------------------

    def review_content(self, content: ApplicationContent) -> str:
        """Show generated content and return user decision.

        Returns one of: ``'approve'``, ``'skip'``, ``'edit'``, ``'quit'``.
        """
        self.console.print()
        self.console.rule(
            f"[bold]Review: {content.program_name}[/bold]"
        )

        # Form fields
        if content.fields:
            for field_name, value in content.fields.items():
                self.console.print(
                    Panel(
                        value,
                        title=f"[cyan]{field_name}[/cyan]",
                        border_style="cyan",
                    )
                )

        # Email content
        if content.email_subject:
            self.console.print(
                Panel(
                    f"[bold]Subject:[/bold] {content.email_subject}\n\n"
                    f"{content.email_body}",
                    title="[magenta]Email[/magenta]",
                    border_style="magenta",
                )
            )

        # Attachments
        if content.attachments:
            self.console.print(
                Panel(
                    "\n".join(f"  - {a}" for a in content.attachments),
                    title="[yellow]Attachments[/yellow]",
                    border_style="yellow",
                )
            )

        # Documents
        if content.documents:
            self.console.print(
                Panel(
                    "\n".join(f"  - {d}" for d in content.documents),
                    title="[green]Generated Documents[/green]",
                    border_style="green",
                )
            )

        # Notes
        if content.notes:
            self.console.print(f"[dim]Notes: {content.notes}[/dim]")

        self.console.print()
        choice = Prompt.ask(
            "[bold]Action[/bold]",
            choices=["a", "s", "e", "q"],
            default="a",
        )
        return {
            "a": "approve",
            "s": "skip",
            "e": "edit",
            "q": "quit",
        }.get(choice, "skip")

    # ------------------------------------------------------------------
    # Submission result
    # ------------------------------------------------------------------

    def show_submission_result(
        self, program_name: str, success: bool, notes: str
    ) -> None:
        """Display a submission outcome."""
        if success:
            self.console.print(
                f"[bold green]SUBMITTED[/bold green] {program_name}"
            )
        else:
            self.console.print(
                f"[bold red]FAILED[/bold red] {program_name}"
            )
        if notes:
            self.console.print(f"  [dim]{notes}[/dim]")

    # ------------------------------------------------------------------
    # Confirm begin
    # ------------------------------------------------------------------

    def confirm_begin(self) -> bool:
        """Ask the user to confirm before starting submissions."""
        self.console.print()
        answer = Prompt.ask(
            "[bold]Start autonomous submission pipeline?[/bold]",
            choices=["y", "n"],
            default="n",
        )
        return answer.lower() == "y"


def _yes_no(val: bool) -> str:
    return "[green]Yes[/green]" if val else "[red]No[/red]"
