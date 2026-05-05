#!/usr/bin/env python3
"""ATOMiK AI relay daemon — file-based RPC over UART.

Runs on the laptop. Polls the board for /tmp/aos_llm_request, calls
the real Anthropic API, writes /tmp/aos_llm_response back. atomik_os
on the board does the rest.

This makes the v0.21 token-pay story actually billable: the user
types '/ai give me a calendar' on the board, our daemon proxies the
call, the response comes back with real input/output token counts and
a real cost in micro-USD, and the board's audit log records every
spend.

Protocol:
    /tmp/aos_llm_request  (board -> daemon, written by atomik_os):
        line 1:  "PROVIDER:<name>\n"  (e.g. "PROVIDER:claude-haiku-4.5")
        rest:    raw user prompt
    /tmp/aos_llm_response (daemon -> board, daemon writes):
        line 1:  "TIN:<n>"
        line 2:  "TOUT:<n>"
        line 3:  "COST_UUSD:<n>"
        line 4:  "---"
        rest:    response text (multi-line ATOMiK command script)

Requires: ANTHROPIC_API_KEY set, anthropic Python SDK installed.

Usage:
    export ANTHROPIC_API_KEY=sk-ant-...
    python3 tools/atomik_ai_daemon.py
"""
import os, re, sys, time, base64
import serial

PORT, BAUD = "/dev/ttyUSB2", 115200
REQ_PATH   = "/tmp/aos_llm_request"
RESP_PATH  = "/tmp/aos_llm_response"
POLL_SEC   = 1.5

# Per-million-token pricing — mirrors src/llm.c PROVIDERS table so the
# uUSD numbers match what atomik_os shows in its cost preview.
PRICING = {
    "claude-haiku-4.5":  ("claude-haiku-4-5",   1, 5),
    "claude-sonnet-4.6": ("claude-sonnet-4-6",  3, 15),
    "gpt-4o-mini":       ("gpt-4o-mini",        0, 1),
}

SYSTEM = """You are the ATOMiK OS Document agent. Translate the
user's intent into ATOMiK field-delta commands, one per line. Output
ONLY the commands, no explanation.

Grammar:
    set primitive <list|card|grid|feed|convo>
    set accent <cyan|amber|pink|green|white|#hex>
    set header "<text>"
    set subtitle "<text>"
    set body "<text>"
    clear list
    add "<item>"
    load <calendar|tasks|code|brief|chat>

Rules: 1-10 lines. Strings with spaces in double quotes. Prefer
'load <preset>' first, then refine. Stay terse."""

_n = [0]
def slow(s, line, per=0.002):
    for c in line:
        s.write(c.encode() if isinstance(c, str) else bytes([c]))
        time.sleep(per)
    s.flush()

def cap(s, sh, t=15):
    _n[0] += 1
    START = f"_DAEM_{_n[0]:04d}_S_"
    END   = f"_DAEM_{_n[0]:04d}_E_"
    slow(s, f"echo {START}; {sh}; echo {END}\n")
    end = time.time() + t
    buf = bytearray()
    while time.time() < end:
        ch = s.read(8192)
        if ch:
            buf.extend(ch)
            if (b"\n" + END.encode()) in buf:
                time.sleep(0.1); buf.extend(s.read(8192))
                break
        else: time.sleep(0.05)
    text = re.sub(r'\x1b\[[0-9;?]*[a-zA-Z]', '',
                  buf.decode(errors='replace'))
    pat = re.compile(re.escape(START) + r'(.*?)' + re.escape(END), re.DOTALL)
    m = pat.findall(text)
    return m[-1].strip("\r\n") if m else None

def call_anthropic(model, prompt):
    try:
        from anthropic import Anthropic
    except ImportError:
        sys.exit("pip install anthropic")
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if not api_key:
        sys.exit("set ANTHROPIC_API_KEY")
    client = Anthropic(api_key=api_key)
    msg = client.messages.create(
        model=model, max_tokens=512, system=SYSTEM,
        messages=[{"role": "user", "content": prompt}],
    )
    text = "".join(b.text for b in msg.content if hasattr(b, "text"))
    return text.strip(), msg.usage.input_tokens, msg.usage.output_tokens

def push_response(s, tin, tout, cost_uusd, text):
    """Write /tmp/aos_llm_response on the board via UART."""
    body = f"TIN:{tin}\nTOUT:{tout}\nCOST_UUSD:{cost_uusd}\n---\n{text}"
    b64  = base64.b64encode(body.encode()).decode()
    # Push in <=512 char chunks so the FIFO + bash line buffer stay sane.
    cap(s, f"rm -f {RESP_PATH}.b64", t=4)
    CHUNK = 512
    for i in range(0, len(b64), CHUNK):
        cap(s, f"printf '%s' '{b64[i:i+CHUNK]}' >> {RESP_PATH}.b64", t=15)
    # Atomic decode + rename so atomik_os only ever sees the complete file.
    cap(s, f"base64 -d {RESP_PATH}.b64 > {RESP_PATH}.tmp && "
          f"mv {RESP_PATH}.tmp {RESP_PATH} && rm -f {RESP_PATH}.b64", t=10)

def serve_once(s):
    """Returns True if a request was processed (so caller can re-poll
    immediately), False otherwise."""
    body = cap(s, f"test -f {REQ_PATH} && cat {REQ_PATH} || true", t=6)
    if body is None or not body.strip():
        return False
    lines = body.splitlines()
    prov  = "claude-haiku-4.5"
    if lines and lines[0].startswith("PROVIDER:"):
        prov = lines[0][len("PROVIDER:"):].strip()
        prompt = "\n".join(lines[1:]).strip()
    else:
        prompt = "\n".join(lines).strip()
    if prov not in PRICING:
        print(f"[daemon] unknown provider {prov!r}; using haiku", flush=True)
        prov = "claude-haiku-4.5"
    model, in_per_M, out_per_M = PRICING[prov]
    print(f"[daemon] req: provider={prov} prompt={prompt!r}", flush=True)
    try:
        text, tin, tout = call_anthropic(model, prompt)
    except Exception as e:
        print(f"[daemon] call FAILED: {e}", flush=True)
        text = "(daemon error: see laptop log)"
        tin = tout = 0
    cost = tin * in_per_M + tout * out_per_M
    print(f"[daemon] tin={tin} tout={tout} cost_uUSD={cost}", flush=True)
    push_response(s, tin, tout, cost, text)
    return True

def main():
    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    slow(s, "\n"); time.sleep(1.0); s.read(8192)
    print(f"[daemon] watching {REQ_PATH}, every {POLL_SEC}s", flush=True)
    while True:
        try:
            if serve_once(s): continue
        except Exception as e:
            print(f"[daemon] serve_once error: {e}", flush=True)
        time.sleep(POLL_SEC)

if __name__ == "__main__":
    main()
