/* bench.c — LIVE parallel-bank throughput producer.
 *
 * Drives the atomik_parallel_bench engine at 0xF0021000 (see
 * hardware/zynq/rtl/atomik_parallel_bench.v).  Measures the hardware cycle
 * count for a fixed-count XOR accumulation across 1/2/4/8 active banks, and
 * verifies the hardware result against an independent software recompute of
 * the same deterministic delta stream.  This is the real, self-verifying data
 * behind the Resource Fabric's "parallel banks" surface — no fabricated
 * numbers: the cycles come from the chip's own counter, the throughput is
 * derived from them, and a run is only marked verified when HW == SW.
 *
 * Honesty: if /dev/mem or the engine is unavailable, bench_source() reports
 * METRIC_WAITING and the cached sweep stays zeroed — the UI must dim, never
 * invent. */
#include "atomik_os.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

/* register map (word offsets), mirrors atomik_parallel_bench.v */
#define R_CTRL    0
#define R_COUNT   1
#define R_ACTIVE  2
#define R_SEEDLO  3
#define R_SEEDHI  4
#define R_CYCLES  5
#define R_RESLO   6
#define R_RESHI   7
#define R_STATUS  8

#define BENCH_COUNT  65537u                 /* fixed sweep size (fast SW ref) */
#define BENCH_SEED   0xDEADBEEF0BADF00DULL
#define BENCH_REFRESH_MS 2000               /* re-measure cadence            */

static int                s_fd  = -1;
static volatile uint32_t *s_map = NULL;
static int                s_nbanks = 0;
static int                s_present = 0;

static bench_point_t  s_pts[BENCH_N_POINTS];
static int            s_npts = 0;
static int            s_source = METRIC_WAITING;
static unsigned long  s_last_ms = 0;
static int            s_active_idx = 0;     /* index into {1,2,4,8} the demo cycles */
static uint64_t       s_sw_ref = 0;         /* cached SW reference XOR        */
static int            s_sw_ref_done = 0;

static const uint32_t k_banks[BENCH_N_POINTS] = {1, 2, 4, 8};

/* must match dmix() in atomik_parallel_bench.v exactly */
static uint64_t dmix(uint64_t x) { x ^= x << 13; x ^= x >> 7; x ^= x << 17; return x; }
static uint64_t ref_xor(uint64_t seed, uint32_t count) {
    uint64_t a = 0;
    for (uint32_t i = 0; i < count; i++) a ^= dmix(seed ^ (uint64_t)i);
    return a;
}

static inline void wr(int i, uint32_t v) {
    s_map[i] = v; __sync_synchronize();
}
static inline uint32_t rd(int i) {
    __sync_synchronize(); return s_map[i];
}

int bench_open(void) {
    if (s_fd >= 0) return s_present ? 0 : -1;
    s_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (s_fd < 0) return -1;
    s_map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, s_fd,
                 BENCH_BASE_PS & ~0xFFFUL);
    if (s_map == MAP_FAILED) {
        close(s_fd); s_fd = -1; s_map = NULL; return -1;
    }
    /* STATUS high bytes carry VERSION/N_BANKS — a present engine returns
     * a sane N_BANKS (1..64); a dead bus typically reads 0 or all-ones. */
    uint32_t st = rd(R_STATUS);
    s_nbanks = (st >> 16) & 0xFF;
    s_present = (s_nbanks >= 1 && s_nbanks <= 64);
    return s_present ? 0 : -1;
}

void bench_close(void) {
    if (s_map && s_map != MAP_FAILED) munmap((void *)s_map, 4096);
    if (s_fd >= 0) close(s_fd);
    s_map = NULL; s_fd = -1; s_present = 0;
}

int bench_available(void) { return s_present; }
int bench_nbanks(void)    { return s_nbanks; }
int bench_source(void)    { return s_source; }
int bench_active_banks(void) {
    return (s_npts > 0) ? (int)s_pts[s_active_idx].banks : (int)k_banks[s_active_idx];
}

const bench_point_t *bench_sweep_points(int *n) {
    if (n) *n = s_npts;
    return s_pts;
}

/* run one measurement at `active` banks; returns cycles, fills *result */
static uint32_t measure(uint32_t active, uint64_t *result) {
    wr(R_COUNT, BENCH_COUNT);
    wr(R_ACTIVE, active);
    wr(R_SEEDLO, (uint32_t)BENCH_SEED);
    wr(R_SEEDHI, (uint32_t)(BENCH_SEED >> 32));
    wr(R_CTRL, 1);
    /* poll done (CTRL bit1); bounded so a wedged engine can't hang the UI */
    long g = 0;
    while (!(rd(R_CTRL) & 0x2)) { if (++g > 5000000) break; }
    uint32_t cyc = rd(R_CYCLES);
    uint64_t res = ((uint64_t)rd(R_RESHI) << 32) | rd(R_RESLO);
    if (result) *result = res;
    return cyc;
}

