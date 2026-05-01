# ATOMiK Two-Tier Claude Architecture

Build brief for autonomous demo development via laptop-side Claude + Zynq-side inside-man agent.

**Date:** 2026-04-29
**Status:** Implementation plan
**Proven foundation:** `board_cmd.py` end-to-end (12/13 pass), `bridge.py` websocket + serial mux, `atomik_live.c` popen() executor with `cmd_running` suppression

---

## SECTION 1 — SYSTEM THESIS

**Laptop Claude** is the strategist, planner, and build environment. It owns the cross-compiler (`riscv64-linux-gnu-gcc`), the FPGA toolchain (Vivado 2025.2), the git repo, the JTAG boot path, and the user interface. It can read and write every source file, synthesize bitstreams, cross-compile binaries, and push artifacts to the board. It cannot see what is actually happening on the board after deployment.

**Zynq-side agent** is the observer, executor, and ground truth. It runs inside the target system — same CPU, same memory space, same kernel, same peripherals. It can read `/proc`, poke MMIO registers via `/dev/mem`, inspect the framebuffer, time hardware operations, check process state, and run Python scripts that interact with ATOMiK hardware. It cannot compile C code, access the internet, or talk to git.

**Why this split wins:**

1. **Closes the observation gap.** Today, laptop Claude cross-compiles a binary, transfers it over UART (slow), runs it, and then hopes the HDMI output looks right. It has no way to verify what the board is actually doing unless the user types what they see. The inside-man provides structured introspection without human relay.

2. **Enables autonomous iteration.** Laptop Claude can: (a) cross-compile a test binary, (b) transfer it to the board, (c) run it via the inside-man, (d) collect structured results, (e) decide what to do next — all without requiring the user to look at the screen or type anything. This turns "try it and tell me what happened" into a closed loop.

3. **The demo keeps running.** The popen() executor in `atomik_live.c` already runs commands without interrupting the HDMI/LCD output. The inside-man piggybacks on this — the audience never sees the scaffolding, only the results.

4. **Matches the hardware reality.** The board has 487MB RAM, a 100MHz soft CPU, Python3, and a UART-only link to the outside world. This is not a machine that should run a language model. It is a machine that should run a lightweight agent that exposes structured capabilities to a smarter system on the other end of the wire.

---

## SECTION 2 — EVOLUTION STRATEGY

### Stage 1: Minimum Viable Inside-Man (target: this week)
A Python script on the board (`zynq_agent.py`) that laptop Claude invokes via `board_cmd.py`. The script accepts a command name and arguments, executes a predefined action, and returns structured output. No daemon. No persistent state. No intelligence on-device.

**What it gives you:** Laptop Claude can query board health, read MMIO registers, check process state, inspect file contents, and run pre-built test binaries — all through a single tool interface.

### Stage 2: Integrated Workflow (target: within 2 weeks)
Laptop Claude uses the inside-man as a first-class tool surface during normal development. Commands are dispatched as part of build-test-iterate cycles. The agent grows a richer command vocabulary (benchmark timing, framebuffer pixel sampling, LCD status, ATOMiK register readback). `bridge.py` routes both event streams and command responses.

**What it gives you:** Development velocity. Laptop Claude can verify its own work without asking the user to report what the screen shows.

### Stage 3: AI-Native Target (stretch goal, not before demo)
A persistent daemon on-board that maintains session state, queues multi-step introspection plans, and can autonomously run diagnostic sequences when anomalies are detected. Possibly backed by a local small model (phi-3-mini or similar) if network access is ever established, but more likely a sophisticated rule engine with laptop Claude as the reasoning backend.

**What it gives you:** A genuinely autonomous system where the board participates in its own debugging and demo improvement.

### Stage Transition Rules
- **1 to 2:** Transition when at least 10 commands are proven end-to-end (query + structured response) and laptop Claude has successfully used the agent in at least 3 real development tasks without user intervention.
- **2 to 3:** Transition only after the fundraising demo is locked and proven reliable. Stage 3 is R&D, not production.
- **Never skip a stage.** Each stage validates assumptions the next stage depends on.

---

## SECTION 3 — PHASED IMPLEMENTATION PLAN

### Phase A: Minimal Bring-Up

**Objective:** A Python-based agent script running on the board that laptop Claude can invoke for structured introspection.

**Scope:** 8-10 commands covering health, state, MMIO reads, process listing, file ops. No daemon. No persistence. No intelligence.

**Components:**
- `zynq_agent.py` — board-side script, invoked via `board_cmd.py "python3 /tmp/zynq_agent.py <cmd> [args]"`
- Extensions to `board_cmd.py` — convenience wrappers for common agent commands
- Transfer mechanism — base64 over UART (already proven in `demo_launch.sh`)

**Deliverables:**
- `zynq_agent.py` deployed to `/tmp/` on board
- At least 8 commands returning structured JSON responses
- Laptop-side wrapper that parses agent output into actionable data

**Proof of completion:** Laptop Claude runs `board_cmd.py "python3 /tmp/zynq_agent.py health"` and receives a JSON blob with uptime, memory, CPU load, process count, HDMI status, and ATOMiK adapter reachability.

**Risks:**
- UART throughput limits response size (~90KB/s at 921600 baud). Mitigate: keep responses compact, use JSON not verbose text, truncate large outputs.
- popen() blocks the main thread of `atomik_live`. Mitigate: commands should complete in <5 seconds; long-running ops get a timeout wrapper.

**Why now:** The popen() executor is proven. `board_cmd.py` works. The only missing piece is a structured command surface on the board side.

### Phase B: Laptop Claude Integration

**Objective:** Laptop Claude treats the inside-man as a tool it reaches for naturally during development, not a novelty it has to be reminded about.

**Scope:** Tool-calling patterns, result parsing, decision loops. Extend command vocabulary based on real development needs encountered during Phase A.

**Components:**
- `board_tool.py` — laptop-side Python module that laptop Claude can call as a Bash tool, with subcommands: `board_tool.py health`, `board_tool.py mmio 0xF0020000`, `board_tool.py run /tmp/test_binary`, etc.
- Command dispatch logic that routes to `board_cmd.py` internally
- Response parsers for JSON, exit codes, and error conditions

