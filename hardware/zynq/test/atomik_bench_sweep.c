/* atomik_bench_sweep.c — measure ATOMiK parallel-bank throughput on hardware.
 *
 * Drives the atomik_parallel_bench engine at 0xF0021000 over /dev/mem, sweeping
 * active_banks 1->8 for a fixed delta count, and reports the HARDWARE cycle
 * count for each.  Because each delta d(i) = xorshift3(seed ^ i) is a stateless
 * function of its index, the accumulated XOR is INDEPENDENT of bank count — so
 * this program also recomputes the same XOR in software and asserts the
 * hardware result matches.  The honest claim it produces:
 *
 *     "N active banks complete the same accumulation in ~1/N the cycles,
 *      measured on silicon, same verified result."
 *
 * Build (on board):   gcc -O2 -o atomik_bench_sweep atomik_bench_sweep.c
 * Cross:              riscv64-linux-gnu-gcc -O2 -static ...
 * Run (as root):      ./atomik_bench_sweep [count] [seed_hex]
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define BENCH_BASE 0xF0021000UL
#define REG_CTRL   0x00
#define REG_COUNT  0x04
#define REG_ACTIVE 0x08
#define REG_SEEDLO 0x0C
#define REG_SEEDHI 0x10
#define REG_CYCLES 0x14
#define REG_RESLO  0x18
#define REG_RESHI  0x1C
#define REG_STATUS 0x20

static volatile uint32_t *bench;

static inline void wr(int off, uint32_t v) {
    bench[off/4] = v;
    __sync_synchronize();
}
static inline uint32_t rd(int off) {
    __sync_synchronize();
    return bench[off/4];
}

/* Must match dmix() in atomik_parallel_bench.v exactly. */
static uint64_t dmix(uint64_t x) {
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return x;
}
static uint64_t ref_xor(uint64_t seed, uint32_t count) {
    uint64_t acc = 0;
    for (uint32_t i = 0; i < count; i++) acc ^= dmix(seed ^ (uint64_t)i);
    return acc;
}

static uint64_t run_bench(uint32_t count, uint32_t active, uint64_t seed,
                          uint32_t *cycles_out) {
    wr(REG_COUNT,  count);
    wr(REG_ACTIVE, active);
    wr(REG_SEEDLO, (uint32_t)(seed & 0xFFFFFFFF));
    wr(REG_SEEDHI, (uint32_t)(seed >> 32));
    wr(REG_CTRL,   0x1);                 /* start */
    /* poll done (CTRL bit1) */
    int guard = 0;
    while (!(rd(REG_CTRL) & 0x2)) {
        if (++guard > 100000000) { fprintf(stderr, "TIMEOUT\n"); break; }
    }
    *cycles_out = rd(REG_CYCLES);
    uint64_t lo = rd(REG_RESLO), hi = rd(REG_RESHI);
    return (hi << 32) | lo;
}

int main(int argc, char **argv) {
    uint32_t count = (argc > 1) ? (uint32_t)strtoul(argv[1], 0, 0) : 65536;
    uint64_t seed  = (argc > 2) ? strtoull(argv[2], 0, 16)
                                : 0xDEADBEEF0BADF00DULL;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("/dev/mem"); return 1; }
    void *map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                     BENCH_BASE & ~0xFFFUL);
    if (map == MAP_FAILED) { perror("mmap"); return 1; }
    bench = (volatile uint32_t *)((char *)map + (BENCH_BASE & 0xFFF));

    uint32_t status = rd(REG_STATUS);
    uint32_t nbanks = (status >> 16) & 0xFF;
    uint32_t ver    = (status >> 24) & 0xFF;
    printf("ATOMiK parallel-bench  v%u  N_BANKS=%u\n", ver, nbanks);
    printf("count=%u  seed=0x%016llx\n\n", count, (unsigned long long)seed);

    uint64_t ref = ref_xor(seed, count);

    printf("%-8s %-12s %-14s %-10s %s\n",
           "banks", "hw_cycles", "deltas/cycle", "speedup", "result");
    uint32_t base_cycles = 0;
    int all_ok = 1;
    uint32_t banks_list[] = {1, 2, 4, 8};
    for (int k = 0; k < 4; k++) {
        uint32_t b = banks_list[k];
        if (b > nbanks) continue;
        uint32_t cyc;
        uint64_t res = run_bench(count, b, seed, &cyc);
        if (k == 0) base_cycles = cyc;
        double dpc = cyc ? (double)count / cyc : 0.0;
        double spd = cyc ? (double)base_cycles / cyc : 0.0;
        int ok = (res == ref);
        all_ok &= ok;
        printf("%-8u %-12u %-14.3f %-10.2f 0x%016llx %s\n",
               b, cyc, dpc, spd, (unsigned long long)res,
               ok ? "OK" : "MISMATCH!");
    }
    printf("\nsoftware reference XOR = 0x%016llx\n", (unsigned long long)ref);
    printf("result verification: %s\n", all_ok ? "ALL MATCH" : "FAILED");

    munmap(map, 4096);
    close(fd);
    return all_ok ? 0 : 2;
}
