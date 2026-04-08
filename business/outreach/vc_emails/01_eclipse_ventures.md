# Eclipse Ventures — Cold Email

**Subject: 1,056 Mops/s on a $13.50 FPGA — formally verified semiconductor IP**

Hi team,

Eclipse's thesis on backing deep tech companies that build real things in atoms, not just bits, is why I'm writing. ATOMiK is a semiconductor IP company, and I have working silicon.

I built a delta-state computation architecture that replaces full-state updates with XOR-based accumulation. Every operation completes in a single clock cycle with zero carry chains. The result: 1,056 Mops/s on a $13.50 Tang Nano 9K FPGA.

What makes this defensible:
- 108 formally verified proofs in Lean4 (not tested — proven)
- Patent pending on the architecture
- 80/80 hardware tests, 353 SDK tests across 5 languages
- Built the entire stack — math, RTL, SDK, SoC — solo, for $225 total

The business model is IP licensing (ARM-style). Applications span HFT, IoT sensor fusion, database replication, and video processing. I'm raising a $3-4M seed to fund ASIC tape-out and first commercial licenses.

Would you have 15 minutes this week for a quick call? I can demo live hardware.

Matthew H. Rockwell
Founder, Rockwell Industries
matthew.h.rockwell@gmail.com
github.com/MatthewHRockwell/ATOMiK
