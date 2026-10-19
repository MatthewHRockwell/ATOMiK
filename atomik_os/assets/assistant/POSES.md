# Atom Pose Frames

Atom (the assistant) is a multi-pose character, not a static sticker. The pose
engine in `src/assistant.c` loads a set of pose assets and picks one per frame:

```
summon  ─► WAVE (≈0.9 s greeting, if present) ─► settle
mood    ─► SUCCESS→HAPPY   WARNING→ALERT   THINKING→THINK   else→IDLE
blink   ─► eyes shut ≈150 ms every ≈3.6 s (procedural; no art needed)
float   ─► mood-aware hover (amplitude/tempo per mood)
```

Any pose whose art is missing **falls back to IDLE** — so the system always
works, and new poses light up the moment their asset is present.

## Pose files

`/tmp/atomik_assets/assistant_<name>_160.atomik_asset`

| name  | when shown                          | status            |
|-------|-------------------------------------|-------------------|
| idle  | default calm float                  | ✅ required, shipped |
| think | THINKING mood                       | ✅ shipped (head-tilt) |
| happy | SUCCESS mood                        | ⬜ add art        |
| alert | WARNING mood                        | ⬜ add art        |
| wave  | ~0.9 s greeting right after summon  | ⬜ add art        |
| blink | (procedural — no art)               | n/a               |

## Adding a real pose

1. Generate Atom in the pose with your image tool — **on-model** (same
   silicon-octopus), plain/contrasting background, character roughly centred.
2. Run the cutout + pack pipeline:
   ```
   python3 tools/prep_assistant_poses.py --pose wave --source ~/atom_wave.png
   ```
   (needs `pip install rembg onnxruntime`; U2-Net matte → #FE00FE key → 160 px → asset)
3. Commit `assets/assistant/assistant_wave_160.atomik_asset` and ship it to the
   board's `/tmp/atomik_assets/`. It appears automatically — no code change.

## Notes

- `idle` and `think` were produced from the canonical `atomik_assistant_final.png`
  (think = a 7° head-tilt of idle).
- The engine blits whichever pose is active at `(av_x, av_y)`; the mood-aware
  float and procedural blink apply on top.
- Multi-frame gestures (e.g. a wave that cycles up/down) can be added later by
  giving a pose several numbered frames and advancing on the anim clock — the
  selection hook (`active_pose`) is where that logic goes.
