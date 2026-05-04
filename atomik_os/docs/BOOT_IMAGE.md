# ATOMiK Laptop Boot Image — Spec

> Spec for the bootable image that ships on the planned ATOMiK
> laptop. Closes task #47 as a build spec; the actual artifact comes
> when the hardware does. This document is what we hand to whoever
> ends up doing the integration.

---

## Goals

The ATOMiK laptop should boot **directly into the Document app** with
zero shell exposure, zero login screen, zero "see Linux underneath"
cracks. The user picks up the device, opens it, the lock screen is
the Document agent prompt; from there everything is field-deltas on
the invariant frame.

Hard requirements:

- Cold boot to interactive desktop in under **6 seconds**.
- No visible Linux boot text. Splash → desktop in one transition.
- Built-in Wi-Fi / cellular for the laptop-side helpers (atomik_ai,
  atomik_speech, atomik_calendar).
- Zero shell access for the end user. Power users get a hidden
  diagnostic mode via key-chord at boot.
- Single-binary OS: `/usr/bin/atomik_os` is PID 1 after the kernel
  hands off. The kernel is just a driver host.

---

## Layered diagram

```
+----------------------------------------------+
| /usr/bin/atomik_os                           | <- the OS (this repo)
| /usr/bin/atomik_bridge                       | <- networking helpers
| /usr/bin/llm_local                           | <- v0.18 intent classifier
| /var/lib/atomik_os/                          | <- persistent state
+----------------------------------------------+
| systemd: 1 unit, atomik_os.service           |
+----------------------------------------------+
| Linux 6.x + busybox (no full GNU coreutils)  |
+----------------------------------------------+
| kernel modules: video, wifi, audio, USB-HID, |
| input, display backlight, power management   |
+----------------------------------------------+
| firmware: ATOMiK FPGA bitstream (if ATOMiK   |
| substrate is on a daughter board) OR ASIC    |
| init blob (when we ship silicon)             |
+----------------------------------------------+
```

---

## Boot sequence

| Phase | Time | What happens |
|-------|------|--------------|
| 0 — bootrom | 0.0 s | ROM, signs/verifies the kernel + initrd |
| 1 — kernel + early FB | 0.5 s | `quiet` cmdline, fbcon disabled, early simplefb at the OS theme accent color (no tty text) |
| 2 — splash | 1.0 s | atomik_os splash screen — wallpaper.c rendered directly |
| 3 — drivers | 2.0 s | Wi-Fi, audio, input, USB-HID up |
| 4 — atomik_os ready | 4.0 s | Document opens with last session's saved state |
| 5 — bridge ready | 5.5 s | atomik_bridge connects to default LLM provider |

Total: 5.5 s cold boot to interactive Document. 6.0 s budget.

---

## File system layout

```
/usr/bin/atomik_os               # the OS (this repo's build/atomik_os)
/usr/bin/atomik_bridge           # network bridge (atomik_ai +
                                 #   atomik_speech + atomik_calendar
                                 #   merged into one daemon)
/usr/bin/llm_local               # v0.18 intent classifier
/usr/lib/atomik/                 # shared assets (font, wallpaper)
/etc/atomik/profile.json         # user profile (name, default LLM
                                 #   provider, accent, default cap)
/var/lib/atomik/
    documents/                   # all per-Document deltas
    audit.log                    # llm_audit_total spend log
    wallet.state                 # wallet binary state
    apps/                        # /store list reads from here
/boot/atomik_os.signed           # signed kernel+initrd image
```

---

## Persistent state migration from /tmp

Today on the AX7020 prototype, all state lives in `/tmp` (tmpfs,
wiped on reboot). The laptop image moves it to `/var/lib/atomik/`:

| /tmp path                              | /var path                              |
|----------------------------------------|----------------------------------------|
| /tmp/atomik_os_document_<id>.deltas    | /var/lib/atomik/documents/<id>.deltas  |
| /tmp/atomik_os_agent.state             | /var/lib/atomik/agent.state            |
| /tmp/atomik_os_wallet.state            | /var/lib/atomik/wallet.state           |
| /tmp/atomik_os_llm_audit.log           | /var/lib/atomik/audit.log              |
| /tmp/atomik_os_notes.txt               | /var/lib/atomik/notes.txt              |
| /tmp/atomik_apps/<name>.deltas         | /var/lib/atomik/apps/<name>.deltas     |

A `#define` in each persistence-writing file picks the path at compile
time based on a `-DATOMIK_LAPTOP_BUILD=1` flag.

---

## Single-process supervision

`atomik_os.service` is the only systemd unit. If atomik_os crashes
the service restarts it, and the user sees a brief "recovering" toast
instead of a console.

`atomik_bridge` is launched as a child of atomik_os via a regular
`fork()` so it shares the wallet + audit log mutations through file
locking and dies cleanly on parent shutdown.

---

## Hidden diagnostic shell

Hold `Ctrl + Shift + Tab` for 2 seconds at the lock screen → drops
into a busybox shell over the diagnostic UART (or the on-screen
Terminal app). For developers and field service only.

---

## Update channel

Every commit to `main` produces a signed `atomik_os.bin`. Devices
periodically check `https://updates.atomik.dev/<channel>/manifest.json`
and apply A/B updates. Same delta-log philosophy applies — only the
changed binary blocks ship over the wire.

---

## Outstanding unknowns

These can't be specified until the laptop SoC is chosen:

- Exact kernel config (DTS, drivers, audio stack)
- GPU vs framebuffer-only (atomik_os only needs `/dev/fb0`; if there
  is a GPU we use it for off-screen compositing in v1.x)
- Bootloader (u-boot vs systemd-boot vs custom)
- Power management (suspend/resume across atomik_os state files)
- Cellular vs Wi-Fi-only

These are integration concerns the laptop-build team owns; the OS
spec above doesn't constrain them.

---

## What ships from this repo

- `atomik_os` binary (already builds)
- `atomik_bridge` daemon = new merged form of `atomik_ai.py` +
  `atomik_speech.py` + `atomik_calendar.py` ported to a single
  daemon process. **TODO** — a v0.18.x task once the laptop SoC is
  picked.
- `llm_local` binary = v0.18 intent classifier compiled from
  `tools/atomik_local_intent.py` semantics. **TODO**.
- A systemd unit file. **TODO** — write once the kernel target is
  known.
- Build scripts (probably yocto recipes or Buildroot configs) that
  produce a signed `atomik_os.bin`. **TODO** when the SoC is picked.

The OS architecture is feature-complete; all of these "TODO"s are
integration plumbing, not new product features.
