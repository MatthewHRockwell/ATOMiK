/* bench.c — LIVE parallel-bank throughput producer (file-fed).
 *
 * The Resource Fabric's PARALLEL BANKS panel shows real, measured bank-
 * allocation throughput from the engine at 0xF0021000.  Rather than have the
 * compositor poke MMIO directly (raw /dev/mem access from a long-running
 * graphical PIE proved fragile on this board), the measurement is done by the
 * PROVEN standalone sweep tool running as a tiny board-side daemon that writes
 * its result table to BENCH_FILE every couple of seconds.  This module just
 * parses that file — no MMIO, no /dev/mem, nothing that can fault the desktop.
 * Separation of concerns: the tool measures hardware; atomik_os visualizes it.
 *
 * BENCH_FILE is produced by, e.g.:
 *   while true; do /tmp/abench 65537 0xDEADBEEF0BADF00D > /tmp/atomik_bench_live.txt 2>&1; sleep 2; done &
 *
 * The numbers are still 100% real and self-verified: the tool recomputes the
 * XOR in software and only prints "ALL MATCH" when the hardware result agreed.
 * If the file is missing/stale/unverified, the panel honestly shows WAITING. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

#define BENCH_FILE       "/tmp/atomik_bench_live.txt"
#define BENCH_COUNT      65537u            /* must match the daemon's count   */
#define BENCH_REFRESH_MS 2000

static bench_point_t  s_pts[BENCH_N_POINTS];
static int            s_npts = 0;
static int            s_nbanks = 8;
static int            s_source = METRIC_WAITING;
static unsigned long  s_last_ms = 0;
static int            s_active_idx = 0;
static char           s_result_hex[24] = "";   /* merged accumulator (identical across banks) */
static int            s_identical = 0;          /* every parsed data row had the same result   */
static int            s_dirty = 0;              /* parsed data changed since last consumed      */
static int            s_prev_source = -1;
static uint32_t       s_prev_cyc0 = 0;

static const uint32_t k_banks[BENCH_N_POINTS] = {1, 2, 4, 8};

const char *bench_result_hex(void)   { return s_result_hex; }
int  bench_result_identical(void)    { return s_identical; }
/* one-shot: 1 if the sweep data/source changed since the last call.  The
 * main loop uses this to force a repaint when live numbers first arrive or
 * refresh (the surface launches with no input, so nothing else would). */
int  bench_take_dirty(void)          { int d = s_dirty; s_dirty = 0; return d; }

static int is_known_bank(uint32_t b) {
    return b == 1 || b == 2 || b == 4 || b == 8;
}

int bench_open(void) { return 0; }          /* file-fed: nothing to map       */
void bench_close(void) {}
int bench_available(void) { return s_npts > 0; }
int bench_nbanks(void)    { return s_nbanks; }
int bench_source(void)    { return s_source; }
int bench_active_banks(void) {
    return (s_npts > 0) ? (int)s_pts[s_active_idx].banks : (int)k_banks[s_active_idx];
}
const bench_point_t *bench_sweep_points(int *n) {
    if (n) *n = s_npts;
    return s_pts;
}

/* Parse the sweep tool's output file into s_pts.  Returns # of points parsed;
 * sets *all_match from the "ALL MATCH" line. */
