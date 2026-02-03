"""Report generation — Rich terminal output + markdown file."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from rich.console import Console
from rich.table import Table


def print_results(results: list[dict[str, Any]]) -> None:
    """Display benchmark results as a Rich table."""
    console = Console()
    console.print()
    console.rule("[bold]ATOMiK State Sync Benchmark Results")
    console.print()

    for r in results:
        name = r.get("name", "Unknown")
        table = Table(title=name, show_lines=True)
        table.add_column("Metric", style="bold")
        table.add_column("ATOMiK", style="green")
        table.add_column("Conventional", style="red")

        if "atomik_ops_sec" in r:
            table.add_row(
                "Throughput (ops/s)",
                f"{r['atomik_ops_sec']:,.0f}",
                f"{r['conventional_ops_sec']:,.0f}",
            )
            table.add_row(
                "Speedup",
                f"{r.get('speedup', 0):.2f}x",
                "1.0x (baseline)",
            )

        if "atomik_bytes" in r:
            table.add_row(
                "Bytes moved",
                f"{r['atomik_bytes']:,}",
                f"{r['conventional_bytes']:,}",
            )

        if "atomik_undo_ops" in r:
            table.add_row(
                "Undo operations",
                str(r["atomik_undo_ops"]),
                str(r["conventional_undo_ops"]),
            )
            table.add_row(
                "Undo latency",
                f"{r['atomik_latency_us']:.2f} us",
                f"{r['conventional_latency_us']:.2f} us",
            )

        if "atomik_messages" in r:
            table.add_row(
                "Messages",
                str(r["atomik_messages"]),
                str(r["conventional_messages"]),
            )
            table.add_row(
                "Correct?",
                str(r.get("atomik_correct", "")),
                str(r.get("conventional_correct", "")),
            )

        if "atomik_bytes_written" in r:
            table.add_row(
                "Bytes written",
                f"{r['atomik_bytes_written']:,}",
                f"{r['conventional_bytes_written']:,}",
            )
            table.add_row(
                "Reduction",
                f"{r.get('reduction_pct', 0):.1f}%",
                "0% (baseline)",
            )

        console.print(table)
        console.print()


def save_results(results: list[dict[str, Any]], output_dir: str) -> None:
    """Save results as JSON and markdown report."""
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)

    # JSON
    json_path = out / "results.json"
    json_path.write_text(
        json.dumps(
            {
                "timestamp": datetime.now(timezone.utc).isoformat(),
                "scenarios": results,
            },
            indent=2,
        ),
        encoding="utf-8",
    )

    # Markdown
    md_lines = [
        "# ATOMiK State Sync Benchmark Report",
        "",
        f"*Generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}*",
        "",
        "## Summary",
        "",
        "| Scenario | ATOMiK Advantage | Key Metric |",
        "|----------|-----------------|------------|",
    ]

    for r in results:
        name = r.get("name", "Unknown")
        if "speedup" in r:
            md_lines.append(
                f"| {name} | {r['speedup']:.1f}x faster "
                f"| {r['atomik_ops_sec']:,.0f} ops/s |"
            )
        elif "atomik_undo_ops" in r:
            md_lines.append(
                f"| {name} | {r['atomik_undo_ops']} op vs "
                f"{r['conventional_undo_ops']} ops "
                f"| {r['atomik_latency_us']:.1f} us |"
            )
        elif "atomik_messages" in r:
            md_lines.append(
                f"| {name} | {r['atomik_messages']} msgs vs "
                f"{r['conventional_messages']} msgs "
                f"| Order-independent |"
            )
        elif "reduction_pct" in r:
            md_lines.append(
                f"| {name} | {r['reduction_pct']:.1f}% reduction "
                f"| {r['atomik_bytes_written']:,} bytes |"
            )

    md_lines.extend([
        "",
        "## Scenarios",
        "",
    ])

    for r in results:
        md_lines.append(f"### {r.get('name', 'Unknown')}")
        md_lines.append("")
        for key, val in r.items():
            if key == "name":
                continue
            md_lines.append(f"- **{key}**: {val}")
        md_lines.append("")

    md_path = out / "benchmark_report.md"
    md_path.write_text("\n".join(md_lines), encoding="utf-8")
