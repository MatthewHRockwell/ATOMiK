"""
ATOMiK Interactive Terminal GUI

Custom tkinter-based terminal window styled with the Catppuccin Mocha
palette from ``docs/diagrams/cli_terminal.svg``.  Launched when
``atomik-gen`` is invoked with no arguments on an interactive TTY.
"""

from __future__ import annotations

import contextlib
import io
import shlex
import sys
import tkinter as tk

from atomik_sdk import __version__

# ---------------------------------------------------------------------------
# Catppuccin Mocha palette
# ---------------------------------------------------------------------------

_PALETTE = {
    "base": "#1e1e2e",
    "surface0": "#313244",
    "surface1": "#45475a",
    "green": "#a6e3a1",
    "yellow": "#f9e2af",
    "blue": "#89b4fa",
    "overlay0": "#6c7086",
    "subtext0": "#a6adc8",
    "text": "#cdd6f4",
    "red": "#f38ba8",
    "tl_red": "#ff5f57",
    "tl_yellow": "#febc2e",
    "tl_green": "#28c840",
    "title": "#a0a0a0",
}

# ---------------------------------------------------------------------------
# Help data — mirrors the SVG layout exactly
# ---------------------------------------------------------------------------

_COMMANDS: list[tuple[str, str, str]] = [
    ("generate", "<schema>", "Generate SDK code from a schema"),
    ("validate", "<schema>", "Validate a schema (no generation)"),
    ("info", "<schema>", "Show schema summary"),
    ("batch", "<directory>", "Batch generate from a directory of schemas"),
    ("list", "", "List available target languages"),
    ("pipeline", "", "Autonomous pipeline execution"),
    ("metrics", "", "Performance metrics"),
    ("demo", "<domain>", "Run domain hardware demonstrator"),
]

_OPTIONS: list[tuple[str, str, str]] = [
    ("--output-dir", "DIR", "Output directory (default: generated)"),
    ("--languages", "LANG [...]", "Target languages (default: all 5)"),
    ("--report", "FILE", "Write JSON report to file"),
    ("-v, --verbose", "", "Verbose output"),
    ("--version", "", "Show version and exit"),
]

_PIPE_CMDS: list[tuple[str, str, str]] = [
    ("run", "<target>", "Execute full pipeline"),
    ("diff", "<target>", "Show what would change (dry run)"),
    ("status", "", "Show pipeline state"),
]

_MET_CMDS: list[tuple[str, str, str]] = [
    ("show", "", "Show metrics for last run"),
    ("compare", "", "Compare metrics across schemas"),
    ("export", "--output FILE", "Export metrics to CSV"),
]

_CMD_COL = 16
_ARG_COL = 20


