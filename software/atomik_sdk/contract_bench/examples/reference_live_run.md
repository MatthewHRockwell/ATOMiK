# ATOMiK Live Run — Distinct-count cardinality rollup

**recorded reference** · 2026-06-21 · ALINX/HamGeek AX7020 (XC7Z020-2CLG484)

`abench 65537 0xDEADBEEF0BADF00D` — 65537 accumulations per bank configuration, self-verifying.

## Measured ladder
| banks | hw cycles | speedup | throughput |
|------:|----------:|--------:|-----------:|
| 1 | 65537 | 1.00x | 100 M/s |
| 2 | 32769 | 1.99x | 200 M/s |
| 4 | 16385 | 3.99x | 400 M/s |
| 8 | 8193 | 7.99x | 800 M/s |

## Order-independence (the proof)
- Merged result: **0x17e9fe29b5dc0aad**
- Across 1 / 2 / 4 / 8 banks: **ALL MATCH** — byte-identical regardless of how the deltas were partitioned or ordered.
- Peak: **800 M/s** (100 M/s per bank).

## Reproduce
`abench 65537 0xDEADBEEF0BADF00D`

## Raw transcript (verbatim)
```
$ abench 65537 0xDEADBEEF0BADF00D
banks  hw_cycles  speedup  result              ok
1      65537      1.00     0x17e9fe29b5dc0aad  OK
2      32769      1.99     0x17e9fe29b5dc0aad  OK
4      16385      3.99     0x17e9fe29b5dc0aad  OK
8      8193       7.99     0x17e9fe29b5dc0aad  OK
software reference = 0x17e9fe29b5dc0aad  ->  ALL MATCH
```
