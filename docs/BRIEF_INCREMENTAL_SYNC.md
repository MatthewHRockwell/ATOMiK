# ATOMiK Incremental Sync

## Problem

Synchronizing state between systems typically copies everything: all N regions, every cycle. Most regions don't change most of the time. The wasted bandwidth is **O(N x region_size)** per cycle regardless of actual change rate.

## Solution

`atomik-sync` uses ATOMiK fingerprint comparison to identify which regions changed, then syncs only those. Unchanged regions are skipped entirely.

## Demo Result (Mock Mode)

16 regions x 4KB = 64 KB tracked state, 20% mutation per cycle:

| Metric | Full Sync | ATOMiK Sync |
|--------|----------|-------------|
| Bytes/cycle | 65,536 | 16,384 |
| Bandwidth saved | — | **75%** |

ATOMiK correctly identified the 4 changed regions each cycle and skipped the other 12. Over 8 cycles: 524 KB full vs 131 KB selective.

## How It Works

Each cycle:
1. Recompute XOR fingerprint per region (O(region_size))
2. Compare against stored reference via ATOMiK `acc_zero` (O(1))
3. If changed: transfer region + update reference
4. If unchanged: skip entirely

Transfer savings scale with the inverse of the change rate. At 10% change rate, 90% of bytes are skipped.

## Use Cases

- **Edge-to-cloud sync**: only upload changed sensor buffers
- **Distributed state replication**: selective page transfer
- **Backup/checkpoint**: skip unchanged memory regions