class AtomikTerminal:
    """Custom GUI terminal window with Catppuccin Mocha theme."""

    def __init__(self) -> None:
        self.root = tk.Tk()
        self.root.withdraw()  # hide while building
        self.root.overrideredirect(True)
        self.root.configure(bg=_PALETTE["base"])
        self.root.geometry("720x560+200+100")
        self.root.minsize(400, 300)

        self._drag_x = 0
        self._drag_y = 0

        self._history: list[str] = []
        self._history_idx = 0

        self._build_title_bar()
        self._build_terminal()
        self._build_resize_grip()

        self._print_welcome()
        self._prompt()

        # Show in Windows taskbar
        self._show_in_taskbar()
        self.root.deiconify()
        self.text.focus_set()

    # -- title bar ------------------------------------------------------------

    def _build_title_bar(self) -> None:
        bar = tk.Frame(self.root, bg=_PALETTE["surface0"], height=36)
        bar.pack(fill=tk.X)
        bar.pack_propagate(False)

        # Traffic-light canvas
        canvas = tk.Canvas(
            bar, width=80, height=36,
            bg=_PALETTE["surface0"], highlightthickness=0,
        )
        canvas.pack(side=tk.LEFT, padx=(8, 0))

        r = 6.5
        cy = 18
        red = canvas.create_oval(
            10 - r, cy - r, 10 + r, cy + r,
            fill=_PALETTE["tl_red"], outline="",
        )
        yellow = canvas.create_oval(
            32 - r, cy - r, 32 + r, cy + r,
            fill=_PALETTE["tl_yellow"], outline="",
        )
        green = canvas.create_oval(
            54 - r, cy - r, 54 + r, cy + r,
            fill=_PALETTE["tl_green"], outline="",
        )

        canvas.tag_bind(red, "<Button-1>", lambda _e: self._close())
        canvas.tag_bind(yellow, "<Button-1>", lambda _e: self._minimize())
        canvas.tag_bind(green, "<Button-1>", lambda _e: self._toggle_maximize())

        title = tk.Label(
            bar, text="ATOMiK (C) 2026",
            bg=_PALETTE["surface0"], fg=_PALETTE["title"],
            font=("Segoe UI", 10),
        )
        title.pack(expand=True)

        # Dragging
        for widget in (bar, title):
            widget.bind("<Button-1>", self._start_drag)
            widget.bind("<B1-Motion>", self._do_drag)

    # -- terminal text area ---------------------------------------------------

    def _build_terminal(self) -> None:
        frame = tk.Frame(self.root, bg=_PALETTE["base"])
        frame.pack(fill=tk.BOTH, expand=True)

        self.text = tk.Text(
            frame,
            bg=_PALETTE["base"],
            fg=_PALETTE["text"],
            font=("Consolas", 11),
            insertbackground=_PALETTE["green"],
            selectbackground=_PALETTE["surface1"],
            selectforeground=_PALETTE["text"],
            borderwidth=0, padx=12, pady=8,
            wrap=tk.WORD, undo=False,
        )

        scrollbar = tk.Scrollbar(frame, command=self.text.yview)
        self.text.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # Colour tags matching the SVG palette
        for tag, colour in (
            ("prompt", _PALETTE["green"]),
            ("cmd", _PALETTE["yellow"]),
            ("flag", _PALETTE["blue"]),
            ("arg", _PALETTE["overlay0"]),
            ("desc", _PALETTE["subtext0"]),
            ("body", _PALETTE["text"]),
            ("success", _PALETTE["green"]),
            ("error", _PALETTE["red"]),
        ):
            self.text.tag_configure(tag, foreground=colour)

        # Input boundary
        self.text.mark_set("input_start", "end-1c")
        self.text.mark_gravity("input_start", tk.LEFT)

        # Key bindings
        self.text.bind("<Return>", self._on_return)
        self.text.bind("<BackSpace>", self._on_backspace)
        self.text.bind("<Up>", self._on_up)
        self.text.bind("<Down>", self._on_down)
        self.text.bind("<Key>", self._on_key)
        self.text.bind("<Button-1>", self._on_click)
        self.text.bind("<<Paste>>", self._on_paste)
        self.text.bind("<Control-a>", self._select_input)

    # -- resize grip ----------------------------------------------------------

    def _build_resize_grip(self) -> None:
        grip = tk.Label(
            self.root, text="\u2847",
            fg=_PALETTE["overlay0"], bg=_PALETTE["base"],
            font=("Consolas", 10), cursor="size_nw_se",
        )
        grip.place(relx=1.0, rely=1.0, anchor="se")
        grip.bind("<Button-1>", self._start_resize)
        grip.bind("<B1-Motion>", self._do_resize)
        self._resize_x = 0
        self._resize_y = 0

    # -- window management ----------------------------------------------------

    def _show_in_taskbar(self) -> None:
        """Ensure the borderless window appears in the Windows taskbar."""
        if sys.platform != "win32":
            return
        try:
            import ctypes
            gwl_exstyle = -20
            ws_ex_appwindow = 0x00040000
            ws_ex_toolwindow = 0x00000080
            hwnd = ctypes.windll.user32.GetParent(self.root.winfo_id())
            style = ctypes.windll.user32.GetWindowLongW(hwnd, gwl_exstyle)
            style = (style & ~ws_ex_toolwindow) | ws_ex_appwindow
            ctypes.windll.user32.SetWindowLongW(hwnd, gwl_exstyle, style)
        except Exception:
            pass

    def _start_drag(self, event: tk.Event) -> None:
        self._drag_x = event.x_root - self.root.winfo_x()
        self._drag_y = event.y_root - self.root.winfo_y()

    def _do_drag(self, event: tk.Event) -> None:
        self.root.geometry(
            f"+{event.x_root - self._drag_x}+{event.y_root - self._drag_y}"
        )

    def _start_resize(self, event: tk.Event) -> None:
        self._resize_x = event.x_root
        self._resize_y = event.y_root

    def _do_resize(self, event: tk.Event) -> None:
        dx = event.x_root - self._resize_x
        dy = event.y_root - self._resize_y
        self._resize_x = event.x_root
        self._resize_y = event.y_root
        w = max(self.root.winfo_width() + dx, 400)
        h = max(self.root.winfo_height() + dy, 300)
        self.root.geometry(f"{w}x{h}")

    def _close(self) -> None:
        self.root.destroy()

    def _minimize(self) -> None:
        self.root.overrideredirect(False)
        self.root.iconify()

        def _restore(_event: tk.Event) -> None:
            if str(self.root.state()) == "normal":
                self.root.overrideredirect(True)
                self._show_in_taskbar()
                self.root.unbind("<Map>")

        self.root.bind("<Map>", _restore)

    def _toggle_maximize(self) -> None:
        if hasattr(self, "_pre_max_geo"):
            self.root.geometry(self._pre_max_geo)
            del self._pre_max_geo
        else:
            self._pre_max_geo = self.root.geometry()
            sw = self.root.winfo_screenwidth()
            sh = self.root.winfo_screenheight()
            self.root.geometry(f"{sw}x{sh}+0+0")

    # -- text helpers ---------------------------------------------------------

    def _write(self, text: str, tag: str = "body") -> None:
        self.text.insert(tk.END, text, tag)
        self.text.see(tk.END)

    def _writeln(self, text: str = "", tag: str = "body") -> None:
        self._write(text + "\n", tag)

    def _write_parts(self, parts: list[tuple[str, str]]) -> None:
        """Write a line from ``[(text, tag), ...]`` tuples."""
        for text, tag in parts:
            self.text.insert(tk.END, text, tag)
        self.text.insert(tk.END, "\n")
        self.text.see(tk.END)

    def _prompt(self) -> None:
        self._write("$ ", "prompt")
        self.text.mark_set("input_start", "end-1c")
        self.text.mark_gravity("input_start", tk.LEFT)
        self.text.see(tk.END)

    def _get_input(self) -> str:
        return self.text.get("input_start", "end-1c")

    def _clear_input(self) -> None:
        self.text.delete("input_start", "end-1c")

    # -- keyboard handlers ----------------------------------------------------

    def _on_return(self, _event: tk.Event) -> str:
        raw = self._get_input().strip()
        self._write("\n")
        if raw:
            self._history.append(raw)
            self._history_idx = len(self._history)
            self._handle_input(raw)
        self._prompt()
        return "break"

    def _on_backspace(self, _event: tk.Event) -> str | None:
        if self.text.compare("insert", "<=", "input_start"):
            return "break"
        return None

    def _on_key(self, event: tk.Event) -> str | None:
        if event.char and event.char.isprintable():
            if self.text.compare("insert", "<", "input_start"):
                self.text.mark_set("insert", "end-1c")
        return None

    def _on_click(self, _event: tk.Event) -> None:
        self.root.after(1, self._clamp_cursor)

    def _clamp_cursor(self) -> None:
        if self.text.compare("insert", "<", "input_start"):
            self.text.mark_set("insert", "end-1c")

    def _on_paste(self, _event: tk.Event) -> None:
        if self.text.compare("insert", "<", "input_start"):
            self.text.mark_set("insert", "end-1c")

    def _select_input(self, _event: tk.Event) -> str:
        self.text.tag_remove("sel", "1.0", tk.END)
        self.text.tag_add("sel", "input_start", "end-1c")
        return "break"

    def _on_up(self, _event: tk.Event) -> str:
        if self._history and self._history_idx > 0:
            self._history_idx -= 1
            self._clear_input()
            self.text.insert("input_start", self._history[self._history_idx])
        return "break"

    def _on_down(self, _event: tk.Event) -> str:
        if self._history_idx < len(self._history) - 1:
            self._history_idx += 1
            self._clear_input()
            self.text.insert("input_start", self._history[self._history_idx])
        elif self._history_idx == len(self._history) - 1:
            self._history_idx = len(self._history)
            self._clear_input()
        return "break"

    # -- help -----------------------------------------------------------------

    def _render_help(self) -> None:
        self._writeln()
        self._write_parts([("Usage: ", "body"), ("atomik-gen <command> [options]", "arg")])
        self._writeln()

        self._writeln("Commands:", "body")
        for name, arg, desc in _COMMANDS:
            self._write_parts([
                ("  ", "body"),
                (name.ljust(_CMD_COL), "cmd"),
                (arg.ljust(_ARG_COL), "arg"),
                (desc, "desc"),
            ])
        self._writeln()

        self._writeln("Options:", "body")
        for flag, arg, desc in _OPTIONS:
            self._write_parts([
                ("  ", "body"),
                (flag.ljust(_CMD_COL), "flag"),
                (arg.ljust(_ARG_COL), "arg"),
                (desc, "desc"),
            ])
        self._writeln()

        self._writeln("Pipeline subcommands:", "body")
        for name, arg, desc in _PIPE_CMDS:
            self._write_parts([
                ("  ", "body"),
                (name.ljust(_CMD_COL), "cmd"),
                (arg.ljust(_ARG_COL), "arg"),
                (desc, "desc"),
            ])
        self._writeln()

        self._writeln("Metrics subcommands:", "body")
        for name, arg, desc in _MET_CMDS:
            self._write_parts([
                ("  ", "body"),
                (name.ljust(_CMD_COL), "cmd"),
                (arg.ljust(_ARG_COL), "arg"),
                (desc, "desc"),
            ])
        self._writeln()

    # -- command dispatch -----------------------------------------------------

    def _dispatch(self, raw: str) -> None:
        from atomik_sdk.cli import (
            build_parser,
            cmd_batch,
            cmd_demo,
            cmd_generate,
            cmd_info,
            cmd_list,
            cmd_metrics_compare,
            cmd_metrics_export,
            cmd_metrics_show,
            cmd_pipeline_diff,
            cmd_pipeline_run,
            cmd_pipeline_status,
            cmd_validate,
        )

        for prefix in ("atomik-gen ", "atomik_gen "):
            if raw.startswith(prefix):
                raw = raw[len(prefix):]
                break

        try:
            tokens = shlex.split(raw)
        except ValueError as exc:
            self._writeln(f"Parse error: {exc}", "error")
            return

        if not tokens:
            return

        parser = build_parser()

        # argparse may print help/errors to stdout/stderr, then call sys.exit
        captured = io.StringIO()
        try:
            with contextlib.redirect_stdout(captured), contextlib.redirect_stderr(captured):
                args = parser.parse_args(tokens)
        except SystemExit:
            out = captured.getvalue()
            if out:
                self._writeln(out.rstrip(), "body")
            return

        cmd = getattr(args, "command", None)
        if cmd is None:
            self._render_help()
            return

        handler = None

        if cmd == "pipeline":
            pipe = {
                "run": cmd_pipeline_run,
                "diff": cmd_pipeline_diff,
                "status": cmd_pipeline_status,
            }
            subcmd = getattr(args, "pipeline_command", None)
            if subcmd in pipe:
                handler = pipe[subcmd]
            else:
                self._writeln("Usage: pipeline <run|diff|status>", "error")
                return

        elif cmd == "metrics":
            met = {
                "show": cmd_metrics_show,
                "compare": cmd_metrics_compare,
                "export": cmd_metrics_export,
            }
            subcmd = getattr(args, "metrics_command", None)
            if subcmd in met:
                handler = met[subcmd]
            else:
                self._writeln("Usage: metrics <show|compare|export>", "error")
                return

        else:
            commands = {
                "generate": cmd_generate,
                "validate": cmd_validate,
                "info": cmd_info,
                "batch": cmd_batch,
                "list": cmd_list,
                "demo": cmd_demo,
            }
            handler = commands.get(cmd)

        if handler is None:
            self._writeln(f"Unknown command: {cmd}", "error")
            return

        stdout_buf = io.StringIO()
        stderr_buf = io.StringIO()
        try:
            with contextlib.redirect_stdout(stdout_buf), \
                 contextlib.redirect_stderr(stderr_buf):
                handler(args)
        except Exception as exc:
            self._writeln(f"Error: {exc}", "error")
            return

        out = stdout_buf.getvalue()
        if out:
            self._writeln(out.rstrip(), "body")
        err = stderr_buf.getvalue()
        if err:
            self._writeln(err.rstrip(), "error")

    # -- input router ---------------------------------------------------------

    def _handle_input(self, raw: str) -> None:
        lower = raw.lower()

        if lower in ("exit", "quit", "q"):
            self._close()
            return
        if lower == "help":
            self._render_help()
            return
        if lower == "clear":
            self.text.delete("1.0", tk.END)
            self._print_welcome()
            return
        if lower == "version":
            self._writeln(f"atomik-gen {__version__}", "body")
            return

        self._dispatch(raw)

    # -- welcome --------------------------------------------------------------

    def _print_welcome(self) -> None:
        self._writeln(
            f"  ATOMiK v{__version__} \u2014 Delta-State Computation in Silicon",
            "body",
        )
        self._write("  Type ", "body")
        self._write("help", "cmd")
        self._write(" for commands, ", "body")
        self._write("exit", "cmd")
        self._writeln(" to quit.", "body")
        self._writeln()

    # -- run ------------------------------------------------------------------

    def run(self) -> None:
        self.root.mainloop()


def main() -> None:
    """Entry point for interactive terminal (called from cli.py)."""
    terminal = AtomikTerminal()
    terminal.run()


if __name__ == "__main__":
    main()
