#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# atomik-workload-bench.sh — Real-world workload COW benchmarks
#
# Measures ATOMiK's COW detection across common server workloads:
#   1. Fork-heavy server pattern (Redis BGSAVE simulation)
#   2. Container launch pattern (Docker-like fork+exec)
#   3. Build system pattern (make -j parallel compilation)
#   4. Database snapshot pattern (large process fork)
#   5. Desktop workload (mixed fork/exec from typical session)
#
# Each benchmark captures before/after COW stats from ATOMiK sysfs
# and reports redundancy rate and wasted memory.
#
# Usage:
#   sudo ./atomik-workload-bench.sh           # Run all benchmarks
#   sudo ./atomik-workload-bench.sh redis     # Run only Redis benchmark
#   sudo ./atomik-workload-bench.sh --quick   # Quick run (fewer iterations)

set -e

SYSFS="/sys/class/atomik/atomik0"
ITERATIONS=${ITERATIONS:-50}
QUICK=0
SPECIFIC=""

for arg in "$@"; do
    case "$arg" in
        --quick) QUICK=1; ITERATIONS=10 ;;
        --help|-h)
            echo "Usage: $0 [--quick] [redis|container|build|database|desktop]"
            echo ""
            echo "Benchmarks ATOMiK COW detection across real-world workloads."
            echo "Requires: ATOMiK module loaded, root access."
            echo ""
            echo "Options:"
            echo "  --quick    Run fewer iterations (faster, less precise)"
            echo ""
            echo "Workloads:"
            echo "  redis      Fork-heavy server (BGSAVE simulation)"
            echo "  container  Container launch (fork+exec pattern)"
            echo "  build      Build system (parallel fork+exec)"
            echo "  database   Database snapshot (large fork)"
            echo "  desktop    Desktop usage (mixed workload)"
            exit 0
            ;;
        *) SPECIFIC="$arg" ;;
    esac
done

# Check prerequisites
if [ ! -f "$SYSFS/cow_faults_total" ]; then
    echo "ERROR: ATOMiK module not loaded or COW monitoring unavailable."
    echo "  Try: sudo insmod atomik.ko"
    exit 1
fi

# Check we're root (needed for module stats)
if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Must run as root (sudo $0)"
    exit 1
fi

# --- Helpers ---

snapshot_before() {
    B_FAULTS=$(cat $SYSFS/cow_faults_total)
    B_COPIES=$(cat $SYSFS/cow_copies_performed)
    B_REDUNDANT=$(cat $SYSFS/cow_copies_redundant)
    B_BYTES_COPIED=$(cat $SYSFS/cow_bytes_copied)
    B_BYTES_SAVED=$(cat $SYSFS/cow_bytes_saved)
    B_TIME=$(date +%s%N)
}

snapshot_after() {
    A_TIME=$(date +%s%N)
    A_FAULTS=$(cat $SYSFS/cow_faults_total)
    A_COPIES=$(cat $SYSFS/cow_copies_performed)
    A_REDUNDANT=$(cat $SYSFS/cow_copies_redundant)
    A_BYTES_COPIED=$(cat $SYSFS/cow_bytes_copied)
    A_BYTES_SAVED=$(cat $SYSFS/cow_bytes_saved)

    D_FAULTS=$((A_FAULTS - B_FAULTS))
    D_COPIES=$((A_COPIES - B_COPIES))
    D_REDUNDANT=$((A_REDUNDANT - B_REDUNDANT))
    D_BYTES_COPIED=$((A_BYTES_COPIED - B_BYTES_COPIED))
    D_BYTES_SAVED=$((A_BYTES_SAVED - B_BYTES_SAVED))
    D_TIME_NS=$((A_TIME - B_TIME))
    D_TIME_MS=$((D_TIME_NS / 1000000))

    if [ "$D_COPIES" -gt 0 ]; then
        RATE=$((D_REDUNDANT * 100 / D_COPIES))
    else
        RATE=0
    fi
}

