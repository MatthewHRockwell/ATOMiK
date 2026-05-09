/* perf_bench_main.c — standalone runner for the four-way matrix.
 *
 * Builds independently of atomik_os (different make rule); links
 * src/perf_counter.o + src/atomik_batch.o + src/perf_bench.o + a
 * stub for anim_now_ms().  Useful for:
 *
 *   - CI smoke ("make sure the matrix runs and produces nonzero
 *     numbers for all 27 cells")
 *   - Cross-platform sanity (laptop vs board) — when /dev/mem isn't
 *     present, ATOMiK MMIO is a no-op but the perf accounting
 *     remains valid, so the matrix still tells the architectural
 *     compounding story
 *   - Baseline-capture for the four-way comparison table the demo
 *     pitch references
 *
 * Build:
 *     riscv64-linux-gnu-gcc -O2 -Iinclude tools/perf_bench_main.c \
 *         src/perf_counter.c src/atomik_batch.c src/perf_bench.c \
 *         -o build/perf_bench
 *
 * Run on board:
 *     scp build/perf_bench board:/root/
 *     ssh board /root/perf_bench
 */
#include "atomik_os.h"
#include <stdio.h>
#include <time.h>

/* Stub for anim_now_ms() since we don't pull in src/anim.c — the
 * AGENT profile uses this for relevance recency.  Real ms-precision
 * monotonic clock; matches the production anim_now_ms() semantics. */
unsigned long anim_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)ts.tv_sec * 1000UL +
           (unsigned long)(ts.tv_nsec / 1000000UL);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("ATOMiK perf-bench (v0.33-D) — four-way comparison matrix\n");
    printf("software / atomik_direct / atomik_batched / atomik_profile\n");
    perf_bench_matrix();
    return 0;
}