**Deliverables:**
- Single-command laptop-side interface for all board operations
- At least 15 proven commands
- Error handling for timeout, serial port busy, board not responding

**Proof of completion:** Laptop Claude cross-compiles a test binary, transfers it, runs it via `board_tool.py run`, parses the output, identifies a bug, fixes the source, recompiles, retransfers, and confirms the fix — all in one conversation without user intervention on the board side.

**Risks:**
- Serial port contention between bridge.py and board_tool.py. Mitigate: route all serial through bridge.py's exec interface (websocket `{"type":"exec", "cmd":"..."}` already implemented).
- Command injection via unsanitized input. Mitigate: allowlist or prefix-validation in `zynq_agent.py`.

**Why now:** Phase A proves the command surface works. Phase B proves it is useful for real development.

### Phase C: Demo Operator Integration

**Objective:** The inside-man actively supports live demo reliability — pre-flight checks, health monitoring, fallback triggers.

**Scope:** Pre-demo validation scripts, live health monitoring, presenter-facing status, graceful degradation.

**Components:**
- `preflight.py` — runs 10+ checks before demo starts (HDMI output active, LCD responding, ATOMiK adapter reachable, memory pressure OK, atomik_live process healthy)
- Health monitor integrated into `bridge.py` — periodic heartbeat via agent, status exposed on websocket for browser dashboard
- Failure recovery scripts — restart atomik_live, re-init LCD, flush framebuffer

**Deliverables:**
- `./demo_launch.sh --preflight` runs automated validation before demo
- Browser dashboard shows board health status (green/yellow/red)
- Documented fallback procedures for the 5 most likely failure modes

**Proof of completion:** Deliberately kill `atomik_live` on the board. The health monitor detects it within 10 seconds. Laptop Claude (or automated script) restarts it. HDMI output recovers without presenter intervention.

**Risks:**
- Health checks that themselves cause failures (e.g., reading MMIO that side-effects hardware state). Mitigate: read-only introspection only; never write during health checks.
- False positives that trigger recovery during a working demo. Mitigate: require 3 consecutive failures before declaring unhealthy.

**Why now:** The demo is the near-term deliverable. Making it robust is not optional.

### Phase D: Ambitious Target-Side Intelligence

**Objective:** The board-side agent becomes a persistent daemon with session state, plan execution, and autonomous diagnostic capability.

**Scope:** Long-running daemon, multi-step task execution, anomaly detection, structured logging, optional small-model reasoning.

**Components:**
- `zynq_daemon.py` — persistent process (not invoked per-command)
- Command queue and result store
- Anomaly detection rules (memory pressure, process crashes, HDMI signal loss)
- Structured log with rotation

**Deliverables:**
- Daemon that survives for hours without intervention
- Multi-step task execution (e.g., "benchmark ATOMiK, compare to last run, report delta")
- Anomaly alerts forwarded to laptop Claude via UART

**Proof of completion:** Board runs unsupervised for 4 hours. Daemon detects an injected anomaly (artificially triggered memory pressure), logs it, and reports to laptop Claude on next query.

**Risks:**
- Daemon consumes RAM that the demo needs (487MB total is not generous). Mitigate: budget 20MB max for daemon, enforce with ulimit.
- Complexity without payoff before fundraising. Mitigate: do not start Phase D until the demo is locked and reliable.

**Why now:** It isn't. This is the stretch goal. Document it so the path is clear, but don't build it until Phases A-C are proven and the demo is done.

---

## SECTION 4 — MINIMUM VIABLE ZYNQ-SIDE AGENT

### Architecture Decision: One-Shot Script (Not Daemon)

The agent is a single Python script (`zynq_agent.py`) invoked per-command via popen(). It is NOT a daemon. Reasons:

1. **No resource leak risk.** A one-shot script that exits after each command cannot leak memory, file descriptors, or accumulate zombie processes. The board has 487MB RAM and a 100MHz CPU — we cannot afford a runaway daemon.

2. **No coordination problem.** A daemon would need IPC with `atomik_live`, which already owns stdin/stdout/UART. The popen() path is proven and simple: laptop sends `~python3 /tmp/zynq_agent.py <cmd>`, `atomik_live` popen()s it, collects stdout, returns as `##RSP:` lines. No sockets, no pipes, no race conditions.

3. **No supervision needed.** If the script crashes, the next invocation starts fresh. There is nothing to restart, no state to recover, no watchdog to implement.

**Rejected alternatives:**
- *Persistent daemon with Unix socket:* Adds complexity, requires coordination with atomik_live, risks port/socket conflicts, and gains nothing at this stage. Revisit in Phase D only.
- *Agent embedded in atomik_live (C code):* Would require recompiling and retransferring the 26KB binary for every agent change. Python on-board means the agent can be updated by transferring a text file. Iteration speed matters more than runtime performance for introspection commands.
- *SSH-based agent:* No network. Dead end until Ethernet works.

### Process Model

```
Laptop Claude
    │
    ├─ board_cmd.py "python3 /tmp/zynq_agent.py health"
    │      │
    │      ├─ Opens /dev/ttyUSB2 at 921600 baud
    │      ├─ Sends: ~python3 /tmp/zynq_agent.py health\n
    │      └─ Collects ##RSP: lines until ##RSP:END
    │
    ▼
atomik_live (on board, C process)
    │
    ├─ Sees '~' prefix on stdin
    ├─ Sets cmd_running = 1  (suppresses ##EVENT output)
    ├─ popen("python3 /tmp/zynq_agent.py health", "r")
    ├─ Reads stdout line by line → printf("##RSP:%s\n", line)
    ├─ pclose() → printf("##RSP:EXIT:%d\n", exit_code)
    ├─ printf("##RSP:END\n")
    └─ Sets cmd_running = 0  (resumes ##EVENT output)
```

### Transport

