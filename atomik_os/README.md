# ATOMiK OS

The first-class desktop operating system for the ATOMiK delta-state architecture.

Targeted at the AX7020 Zynq RV64 reference hardware in the short term, with a
clear path to the planned ATOMiK laptop build.

## Goals

- Aesthetically competitive with macOS / Windows.
- Apps installable on top of the OS, accelerated by ATOMiK delta-state hardware
  primitives.
- Standalone end-user demo: HDMI display + (eventually) USB HID input, no
  laptop tether required.
- Short-term: bridge.py over UART carries keyboard input from a host laptop
  while USB host on the AX7020 stack stays paused.

## Build

```
make -C atomik_os               # cross-compile to riscv64
```

## Source layout

| Path | Purpose |
|------|---------|
| `src/main.c`       | Entry point, event loop, mode switcher |
| `src/fb.c`         | `/dev/fb0` mmap + double-buffered software compositor |
| `src/draw.c`       | Primitives: rect, gradient, rounded rect, alpha blend |
| `src/font.c`       | Anti-aliased text rendering (8×16 + scaled bitmap) |
| `src/wm.c`         | Window manager: stacking, focus, drag, resize |
| `src/dock.c`       | Bottom dock with app icons + active indicator |
| `src/wallpaper.c`  | Procedural wallpaper (gradient + noise + brand mark) |
| `src/input.c`      | Stdin (UART) → key/mouse events |
| `include/`         | Public headers shared between modules |

## Status

v0 in progress — bring up empty desktop with wallpaper + dock first, then
add windows, then apps.
