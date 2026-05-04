# 2-min demo script — Vercel v0

**Frame:** "I'm going to render a v0 component on hardware that can't
run a browser."

**0:00 — board on camera.** Visible: AX7020 dev board, HDMI cable,
1080p monitor showing ATOMiK OS desktop.

**0:10 — opening line.** "This is a $200 FPGA running a custom OS.
The CPU is a 100 MHz soft RISC-V — about 1980s clock speed. No GPU.
No JavaScript runtime. The whole OS is 72 KB."

**0:25 — open Document app.** Press D. Window opens with chat input.
"Document is one app that morphs into anything by changing field
values on a shared compiled UI frame. It's the invariant-frame
architecture — you ship deltas, not pixels, not DOM trees."

**0:50 — type "show me a calendar".** OS routes through local-intent
classifier (no network), changes primitive to grid, accent to cyan.
Calendar appears. "That command was classified on-device in under
1 ms. No round-trip."

**1:10 — show the AdaptiveCards adapter.** Open a terminal on the
laptop. `cat sample_card.json | python3 adapters/adaptivecards_to_atomik.py`.
Same UI re-renders on the board. "We already accept AdaptiveCards.
A v0 component output adapter is the same shape — JSON in, deltas out."

**1:30 — the ask.** "I want to render v0's output here. The market
isn't 'browsers replaced'; it's 'displays added.' Anywhere v0 can't
reach today: kiosks, dashboards, in-vehicle, dev boards. I'd take
20 minutes to walk through what an integration would look like."

**1:50 — close.** "Code is open. Hardware is sitting next to me. I
can run it for you live tomorrow."