void bench_tick(void) {
    unsigned long now = anim_now_ms();

    if (!s_present) {
        /* attempt a (cheap) open once; stay WAITING until it succeeds */
        if (s_fd < 0) bench_open();
        if (!s_present) { s_source = METRIC_WAITING; return; }
    }

    if (!s_sw_ref_done) {
        s_sw_ref = ref_xor(BENCH_SEED, BENCH_COUNT);
        s_sw_ref_done = 1;
    }

    /* Measure the sweep ONCE and cache it.  The deltas are deterministic, so
     * re-measuring every frame buys nothing and only adds MMIO risk; doing it
     * once keeps the panel rock-stable.  Stage markers are written to
     * /tmp/atomik_bench.txt so a crash can be localized without a screenshot:
     *   "stage=open_ok"  reached bench_tick with the engine mapped
     *   "  banks=.."     each completed measurement
     *   "stage=swept"    full sweep done, data cached */
    if (s_npts == 0) {
        FILE *mf = fopen("/tmp/atomik_bench.txt", "w");
        if (mf) { fprintf(mf, "stage=open_ok nbanks=%d\n", s_nbanks); fclose(mf); }

        int npts = 0; uint32_t base_cyc = 0; int all_verified = 1;
        for (int k = 0; k < BENCH_N_POINTS; k++) {
            uint32_t b = k_banks[k];
            if ((int)b > s_nbanks) continue;
            uint64_t res = 0;
            uint32_t cyc = measure(b, &res);
            if (k == 0) base_cyc = cyc;
            bench_point_t *p = &s_pts[npts++];
            p->banks     = b;
            p->cycles    = cyc;
            p->mdeltas_s = cyc ? ((double)BENCH_COUNT / cyc) * (BENCH_SYS_HZ / 1.0e6) : 0.0;
            p->speedup   = (cyc && base_cyc) ? (double)base_cyc / cyc : 0.0;
            p->verified  = (res == s_sw_ref);
            all_verified &= p->verified;
            mf = fopen("/tmp/atomik_bench.txt", "a");
            if (mf) { fprintf(mf, "  banks=%u cycles=%u %.1f Md/s %.2fx %s\n",
                              p->banks, p->cycles, p->mdeltas_s, p->speedup,
                              p->verified ? "OK" : "MISMATCH"); fclose(mf); }
        }
        s_npts   = npts;
        s_source = (npts > 0 && all_verified) ? METRIC_LIVE : METRIC_STALE;
        s_last_ms = now;
        mf = fopen("/tmp/atomik_bench.txt", "a");
        if (mf) { fprintf(mf, "stage=swept npts=%d src=%d\n", s_npts, s_source); fclose(mf); }
        return;
    }

    /* Already measured.  Just animate the "currently allocated" highlight
     * cursor every BENCH_REFRESH_MS so the surface visibly reallocates
     * 1 -> 2 -> 4 -> 8 -> 1 ...  No re-measurement, no MMIO. */
    if (s_last_ms != 0 && (now - s_last_ms) < BENCH_REFRESH_MS) return;
    s_last_ms = now;
    if (s_npts > 0) s_active_idx = (s_active_idx + 1) % s_npts;
}

/* ── metric getters (registered in bench_register_metrics) ───────────── */
static metric_value_t getter_speedup(void *ctx) {
    (void)ctx;
    metric_value_t v = { 0.0, METRIC_WAITING };
    if (s_npts > 0) {
        v.value  = s_pts[s_npts - 1].speedup;   /* max-bank speedup */
        v.source = s_source;
    }
    return v;
}
static metric_value_t getter_throughput(void *ctx) {
    (void)ctx;
    metric_value_t v = { 0.0, METRIC_WAITING };
    if (s_npts > 0) {
        v.value  = s_pts[s_active_idx].mdeltas_s; /* at current allocation */
        v.source = s_source;
    }
    return v;
}

void bench_register_metrics(void) {
    metric_register("bench.speedup",    "Bank Speedup",     "x",
                    getter_speedup, NULL);
    metric_register("bench.throughput", "Bank Throughput",  "Md/s",
                    getter_throughput, NULL);
}
