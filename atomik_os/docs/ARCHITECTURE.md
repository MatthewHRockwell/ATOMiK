# ATOMiK OS — Architecture

This document is the canonical technical description of the OS as it
exists in the source tree today (`atomik_os/`). The roadmap lives in
`TODO.md`; the business model lives in `BUSINESS_MODEL.md`; this file
is **what the code does**.

---

## 1. The reduction

Every running app's visible state is

```
visible_state  =  invariant_frame  ⊕  Σ field_deltas
```

which is exactly the ATOMiK delta-state algebra implemented in hardware:

```
current_state  =  initial_state    ⊕  accumulator
```

The OS itself is the canonical ATOMiK workload. Not a benchmark, not a
demo — the actual product runs on the algebra it was built for.

---

## 2. Layers

```
+-----------------------------------------------------+
|  Document app   (v0.10 — chat-driven UI morphing)   |
|  Edge apps      (v0.9  — manifest-only renderers)   |
|  System apps    (About, Monitor, Terminal, Files,   |
|                  Notes — reference primitives)      |
+-----------------------------------------------------+
|  Window manager + agent layer + dock + status bar   |
+-----------------------------------------------------+
|  Invariant-frame primitives                         |
|    LIST | CARD | GRID | FEED | CONVO                |
+-----------------------------------------------------+
|  Compositor: fb_back() + draw_*  + cached wallpaper |
+-----------------------------------------------------+
|  /dev/fb0 mmap    +    /dev/mem ATOMiK adapter      |
+-----------------------------------------------------+
|  Linux 6.9 / NaxRiscv RV64GC / ATOMiK delta-state   |
+-----------------------------------------------------+
```

Each line is a real module in `atomik_os/src/`.

---

## 3. Source layout

| File | Role |
|------|------|
| `include/atomik_os.h` | All public types + APIs |
| `src/main.c`          | Event loop, app dispatch, hybrid frame/event poller |
| `src/fb.c`            | `/dev/fb0` mmap, double buffer, VTG/DMA enable |
| `src/draw.c`          | rect / gradient / rounded rect (corner-AA) / pixel blend |
| `src/font.c`          | 8×16 VGA font, scaled |
| `src/wallpaper.c`     | Gradient + accent vignette + ATOMiK wordmark, cached |
| `src/wm.c`            | Window stacking, focus, draw, fade-in tween |
| `src/dock.c`          | Adaptive dock, score-driven sort, predicted-icon pulse |
| `src/agent.c`         | Frequency + recency + Markov transitions, persisted |
| `src/anim.c`          | `anim_now_ms`, ease-out, frame liveness |
| `src/notify.c`        | Toast notifications (slide-in, fade-out) |
| `src/status.c`        | Top status bar — CPU + uptime + agent prediction |
| `src/about.c`         | About app — system info / key reference |
| `src/monitor.c`       | Live ATOMiK slot reader |
| `src/terminal.c`      | `forkpty()` + `/bin/sh` |
| `src/files.c`         | Read-only directory navigator |
| `src/notes.c`         | Persistent text editor |
| `src/atomik.c`        | `/dev/mem` map of the ATOMiK adapter |
| `src/eapp.c`          | `edge_app_t` accumulator + delta-apply primitives |
| `src/eapp_render.c`   | One renderer for ALL edge apps (5 primitives) |
| `src/edge_demo.c`     | Calendar / Tasks / Code / Brief / Chat reference apps |
| `src/document.c`      | The Document app — chat panel + grammar parser |
| `deploy.py`           | base64-over-UART transfer + nohup launch |
| `Makefile`            | Cross-compile to `riscv64-linux-gnu-` |

---

## 4. The invariant frame

Defined in `atomik_os.h` as five primitives, each rendered by
`eapp_render.c`:

| Primitive | Field convention | Use cases |
|-----------|------------------|-----------|
| `PRIM_LIST`  | `0`=header, `1`=list, `2`=footer | Tasks, Inbox |
| `PRIM_CARD`  | `0`=title, `1`=subtitle, `2`=body | Brief, Notification details |
| `PRIM_GRID`  | `0`=title, `1`=cells | Calendar, Album view |
| `PRIM_FEED`  | `0`=title, `1`=feed-items | PR queue, Activity |
| `PRIM_CONVO` | `0`=title, `1`=alternating turns | Chat, Comments |

Apps don't write rendering code. They populate `edge_app_t.fields[]`
and pick a primitive. This is the entire architectural reduction.

---

## 5. The Document app

`document.c` implements the universal Document — the shape-shifting app
that replaces a fixed app list. It owns:

- A single `edge_app_t s_doc` mutated by chat commands
- A chat history ring (`s_history[]`) with separate user/agent lines
- A typed input buffer (`s_input[]`)

