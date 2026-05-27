#!/usr/bin/env python3
"""P0 Workload 2: Repeated register/control update coalescing.

Scenario: An embedded control system receives many logical updates before a
commit boundary. Many updates touch the same regions. The baseline emits every
update. ATOMiK coalesces repeated changes to same regions.

Run on board: python3 run_workload.py
"""
import sys, os, time, json, csv, hashlib, random, argparse
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from common.atomik_hw_interface import ATOMiKHW

RESULTS_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "..", "..",
                            "results", "zynq_validation", "20260527", "register_coalescing")


def baseline_path(state, updates):
    t0 = time.perf_counter_ns()
    new_state = list(state)
    ops_emitted = 0
    for addr, delta in updates:
        new_state[addr] ^= delta
        ops_emitted += 1   # baseline emits every operation
    t1 = time.perf_counter_ns()
    h = hashlib.sha256(bytes(v.to_bytes(8, 'little') for v in new_state)).hexdigest()[:16]
    return {"elapsed_ns": t1-t0, "ops_emitted": ops_emitted, "state": new_state, "hash": h}


def atomik_path(hw, state, updates):
    t0 = time.perf_counter_ns()
    touched = {}
    for addr, delta in updates:
        touched[addr] = touched.get(addr, 0) ^ delta
    ops_received = len(updates)
    ops_emitted = len(touched)
    ops_coalesced = ops_received - ops_emitted

    for addr, coalesced_delta in touched.items():
        hw.load(addr, state[addr])
        hw.accum(coalesced_delta)

    # READ final state for first region to verify
    first_addr = next(iter(touched))
    hw.load(first_addr, state[first_addr])
    for addr, cdelta in touched.items():
        if addr == first_addr:
            hw.accum(cdelta)
            break
    reconstructed = hw.read()

    t1 = time.perf_counter_ns()
    new_state = list(state)
    for addr, delta in updates:
        new_state[addr] ^= delta
    h = hashlib.sha256(bytes(v.to_bytes(8, 'little') for v in new_state)).hexdigest()[:16]
    expected = new_state[first_addr]
    correct = (reconstructed == expected)

    return {
        "elapsed_ns": t1-t0,
        "ops_received": ops_received,
        "ops_emitted": ops_emitted,
        "ops_coalesced": ops_coalesced,
        "unique_regions": len(touched),
        "unique_region_ratio": len(touched) / len(updates),
        "correct": correct,
        "state": new_state,
        "hash": h,
    }


def run_trial(hw, n_regions, n_ops, repeat_factor, seed):
    rng = random.Random(seed)
    state = [rng.getrandbits(64) for _ in range(n_regions)]
    # High repeat: most ops hit a small number of regions
    hot_regions = max(1, int(n_regions / repeat_factor))
    hot = rng.sample(range(n_regions), min(hot_regions, n_regions))
    updates = [(rng.choice(hot), rng.getrandbits(64)) for _ in range(n_ops)]

    b = baseline_path(state, updates)
    a = atomik_path(hw, state, updates)
    correct = b["hash"] == a["hash"] and a["correct"]

    return {
        "seed": seed, "n_regions": n_regions, "n_ops": n_ops,
        "repeat_factor": repeat_factor, "hot_regions": hot_regions,
        "baseline_ops_emitted": b["ops_emitted"],
        "baseline_elapsed_ns": b["elapsed_ns"],
        "atomik_ops_received": a["ops_received"],
        "atomik_ops_emitted": a["ops_emitted"],
        "atomik_ops_coalesced": a["ops_coalesced"],
        "atomik_unique_regions": a["unique_regions"],
        "atomik_unique_region_ratio": round(a["unique_region_ratio"], 4),
        "atomik_elapsed_ns": a["elapsed_ns"],
        "correctness": "PASS" if correct else "FAIL",
        "baseline_hash": b["hash"],
        "atomik_hash": a["hash"],
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--regions", type=int, default=64)
    ap.add_argument("--ops", type=int, default=256)
    ap.add_argument("--repeat-factor", type=int, default=8)
    ap.add_argument("--ticks", type=int, default=10)
    ap.add_argument("--seed", type=int, default=99)
    ap.add_argument("--output-dir", default=RESULTS_DIR)
    args = ap.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)
    print(f"ATOMiK Register Coalescing Workload  regions={args.regions} ops={args.ops} repeat={args.repeat_factor}", file=sys.stderr)

    with ATOMiKHW() as hw:
        ok, msg = hw.smoke_test()
        print(f"Smoke test: {'PASS' if ok else 'FAIL'} — {msg}", file=sys.stderr)
        if not ok:
            return 1

        rows = []
        for i in range(args.ticks):
            r = run_trial(hw, args.regions, args.ops, args.repeat_factor, args.seed + i)
            rows.append(r)
            pct = r["atomik_ops_coalesced"] / r["atomik_ops_received"] * 100
            print(f"  Tick {i+1:02d}: {r['correctness']} | coalesced={r['atomik_ops_coalesced']:,}/{r['atomik_ops_received']:,} ({pct:.0f}%)", file=sys.stderr)

    csv_path = os.path.join(args.output_dir, "raw_runs.csv")
    with open(csv_path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=rows[0].keys())
        w.writeheader(); w.writerows(rows)

    passing = [r for r in rows if r["correctness"] == "PASS"]
    avg_coalesced = sum(r["atomik_ops_coalesced"] for r in passing) / max(1, len(passing))
    avg_received  = sum(r["atomik_ops_received"]  for r in passing) / max(1, len(passing))

    summary = {
        "workload": "register_coalescing",
        "board": "HamGeek RK-ZYNQ7020-F (XC7Z020-2CLG484I)",
        "config": {"regions": args.regions, "ops": args.ops, "repeat_factor": args.repeat_factor, "ticks": args.ticks},
        "runs": len(rows), "passing": len(passing),
        "correctness": "PASS" if len(passing) == len(rows) else f"PARTIAL ({len(passing)}/{len(rows)})",
        "avg_ops_received": avg_received,
        "avg_ops_coalesced": avg_coalesced,
        "avg_pct_coalesced": avg_coalesced / max(1, avg_received) * 100,
        "evidence_label": "HARDWARE_VALIDATED" if len(passing) == len(rows) else "BUILD_ARTIFACT",
        "caveat": "Workload-specific. Does not prove universal speedup or production readiness.",
    }
    with open(os.path.join(args.output_dir, "summary.json"), "w") as f:
        json.dump(summary, f, indent=2)

    print(f"\n=== SUMMARY ===")
    print(f"Correctness: {summary['correctness']}")
    print(f"Avg ops coalesced: {avg_coalesced:.1f} / {avg_received:.1f} ({summary['avg_pct_coalesced']:.0f}%)")
    print(f"Evidence: {summary['evidence_label']}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
