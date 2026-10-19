/* aprobe.c — directed characterization of the adapter quirk found 2026-06-09:
 * LOAD(addr,0) + ACCUM(0x340032afbec9d54f) + READ returns 0 on the board,
 * while other values through the IDENTICAL code path verify fine.
 *
 * Tests, each printed as PASS/FAIL with expect/got:
 *   A. the exact failing value at several addresses
 *   B. its halves in isolation (lo with benign hi, hi with benign lo)
 *   C. top-3-bit patterns of each half (funct3-aliasing hypothesis: RS1/RS2
 *      data bits [31:29] colliding with command decode)
 *   D. the failing value twice in a row (transient vs persistent)
 *   E. the real first 13 telemetry deltas in sequence (the failing set)
 *
 *   riscv64-linux-gnu-gcc -O2 aprobe.c -o aprobe
 */
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define ADAPTER_BASE 0xF0020000UL
#define A_CMD 0
#define A_RS1 1
#define A_RS2 2
#define A_RD  3
#define A_STATUS 4
#define FN(f) ((uint32_t)(f))   /* funct3 in dat_w[2:0] */

static volatile uint32_t *g_adp = 0;
static inline void mfence(void) {
#if defined(__riscv)
    __asm__ volatile("fence iorw,iorw" ::: "memory");
#else
    __asm__ volatile("" ::: "memory");
#endif
}
static inline void wr(int off, uint32_t v) { g_adp[off] = v; mfence(); }
static inline uint32_t rd(int off) { mfence(); return g_adp[off]; }
static void wait_ready(void) {
    for (int i = 0; i < 20000; i++) { if (!(rd(A_STATUS) & 1u)) return; }
}
static void adp_cmd(uint32_t f) { wr(A_CMD, FN(f)); wait_ready(); }
static void adp_load(uint32_t addr, uint64_t init) {
    wr(A_RS1, addr); wr(A_RS2, (uint32_t)init);        adp_cmd(0);
                     wr(A_RS2, (uint32_t)(init >> 32)); adp_cmd(4);
}
static void adp_accum(uint64_t d) {
    wr(A_RS1, (uint32_t)d);         adp_cmd(1);
    wr(A_RS1, (uint32_t)(d >> 32)); adp_cmd(5);
}
static uint64_t adp_read(void) {
    adp_cmd(2); (void)rd(A_RD); uint32_t lo = rd(A_RD);
    adp_cmd(6); (void)rd(A_RD); uint32_t hi = rd(A_RD);
    return ((uint64_t)hi << 32) | lo;
}
static uint64_t mix(uint64_t x) { x ^= x << 13; x ^= x >> 7; x ^= x << 17; return x; }

static int tcase(const char *name, uint32_t addr, uint64_t val) {
    adp_load(addr, 0);
    adp_accum(val);
    uint64_t hw = adp_read();
    int ok = (hw == val);
    printf("%-26s addr=%-3u val=0x%016llx got=0x%016llx %s\n",
           name, addr, (unsigned long long)val, (unsigned long long)hw,
           ok ? "PASS" : "FAIL");
    return ok;
}

int main(void) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { printf("no /dev/mem\n"); return 1; }
    void *m = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)ADAPTER_BASE);
    if (m == MAP_FAILED) { printf("mmap fail\n"); return 1; }
    g_adp = (volatile uint32_t *)m;

    /* prime + throwaway, same as aworkload */
    for (int i = 0; i < 4; i++) (void)rd(A_STATUS);
    adp_load(0, 0); adp_accum(0x1ULL); (void)adp_read();

    const uint64_t BAD = 0x340032afbec9d54fULL;
    int fails = 0;

    puts("== A: exact failing value at several addresses ==");
    fails += !tcase("A.addr0",   0,   BAD);
    fails += !tcase("A.addr1",   1,   BAD);
    fails += !tcase("A.addr21",  21,  BAD);
    fails += !tcase("A.addr100", 100, BAD);

    puts("== B: halves in isolation ==");
    fails += !tcase("B.lo_only",  21, 0x00000001bec9d54fULL);
    fails += !tcase("B.hi_only",  21, 0x340032af00000001ULL);
    fails += !tcase("B.lo_alone", 21, 0x00000000bec9d54fULL);
    fails += !tcase("B.hi_alone", 21, 0x340032af00000000ULL);

    puts("== C: top-3-bit patterns (funct3-alias hypothesis) ==");
    for (uint32_t p = 0; p < 8; p++) {
        char nm[24]; snprintf(nm, sizeof nm, "C.lo_top%u", p);
        fails += !tcase(nm, 21, ((uint64_t)0x01010101u << 32) | ((uint64_t)p << 29) | 0x1234567u);
    }
    for (uint32_t p = 0; p < 8; p++) {
        char nm[24]; snprintf(nm, sizeof nm, "C.hi_top%u", p);
        fails += !tcase(nm, 21, (((uint64_t)p << 29 | 0x1234567u) << 32) | 0x01010101u);
    }

    puts("== D: failing value repeated ==");
    fails += !tcase("D.first",  21, BAD);
    fails += !tcase("D.second", 21, BAD);

    puts("== E: real first 13 telemetry deltas in their sequence ==");
    {
        uint64_t seed = 0xD1A57A7Eu;
        int printed = 0;
        for (int i = 0; i < 256 && printed < 13; i++) {
            int hit = (int)(mix(seed ^ 0xA5A5u ^ (uint64_t)i) % 100u) < 5;
            if (!hit) continue;
            uint64_t d = mix(seed ^ 0x1234u ^ (uint64_t)i) | 1;
            char nm[24]; snprintf(nm, sizeof nm, "E.region%d", i);
            fails += !tcase(nm, (uint32_t)i, d);
            printed++;
        }
    }

    printf("== TOTAL FAILS: %d ==\n", fails);
    return 0;
}