UART at 921600 baud through `/dev/ttyUSB2`. This is the only path. There is no ethernet, no WiFi, no USB mass storage, no SD card accessible from the laptop. All communication goes through this single serial link.

Bandwidth: ~90 KB/s theoretical. Practical sustained throughput is lower due to UART framing and popen() overhead. Budget 50 KB/s for planning purposes.

### Command/Response Format

**Request (laptop to board):**
```
~python3 /tmp/zynq_agent.py <command> [arg1] [arg2] ...
```

**Response (board to laptop via ##RSP: lines):**
```json
{"status":"ok","command":"health","data":{"uptime_s":3847,"mem_free_kb":412000,"load_1m":0.12,"atomik_live_pid":1423,"hdmi":"active","lcd":"active","adapter":"ok"}}
```

Single-line JSON. No multi-line formatting. This keeps the ##RSP: protocol simple — one JSON blob per response, parseable by `json.loads()` on the laptop side.

**Error response:**
```json
{"status":"error","command":"health","error":"failed to open /dev/mem","code":1}
```

### Logging

Agent writes to `/tmp/zynq_agent.log` (append mode). Logs are rotated by truncation when >1MB. Laptop Claude can read the log via `board_cmd.py "tail -50 /tmp/zynq_agent.log"`.

### Timeout and Recovery

- `board_cmd.py` has a 30-second default timeout (configurable via `--timeout`).
- `zynq_agent.py` sets an alarm signal: any single command that takes >20 seconds is killed by SIGALRM.
- If the agent hangs (popen blocks in atomik_live), the laptop side sees a timeout on ##RSP:END. Recovery: send `~kill -9 $(pgrep -f zynq_agent)` followed by a 2-second wait, then retry.

### Security

This is a development board on a desk with no network connection. Security is not a primary concern. However:
- The agent does NOT accept raw shell commands. It accepts a fixed command vocabulary. Unknown commands return an error.
- File read/write commands are restricted to `/tmp/` and `/proc/` paths. No writing to `/dev/mem` except through explicit MMIO commands with address validation.
- The `atomik_live.c` popen() executor has a 510-byte command buffer limit. Commands longer than this are silently truncated.

### Startup and Deployment

The agent is deployed as a Python script transferred via base64-over-UART (the same mechanism `demo_launch.sh` uses for `atomik_live_dyn`):

```bash
# On laptop:
base64 zynq_agent.py | board_cmd.py "cat | base64 -d > /tmp/zynq_agent.py"
# Or via the existing slow_send pattern from demo_launch.sh
```

In practice, transfer via `board_cmd.py` one-liner:
```bash
python3 board_cmd.py "cat > /tmp/zynq_agent.py << 'AGENTEOF'
$(cat zynq_agent.py)
AGENTEOF"
```

Or chunk the base64 approach already proven in `demo_launch.sh` lines 106-113.

### Supervision and Failure Modes

| Failure | Detection | Recovery |
|---------|-----------|----------|
| Agent script has syntax error | ##RSP:EXIT:1, error text in response | Fix script on laptop, retransfer |
| Agent hangs (e.g., /dev/mem read blocks) | board_cmd.py timeout (30s) | Kill via `~kill`, retry |
| atomik_live crashes | No ##RSP:END, no ##EVENT | Re-launch atomik_live via UART shell |
| Serial port locked by bridge.py | board_cmd.py cannot open /dev/ttyUSB2 | Stop bridge, run command, restart bridge — OR route through bridge websocket exec |
| Agent script deleted (/tmp cleared) | "No such file" in response | Retransfer |
| Board rebooted | Serial shows login prompt instead of ##RSP | Re-run demo_launch.sh |

---

## SECTION 5 — INSIDE-MAN COMMAND SURFACE

### A. Introspection

**Must-have (Phase A):**
| Command | What it returns |
|---------|----------------|
| `health` | Uptime, free RAM, load average, atomik_live PID, disk free on /tmp |
| `status` | Current demo mode, cycle count, last event timestamp, HDMI/LCD state |
| `mmio_read <addr>` | 32-bit read from physical address via /dev/mem mmap |
| `adapter_state` | ATOMiK adapter registers: CMD, RS1, RS2, RD at 0xF0020000 |
| `proc_list` | Running processes (ps aux output, parsed to JSON) |
| `file_read <path>` | Contents of a file (capped at 4KB) |
| `dmesg_tail` | Last 20 lines of kernel log |
| `meminfo` | Parsed /proc/meminfo (total, free, available, buffers, cached) |

**Nice-to-have (Phase B):**
| Command | What it returns |
|---------|----------------|
| `fb_sample <x> <y> [w] [h]` | Pixel values from framebuffer at given coordinates (verify HDMI output) |
| `lcd_status` | LCD SPI GPIO pin states (CLK, MOSI, CS, DC, RST, LED via CSR reads) |
| `benchmark_quick` | ATOMiK round-trip timing (load+accum+read) in cycles, 10 iterations |
| `bridge_status` | Whether bridge.py websocket has connected clients |
| `net_status` | Network interfaces, IP addresses (for when Ethernet works) |
| `thermal` | CPU temperature if available via /sys/class/thermal |

**Wait-until-later (Phase D):**
| Command | What it returns |
|---------|----------------|
| `fb_screenshot` | Full framebuffer dump (8MB — too large for UART, needs compression or sampling) |
| `perf_counters` | NaxRiscv hardware performance counters (requires CSR access from userspace) |
| `trace_atomik <n>` | Trace N ATOMiK operations with cycle-accurate timing |
| `watchdog_status` | Daemon health, uptime, anomaly count |

### B. Control

**Must-have (Phase A):**
| Command | What it does |
|---------|-------------|
| `demo_mode <mode>` | Send keypress to atomik_live stdin (e.g., '1' to modify buffer 1, 'r' to reset) |
| `demo_keystroke <key>` | Send arbitrary single character to atomik_live |

**Nice-to-have (Phase B):**
| Command | What it does |
|---------|-------------|
| `inject_event <buf_id>` | Modify a specific buffer to trigger ATOMiK detection |
| `reset_demo` | Reset all buffers, clear stats |
| `run_presentation` | Trigger the scripted 47-second presentation sequence |
| `lcd_backlight <on/off>` | Toggle LCD backlight via CSR write |

**Wait-until-later (Phase D):**
| Command | What it does |
|---------|-------------|
| `restart_atomik_live` | Kill and relaunch the demo binary |
| `reload_agent` | Pull fresh zynq_agent.py from /tmp and re-exec |
| `set_uart_echo <on/off>` | Toggle local echo for debugging |

### C. Build/Run

**Must-have (Phase A):**
| Command | What it does |
|---------|-------------|
| `run <path>` | Execute a binary, return stdout/stderr and exit code |
| `run_python <script>` | Execute a Python script inline or from file |

**Nice-to-have (Phase B):**
| Command | What it does |
|---------|-------------|
| `transfer_b64 <dest>` | Receive base64-encoded file content, decode to destination path |
| `compare_sw_hw <bufsize>` | Run software memcmp vs ATOMiK change detection, return timing comparison |
| `sha256 <path>` | Hash a file on board (verify transfer integrity) |

**Wait-until-later:**
| Command | What it does |
|---------|-------------|
| `compile_python <script.py>` | Not possible without gcc. Python-only test harness on board. |
| `install_pkg <name>` | apt install — requires network, not available now |

### D. Hardware/Diagnostics

**Must-have (Phase A):**
| Command | What it does |
|---------|-------------|
| `csr_read <offset>` | Read CSR register at CSR_BASE + offset (0xF0000000 region) |
| `atomik_roundtrip` | Load value, accumulate, read back — verify adapter is alive |

**Nice-to-have (Phase B):**
| Command | What it does |
|---------|-------------|
| `csr_write <offset> <value>` | Write CSR register (careful — can break display) |
| `mmio_write <addr> <value>` | Write to physical address (requires explicit --force flag) |
| `uart_loopback` | Send test pattern on UART, verify echo (diagnostic for serial issues) |
| `irq_status` | Read /proc/interrupts, report active IRQ lines |

**Wait-until-later:**
| Command | What it does |
|---------|-------------|
| `pcie_scan` | No PCIe on this board |
| `ddr_bandwidth` | Memory bandwidth test — useful but complex, defer |

---

## SECTION 6 — LAPTOP CLAUDE INTEGRATION MODEL

### Interaction Pattern

Laptop Claude invokes board commands through Bash tool calls. The interface is a single script:

```bash
python3 board_tool.py <command> [args]
```

This wraps `board_cmd.py` and handles:
1. Routing the command through the correct serial path
2. Parsing JSON responses into structured data
3. Handling timeouts, retries, and error conditions
4. Formatting output for Claude's consumption

### Planning Model

When laptop Claude needs to accomplish something on the board, it follows this sequence:

1. **Query state:** `board_tool.py health` and `board_tool.py status` to understand current board condition.
2. **Plan locally:** Decide what to build/modify using local files and cross-compiler.
3. **Build:** `riscv64-linux-gnu-gcc -O2 -static -o /tmp/test_binary test.c`
4. **Transfer:** Push the binary to the board via base64-over-UART.
5. **Execute:** `board_tool.py run /tmp/test_binary`
6. **Observe:** Parse output, check exit code, optionally query MMIO or framebuffer state.
7. **Iterate or commit:** Fix and retry, or declare success.

### Command Dispatch

All commands go through `board_cmd.py` → UART → `atomik_live` popen() → agent script.

There is no SSH. There is no direct serial bypass. There is one path. This is a feature, not a limitation — it means there is exactly one place where serial port contention can occur, and exactly one protocol to debug.

### Local vs. Remote Reasoning

| Task | Where it runs | Why |
|------|--------------|-----|
| Code analysis, design decisions | Laptop | Full codebase, git history, reasoning ability |
| Cross-compilation | Laptop | gcc not on board |
| FPGA synthesis | Laptop | Vivado not on board |
| Binary transfer | Laptop→Board | One direction only |
| Hardware state queries | Board | Direct /dev/mem access |
| Framebuffer verification | Board | Can read pixel values |
| Process management | Board | Can see running processes |
| Test execution | Board | Binaries run on target |
| Result analysis | Laptop | Parse structured output, decide next step |

### Response Summarization

Agent responses are JSON. Laptop Claude should:
- Parse the JSON, extract relevant fields
- Summarize findings in natural language for the user
- Flag anomalies (unexpected values, errors, timeouts)
- Never dump raw JSON to the user unless they ask for it

### What Stays Manual

- **Physical board operations:** Power cycling, plugging/unplugging HDMI, pressing buttons.
- **Visual verification of HDMI quality:** Pixel sampling helps, but "does this look good?" requires human eyes for now.
- **Demo narration decisions:** What to say, when to pause, audience-facing choices.

### What's Automated

- **Pre-flight health checks** before demo.
- **Build-transfer-test cycles** during development.
- **Board health monitoring** during demo via bridge.py.
- **Regression testing** of agent commands after changes.

### What's NEVER Auto-Delegated During Live Demo

- **Writes to /dev/mem.** A bad MMIO write during a live demo can crash the board or corrupt HDMI output. All writes require explicit human approval during demo mode.
- **Process kill/restart.** Killing atomik_live during a demo is catastrophic. Never automated.
- **Firmware/bitstream changes.** Never during a demo. Period.
- **Large file transfers.** UART is shared with event stream. A multi-KB transfer during demo would stall event reporting.

---

## SECTION 7 — DEMO ROLE

### 1. Should the audience see the board-side agent?

Yes, but only for one carefully staged moment. The agent is not the demo. ATOMiK is the demo. But showing the agent reveals a deeper truth: this hardware platform is not just running a canned animation — it is a live system with introspection, command execution, and real-time monitoring by an AI. That is a differentiated capability that no other hardware demo has.

### 2. If yes, when and how long?

**When:** After the core ATOMiK value has been demonstrated (state change detection, selective sync, energy savings). Approximately 60-90 seconds into the demo, after the audience has absorbed the main message.

**How long:** 15-20 seconds. One command, one response, one reaction. Do not belabor it.

### 3. Best single moment to communicate the idea

The presenter says: "This board is running a 64-bit RISC-V Linux system with our hardware accelerator. Let me ask it how it's doing."

On the laptop, a single command runs:
```
board_tool.py health
```

The response appears (formatted for readability):
```
NaxRiscv RV64GC @ 100MHz | Uptime: 47m | RAM: 412/487 MB free
ATOMiK adapter: responsive | HDMI: 1920x1080 active | LCD: active
Demo cycles: 342 | Changes detected: 1,847 | Data avoided: 94.2%
```

The presenter says: "That's a live query from our AI development tools to the board — same UART, same hardware, while the demo keeps running. We use this to autonomously build and test on the target."

Then move on. Do not explain the architecture. Do not show code. The audience takeaway is: "Their AI can talk to their hardware in real time. That's unusual."

### 4. How the inside-man improves live demo behind the scenes

- **Pre-flight:** Before the demo starts, the inside-man validates that HDMI is outputting, LCD is responding, ATOMiK adapter is reachable, and memory pressure is acceptable. If anything fails, the presenter knows before the audience arrives.
- **Recovery:** If atomik_live crashes mid-demo (unlikely but possible), the inside-man can detect it and the presenter can trigger a restart from the laptop without touching the board.
- **Confidence:** The presenter knows the board is healthy because they checked 30 seconds ago. This eliminates the "I hope it's still working" anxiety that plagues every hardware demo.

---

## SECTION 8 — ADOPTION STORY

### How this strengthens the adoption narrative

The ATOMiK adoption story has three pillars:
1. **Hardware works** — proven on FPGA, validated from Linux userspace.
2. **Standard toolchain** — GCC compiles C code that uses ATOMiK via MMIO. No custom compiler, no special language.
3. **AI-native workflow** — The development process itself uses AI agents that interact with the hardware directly.

The two-tier architecture is the proof of pillar 3. It demonstrates that ATOMiK hardware is not just "AI-compatible" in the abstract sense of accelerating AI workloads — it is AI-native in the concrete sense that AI systems can directly instrument, test, and develop against it.

### Connection to GCC/C compatibility lane

The `atomik_example.c` workflow is already proven: write C, cross-compile with `riscv64-linux-gnu-gcc -O2`, run on board, observe ATOMiK detecting changes. The inside-man extends this from "human writes C, human runs it, human reads output" to "AI writes C, AI cross-compiles it, AI transfers it, AI runs it, AI reads output, AI iterates." Same toolchain, same hardware path, automated end-to-end.

This is the adoption story for technical audiences: "You write standard C. Our AI verifies it on real hardware. You get results in minutes, not days."

### What to show

- The `board_tool.py health` one-liner during a live demo (Section 7).
- If asked: a screen recording of laptop Claude autonomously iterating on a test binary. This shows the full loop but takes too long for live demo.

### What must be real

Everything. There are no mock responses, no pre-recorded outputs, no simulated board state. The inside-man talks to real hardware over a real serial link. If the board is off, the command fails visibly. This authenticity is the entire point.

---

## SECTION 9 — TECHNICAL RECOMMENDATION: FULL CLAUDE ON ZYNQ OR NOT?

### Decision: Agent-first. No Claude on-device. Revisit only with network.

**Hard facts:**

1. **No network.** Claude API requires internet access. The board has no working Ethernet. Until PL Ethernet is operational (not on the critical path for the demo), there is no way to run Claude API calls from the board.

2. **CPU is 100MHz RISC-V.** Even if a small local model existed for RV64GC (it does not in a practical form), inference at 100MHz with 487MB RAM would be catastrophically slow. A single response would take minutes.

3. **RAM is 487MB.** The smallest useful language models (phi-3-mini, llama-3.2-1B) need 1-4GB minimum. The board cannot host a model.

4. **The UART is the bottleneck, not the board-side intelligence.** At 90KB/s, the constraint is moving data between laptop and board. Making the board "smarter" does not help if every result has to traverse a 90KB/s pipe anyway. The right architecture is: smart laptop, structured board agent, fast protocol.

**What's worth building now:**
- `zynq_agent.py` with 15-20 structured commands (Phase A+B).
- `board_tool.py` as the laptop-side interface.
- Integration into development workflow so laptop Claude naturally uses the board as a tool.

**What's fragile:**
- Any attempt to run AI inference on the board. The hardware cannot support it.
- Any attempt to use network-dependent services before Ethernet works.
- Any daemon that accumulates state and eventually OOMs the 487MB system.

**What comes later:**
- Phase D daemon (after demo is locked).
- On-device intelligence (after Ethernet, if ever — laptop Claude may always be sufficient).

**Upgrade trigger:** If PL Ethernet is brought up and the board gets reliable internet access, reconsider a thin Claude API client on-board that can make reasoning calls to the Anthropic API. This would enable autonomous diagnostic loops without laptop involvement. But this is not on the roadmap for the fundraising demo.

---

## SECTION 10 — OPERATOR WORKFLOW

### Engineering Workflow (Founder/Developer)

The founder is using laptop Claude (Claude Code) for all development. The inside-man extends this workflow:

**Session start:**
1. Power on board, verify UART at `/dev/ttyUSB2`.
2. Run `demo_launch.sh` or `demo_launch.sh --quick` (if board is already booted).
3. Laptop Claude runs `board_tool.py health` to confirm board is alive and agent is deployed.
4. Begin development. Laptop Claude uses `board_tool.py` as needed during build-test cycles.

**Development loop:**
1. Edit source on laptop.
2. Cross-compile: `riscv64-linux-gnu-gcc -O2 -static -o /tmp/test test.c`
3. Transfer to board: `board_tool.py transfer /tmp/test /tmp/test`
4. Run on board: `board_tool.py run /tmp/test`
5. Parse results. If failure, go to step 1.
6. If success, integrate into main demo binary.

**Session end:**
1. Commit source changes on laptop.
2. Optionally: `board_tool.py health` to log final board state.
3. Leave board running or power down.

### Investor Workflow (Presenter During Live Meeting)

**Pre-flight (5 minutes before):**
1. Run `demo_launch.sh --quick` (board should already be booted).
2. Run `board_tool.py health` — verify all green.
3. Open browser replica page: `file:///tmp/atomik_replica.html`
4. Verify HDMI output on external monitor.
5. Verify LCD shows replica endpoint view.
6. Run `board_tool.py demo_keystroke r` — reset to clean state.

**During demo:**
1. Narrate while pressing keys in browser (1-8, a, r) to trigger state changes.
2. At the planned moment (~90s in), run `board_tool.py health` for the "ask the board" moment.
3. Continue narration. Do NOT run other commands during the demo.

**If something goes wrong:**
| Symptom | Action |
|---------|--------|
| HDMI goes black | Run `board_tool.py health`. If atomik_live is gone, say "Let me restart that" and run recovery script. If board is unresponsive, power cycle — takes ~3 minutes to reboot. |
| LCD goes blank | Ignore it. LCD is secondary. Continue with HDMI + browser. |
| Browser disconnects | Refresh the page. Bridge reconnects automatically. |
| Serial port error | Check cable. `/dev/ttyUSB2` may have renumbered. `ls /dev/ttyUSB*` to find it. |
| Board fully unresponsive | "Let me bring the hardware back up." Power cycle, run demo_launch.sh. Fill the 3 minutes with Q&A about the architecture. |

### Trust Boundaries

- **Laptop Claude can read anything on the board** via the agent. This is intentional.
- **Laptop Claude can execute commands on the board** via popen(). This is intentional but constrained to the agent's command vocabulary.
- **Laptop Claude cannot directly write to /dev/mem** unless explicitly invoking `mmio_write` with `--force`. The default posture is read-only.
- **The user (founder) has full trust** in laptop Claude. There is no adversarial threat model here — this is a solo founder's development environment.
- **Demo audience has zero access.** They see the HDMI screen and hear the narration. They do not touch the laptop or the board.

---

## SECTION 11 — MILESTONES

### M1: Agent Script Deployed

**Objective:** `zynq_agent.py` running on board, responding to `health` command.

**Prerequisites:** Board booted with atomik_live running. `board_cmd.py` working.

**Tasks:**
1. Write `zynq_agent.py` with `health` command (reads /proc/uptime, /proc/meminfo, checks atomik_live PID).
2. Transfer to board via base64-over-UART.
3. Invoke via `board_cmd.py "python3 /tmp/zynq_agent.py health"`.
4. Parse JSON response on laptop.

**Deliverables:** `zynq_agent.py` file in repo, transfer script, proof output.

**Proof:** JSON response with valid uptime, memory, and PID values.

**Risks:** Python3 import issues on minimal Ubuntu. **Mitigation:** Test `python3 -c "import json, os, sys"` first.

---

### M2: Core Introspection Commands (8 commands)

**Objective:** Full Phase A command vocabulary.

**Prerequisites:** M1 complete.

**Tasks:**
1. Implement: `health`, `status`, `mmio_read`, `adapter_state`, `proc_list`, `file_read`, `dmesg_tail`, `meminfo`.
2. Test each command end-to-end.
3. Handle errors gracefully (bad address, missing file, permission denied).

**Deliverables:** Updated `zynq_agent.py` with 8 commands, test results.

**Proof:** All 8 commands return valid JSON. `adapter_state` shows ATOMiK registers. `mmio_read 0xF0020000` matches `adapter_state` CMD register.

**Risks:** /dev/mem permission on Ubuntu. **Mitigation:** atomik_live runs as root (required for MMIO), so popen() children inherit root.

---

### M3: Laptop-Side Tool Wrapper

**Objective:** `board_tool.py` as single-entry-point for all board operations.

**Prerequisites:** M2 complete.

**Tasks:**
1. Write `board_tool.py` that dispatches to `board_cmd.py` with proper argument formatting.
2. JSON response parsing and pretty-printing.
3. Error handling: timeout, parse failure, board not responding.
4. Subcommands mirror agent commands: `board_tool.py health`, `board_tool.py mmio_read 0xF0020000`, etc.

**Deliverables:** `board_tool.py` in repo.

**Proof:** `board_tool.py health` returns human-readable output with all fields. `board_tool.py --json health` returns raw JSON.

**Risks:** Serial port contention with bridge.py. **Mitigation:** Document that bridge must be stopped for direct board_tool.py use, OR route through bridge websocket.

---

### M4: Binary Transfer Pipeline

**Objective:** Laptop Claude can cross-compile and deploy a binary to the board in one automated sequence.

**Prerequisites:** M3 complete.

**Tasks:**
1. Add `transfer` command to `board_tool.py` that base64-encodes a local file and sends it to the board in chunks.
2. Add `sha256` command to agent for integrity verification.
3. Add `run` command to agent that executes a binary and returns stdout/stderr/exit code.
4. Test full pipeline: compile → transfer → verify → run → collect output.

**Deliverables:** Transfer pipeline in `board_tool.py`, `sha256` and `run` commands in agent.

**Proof:** Cross-compile a trivial C program, transfer it, run it, get "Hello from board" back, SHA256 matches on both sides.

**Risks:** Large binary transfer at 90KB/s. A 200KB static binary takes ~4 seconds to transfer as base64 (~267KB encoded) — acceptable. A 2MB binary would take ~40 seconds — may need chunked transfer with progress reporting.

**Mitigation:** Use dynamic linking (`atomik_live_dyn` is 26KB vs static at ~700KB). Transfer only what's needed.

---

### M5: ATOMiK Hardware Verification Commands

**Objective:** Laptop Claude can verify ATOMiK adapter state from the board.

**Prerequisites:** M2 complete.

**Tasks:**
1. Implement `atomik_roundtrip`: load a value, accumulate a delta, read back, verify result.
2. Implement `adapter_state`: read all 4 ATOMiK adapter registers.
3. Implement `benchmark_quick`: 10 iterations of load+accum+read with cycle timing.

**Deliverables:** Three hardware verification commands in agent.

**Proof:** `atomik_roundtrip` returns correct XOR result. `benchmark_quick` returns cycle counts consistent with known performance (load~64cy, accum~70cy, read~99cy from Tang Nano 9K baseline — Zynq numbers will differ).

**Risks:** ATOMiK adapter MMIO access from Python via /dev/mem may have alignment or caching issues. **Mitigation:** Use the same mmap pattern proven in the C demos.

---

### M6: Pre-Flight Check System

**Objective:** Automated demo validation before a presentation.

**Prerequisites:** M3 and M5 complete.

**Tasks:**
1. Write `preflight.py` on laptop side that runs a sequence of board_tool.py commands.
2. Checks: atomik_live running, HDMI active (fb_sample returns non-zero pixels), LCD active (CSR pin states), ATOMiK adapter responsive (roundtrip passes), memory pressure acceptable (>100MB free), /tmp has >50MB free.
3. Green/red pass/fail output.

**Deliverables:** `preflight.py` script, integrated into `demo_launch.sh --preflight`.

**Proof:** Run preflight with healthy board → all green. Kill atomik_live → preflight detects it and reports red.

**Risks:** False negatives (preflight says OK but demo fails). **Mitigation:** Preflight checks the specific things that actually cause demo failures, not generic health metrics.

---

### M7: Bridge Integration

**Objective:** `bridge.py` routes agent commands alongside event streams, eliminating serial port contention.

**Prerequisites:** M3 complete.

**Tasks:**
1. Extend `bridge.py` websocket protocol to accept `{"type":"agent","cmd":"health"}` messages.
2. Route these through the existing `~` prefix command path on UART.
3. Return structured JSON responses to websocket clients.
4. Update `board_tool.py` to optionally route through bridge websocket instead of direct serial.

**Deliverables:** Updated `bridge.py`, `board_tool.py --via-bridge` option.

**Proof:** `board_tool.py --via-bridge health` returns same results as `board_tool.py health`, while bridge.py is running and serving events to the browser.

**Risks:** Websocket adds latency. **Mitigation:** Acceptable for introspection commands (100ms extra is fine). Not suitable for tight timing loops.

---

### M8: Autonomous Development Loop

**Objective:** Laptop Claude completes a full build-transfer-test-iterate cycle on the board without user intervention.

**Prerequisites:** M4 and M5 complete.

**Tasks:**
1. Laptop Claude writes a test program that exercises ATOMiK hardware.
2. Cross-compiles it.
3. Transfers it to board via `board_tool.py transfer`.
4. Runs it via `board_tool.py run`.
5. Parses output, identifies a deliberate bug.
6. Fixes the source, recompiles, retransfers, re-runs.
7. Confirms fix.

**Deliverables:** Session transcript showing the full loop.

**Proof:** The entire sequence completes with correct final output, and no step required the user to type anything on the board or report what the screen shows.

**Risks:** Any step failing silently breaks the loop. **Mitigation:** Every step returns explicit success/failure. Laptop Claude checks each step before proceeding.

---

## SECTION 12 — CRITICAL PATH

### What First

1. **M1 (Agent deployed)** — unlocks everything else. Without a working agent on the board, nothing in this document is real.
2. **M2 (Core commands)** — provides the vocabulary laptop Claude needs.
3. **M3 (Laptop wrapper)** — makes the vocabulary ergonomic.
4. **M4 (Transfer pipeline)** — enables the autonomous development loop.

This sequence (M1 → M2 → M3 → M4) is the critical path. Each depends on the previous. Total estimated effort: 2-3 focused sessions.

### What Waits

- **M5 (ATOMiK verification)** — important but not blocking. Can be done in parallel with M3/M4.
- **M6 (Pre-flight)** — depends on M5 but is demo-time polish, not development infrastructure.
- **M7 (Bridge integration)** — quality-of-life improvement, not critical for basic operation.

### What's Optional Polish

- Framebuffer pixel sampling (`fb_sample`) — useful for verifying HDMI output autonomously, but the user can still just look at the screen.
- LCD status checking — LCD is secondary to HDMI.
- Browser dashboard health indicators — nice for demo but not required for development.

### What's Scope Creep

- **Running a language model on the board.** Stop. 100MHz, 487MB, no network. This is not the right machine for inference.
- **Building a general-purpose remote execution framework.** The agent is for ATOMiK development. It does not need to be a generic DevOps tool.
- **Multi-board coordination.** There is one board. One serial port. One agent. Do not design for N boards.
- **Web-based agent UI.** The user interacts through laptop Claude. Adding a separate web UI for the agent is redundant complexity.

### Most Likely Failure Mode That Wastes Days

**Serial port contention between bridge.py and board_tool.py.** Both need `/dev/ttyUSB2`. If they fight over it, neither works reliably. This has already been solved in principle (bridge.py has websocket exec support), but the integration needs to be clean and tested. If you ignore this and hack around it with "stop bridge, run command, restart bridge," you will waste a day on intermittent serial errors before giving up and doing the integration properly.

**Solve M7 (bridge integration) earlier than the milestone order suggests** if you find yourself fighting serial contention during M2-M4 development.

### Most Worthwhile Shortcut

**Skip the formal `board_tool.py` wrapper (M3) initially** and have laptop Claude call `board_cmd.py "python3 /tmp/zynq_agent.py <cmd>"` directly. The wrapper is nice, but the raw command works fine and saves writing another layer. Build the wrapper only when the raw command becomes annoying (probably after 10+ uses in a session).

---

## SECTION 13 — IMPLEMENTATION CHOICES

### 1. Transport: Serial via popen() (through atomik_live)

**Decision:** All commands go through UART at 921600 baud, via the `~` prefix in `atomik_live.c`'s popen() executor.

**Rejected:**
- *SSH:* No network. Dead.
- *TCP/websocket direct to board:* No network. Dead.
- *Serial proxy daemon on board:* Adds a process, requires IPC with atomik_live, gains nothing over popen().
- *Separate UART for agent:* Only one USB-UART adapter. Would require hardware changes.

**Risk:** popen() is synchronous and blocks atomik_live's main loop during command execution. Commands must complete quickly (<5s for routine, <20s max).

### 2. Agent Model: One-Shot Script

**Decision:** `zynq_agent.py` is invoked per-command, exits after each command. No daemon. No persistent state between invocations.

**Rejected:**
- *Persistent daemon:* Unnecessary complexity at this stage. Risks OOM, requires supervision, complicates the serial protocol. Revisit in Phase D.
- *Agent embedded in atomik_live C code:* Cannot iterate on agent logic without recompiling and retransferring the C binary. Python is editable on the wire.
- *Shell script agent:* Python is available and better for JSON formatting, /dev/mem access, and structured output.

### 3. Output Format: Single-Line JSON

**Decision:** Every agent command returns a single line of JSON to stdout. The line is captured by atomik_live's popen() reader and prefixed with `##RSP:`.

**Rejected:**
- *Multi-line text:* Harder to parse on the laptop side. Fragile with varying output formats.
- *Binary/protobuf:* Overkill for 50KB/s UART. JSON is human-readable for debugging.
- *Mixed text+JSON:* Ambiguous. Is a line text or JSON? Pick one. JSON.

**Exception:** The `run` command returns the raw stdout/stderr of the executed binary, which may be multi-line text. This is wrapped in a JSON envelope: `{"status":"ok","command":"run","stdout":"line1\nline2\n","stderr":"","exit_code":0}`. Newlines in stdout are escaped in JSON.

### 4. Claude Integration: Direct Bash Tool Calls

**Decision:** Laptop Claude invokes `board_cmd.py` (or `board_tool.py`) via the Bash tool in Claude Code. No MCP server. No custom tool definitions. No SDK integration.

**Rejected:**
- *MCP server:* Would formalize the interface but adds a layer. The Bash tool already works. When the command set stabilizes and this is used daily, an MCP server would be a clean upgrade path.
- *Custom Claude Code tool plugin:* Does not exist yet as a first-class concept. Bash tool is the interface.
- *Standalone CLI with Claude API calls:* The user is already in Claude Code. Adding a separate Claude instance for board operations fragments the conversation context.

**Upgrade path:** If board operations become a major fraction of Claude Code usage (>20% of tool calls in typical sessions), build an MCP server that exposes board commands as structured tools. This gives Claude native tool-calling semantics (parameters, return types, error handling) instead of parsing stdout text.

---

## SECTION 14 — MVP VS FLAGSHIP

### MVP: Structured Board Introspection

**What's included:**
- `zynq_agent.py` with 8 commands (health, status, mmio_read, adapter_state, proc_list, file_read, dmesg_tail, meminfo)
- `board_cmd.py` as transport (already done)
- Transfer mechanism for agent updates (base64-over-UART, already proven)
- Direct Bash invocation from laptop Claude

**What's excluded:**
- Laptop-side wrapper script
- Bridge integration
- Pre-flight checks
- Binary transfer pipeline
- ATOMiK hardware verification commands
- Any daemon or persistent state

**Effort:** 1 focused session (2-4 hours). The popen() path is proven. The agent is a Python script with `if cmd == "health": ...` branches. The hard part is already done.

**Value:** Laptop Claude gains eyes on the board. Every development session becomes faster because "what's happening on the board?" has an answer that doesn't require asking the user.

### Flagship: AI-Native Development Platform

**What's included:**
- Everything in MVP
- Full 20+ command vocabulary (introspection, control, build/run, hardware diagnostics)
- `board_tool.py` with JSON parsing, error handling, retry logic
- Binary transfer pipeline (compile → transfer → verify → run)
- ATOMiK hardware verification commands
- Pre-flight check system
- Bridge integration (eliminates serial contention)
- Autonomous development loop capability
- Demo health monitoring
- Framebuffer pixel sampling for visual verification

**What's excluded:**
- On-device language model
- Persistent daemon
- Multi-board support
- Web UI for agent
- Network-dependent features

**Effort:** 4-6 focused sessions over 1-2 weeks.

**Value:** Laptop Claude can autonomously develop, test, and iterate on ATOMiK code running on real hardware. The demo gains reliability through automated pre-flight and health monitoring. The adoption story gains a concrete AI-native workflow to demonstrate.

### Which to Target First

**Build the MVP immediately.** It costs one session and provides ongoing value. Then grow toward the flagship incrementally, driven by real needs encountered during demo development. Do not spec out the flagship and build it all at once — let usage drive which commands to add next.

The MVP-to-flagship path is not a rewrite. It is additive. Every command added to `zynq_agent.py` extends the MVP toward the flagship without changing anything that already works.

---

## SECTION 15 — FINAL RECOMMENDATION

### Build First
`zynq_agent.py` with the 8 must-have introspection commands (M1 + M2). Deploy it to the board this session. Start using it immediately for development. This is a 2-4 hour investment that pays back on every subsequent session.

### Delay
- `board_tool.py` wrapper — use raw `board_cmd.py` commands until the pattern stabilizes.
- Bridge integration — solve on first encounter with serial contention, not preemptively.
- Pre-flight system — build when the demo is mature enough to have defined failure modes.
- Phase D daemon — not before the fundraising demo is locked.

### Cut
- On-device Claude / language model. The hardware cannot support it and the UART bottleneck makes it pointless. This is not a compromise — the two-tier architecture with a smart laptop and a structured board agent is the correct architecture for this hardware.
- Multi-board, web UI, generic DevOps framework. These solve problems that do not exist.

### The Real Win
The inside-man turns laptop Claude from an advisor who says "try running this on the board" into an operator who runs it, reads the result, and acts on it. The user stops being a relay between two systems and starts being a director who says "make the demo better" and watches it happen.

### Alignment
The two-tier architecture exists to make the demo extraordinary, not to be the demo itself — it is invisible scaffolding that produces visible results.