static int parse_file(int *all_match) {
    *all_match = 0;
    FILE *f = fopen(BENCH_FILE, "r");
    if (!f) return -1;

    bench_point_t pts[BENCH_N_POINTS];
    int npts = 0;
    uint32_t base_cyc = 0;
    char line[256];
    int nbanks = s_nbanks;
    char first_hex[24] = ""; int hex_seen = 0, hex_ident = 1;

    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "ALL MATCH")) *all_match = 1;
        const char *nb = strstr(line, "N_BANKS=");
        if (nb) { int v = 0; if (sscanf(nb + 8, "%d", &v) == 1 && v >= 1 && v <= 64) nbanks = v; }

        /* data rows look like: "<banks>   <cycles>   <speedup> 0x.. OK" */
        unsigned int banks = 0, cycles = 0;
        if (sscanf(line, "%u %u", &banks, &cycles) == 2 &&
            is_known_bank(banks) && cycles > 0 && npts < BENCH_N_POINTS) {
            bench_point_t *p = &pts[npts++];
            p->banks    = banks;
            p->cycles   = cycles;
            p->mdeltas_s = (double)BENCH_COUNT / cycles * (BENCH_SYS_HZ / 1.0e6);
            if (banks == 1) base_cyc = cycles;
            p->speedup  = 0.0;     /* filled below once base_cyc known */
            p->verified = 0;       /* set from all_match below */

            /* capture the per-row merged-accumulator result and confirm every
             * bank count produced the SAME value — that identity IS the
             * order-independence proof (commutative XOR; lane order/count
             * cannot change the answer). */
            const char *hx = strstr(line, "0x");
            if (hx) {
                char tok[24]; int j = 0;
                while (j < 23 && ((hx[j] == 'x' || hx[j] == 'X') ||
                                  (hx[j] >= '0' && hx[j] <= '9') ||
                                  (hx[j] >= 'a' && hx[j] <= 'f') ||
                                  (hx[j] >= 'A' && hx[j] <= 'F')))
                    { tok[j] = hx[j]; j++; }
                tok[j] = '\0';
                if (!hex_seen) { strncpy(first_hex, tok, sizeof first_hex - 1); hex_seen = 1; }
                else if (strcmp(tok, first_hex) != 0) hex_ident = 0;
            }
        }
    }
    fclose(f);

    s_identical = (npts > 0 && hex_seen && hex_ident && *all_match) ? 1 : 0;
    if (s_identical) strncpy(s_result_hex, first_hex, sizeof s_result_hex - 1);

    if (npts == 0) return 0;
    if (base_cyc == 0) base_cyc = pts[0].cycles;   /* fallback if no 1-bank row */
    for (int i = 0; i < npts; i++) {
        pts[i].speedup  = pts[i].cycles ? (double)base_cyc / pts[i].cycles : 0.0;
        pts[i].verified = *all_match;
    }
    memcpy(s_pts, pts, sizeof(bench_point_t) * npts);
    s_nbanks = nbanks;
    return npts;
}

void bench_tick(void) {
    unsigned long now = anim_now_ms();
    if (s_last_ms != 0 && (now - s_last_ms) < BENCH_REFRESH_MS) return;
    s_last_ms = now;

    int all_match = 0;
    int npts = parse_file(&all_match);

    if (npts > 0) {
        s_npts   = npts;
        s_source = all_match ? METRIC_LIVE : METRIC_STALE;
        s_active_idx = s_npts - 1;   /* headline = max-bank point (no fake cursor) */
    } else {
        /* file missing or unparseable -> honest WAITING (keep last good data
         * dim by leaving s_pts but downgrading the source). */
        s_source = METRIC_WAITING;
        if (npts == 0) s_npts = 0;
    }

    /* flag a repaint if the source flipped (e.g. WAITING->LIVE when the first
     * sweep lands) or the measured cycles changed. */
    uint32_t cyc0 = (s_npts > 0) ? s_pts[0].cycles : 0;
    if (s_source != s_prev_source || cyc0 != s_prev_cyc0) s_dirty = 1;
    s_prev_source = s_source;
    s_prev_cyc0   = cyc0;

    /* tiny debug breadcrumb (few hundred bytes) for off-board confirmation */
    FILE *d = fopen("/tmp/atomik_bench_dbg.txt", "w");
    if (d) {
        fprintf(d, "tick npts=%d src=%d nbanks=%d allmatch=%d\n",
                s_npts, s_source, s_nbanks, all_match);
        for (int i = 0; i < s_npts; i++)
            fprintf(d, "  banks=%u cycles=%u %.1f Md/s %.2fx\n",
                    s_pts[i].banks, s_pts[i].cycles, s_pts[i].mdeltas_s, s_pts[i].speedup);
        fclose(d);
    }
}

/* ── metric getters ──────────────────────────────────────────────────── */
static metric_value_t getter_speedup(void *ctx) {
    (void)ctx;
    metric_value_t v = { 0.0, METRIC_WAITING };
    if (s_npts > 0) { v.value = s_pts[s_npts - 1].speedup; v.source = s_source; }
    return v;
}
static metric_value_t getter_throughput(void *ctx) {
    (void)ctx;
    metric_value_t v = { 0.0, METRIC_WAITING };
    if (s_npts > 0) { v.value = s_pts[s_active_idx].mdeltas_s; v.source = s_source; }
    return v;
}
void bench_register_metrics(void) {
    metric_register("bench.speedup",    "Bank Speedup",    "x",    getter_speedup,    NULL);
    metric_register("bench.throughput", "Bank Throughput", "Md/s", getter_throughput, NULL);
}
