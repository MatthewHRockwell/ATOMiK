# Demo Recording Checklist

## Pre-Recording Setup

### Hardware
- [ ] 3x Tang Nano 9K boards connected via USB
- [ ] Verify all boards detected: `python -m demo.run_demo --headless --mode hardware`
- [ ] Bitstreams programmed: N4@81MHz, N8@67.5MHz, N16@67.5MHz
- [ ] Power LEDs visible on all boards
- [ ] USB hub if needed (3 USB ports)

### Software
- [ ] Python environment activated with demo dependencies
- [ ] `python -m demo.run_demo --mode simulate --headless` passes all 5 acts
- [ ] TUI renders correctly: `python -m demo.run_demo --mode simulate`
- [ ] Web dashboard loads: `python -m demo.run_demo --mode simulate --web-only`
- [ ] Presentation mode works: `python -m demo.run_demo --mode simulate --presentation`

### Recording Software
- [ ] OBS Studio installed and configured
- [ ] Scene 1: Terminal (TUI dashboard, full screen)
- [ ] Scene 2: Browser (web dashboard)
- [ ] Scene 3: Slides (pitch deck PDF)
- [ ] Scene 4: Board close-up (webcam/phone cam)
- [ ] Audio: microphone tested, noise gate configured
- [ ] Output: 1920x1080, 30fps, H.264, high bitrate (15+ Mbps)

### Terminal
- [ ] Font: JetBrains Mono or similar monospace, 14-16pt
- [ ] Terminal window maximised
- [ ] Clear terminal history
- [ ] No notifications/popups visible
- [ ] Dark theme (Catppuccin Mocha matches the dashboard)

## Recording Flow

### 3-Minute Highlight
1. [ ] Record intro hook (board close-up + dashboard)
2. [ ] Record Act 3 (scaling demo) with TUI
3. [ ] Record Act 5 (distributed merge) with TUI
4. [ ] Record slides (proofs, SDK, architecture)
5. [ ] Record CTA slide
6. [ ] Record voiceover separately
7. [ ] Edit and mix

### 10-Minute Full Demo
1. [ ] Record intro sequence
2. [ ] Record slides 2-5 (problem, solution, math)
3. [ ] Record Acts 1-2 with TUI (algebra, undo)
4. [ ] Record Act 3 with TUI (scaling)
5. [ ] Record Act 4 with TUI (domain apps)
6. [ ] Record Act 5 with TUI (distributed merge)
7. [ ] Record slides 8-15 (architecture, market, business)
8. [ ] Record voiceover
9. [ ] Edit, add transitions, mix audio

## Post-Recording
- [ ] Export 3-minute version (1080p H.264)
- [ ] Export 10-minute version (1080p H.264)
- [ ] Generate thumbnail (dashboard screenshot + "1 Gops/s" text)
- [ ] Upload to private hosting for investor sharing
- [ ] Test playback on different devices