When the user presses Enter, `apply_command()` parses the line via
`pop_token()` + `ieq()` matching and routes to one of:

- `set primitive <list|card|grid|feed|convo>` → mutates
  `s_doc.primitive`
- `set accent <name|#hex>` → mutates `s_doc.accent`
- `set header|subtitle|body|footer "..."` → field 0 / 2 string
- `clear list` → wipes field 1 array
- `add "<item>"` → appends to field 1 array
- `load <calendar|tasks|code|brief|chat>` → preset jump
- `save` / `reset` / `help`

After every command, `doc_save_state()` writes the entire
`edge_app_t` to `/tmp/atomik_os_document.state`. On next launch
`doc_load_state()` restores it. Documents accumulate across runs.

In v0.12 the parser is replaced by an LLM call. The contract is
"chat in → field-delta operations on `edge_app_t` out", which doesn't
change.

---

## 6. The agent

`agent.c` tracks user behavior as typed `action_t` values. Three signals:

- **Frequency** (`s_count[]`) — lifetime count per action
- **Recency**   (`s_last[]`)  — logical timestamp of last occurrence
- **Markov transitions** (`s_trans[a][b]`) — count of (prev, curr) pairs

`agent_predict()` argmaxes `s_trans[s_prev][k]` if any transition has
been observed; otherwise falls back to `freq × recency`. Persisted to
`/tmp/atomik_os_agent.state` every 8 events.

The dock reads `agent_score()` per icon and sorts left-to-right
descending. The status bar surfaces `agent_predict()` as
`"next likely: <action>"` text. The predicted icon gets a pulsing
cyan dot.

---

## 7. Frame loop

`main.c` runs a hybrid event/frame loop:

- Idle: 100 ms `poll()` on stdin (UART). Saves CPU.
- Active animation: 16 ms (~60 Hz) when `anim_active()` is true.
- Terminal-focused: 16 ms regardless, so async pty output flushes.
- Document-focused: same as terminal — chat input + state changes
  drive the redraw.

`anim_active()` decays naturally 3 frames after the last `anim_tick()`
so one-shot animations wind down without explicit unregistration.

---

## 8. Persistence map

| Data | Path | Wire | Auto-save |
|------|------|------|-----------|
| Agent model | `/tmp/atomik_os_agent.state`    | binary, magic `0xA01D5742` | every 8 events |
| Document state | `/tmp/atomik_os_document.state` | binary, magic `0xA01D0CE7` | every command |
| Notes buffer | `/tmp/atomik_os_notes.txt` | plain text | every 64 keystrokes |

Today these live in `/tmp` (tmpfs — wiped on reboot). v1.0 moves them
to a writable `/var/lib/atomik_os/` mount on real storage.

---

## 9. Schema interop (partnership strategy)

The `edge_app_t` accumulator is the *internal* representation. The public
manifest format is intentionally negotiable. ATOMiK OS is positioned as
the canonical edge renderer for generative-UI output from existing
players — not as a competing schema. Concrete adapter targets:

- **AdaptiveCards (Microsoft)** — most mature JSON UI schema. Our 5
  primitives map onto a subset cleanly.
- **Thesys / GenUI** — most LLM-native gen-UI service.
- **Vercel v0** — text prompt → React tree (subset reduces to deltas).
- **Anthropic Artifacts** — structured UI hints from Claude.
- **OpenAI structured outputs / function-calling UIs** — same shape.

Each adapter is a small file (`adapters/from_<schema>.c`) that walks the
foreign schema and emits `delta_emit_*()` calls. Internal wire format
stays ATOMiK-native; ingress side speaks whatever the partner uses.

The pitch is uniform: *"Your model produces UI intent. We render that
intent on a $200 edge device for a memcpy's worth of compute and a
few-hundred-bytes-per-update of bandwidth."*

See `~/.claude/.../memory/project_atomik_os_partnerships.md` for the
target list and outreach plan.

## 10. Build + deploy

```sh
# from atomik_os/
make                                  # cross-compiles to build/atomik_os
python3 deploy.py [--no-launch]       # base64-over-UART -> /tmp/atomik_os
```

`deploy.py` flow:
1. Read local binary, gzip, base64
2. Stream chunks of 1024 chars to the board with 0.8 ms per-char throttle
3. `base64 -d | gunzip > /tmp/atomik_os; chmod +x`
4. Detach `vtcon1` from `/dev/fb0` so atomik_os owns the framebuffer
5. `nohup /tmp/atomik_os > /tmp/aos.out 2> /tmp/aos.err < /dev/null &`
6. `pgrep atomik_os && echo OS_RUNNING`

A 50-ish KB binary takes ~9 minutes over the 115200-baud LiteX UART.
Real hardware (laptop build) gets a proper distribution path.
