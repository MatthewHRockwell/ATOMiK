#!/usr/bin/env python3
"""ATOMiK speech-input relay.

Architecture: the on-board atomik_os has no audio hardware. Speech
recognition runs on the laptop; the resulting text is piped through
the SAME UART channel `atomik_ai.py` uses to inject typed commands
into the Document's chat FIFO.

Two modes:

  --record SECONDS     : capture <SECONDS> of mic audio via sounddevice,
                         transcribe via OpenAI Whisper API, optionally
                         hand the transcript to atomik_ai.py for the
                         agent to interpret as a command, then inject
                         the resulting field-delta script into the FIFO.

  --text "..."         : skip the mic; treat the given text as the
                         already-transcribed prompt. Useful when paired
                         with the OS's native voice-to-text (e.g. Mac
                         dictation) — speak, paste, send.

  --raw                : skip the agent step and inject the transcript
                         verbatim into the chat FIFO (lets the user
                         dictate raw commands like 'load calendar').

Token-pay alignment: every transcription cost gets reported on stdout
in the same uUSD format the on-board status bar uses, so you see the
total pipeline cost (Whisper + Claude + ATOMiK markup if any).
"""
import argparse, os, subprocess, sys, time, tempfile

PORT, BAUD = "/dev/ttyUSB2", 115200
FIFO_PATH  = "/tmp/aos_keys"


def send_slow(s, line, per=0.0008):
    for c in line:
        s.write(c.encode() if isinstance(c, str) else bytes([c]))
        time.sleep(per)
    s.flush()


def record_audio(seconds):
    """Capture `seconds` from the default input device into a temp wav."""
    try:
        import sounddevice as sd
        import scipy.io.wavfile as wav
        import numpy as np
    except ImportError:
        sys.exit("pip install sounddevice scipy numpy")
    sr = 16000
    print(f"[speech] recording {seconds}s ...", flush=True)
    audio = sd.rec(int(seconds * sr), samplerate=sr, channels=1, dtype='int16')
    sd.wait()
    f = tempfile.NamedTemporaryFile(suffix=".wav", delete=False)
    wav.write(f.name, sr, audio)
    return f.name


def transcribe_openai(wav_path):
    """Whisper API. Returns (text, cost_uusd)."""
    try:
        from openai import OpenAI
    except ImportError:
        sys.exit("pip install openai")
    if not os.environ.get("OPENAI_API_KEY"):
        sys.exit("set OPENAI_API_KEY")
    client = OpenAI()
    print("[speech] transcribing via Whisper ...", flush=True)
    with open(wav_path, "rb") as f:
        rsp = client.audio.transcriptions.create(model="whisper-1", file=f)
    # Whisper-1 cost: $0.006/minute. We don't get exact billing back —
    # estimate from file size as a 16kHz int16 wav (~32 KB/s).
    sz = os.path.getsize(wav_path)
    seconds = sz / 32000.0
    cost_uusd = int(seconds / 60.0 * 6000)   # $0.006/min = 6000 uUSD/min
    return rsp.text, cost_uusd


def push_to_board(text, dry_run=False):
    if dry_run:
        print("[dry-run] would push:")
        for ln in text.splitlines(): print(f"   {ln}")
        return
    import serial
    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    send_slow(s, "\n"); time.sleep(0.4); s.read(8192)
    safe = text.replace("'", "'\\''")
    send_slow(s, f"printf '%s' '{safe}\\n' >> {FIFO_PATH}\n")
    time.sleep(0.5)
    s.close()
    print(f"[speech] pushed {len(text.splitlines())} line(s) to {FIFO_PATH}")


def hand_off_to_agent(prompt, provider, dry_run):
    """Invoke atomik_ai.py with the transcript so the LLM converts the
    natural-language utterance into a script of field-delta commands."""
    here = os.path.dirname(os.path.abspath(__file__))
    aim  = os.path.join(here, "atomik_ai.py")
    args = [sys.executable, aim, "--provider", provider]
    if dry_run: args.append("--dry-run")
    args.append(prompt)
    subprocess.run(args, check=False)


def main():
    ap = argparse.ArgumentParser()
    g  = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--record", type=float, metavar="SECONDS",
                   help="Capture audio from default mic for N seconds.")
    g.add_argument("--text", metavar="TEXT",
                   help="Skip the mic; use the given text directly.")
    ap.add_argument("--raw", action="store_true",
                    help="Inject the transcript verbatim instead of "
                         "routing through the AI agent.")
    ap.add_argument("--provider", default="claude-haiku-4.5",
                    help="LLM provider used by atomik_ai.py (when --raw "
                         "is not set).")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    if args.record:
        wav  = record_audio(args.record)
        text, cost_uusd = transcribe_openai(wav)
        print(f"[speech] transcript: {text!r}", flush=True)
        print(f"[speech] whisper cost ~{cost_uusd} uUSD", flush=True)
    else:
        text     = args.text
        cost_uusd = 0

    if args.raw:
        push_to_board(text, dry_run=args.dry_run)
    else:
        # Hand off to the LLM to translate intent -> field-delta commands.
        hand_off_to_agent(text, args.provider, args.dry_run)


if __name__ == "__main__":
    main()
