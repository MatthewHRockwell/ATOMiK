// ATOMiK Performance Benchmark Suite
//
// Emits machine-parseable ##PERF lines over UART for automated capture.
// All timing via rdcycle CSR. Statistics computed on host (Python).
//
// Categories:
//   A: ATOMiK core ops (MMIO latency)
//   B: Memory operations (size sweep)
//   C: Burst & scaling
//   D: CPU baselines

#include <stdint.h>
#include "atomik.h"
#include "atomik_mem.h"
#include "printf.h"

// print() is defined in firmware.c
extern void print(const char *p);

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

#define ATOMIK_OPS_ITER   20
#define MEMORY_OPS_ITER   10
#define BURST_ITER        10
#define CPU_ITER          10

// Size sweep in bytes (max 256 = 64 words to fit static buffers)
static const uint32_t mem_sizes[] = { 32, 64, 128, 256 };
#define N_MEM_SIZES 4

// Burst accumulate counts
static const uint32_t burst_counts[] = { 10, 50, 100, 500 };
#define N_BURST_COUNTS 4

// Checkpoint word counts
static const uint32_t ckpt_sizes[] = { 4, 8, 16, 32 };
#define N_CKPT_SIZES 4

// ---------------------------------------------------------------------------
// Buffers (256 bytes each, reuses same footprint as cmd_mem_benchmark)
// ---------------------------------------------------------------------------

static uint32_t perf_buf_a[64];
static uint32_t perf_buf_b[64];

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Trivial noinline function for call overhead measurement
static void __attribute__((noinline)) trivial_func(void)
{
    __asm__ volatile ("" ::: "memory");
}

// Fill buffer with deterministic pattern
static void fill_pattern(uint32_t *buf, uint32_t n_words)
{
    for (uint32_t i = 0; i < n_words; i++)
        buf[i] = 0xA5A5A5A5 ^ i;
}

// ---------------------------------------------------------------------------
// Category A: ATOMiK Core Ops
// ---------------------------------------------------------------------------

static void bench_atomik_ops(void)
{
    uint32_t c0, c1;
    volatile uint32_t val;

    for (int iter = 1; iter <= ATOMIK_OPS_ITER; iter++) {
        // atomik_load
        atomik_init(0);
        c0 = cycles();
        atomik_load(0, 0xDEADBEEF);
        c1 = cycles();
        mini_printf("##PERF:test=atomik_load,iter=%d,cycles=%u\n", iter, c1 - c0);

        // atomik_accum
        c0 = cycles();
        atomik_accumulate(0, 0x12345678);
        c1 = cycles();
        mini_printf("##PERF:test=atomik_accum,iter=%d,cycles=%u\n", iter, c1 - c0);

        // atomik_read
        c0 = cycles();
        val = atomik_state(0);
        c1 = cycles();
        (void)val;
        mini_printf("##PERF:test=atomik_read,iter=%d,cycles=%u\n", iter, c1 - c0);

        // atomik_roundtrip (load + accumulate + read)
        atomik_init(0);
        c0 = cycles();
        atomik_load(0, 0xAAAAAAAA);
        atomik_accumulate(0, 0x55555555);
        val = atomik_state(0);
        c1 = cycles();
        mini_printf("##PERF:test=atomik_roundtrip,iter=%d,cycles=%u\n", iter, c1 - c0);

        // mmio_write (raw register write)
        c0 = cycles();
        ATOMIK_REG(0, ATOMIK_OFF_ACCUM) = 0;
        c1 = cycles();
        mini_printf("##PERF:test=mmio_write,iter=%d,cycles=%u\n", iter, c1 - c0);

        // mmio_read (raw register read)
        c0 = cycles();
        val = ATOMIK_REG(0, ATOMIK_OFF_STATE);
        c1 = cycles();
        mini_printf("##PERF:test=mmio_read,iter=%d,cycles=%u\n", iter, c1 - c0);
    }
}

// ---------------------------------------------------------------------------
// Category B: Memory Operations (size sweep)
// ---------------------------------------------------------------------------

