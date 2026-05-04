---
name: ATOMiK OS UI work was lost — commit obsessively from now on
description: Previous Claude session built a Windows/Mac-competitive desktop UI on the board. None of it made it into git. tmpfs wipe + chat disconnect = total loss. Never let this happen again.
type: feedback
originSessionId: 0b1da619-fb37-4461-99b3-200ab2dbce84
---
The previous Claude session was building the **first ATOMiK OS** — an aesthetically-appealing desktop UI/UX that the user described as "competitive with Windows and Mac," intended to host downloadable apps that run with ATOMiK acceleration. The user has companion designs for a full laptop build concept around this OS. This was the centerpiece of the project, not a side experiment.

When the previous chat's API connection dropped, all of that work vanished:
- Not in git (no commits, no stash)
- Not in `~/.claude/.../memory/` (no memory file describing it)
- The on-board files lived in `/tmp` (tmpfs, wiped on every Linux reboot)
- The cold-cycle USB debugging this session destroyed any in-RAM state from the previous session
- No salvage was possible

**Result:** the user came back expecting the OS, found atomik_live (the older investor demo) and went off — entirely justifiably.

**Rules going forward — this is non-negotiable:**

1. **Every working board-side iteration gets pulled to the laptop tree and committed within minutes of building.** Source files (.c/.h/.css/.html) live in git, not on the board's tmpfs. The board is a runtime target, not a source-of-truth.
2. **`atomik_live.c` was the worst-case anti-pattern** — a 133KB monolithic C file that grew on the board and lived 100% in tmpfs. New OS work should be modular, in-tree from line one, with each module committed individually so a disconnect at any moment leaves something working.
3. **Memory writes for OS-design decisions are mandatory** — design choices, layout, color palette, font, animation timing all get captured as `project_atomik_os_*.md` so a fresh Claude can pick up exactly where the prior session left off, even with no commits in flight.
4. **Even WIP commits are fine.** `WIP: window manager skeleton` is infinitely better than 4 hours of un-committed compositor code lost to a network blip.
5. **NEVER do USB work that requires cold-cycles for more than 1 hypothesis** — the user is the one cycling the board and they get fucking pissed (rightfully). Batch experiments. Prove via simulation or rebuild paths first.

**Why:** The user's rage about "regression and repeat" after the disconnect is the result of building anything important outside git. They thought the work was committed and noted because that should be table stakes. It wasn't, and it's gone, and the trust hit is real.

**How to apply:** Before building anything new on the board, the FIRST commit must be the empty source file in the laptop tree. Then every diff goes in. Push to GitHub remote (`MatthewHRockwell/ATOMiK`) at end of every meaningful work block.