format_bytes() {
    local bytes=$1
    if [ "$bytes" -gt $((1024*1024*1024)) ]; then
        echo "$(echo "scale=2; $bytes / 1073741824" | bc) GB"
    elif [ "$bytes" -gt $((1024*1024)) ]; then
        echo "$(echo "scale=2; $bytes / 1048576" | bc) MB"
    elif [ "$bytes" -gt 1024 ]; then
        echo "$(echo "scale=1; $bytes / 1024" | bc) KB"
    else
        echo "$bytes bytes"
    fi
}

print_results() {
    local name="$1"
    echo ""
    echo "  Results:"
    echo "    Duration:           ${D_TIME_MS}ms"
    echo "    COW faults:         $D_FAULTS"
    echo "    Page copies:        $D_COPIES"
    echo "    Redundant copies:   $D_REDUNDANT ($RATE% redundancy)"
    echo "    Memory copied:      $(format_bytes $D_BYTES_COPIED)"
    echo "    Memory wasted:      $(format_bytes $D_BYTES_SAVED)"

    if [ "$D_TIME_MS" -gt 0 ]; then
        local faults_per_sec=$((D_FAULTS * 1000 / D_TIME_MS))
        local waste_per_hour=$(echo "scale=0; $D_BYTES_SAVED * 3600000 / $D_TIME_MS" | bc 2>/dev/null || echo "0")
        echo "    Faults/sec:         $faults_per_sec"
        echo "    Projected waste:    $(format_bytes $waste_per_hour)/hour"
    fi
}

# --- Benchmark 1: Redis BGSAVE simulation ---

bench_redis() {
    echo "=== Benchmark 1: Redis BGSAVE (fork-heavy server) ==="
    echo "  Simulates Redis background save: parent holds large dataset,"
    echo "  child forks and iterates to serialize. Most pages are read-only"
    echo "  — COW copies are almost entirely redundant."
    echo "  Iterations: $ITERATIONS (each fork: 8MB dataset)"
    echo ""

    snapshot_before

    # Simulate: allocate large buffer (Redis dataset), fork, child reads it
    python3 -c "
import os, sys

DATASET_SIZE = 8 * 1024 * 1024  # 8MB 'dataset'

# Pre-allocate dataset (simulates Redis in-memory DB)
dataset = bytearray(DATASET_SIZE)
for i in range(0, DATASET_SIZE, 8):
    dataset[i] = (i >> 3) & 0xFF  # Fill with deterministic data

for iteration in range($ITERATIONS):
    pid = os.fork()
    if pid == 0:
        # Child: 'serialize' the dataset (read-only access)
        # This triggers COW on pages the parent is still using
        checksum = 0
        for i in range(0, len(dataset), 4096):
            checksum ^= dataset[i]
        os._exit(0)
    else:
        os.waitpid(pid, 0)
        # Parent: simulate continued writes (few pages)
        dataset[iteration % DATASET_SIZE] = iteration & 0xFF

sys.stdout.write('Done\n')
"

    snapshot_after
    print_results "Redis BGSAVE"
}

# --- Benchmark 2: Container launch simulation ---

bench_container() {
    echo "=== Benchmark 2: Container Launch (fork+exec pattern) ==="
    echo "  Simulates container startup: fork → brief setup → exec."
    echo "  COW pages allocated during fork are immediately replaced"
    echo "  by exec — 100% of those copies are wasted."
    echo "  Iterations: $((ITERATIONS * 4)) (fork+exec cycles)"
    echo ""

    snapshot_before

    for i in $(seq 1 $((ITERATIONS * 4))); do
        /bin/true  # fork+exec — the simplest container analog
    done

    snapshot_after
    print_results "Container Launch"
}

# --- Benchmark 3: Build system simulation ---

bench_build() {
    echo "=== Benchmark 3: Build System (parallel fork+exec) ==="
    echo "  Simulates 'make -j': parent process forks multiple children"
    echo "  that each exec a compiler. Parallel COW pressure."
    echo "  Iterations: $ITERATIONS batches of 8 parallel forks"
    echo ""

    snapshot_before

    for i in $(seq 1 $ITERATIONS); do
        # Launch 8 parallel 'compile' jobs
        for j in $(seq 1 8); do
            /bin/true &
        done
        wait
    done

    snapshot_after
    print_results "Build System"
}