static void bench_memory_ops(void)
{
    uint32_t c0, c1;
    uint32_t fp;

    fill_pattern(perf_buf_a, 64);

    for (int si = 0; si < N_MEM_SIZES; si++) {
        uint32_t sz = mem_sizes[si];
        uint32_t n_words = sz / 4;

        for (int iter = 1; iter <= MEMORY_OPS_ITER; iter++) {
            // memcpy_sw
            c0 = cycles();
            sw_memcpy(perf_buf_b, perf_buf_a, sz);
            c1 = cycles();
            mini_printf("##PERF:test=memcpy_sw,size=%u,iter=%d,cycles=%u\n",
                        sz, iter, c1 - c0);

            // memcpy_atomik
            c0 = cycles();
            atomik_memcpy_tracked(perf_buf_b, perf_buf_a, sz, &fp);
            c1 = cycles();
            mini_printf("##PERF:test=memcpy_atomik,size=%u,iter=%d,cycles=%u\n",
                        sz, iter, c1 - c0);

            // memset_sw
            c0 = cycles();
            sw_memset(perf_buf_b, 0x42, sz);
            c1 = cycles();
            mini_printf("##PERF:test=memset_sw,size=%u,iter=%d,cycles=%u\n",
                        sz, iter, c1 - c0);

            // memset_atomik
            c0 = cycles();
            atomik_memset_verified(perf_buf_b, 0x42, sz, &fp);
            c1 = cycles();
            mini_printf("##PERF:test=memset_atomik,size=%u,iter=%d,cycles=%u\n",
                        sz, iter, c1 - c0);

            // fingerprint
            c0 = cycles();
            fp = atomik_fingerprint(0, perf_buf_a, n_words);
            c1 = cycles();
            mini_printf("##PERF:test=fingerprint,size=%u,iter=%d,cycles=%u\n",
                        sz, iter, c1 - c0);

            // change_detect (no change case)
            uint32_t saved_fp = atomik_fingerprint(0, perf_buf_a, n_words);
            c0 = cycles();
            atomik_region_changed(perf_buf_a, n_words, saved_fp);
            c1 = cycles();
            mini_printf("##PERF:test=change_detect,size=%u,iter=%d,cycles=%u\n",
                        sz, iter, c1 - c0);

            // memcmp_sw
            sw_memcpy(perf_buf_b, perf_buf_a, sz);
            c0 = cycles();
            sw_memcmp(perf_buf_a, perf_buf_b, sz);
            c1 = cycles();
            mini_printf("##PERF:test=memcmp_sw,size=%u,iter=%d,cycles=%u\n",
                        sz, iter, c1 - c0);
        }
    }
}

// ---------------------------------------------------------------------------
// Category C: Burst & Scaling
// ---------------------------------------------------------------------------

static void bench_burst_scaling(void)
{
    uint32_t c0, c1;

    // Burst accumulates
    for (int bi = 0; bi < N_BURST_COUNTS; bi++) {
        uint32_t count = burst_counts[bi];

        for (int iter = 1; iter <= BURST_ITER; iter++) {
            atomik_init(0);
            atomik_load(0, 0);
            c0 = cycles();
            for (uint32_t j = 0; j < count; j++)
                atomik_accumulate(0, j);
            c1 = cycles();
            mini_printf("##PERF:test=burst_accum,count=%u,iter=%d,cycles=%u\n",
                        count, iter, c1 - c0);
        }
    }

    // Checkpoint create (fingerprint at various word counts)
    fill_pattern(perf_buf_a, 64);

    for (int ci = 0; ci < N_CKPT_SIZES; ci++) {
        uint32_t n_words = ckpt_sizes[ci];

        for (int iter = 1; iter <= BURST_ITER; iter++) {
            c0 = cycles();
            volatile uint32_t fp = atomik_fingerprint(0, perf_buf_a, n_words);
            c1 = cycles();
            (void)fp;
            mini_printf("##PERF:test=checkpoint_create,count=%u,iter=%d,cycles=%u\n",
                        n_words, iter, c1 - c0);
        }
    }

    // Checkpoint verify
    for (int ci = 0; ci < N_CKPT_SIZES; ci++) {
        uint32_t n_words = ckpt_sizes[ci];
        uint32_t expected = atomik_fingerprint(0, perf_buf_a, n_words);

        for (int iter = 1; iter <= BURST_ITER; iter++) {
            c0 = cycles();
            volatile int ok = atomik_verify(0, perf_buf_a, n_words, expected);
            c1 = cycles();
            (void)ok;
            mini_printf("##PERF:test=checkpoint_verify,count=%u,iter=%d,cycles=%u\n",
                        n_words, iter, c1 - c0);
        }
    }
}

// ---------------------------------------------------------------------------
// Category D: CPU Baselines
// ---------------------------------------------------------------------------

static void bench_cpu_baselines(void)
{
    uint32_t c0, c1;

    for (int iter = 1; iter <= CPU_ITER; iter++) {
        // loop_overhead (volatile counter prevents optimization)
        c0 = cycles();
        for (volatile int j = 0; j < 1000; j++) { }
        c1 = cycles();
        mini_printf("##PERF:test=loop_overhead,count=1000,iter=%d,cycles=%u\n",
                    iter, c1 - c0);

        // func_call (noinline function call overhead)
        c0 = cycles();
        for (int j = 0; j < 1000; j++)
            trivial_func();
        c1 = cycles();
        mini_printf("##PERF:test=func_call,count=1000,iter=%d,cycles=%u\n",
                    iter, c1 - c0);

        // xor_bench (matches existing B command)
        uint32_t x = 0xDEADBEEF;
        c0 = cycles();
        for (int j = 0; j < 1000; j++) {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
        }
        c1 = cycles();
        mini_printf("##PERF:test=xor_bench,count=1000,iter=%d,cycles=%u,chksum=0x%08x\n",
                    iter, c1 - c0, x);
    }
}

// ---------------------------------------------------------------------------
// Main entry point
// ---------------------------------------------------------------------------

void cmd_perf_suite(void)
{
    print("\n##PERF_START\n");
    mini_printf("##META:suite=atomik_hw,cpu_khz=25175,atomik_khz=81000,banks=1\n");

    bench_atomik_ops();
    bench_memory_ops();
    bench_burst_scaling();
    bench_cpu_baselines();

    print("##PERF_END\n");
}
