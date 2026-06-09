/* wlmeasure.c — parses the board-side memory-workload measurements that the
 * `aworkload` tool (hardware/zynq/test/aworkload.c) writes to
 * /tmp/atomik_workloads_live.txt, and exposes them to the WORKLOADS surface so
 * the Telemetry-Sync and Control-Coalescing scenarios can show REAL, hardware-
 * verified numbers instead of SCENARIO placeholders.  Same file-fed pattern as
 * bench.c — no MMIO in the compositor; the tool measures, atomik_os reads.
 *
 * File format (key=value lines, two WORKLOAD blocks):
 *   WORKLOAD telemetry_sync
 *   regions=256 changed=13 density=5
 *   conv_bytes=2048 atomik_bytes=104 saved_pct=94
 *   verified=1
 *   WORKLOAD control_coalescing
 *   updates=256 unique=32
 *   conv_writes=256 atomik_writes=32 saved_pct=87
 *   verified=1
 */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

#define WL_FILE "/tmp/atomik_workloads_live.txt"
#define WL_REFRESH_MS 2000

static wl_measure_t s_m[2];          /* 0 = telemetry, 1 = coalescing */
static unsigned long s_last_ms = 0;

int wlmeasure_get(int which, wl_measure_t *out) {
    if (which < 0 || which > 1 || !out) return 0;
    *out = s_m[which];
    return s_m[which].valid;
}

static long getval(const char *line, const char *key) {
    const char *p = strstr(line, key);
    if (!p) return -1;
    p += strlen(key);
    long v = 0; int got = 0, neg = 0;
    if (*p == '-') { neg = 1; p++; }
    while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; got = 1; }
    return got ? (neg ? -v : v) : -1;
}

void wlmeasure_tick(void) {
    unsigned long now = anim_now_ms();
    if (s_last_ms != 0 && (now - s_last_ms) < WL_REFRESH_MS) return;
    s_last_ms = now;

    FILE *f = fopen(WL_FILE, "r");
    if (!f) return;
    wl_measure_t m[2]; memset(m, 0, sizeof m);
    int cur = -1;          /* current block index */
    char line[160];
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "WORKLOAD telemetry_sync"))      { cur = 0; continue; }
        if (strstr(line, "WORKLOAD control_coalescing"))  { cur = 1; continue; }
        if (cur < 0) continue;
        long v;
        if ((v = getval(line, "conv_bytes="))   >= 0) { m[cur].conv   = v; m[cur].is_bytes = 1; }
        if ((v = getval(line, "atomik_bytes=")) >= 0)   m[cur].atomik = v;
        if ((v = getval(line, "conv_writes="))  >= 0) { m[cur].conv   = v; m[cur].is_bytes = 0; }
        if ((v = getval(line, "atomik_writes="))>= 0)   m[cur].atomik = v;
        if ((v = getval(line, "saved_pct="))    >= 0)   m[cur].saved_pct = (int)v;
        if ((v = getval(line, "changed="))      >= 0)   m[cur].extra = (int)v;
        if ((v = getval(line, "unique="))       >= 0)   m[cur].extra = (int)v;
        if ((v = getval(line, "verified="))     >= 0) {
            m[cur].verified = (int)v;
            /* a block is valid once we've seen its conv+atomik+verified */
            if (m[cur].conv > 0) m[cur].valid = 1;
        }
    }
    fclose(f);
    s_m[0] = m[0];
    s_m[1] = m[1];
}
