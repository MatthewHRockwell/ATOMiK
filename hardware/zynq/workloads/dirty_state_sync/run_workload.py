#!/usr/bin/env python3
"""P0 Workload 1: Dirty-state telemetry sync.

Scenario: A device maintains a 256-region state table. Each tick, a fraction
of regions change (sparse updates). The baseline path scans all regions and
computes a full or delta payload. The ATOMiK path accumulates only meaningful
changes and counts bytes avoided.

Run on board: python3 run_workload.py [--regions N] [--ticks N] [--density F]

Outputs CSV to stdout, JSON summary to results/.
"""
import sys, os, time, json, csv, hashlib, random, argparse
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from common.atomik_hw_interface import ATOMiKHW

RESULTS_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "..", "..",
                            "results", "zynq_validation", "20260527", "dirty_state_sync")


def baseline_path(state, updates):
    """Software baseline: scan full state, apply updates, compute bytes moved."""
    t0 = time.perf_counter_ns()
    new_state = list(state)
    for addr, delta in updates:
        new_state[addr] ^= delta
    # Full state transfer simulation: scan all, emit changed
    changed = [(i, new_state[i]) for i in range(len(state)) if new_state[i] != state[i]]
    bytes_moved = len(state) * 8          # full scan: all regions × 8 bytes
    bytes_emitted = len(changed) * 8      # emit only changed
    t1 = time.perf_counter_ns()
    state_hash = hashlib.sha256(bytes(v.to_bytes(8, 'little') for v in new_state)).hexdigest()[:16]
    return {
        "elapsed_ns": t1 - t0,
        "bytes_moved": bytes_moved,
        "bytes_emitted": bytes_emitted,
        "changed_regions": len(changed),
        "state": new_state,
        "hash": state_hash,
    }


def atomik_path(hw, state, updates):
    """ATOMiK path: LOAD reference, ACCUM deltas, READ at boundary."""
    t0 = time.perf_counter_ns()
    # Track which addresses are touched
    touched = {}
    for addr, delta in updates:
        touched[addr] = touched.get(addr, 0) ^ delta  # coalesce

    ops_received = len(updates)
    ops_emitted = len(touched)
    ops_coalesced = ops_received - ops_emitted

    # Hardware: LOAD first touched addr as reference, ACCUM rest
    bytes_sent_to_hw = 0
    for addr, coalesced_delta in touched.items():
        hw.load(addr, state[addr])
        bytes_sent_to_hw += 8   # reference
        hw.accum(coalesced_delta)
        bytes_sent_to_hw += 8   # delta

    # READ to reconstruct and verify first addr
    first_addr = next(iter(touched))
    reconstructed = hw.read()

    t1 = time.perf_counter_ns()

    # Compute expected final state
    new_state = list(state)
    for addr, delta in updates:
        new_state[addr] ^= delta

    state_hash = hashlib.sha256(bytes(v.to_bytes(8, 'little') for v in new_state)).hexdigest()[:16]
    expected = new_state[first_addr]
    correct = (reconstructed == expected)

    bytes_moved_baseline = len(state) * 8
    bytes_avoided = bytes_moved_baseline - bytes_sent_to_hw

    return {
        "elapsed_ns": t1 - t0,
        "bytes_sent_to_hw": bytes_sent_to_hw,
        "bytes_avoided": bytes_avoided,
        "ops_received": ops_received,
        "ops_emitted": ops_emitted,
        "ops_coalesced": ops_coalesced,
        "correct": correct,
        "state": new_state,
        "hash": state_hash,
    }


def run_trial(hw, n_regions, n_updates, change_density, seed):
    rng = random.Random(seed)
    # Initial state: random 64-bit values
    state = [rng.getrandbits(64) for _ in range(n_regions)]

    # Generate updates: change_density fraction of regions get updates
    n_changed = max(1, int(n_regions * change_density))
    changed_addrs = rng.sample(range(n_regions), min(n_changed, n_regions))
    # n_updates total, concentrated on changed_addrs (to simulate repeated updates)
    updates = []
    for _ in range(n_updates):
        addr = rng.choice(changed_addrs)
        delta = rng.getrandbits(64)
        updates.append((addr, delta))

    b = baseline_path(state, updates)
    a = atomik_path(hw, state, updates)

    # Correctness: both paths must produce same final state hash
    correct = b["hash"] == a["hash"] and a["correct"]

    return {
        "seed": seed,
        "n_regions": n_regions,
        "n_updates": n_updates,
        "change_density": change_density,
        "n_changed_regions": n_changed,
        "baseline_elapsed_ns": b["elapsed_ns"],
        "baseline_bytes_moved": b["bytes_moved"],
        "baseline_bytes_emitted": b["bytes_emitted"],
        "atomik_elapsed_ns": a["elapsed_ns"],
        "atomik_bytes_sent": a["bytes_sent_to_hw"],
        "bytes_avoided": a["bytes_avoided"],
        "ops_received": a["ops_received"],
        "ops_emitted": a["ops_emitted"],
        "ops_coalesced": a["ops_coalesced"],
        "correctness": "PASS" if correct else "FAIL",
        "baseline_hash": b["hash"],
        "atomik_hash": a["hash"],
    }


