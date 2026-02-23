// ATOMiK v3 Performance Benchmark Suite — Implementation
// Outputs machine-parseable lines for automated data collection.

#include "perf_bench_v3.h"
#include "atomik_v3_hal.h"
#include "atomik_v3_mem.h"
#include "printf_v3.h"
#include <stdint.h>

// Test buffer (keep small to fit in SRAM)
static uint64_t test_buf[32];  // 256 bytes

void cmd_perf_suite(void)
{
    uint64_t c0, c1;
    uint64_t fp;

    mini_printf("\n##PERF_START\n");
    mini_printf("##META:core=atomik_v3,cpu=rv64i,freq_mhz=13.5\n\n");

    // --- Test 1: ATOMiK Load ---
    mini_printf("##PERF:test=atomik_load,");
    c0 = cycles64();
    atomik_load_state(0, 0xDEADBEEF12345678ULL);
    c1 = cycles64();
    mini_printf("cycles=%u\n", (uint32_t)(c1 - c0));

    // --- Test 2: ATOMiK Accumulate ---
    mini_printf("##PERF:test=atomik_accum,");
    c0 = cycles64();
    atomik_accumulate(0, 0xAAAAAAAA55555555ULL);
    c1 = cycles64();
    mini_printf("cycles=%u\n", (uint32_t)(c1 - c0));

    // --- Test 3: ATOMiK Read ---
    mini_printf("##PERF:test=atomik_read,");
    c0 = cycles64();
    fp = atomik_state(0);
    c1 = cycles64();
    mini_printf("cycles=%u,value=0x%lx\n", (uint32_t)(c1 - c0), fp);

    // --- Test 4: ATOMiK Roundtrip (load + accum + read) ---
    mini_printf("##PERF:test=atomik_roundtrip,");
    c0 = cycles64();
    atomik_load_state(0, 0xCAFEBABEULL);
    atomik_accumulate(0, 0x12345678ULL);
    fp = atomik_state(0);
    c1 = cycles64();
    mini_printf("cycles=%u\n", (uint32_t)(c1 - c0));

    // --- Test 5: Fingerprint 256B ---
    // Initialize buffer
    for (int i = 0; i < 32; i++)
        test_buf[i] = 0xA5A5A5A5A5A5A5A5ULL ^ (uint64_t)i;

    mini_printf("##PERF:test=fingerprint_256B,");
    c0 = cycles64();
    fp = atomik_fingerprint(0, test_buf, 32);
    c1 = cycles64();
    mini_printf("cycles=%u,fp=0x%lx\n", (uint32_t)(c1 - c0), fp);

    // --- Test 6: Change detection (unchanged) ---
    uint64_t saved_fp = atomik_fingerprint(0, test_buf, 32);
    mini_printf("##PERF:test=change_detect_unchanged,");
    c0 = cycles64();
    int changed = atomik_region_changed(test_buf, 32, saved_fp);
    c1 = cycles64();
    mini_printf("cycles=%u,changed=%d\n", (uint32_t)(c1 - c0), changed);

    // --- Test 7: Change detection (1-bit flip) ---
    test_buf[16] ^= 1;
    mini_printf("##PERF:test=change_detect_1bit,");
    c0 = cycles64();
    changed = atomik_region_changed(test_buf, 32, saved_fp);
    c1 = cycles64();
    mini_printf("cycles=%u,changed=%d\n", (uint32_t)(c1 - c0), changed);
    test_buf[16] ^= 1;  // restore

    // --- Test 8: Software memcpy baseline ---
    uint64_t dst_buf[32];
    mini_printf("##PERF:test=sw_memcpy_256B,");
    c0 = cycles64();
    sw_memcpy(dst_buf, test_buf, 256);
    c1 = cycles64();
    mini_printf("cycles=%u\n", (uint32_t)(c1 - c0));

    // --- Test 9: ATOMiK tracked memcpy ---
    mini_printf("##PERF:test=atomik_memcpy_256B,");
    c0 = cycles64();
    atomik_memcpy_tracked(dst_buf, test_buf, 256, &fp);
    c1 = cycles64();
    mini_printf("cycles=%u,fp=0x%lx\n", (uint32_t)(c1 - c0), fp);

    // --- Test 10: Software memcmp baseline ---
    mini_printf("##PERF:test=sw_memcmp_256B,");
    c0 = cycles64();
    int cmp = sw_memcmp(test_buf, dst_buf, 256);
    c1 = cycles64();
    mini_printf("cycles=%u,result=%d\n", (uint32_t)(c1 - c0), cmp);

    mini_printf("\n##PERF_END\n");
}
