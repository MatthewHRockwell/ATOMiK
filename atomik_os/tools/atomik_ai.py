#!/usr/bin/env python3
"""ATOMiK AI relay — real LLM calls bridged to the on-board Document.

The board has no internet. This laptop-side script makes the v0.12 LLM
abstraction real: take a natural-language prompt, send it to a real
Claude API, get back a multi-line script of ATOMiK field-delta
commands, push them through the UART into the board's chat input FIFO.
The on-board atomik_os doesn't know or care that the response came
from a real model — it sees the same field-delta commands the
hand-rolled stub would have emitted.

The LLM is asked (system prompt) to ONLY emit our small command
grammar. That keeps responses short, structured, and cheap.

Usage:
    export ANTHROPIC_API_KEY=sk-ant-...
    python3 atomik_ai.py "show me a calendar of May"
    python3 atomik_ai.py --provider claude-haiku-4.5 "make this a feed"
    python3 atomik_ai.py --dry-run "kanban for sprint 14"   # no UART write

Token-pay pitch made real: the script prints tokens-in / tokens-out
and an estimated cost in the same shape the on-board status bar uses
(uUSD = micro-USD = 1e-6 USD).
"""
import argparse, os, sys, time

PORT, BAUD = "/dev/ttyUSB2", 115200
FIFO_PATH  = "/tmp/aos_keys"

# Per-million-token cost map. Mirror of the table in src/llm.c so the
# laptop side reports the same numbers the OS surfaces.
PROVIDERS = {
    "claude-haiku-4.5":  ("claude-haiku-4-5",   1, 5),
    "claude-sonnet-4.6": ("claude-sonnet-4-6",  3, 15),
    "gpt-4o-mini":       ("gpt-4o-mini",        0, 1),
}
DEFAULT_PROVIDER = "claude-haiku-4.5"

SYSTEM_PROMPT = """You are the ATOMiK OS Document agent. The user's
desktop has ONE polymorphic Document. Your job: translate the user's
intent into a script of ATOMiK field-delta commands, one per line. Do
NOT explain. Output ONLY the commands.

Available command grammar:

    set primitive <list|card|grid|feed|convo>
    set accent <cyan|amber|pink|green|white|#hex>
    set header "<text>"
    set subtitle "<text>"
    set body "<text>"
    clear list
    add "<item>"
    load <calendar|tasks|code|brief|chat>

Rules:
- Always emit between 1 and 10 lines.
- Strings with spaces MUST be wrapped in double quotes.
- If the user wants a known shape, prefer 'load <preset>' first, then
  refine with set/add commands.
- Stay terse. Cheap tokens, fast UI.
"""


def send_slow(s, line, per=0.0008):
    for c in line:
        s.write(c.encode() if isinstance(c, str) else bytes([c]))
        time.sleep(per)
    s.flush()


def call_anthropic(model, prompt):
    """Returns (response_text, tokens_in, tokens_out)."""
    try:
        from anthropic import Anthropic
    except ImportError:
        sys.exit("pip install anthropic")
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if not api_key:
        sys.exit("set ANTHROPIC_API_KEY")
    client = Anthropic(api_key=api_key)
    msg = client.messages.create(
        model=model,
        max_tokens=512,
        system=SYSTEM_PROMPT,
        messages=[{"role": "user", "content": prompt}],
    )
    text = "".join(b.text for b in msg.content if hasattr(b, "text"))
    return text.strip(), msg.usage.input_tokens, msg.usage.output_tokens


def push_to_board(commands_text, dry_run=False):
    """Inject the response into the board's chat FIFO. Each newline-
    terminated line in `commands_text` becomes one entered chat command.
    The shell on the board is the writer; atomik_os reads /tmp/aos_keys."""
    if dry_run:
        print("[dry-run] would push:")
        for ln in commands_text.splitlines():
            print(f"   {ln}")
        return
    import serial
    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    send_slow(s, "\n"); time.sleep(0.4); s.read(8192)
    # printf to FIFO. We escape single quotes by closing/opening the literal.
    safe = commands_text.replace("'", "'\\''")
    send_slow(s, f"printf '%s' '{safe}\\n' >> {FIFO_PATH}\n")
    time.sleep(0.5)
    s.close()
    print(f"[ai] pushed {len(commands_text.splitlines())} commands to {FIFO_PATH}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("prompt", help="Natural-language intent")
    ap.add_argument("--provider", default=DEFAULT_PROVIDER,
                    choices=list(PROVIDERS.keys()))
    ap.add_argument("--dry-run", action="store_true",
                    help="Don't touch the UART; just print what would happen")
    args = ap.parse_args()

    model, in_per_M, out_per_M = PROVIDERS[args.provider]
    print(f"[ai] provider={args.provider}  model={model}", flush=True)
    print(f"[ai] prompt: {args.prompt!r}", flush=True)

    text, t_in, t_out = call_anthropic(model, args.prompt)
    cost_uusd = (t_in * in_per_M + t_out * out_per_M)   # uUSD per million
    print(f"[ai] tokens_in={t_in} tokens_out={t_out}  cost_uUSD={cost_uusd}",
          flush=True)
    print(f"[ai] cost_USD={cost_uusd / 1e6:.6f}", flush=True)
    print(f"[ai] response:\n{text}\n", flush=True)

    push_to_board(text, dry_run=args.dry_run)


if __name__ == "__main__":
    main()