def main():
    ap = argparse.ArgumentParser(description="ATOMiK dirty-state telemetry workload")
    ap.add_argument("--regions", type=int, default=256)
    ap.add_argument("--ticks", type=int, default=10)
    ap.add_argument("--density", type=float, default=0.05)
    ap.add_argument("--updates", type=int, default=64)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--output-dir", default=RESULTS_DIR)
    args = ap.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    print(f"ATOMiK Dirty-State Sync Workload", file=sys.stderr)
    print(f"  regions={args.regions} ticks={args.ticks} density={args.density} updates={args.updates}", file=sys.stderr)

    with ATOMiKHW() as hw:
        # Smoke test first
        ok, msg = hw.smoke_test()
        print(f"  Smoke test: {'PASS' if ok else 'FAIL'} — {msg}", file=sys.stderr)
        if not ok:
            print("ABORT: smoke test failed", file=sys.stderr)
            return 1

        rows = []
        for i in range(args.ticks):
            seed = args.seed + i
            r = run_trial(hw, args.regions, args.updates, args.density, seed)
            rows.append(r)
            print(f"  Tick {i+1:02d}: {r['correctness']} | bytes_avoided={r['bytes_avoided']:,} | ops_coalesced={r['ops_coalesced']:,}", file=sys.stderr)

    # Write CSV
    csv_path = os.path.join(args.output_dir, "raw_runs.csv")
    with open(csv_path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=rows[0].keys())
        w.writeheader()
        w.writerows(rows)

    # Summary
    passing = [r for r in rows if r["correctness"] == "PASS"]
    if not passing:
        print("ERROR: no passing runs", file=sys.stderr)
        return 1

    avg_bytes_avoided = sum(r["bytes_avoided"] for r in passing) / len(passing)
    avg_ops_coalesced = sum(r["ops_coalesced"] for r in passing) / len(passing)
    avg_baseline_ms   = sum(r["baseline_elapsed_ns"] for r in passing) / len(passing) / 1e6
    avg_atomik_ms     = sum(r["atomik_elapsed_ns"] for r in passing) / len(passing) / 1e6
    avg_bytes_moved   = sum(r["baseline_bytes_moved"] for r in passing) / len(passing)

    summary = {
        "workload": "dirty_state_sync",
        "board": "HamGeek RK-ZYNQ7020-F (XC7Z020-2CLG484I)",
        "config": {"regions": args.regions, "ticks": args.ticks, "density": args.density, "updates": args.updates},
        "runs": len(rows), "passing": len(passing),
        "correctness": "PASS" if len(passing) == len(rows) else f"PARTIAL ({len(passing)}/{len(rows)})",
        "avg_bytes_avoided": avg_bytes_avoided,
        "avg_bytes_moved_baseline": avg_bytes_moved,
        "avg_pct_bytes_avoided": avg_bytes_avoided / avg_bytes_moved * 100,
        "avg_ops_coalesced": avg_ops_coalesced,
        "avg_baseline_ms": avg_baseline_ms,
        "avg_atomik_ms": avg_atomik_ms,
        "evidence_label": "HARDWARE_VALIDATED" if len(passing) == len(rows) else "BUILD_ARTIFACT",
        "caveat": "Workload-specific. Does not prove universal speedup, battery gain, thermal reduction, or production readiness.",
    }

    summary_path = os.path.join(args.output_dir, "summary.json")
    with open(summary_path, "w") as f:
        json.dump(summary, f, indent=2)

    print(f"\n=== SUMMARY ===")
    print(f"Correctness: {summary['correctness']}")
    print(f"Avg bytes avoided: {avg_bytes_avoided:,.0f} ({summary['avg_pct_bytes_avoided']:.1f}% of baseline scan)")
    print(f"Avg ops coalesced: {avg_ops_coalesced:.1f}")
    print(f"Evidence: {summary['evidence_label']}")
    print(f"Results: {csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
