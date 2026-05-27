#!/bin/bash
# Run all P0 workloads on the board. Execute from the board's userspace.
# Requires: Python 3, /dev/mem access (run as root or with proper permissions)

set -e
cd "$(dirname "$0")"

echo "=== ATOMiK Zynq Workload Validation | $(date) ==="
echo "Board: HamGeek RK-ZYNQ7020-F (XC7Z020)"

# Check ATOMiK hardware reachable
python3 - <<'PYEOF'
import sys
sys.path.insert(0, 'common')
from atomik_hw_interface import ATOMiKHW
with ATOMiKHW() as hw:
    ok, msg = hw.smoke_test()
    print(f"Hardware smoke test: {'PASS' if ok else 'FAIL'} — {msg}")
    if not ok: sys.exit(1)
PYEOF

echo ""
echo "=== P0 Workload 1: Dirty-State Telemetry Sync ==="
python3 dirty_state_sync/run_workload.py --regions 256 --ticks 5 --density 0.05 --updates 64

echo ""
echo "=== P0 Workload 1b: Higher density ==="
python3 dirty_state_sync/run_workload.py --regions 256 --ticks 5 --density 0.20 --updates 256

echo ""
echo "=== P0 Workload 2: Register Coalescing ==="
python3 register_coalescing/run_workload.py --regions 64 --ops 256 --repeat-factor 8 --ticks 5

echo ""
echo "=== P0 Workload 2b: High ops ==="
python3 register_coalescing/run_workload.py --regions 64 --ops 1024 --repeat-factor 16 --ticks 5

echo ""
echo "=== All workloads complete ==="
echo "Results in: $(pwd)/../../../../results/zynq_validation/20260527/"
