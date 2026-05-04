# 2-min demo script — AdaptiveCards

**Frame:** "I want to show you AdaptiveCards rendered on hardware
that's outside your current host ecosystem."

**0:00 — board on camera.** "AX7020 FPGA. 100 MHz soft RISC-V. 1080p
HDMI. The whole OS is 72 KB."

**0:15 — show a stock AdaptiveCards JSON.** Anything from your samples
gallery — a hotel-booking card, a flight status card.

**0:30 — run the adapter.** `python3 adapters/adaptivecards_to_atomik.py
samples/hotel_card.json | nc board 5000` (or via UART relay). Card
appears on the FPGA's HDMI output.

**0:50 — change a field.** `echo "set field price '$320'" | nc board 5000`.
Just the price field updates — every other pixel stays put. "That's
the field-delta wire format — change one field, one delta on the
wire, no full re-render."

**1:15 — the architectural pitch.** "AdaptiveCards is a schema. We're
a renderer extension into hardware your host ecosystem doesn't reach.
This isn't a competitor; it's expansion."

**1:30 — the ask.** "Two things: (1) feedback on whether this fits
the host ecosystem positioning, and (2) introductions to anyone in
the AC community asking for embedded/edge rendering."

**1:45 — close.** "All open source, all reproducible on a $200 board.
Happy to demo live."
