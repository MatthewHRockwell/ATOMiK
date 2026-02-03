# ATOMiK Demo Quickstart

*Step-by-step instructions to run the ATOMiK demo in simulation mode. No hardware required.*

---

## What You'll See

The ATOMiK demo is a live simulation that demonstrates the core capabilities of delta-state computing through 5 "acts" — like scenes in a play. Each act shows a different capability, translated into business-relevant scenarios.

You can view the demo in two ways:
- **Web Dashboard** — Opens in your web browser (recommended for non-technical users)
- **Terminal UI** — Runs in a command-line window (richer detail)

Both show the same demo. The web dashboard is easier to navigate; the terminal UI shows more technical detail.

---

## Prerequisites

You need:
- **Python 3.9 or newer** — Most modern computers have this. To check, open a command prompt or terminal and type: `python --version`
- **An internet connection** — Only needed for the initial setup (downloading the software)

### If Python Is Not Installed

**Windows:**
1. Go to [python.org/downloads](https://www.python.org/downloads/)
2. Download the latest Python installer
3. Run the installer — **check the box that says "Add Python to PATH"**
4. Click "Install Now"

**Mac:**
1. Open Terminal (search for "Terminal" in Spotlight)
2. Type: `python3 --version`
3. If not installed, it will prompt you to install command-line tools — click "Install"

---

## Setup (One Time)

Open a command prompt (Windows) or terminal (Mac/Linux) and run these commands one at a time:

### Step 1: Navigate to the ATOMiK folder

```
cd path/to/ATOMiK
```

Replace `path/to/ATOMiK` with the actual location of the ATOMiK project folder on your computer.

### Step 2: Install the ATOMiK software

```
pip install -e .
```

This installs the ATOMiK SDK and demo tools. You'll see some text scroll by — wait until it finishes and you see your command prompt again.

If you see an error about "pip not found," try:
```
python -m pip install -e .
```

---

## Running the Demo

### Option A: Web Dashboard (Recommended)

Run this command:

```
python -m demos.run_demo --mode simulate --web-only
```

**What happens:**
1. The demo starts and opens a web dashboard
2. Your default web browser opens to `http://127.0.0.1:8000`
3. You'll see the ATOMiK demo dashboard with live charts and metrics

**To stop:** Press `Ctrl+C` in the command prompt/terminal window.

### Option B: Terminal UI + Web Dashboard

Run this command:

```
python -m demos.run_demo --mode simulate --web
```

**What happens:**
1. A terminal-based interface appears showing the demo progress
2. The web dashboard also starts at `http://127.0.0.1:8000`
3. You can watch in either the terminal or the browser (or both)

**To stop:** Press `q` to quit the terminal UI, or press `Ctrl+C`.

### Option C: Terminal UI Only

```
python -m demos.run_demo --mode simulate
```

### Option D: Simple Text Output (No Graphics)

If you prefer plain text without any graphical interface:

```
python -m demos.run_demo --mode simulate --headless
```

This prints results directly to the terminal as plain text.

---

## The 5 Acts — What Each One Demonstrates

### Act 1: Basic Delta-State Algebra
**What it shows:** The fundamental operation — applying a change (delta) to a state and getting the correct result.

**In business terms:** This proves the core concept works: you can track changes instead of full copies, and reconstruct the current state correctly at any time.

### Act 2: Self-Inverse Property
**What it shows:** Applying the same change twice returns you to the original state — every operation is its own undo.

**In business terms:** This means instant rollback with zero overhead. In financial trading, a reversed trade costs nothing. In a database, an undo is a single operation, not a restore from backup.

### Act 3: Parallel Scaling
**What it shows:** Performance scales linearly as you add parallel processing banks (4x, 8x, 16x).

**In business terms:** Double the hardware resources, double the throughput — no diminishing returns. This is the foundation of ATOMiK's scalability story. The $10 chip with 16 banks already breaks 1 billion operations per second.

### Act 4: Domain Applications
**What it shows:** Delta-state operations applied to real-world scenarios — trading, IoT, video processing.

**In business terms:** This isn't just a math exercise. The demo shows ATOMiK solving actual industry problems: processing market ticks, merging sensor data, computing video frame differences.

### Act 5: Distributed Merge (3-Node Network)
**What it shows:** Three nodes (simulating separate computers or devices) independently process changes and then merge their results — the final state is correct regardless of which order the changes arrived.

**In business terms:** This is the distributed systems breakthrough. Multiple locations, sensors, or servers can process data independently and merge later with guaranteed correctness. No conflicts, no coordination overhead, no data loss.

---

## Running a Single Act

To run just one act (useful for demonstrating a specific capability):

```
python -m demos.run_demo --mode simulate --act 3
```

Replace `3` with any act number (1-5).

---

## What Success Looks Like

When the demo completes successfully, you'll see:

- **All 5 acts pass** — Each act reports PASS with a summary of what was demonstrated
- **Performance metrics** — Throughput numbers, latency measurements, scaling ratios
- **Final summary** — "5/5 acts passed" confirmation

If any act shows FAIL, the simulation environment may not be set up correctly. Try running the setup step again (`pip install -e .`).

---

## Presentation Mode

For a guided walkthrough with narration text (ideal for presenting to others):

```
python -m demos.run_demo --mode simulate --presentation
```

This adds explanatory text between acts, making it easier to present the demo to an audience.

---

## Exporting Results

### From the Web Dashboard

The web dashboard displays all demo results with interactive charts. To save the results:
1. Use your browser's print function (`Ctrl+P` or `Cmd+P`) to save as PDF
2. Or take screenshots of the dashboard for your records

### From the Command Line

The demo generates detailed logs during execution. For a complete record:

```
python -m demos.run_demo --mode simulate --headless > demo_results.txt
```

This saves all output to a text file called `demo_results.txt`.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "python not found" | Try `python3` instead of `python`, or reinstall Python with "Add to PATH" checked |
| "pip not found" | Try `python -m pip install -e .` |
| "Module not found" | Make sure you ran `pip install -e .` from the ATOMiK root folder |
| Web dashboard doesn't open | Manually open your browser and go to `http://127.0.0.1:8000` |
| Port 8000 in use | Add `--port 8080` to use a different port |
| Terminal UI looks garbled | Use the web dashboard instead (`--web-only`) |

---

## Quick Reference

| What You Want | Command |
|--------------|---------|
| Web dashboard (recommended) | `python -m demos.run_demo --mode simulate --web-only` |
| Terminal + web | `python -m demos.run_demo --mode simulate --web` |
| Terminal only | `python -m demos.run_demo --mode simulate` |
| Plain text | `python -m demos.run_demo --mode simulate --headless` |
| Single act | `python -m demos.run_demo --mode simulate --act 3` |
| Presentation mode | `python -m demos.run_demo --mode simulate --presentation` |
| Different port | Add `--port 8080` to any command |

---

*ATOMiK — Delta-State Computing in Silicon*
*Patent Pending*
