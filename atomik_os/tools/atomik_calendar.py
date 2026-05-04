#!/usr/bin/env python3
"""ATOMiK Calendar bridge — first real cloud-service integration.

Pulls upcoming events from Google Calendar (when an OAuth token is
available), formats them as ATOMiK field-delta commands, and pushes
the commands through the UART into the on-board Document's chat
FIFO. The Document morphs into a calendar populated with the user's
actual events.

Auth modes (in priority order):

  1. GOOGLE_CALENDAR_TOKEN env var = an OAuth bearer token issued for
     the calendar.events.readonly scope. Hand-grant via the OAuth
     playground for the demo, or wire a real flow in v1.0+.

  2. GOOGLE_CALENDAR_API_KEY env var = a public-data API key (only
     usable for public calendars).

  3. No creds: fall back to a built-in mock fixture so the script
     always demos. Mock mode prints '(mock)' to stdout to make it
     obvious nothing real is happening.

Usage:
    python3 atomik_calendar.py                   # next 10 events
    python3 atomik_calendar.py --range week      # this week's events
    python3 atomik_calendar.py --dry-run         # don't touch UART

This is the v1.0 'first real cloud service' milestone. The same
architecture (laptop-side fetch + UART injection of field-delta
commands) generalizes to any API: GitHub PRs, Slack messages, RSS,
Spotify, anything REST/GraphQL/MCP.
"""
import argparse, json, os, sys, time, urllib.request

PORT, BAUD = "/dev/ttyUSB2", 115200
FIFO_PATH  = "/tmp/aos_keys"


def slow(s, line, per=0.0008):
    for c in line:
        s.write(c.encode() if isinstance(c, str) else bytes([c]))
        time.sleep(per)
    s.flush()


def fetch_real(token=None, api_key=None, calendar_id="primary",
               max_results=10):
    """Returns list of {summary, start_local, end_local}. None on failure."""
    base = ("https://www.googleapis.com/calendar/v3/calendars/"
            f"{urllib.request.quote(calendar_id)}/events")
    now  = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    params = (f"?timeMin={now}&singleEvents=true&orderBy=startTime"
              f"&maxResults={max_results}")
    if api_key and not token:
        params += f"&key={api_key}"
    url = base + params
    req = urllib.request.Request(url)
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            data = json.loads(r.read().decode())
    except Exception as e:
        print(f"[cal] fetch failed: {e}", file=sys.stderr)
        return None
    out = []
    for it in data.get("items", []):
        s = it.get("start", {})
        start = s.get("dateTime") or s.get("date") or ""
        out.append({
            "summary":     it.get("summary", "(untitled)"),
            "start_local": start.replace("T", " ")[:16],
        })
    return out


def fetch_mock():
    print("[cal] (mock) no creds — using demo fixture")
    return [
        {"summary": "sprint sync",       "start_local": "2026-05-06 10:00"},
        {"summary": "1:1 with Bob",      "start_local": "2026-05-06 14:00"},
        {"summary": "lunch w/ design",   "start_local": "2026-05-07 12:30"},
        {"summary": "ATOMiK OS review",  "start_local": "2026-05-07 15:00"},
        {"summary": "investor call",     "start_local": "2026-05-08 09:00"},
        {"summary": "deep work block",   "start_local": "2026-05-08 13:00"},
    ]


def render_to_commands(events):
    """Emit a script of ATOMiK Document chat commands that paint a feed."""
    lines = [
        'set primitive feed',
        'set accent cyan',
        'set header "Upcoming"',
        'clear list',
    ]
    for ev in events:
        text = f'{ev["start_local"]}  -  {ev["summary"]}'
        # Escape any embedded double quotes in the title
        text = text.replace('"', "'")
        lines.append(f'add "{text}"')
    if not events:
        lines.append('add "(no upcoming events)"')
    return "\n".join(lines)


def push_to_board(commands_text, dry_run=False):
    if dry_run:
        print("[dry-run] would push:")
        for ln in commands_text.splitlines(): print(f"   {ln}")
        return
    try:
        import serial
    except ImportError:
        sys.exit("pip install pyserial")
    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    slow(s, "\n"); time.sleep(0.4); s.read(8192)
    safe = commands_text.replace("'", "'\\''")
    slow(s, f"printf '%s' '{safe}\\n' >> {FIFO_PATH}\n")
    time.sleep(0.5)
    s.close()
    print(f"[cal] pushed {len(commands_text.splitlines())} commands "
          f"to {FIFO_PATH}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--max", type=int, default=8,
                    help="Max events to fetch (default 8)")
    ap.add_argument("--calendar", default="primary",
                    help="Calendar id (default 'primary')")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    token   = os.environ.get("GOOGLE_CALENDAR_TOKEN")
    api_key = os.environ.get("GOOGLE_CALENDAR_API_KEY")
    events  = None
    if token or api_key:
        events = fetch_real(token=token, api_key=api_key,
                            calendar_id=args.calendar,
                            max_results=args.max)
    if not events:
        events = fetch_mock()

    print(f"[cal] {len(events)} events")
    for ev in events:
        print(f"   {ev['start_local']}  {ev['summary']}")

    push_to_board(render_to_commands(events), dry_run=args.dry_run)


if __name__ == "__main__":
    main()
