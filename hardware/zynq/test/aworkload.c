/* aworkload.c — board-side measurement of the two memory-savings workloads
 * (telemetry sync + control coalescing), feeding the atomik_os WORKLOADS
 * surface's memory scenarios with REAL numbers.
 *
 * For each workload we compute the conventional cost vs the ATOMiK cost from a
 * real, deterministic data pattern, and we VERIFY the delta-state math on the
 * ATOMiK adapter hardware @0xF0020000 (LOAD reference, ACCUM coalesced delta,
 * READ back, compare to the software reference).  Output goes to
 * /tmp/atomik_workloads_live.txt; atomik_os parses it (wlmeasure.c).
 *
 * Adapter register map (atomik_cfu_wishbone.v, word-addressed):
 *   0x00 CMD={funct3<<29}  0x04 RS1  0x08 RS2  0x0C RD  0x10 STATUS
 *   funct3: LOAD=0 ACCUM=1 READ=2 LOAD_HI=4 ACCUM_HI=5 READ_HI=6
 *
 *   riscv64-linux-gnu-gcc -O2 -static aworkload.c -o aworkload
 */
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define ADAPTER_BASE 0xF0020000UL
#define A_CMD  0
#define A_RS1  1
#define A_RS2  2
#define A_RD   3
#define A_STATUS 4
#define FN(f) ((uint32_t)(f) << 29)

static volatile uint32_t *g_adp = 0;

static inline void mfence(void) {
#if defined(__riscv)
    __asm__ volatile("fence iorw,iorw" ::: "memory");
#else
    __asm__ volatile("" ::: "memory");   /* host build (logic test only) */
#endif
}
static inline void wr(int off, uint32_t v) { g_adp[off] = v; mfence(); }
static inline uint32_t rd(int off) { mfence(); return g_adp[off]; }

static void adp_load(uint32_t addr, uint64_t init) {
    wr(A_RS1, addr); wr(A_RS2, (uint32_t)init);        wr(A_CMD, FN(0));
                     wr(A_RS2, (uint32_t)(init >> 32)); wr(A_CMD, FN(4));
}
static void adp_accum(uint64_t delta) {
    wr(A_RS1, (uint32_t)delta);         wr(A_CMD, FN(1));
    wr(A_RS1, (uint32_t)(delta >> 32)); wr(A_CMD, FN(5));
}
static uint64_t adp_read(void) {
    wr(A_CMD, FN(2)); uint32_t lo = rd(A_RD);
    wr(A_CMD, FN(6)); uint32_t hi = rd(A_RD);
    return ((uint64_t)hi << 32) | lo;
}

static uint64_t mix(uint64_t x) { x ^= x << 13; x ^= x >> 7; x ^= x << 17; return x; }

#define N_REGIONS 256

/* Telemetry sync: 256 sensor regions, a sparse fraction change each tick.
 * Conventional moves the full state; ATOMiK sends only the changed deltas. */
static int telemetry_sync(FILE *f, uint64_t seed, int density_pct) {
    uint64_t state[N_REGIONS], delta[N_REGIONS];
    int changed[N_REGIONS], n_changed = 0;
    for (int i = 0; i < N_REGIONS; i++) {
        state[i]  = mix(seed ^ (uint64_t)i);
        /* deterministic "did it change this tick" at the given density */
        int hit = (int)(mix(seed ^ 0xA5A5u ^ (uint64_t)i) % 100u) < density_pct;
        if (hit) { delta[i] = mix(seed ^ 0x1234u ^ (uint64_t)i) | 1; changed[n_changed++] = i; }
        else       delta[i] = 0;
    }
    long conv_bytes   = (long)N_REGIONS * 8;          /* full state, every tick */
    long atomik_bytes = (long)n_changed * 8;          /* only the changed values */

    /* VERIFY on hardware: for each changed region, new = old ^ delta. */
    int verified = (g_adp != 0) && (n_changed > 0);
    for (int k = 0; k < n_changed && verified; k++) {
        int a = changed[k];
        adp_load((uint32_t)a, state[a]);
        adp_accum(delta[a]);
        uint64_t hw = adp_read();
        if (hw != (state[a] ^ delta[a])) verified = 0;
    }
    int saved = conv_bytes ? (int)((conv_bytes - atomik_bytes) * 100 / conv_bytes) : 0;
    fprintf(f, "WORKLOAD telemetry_sync\n");
    fprintf(f, "regions=%d changed=%d density=%d\n", N_REGIONS, n_changed, density_pct);
    fprintf(f, "conv_bytes=%ld atomik_bytes=%ld saved_pct=%d\n", conv_bytes, atomik_bytes, saved);
    fprintf(f, "verified=%d\n", verified);
    return verified;
}

/* Control coalescing: many control updates before a commit, many to the same
 * registers.  Conventional emits every update; ATOMiK coalesces repeats into
 * one net delta per register (XOR is its own inverse, so repeats cancel). */
static int control_coalescing(FILE *f, uint64_t seed, int n_updates, int n_regs) {
    if (n_regs > N_REGIONS) n_regs = N_REGIONS;
    uint64_t coalesced[N_REGIONS];
    int touched[N_REGIONS];
    for (int i = 0; i < n_regs; i++) { coalesced[i] = 0; touched[i] = 0; }
    for (int u = 0; u < n_updates; u++) {
        int r = (int)(mix(seed ^ 0xC0FFEEu ^ (uint64_t)u) % (uint64_t)n_regs);
        uint64_t d = mix(seed ^ 0xBEEFu ^ (uint64_t)u) | 1;
        coalesced[r] ^= d; touched[r] = 1;
    }
    int n_touched = 0;
    for (int i = 0; i < n_regs; i++) if (touched[i]) n_touched++;

    /* VERIFY: accumulate the coalesced per-register deltas on hardware. */
    int verified = (g_adp != 0) && (n_touched > 0);
    for (int i = 0; i < n_regs && verified; i++) {
        if (!touched[i]) continue;
        adp_load((uint32_t)i, 0);
        adp_accum(coalesced[i]);
        if (adp_read() != coalesced[i]) verified = 0;
    }
    int saved = n_updates ? (int)((long)(n_updates - n_touched) * 100 / n_updates) : 0;
    fprintf(f, "WORKLOAD control_coalescing\n");
    fprintf(f, "updates=%d unique=%d\n", n_updates, n_touched);
    fprintf(f, "conv_writes=%d atomik_writes=%d saved_pct=%d\n", n_updates, n_touched, saved);
    fprintf(f, "verified=%d\n", verified);
    return verified;
}

int main(int argc, char **argv) {
    const char *out = (argc > 1) ? argv[1] : "/tmp/atomik_workloads_live.txt";
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd >= 0) {
        void *m = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)ADAPTER_BASE);
        if (m != MAP_FAILED) g_adp = (volatile uint32_t *)m;
    }
    FILE *f = fopen(out, "w");
    if (!f) { fprintf(stderr, "cannot open %s\n", out); return 1; }
    uint64_t seed = 0xD1A57A7Eu;
    telemetry_sync(f, seed, 5);             /* 256 regions, 5% change/tick */
    control_coalescing(f, seed, 256, 32);   /* 256 updates over 32 registers */
    fclose(f);
    if (g_adp) printf("aworkload: adapter @0x%lx, wrote %s\n", ADAPTER_BASE, out);
    else       printf("aworkload: NO /dev/mem adapter (unverified), wrote %s\n", out);
    return 0;
}