# --- Benchmark 4: Database snapshot simulation ---

bench_database() {
    echo "=== Benchmark 4: Database Snapshot (large process fork) ==="
    echo "  Simulates pg_dump: large process with significant memory"
    echo "  forks to create consistent snapshot. Child reads data,"
    echo "  parent continues serving. Most COW is redundant."
    echo "  Iterations: $((ITERATIONS / 5 > 0 ? ITERATIONS / 5 : 1))"
    echo "  (32MB dataset each)"
    echo ""

    local db_iterations=$((ITERATIONS / 5))
    [ "$db_iterations" -lt 1 ] && db_iterations=1

    snapshot_before

    python3 -c "
import os, sys

DB_SIZE = 32 * 1024 * 1024  # 32MB database

# Simulate database buffer pool
db = bytearray(DB_SIZE)
for i in range(0, DB_SIZE, 64):
    # Simulate row data
    for j in range(8):
        db[i + j] = ((i >> 6) + j) & 0xFF

for snap in range($db_iterations):
    pid = os.fork()
    if pid == 0:
        # Child: 'dump' the database (sequential read)
        total = 0
        for i in range(0, len(db), 4096):
            total += db[i]
        os._exit(0)
    else:
        os.waitpid(pid, 0)
        # Parent: simulate ongoing transactions (sparse writes)
        for tx in range(100):
            offset = (snap * 100 + tx) * 64 % DB_SIZE
            db[offset] = (snap + tx) & 0xFF

sys.stdout.write('Done\n')
"

    snapshot_after
    print_results "Database Snapshot"
}

# --- Benchmark 5: Desktop workload ---

bench_desktop() {
    echo "=== Benchmark 5: Desktop Workload (mixed patterns) ==="
    echo "  Simulates typical desktop: spawning subprocesses, shell"
    echo "  commands, Python scripts, file operations. The kind of"
    echo "  workload every ATOMiK subscriber runs daily."
    echo "  Duration: ~${ITERATIONS} shell operations"
    echo ""

    snapshot_before

    for i in $(seq 1 $ITERATIONS); do
        # Simulate various desktop operations
        echo "test" | wc -c > /dev/null 2>&1
        ls /tmp > /dev/null 2>&1
        python3 -c "pass" 2>/dev/null
        cat /proc/uptime > /dev/null 2>&1
    done

    snapshot_after
    print_results "Desktop Workload"
}

# --- Main ---

echo "ATOMiK Workload Benchmark v0.4.0"
echo "================================="
echo ""
echo "System: $(uname -r) on $(uname -m)"
echo "CPU:    $(grep 'model name' /proc/cpuinfo | head -1 | cut -d: -f2 | xargs)"
echo "Memory: $(free -h | awk '/^Mem:/ {print $2}') total"
echo "ATOMiK: v$(cat $SYSFS/version) [$(cat $SYSFS/backend)]"
echo ""

if [ -n "$SPECIFIC" ]; then
    case "$SPECIFIC" in
        redis)     bench_redis ;;
        container) bench_container ;;
        build)     bench_build ;;
        database)  bench_database ;;
        desktop)   bench_desktop ;;
        *)
            echo "Unknown workload: $SPECIFIC"
            echo "Available: redis, container, build, database, desktop"
            exit 1
            ;;
    esac
else
    bench_redis
    echo ""
    bench_container
    echo ""
    bench_build
    echo ""
    bench_database
    echo ""
    bench_desktop
fi

echo ""
echo "========================================="
echo "  ATOMiK Workload Benchmark Complete"
echo "========================================="
echo ""
echo "Summary:"
echo "  Total COW faults:    $(cat $SYSFS/cow_faults_total)"
echo "  Total copies:        $(cat $SYSFS/cow_copies_performed)"
echo "  Total redundant:     $(cat $SYSFS/cow_copies_redundant)"
echo "  Redundancy rate:     $(cat $SYSFS/cow_redundancy_rate)%"
echo "  Total wasted:        $(format_bytes $(cat $SYSFS/cow_bytes_saved))"
echo ""
echo "These results demonstrate the memory ATOMiK can save on"
echo "production systems. Visit https://atomik.tech to learn more."
