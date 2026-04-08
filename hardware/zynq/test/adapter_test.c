/* ATOMiK CFU Adapter — Linux Userspace Test (RV32, nostdlib) */
#include <stdint.h>

/* Raw syscalls for rv32 */
static long sys3(long n, long a, long b, long c) {
    register long a0 asm("a0") = a;
    register long a1 asm("a1") = b;
    register long a2 asm("a2") = c;
    register long a7 asm("a7") = n;
    asm volatile("ecall" : "+r"(a0) : "r"(a1),"r"(a2),"r"(a7) : "memory");
    return a0;
}
static long sys6(long n, long a, long b, long c, long d, long e, long f) {
    register long a0 asm("a0") = a;
    register long a1 asm("a1") = b;
    register long a2 asm("a2") = c;
    register long a3 asm("a3") = d;
    register long a4 asm("a4") = e;
    register long a5 asm("a5") = f;
    register long a7 asm("a7") = n;
    asm volatile("ecall" : "+r"(a0) : "r"(a1),"r"(a2),"r"(a3),"r"(a4),"r"(a5),"r"(a7) : "memory");
    return a0;
}
#define SYS_write 64
#define SYS_exit  93
#define SYS_mmap2 222
#define SYS_open  1024
#define SYS_close 57

static void putstr(const char *s) {
    int n = 0; while (s[n]) n++;
    sys3(SYS_write, 1, (long)s, n);
}
static void puthex32(uint32_t v) {
    char buf[11] = "0x00000000";
    for (int i = 9; i >= 2; i--) { buf[i] = "0123456789ABCDEF"[v & 0xF]; v >>= 4; }
    sys3(SYS_write, 1, (long)buf, 10);
}
static void puthex64(uint64_t v) {
    puthex32((uint32_t)(v >> 32)); puthex32((uint32_t)(v & 0xFFFFFFFF));
}

/* Adapter register map */
#define ADAPTER_BASE 0xF0020000
#define REG_CMD   0x00
#define REG_RS1   0x04
#define REG_RS2   0x08
#define REG_RD    0x0C
#define REG_STAT  0x10

static volatile uint32_t *adapter;

static void wr(int off, uint32_t val) {
    adapter[off/4] = val;
    asm volatile("fence iorw,iorw");
}
static uint32_t rr(int off) {
    asm volatile("fence iorw,iorw");
    return adapter[off/4];
}

static void load64(uint8_t addr, uint64_t val) {
    wr(REG_RS1, addr);
    wr(REG_RS2, (uint32_t)(val & 0xFFFFFFFF));
    wr(REG_CMD, 0);  /* F_LOAD */
    wr(REG_RS2, (uint32_t)(val >> 32));
    wr(REG_CMD, 4);  /* F_LOAD_HI */
}
static void accum64(uint64_t delta) {
    wr(REG_RS1, (uint32_t)(delta & 0xFFFFFFFF));
    wr(REG_CMD, 1);  /* F_ACCUM */
    wr(REG_RS1, (uint32_t)(delta >> 32));
    wr(REG_CMD, 5);  /* F_ACCUM_HI */
}
static uint64_t read64(void) {
    wr(REG_CMD, 2);  /* F_READ */
    uint32_t lo = rr(REG_RD);
    wr(REG_CMD, 6);  /* F_READ_HI */
    uint32_t hi = rr(REG_RD);
    return ((uint64_t)hi << 32) | lo;
}
static uint32_t atomik_status(void) {
    wr(REG_CMD, 7);
    return rr(REG_RD);
}
static void atomik_swap(uint8_t addr) {
    wr(REG_RS1, addr);
    wr(REG_CMD, 3);
}

static int pass_count, fail_count;

static void check(const char *name, uint64_t got, uint64_t expected) {
    if (got == expected) {
        putstr("  PASS: "); putstr(name); putstr("\n");
        pass_count++;
    } else {
        putstr("  FAIL: "); putstr(name);
        putstr(" got="); puthex64(got);
        putstr(" exp="); puthex64(expected);
        putstr("\n");
        fail_count++;
    }
}

void _start(void) {
    /* Open /dev/mem */
    int fd = sys3(SYS_open, (long)"/dev/mem", 2, 0); /* O_RDWR=2 */
    if (fd < 0) {
        putstr("ERROR: open /dev/mem failed\n");
        sys3(SYS_exit, 1, 0, 0);
    }

    /* mmap adapter registers */
    adapter = (volatile uint32_t *)sys6(SYS_mmap2,
        0, 4096, 3, 1, fd, ADAPTER_BASE >> 12);
    /* PROT_READ|PROT_WRITE=3, MAP_SHARED=1, offset in pages */

    if ((long)adapter < 0 || (long)adapter > (long)-4096) {
        putstr("ERROR: mmap failed\n");
        sys3(SYS_exit, 1, 0, 0);
    }

    putstr("=== ATOMiK CFU Adapter Test ===\n");
    putstr("Base: 0xF0020000 | Zynq + VexRiscvSMP\n");

    uint32_t st = rr(REG_STAT);
    putstr("STATUS: "); puthex32(st); putstr("\n\n");

    /* T1: LOAD + READ */
    load64(0, 0xDEADBEEFCAFEBABEULL);
    check("T1 LOAD/READ", read64(), 0xDEADBEEFCAFEBABEULL);

    /* T2: ACCUM */
    accum64(0x1111111111111111ULL);
    check("T2 ACCUM", read64(), 0xDEADBEEFCAFEBABEULL ^ 0x1111111111111111ULL);

    /* T3: Self-inverse */
    accum64(0x1111111111111111ULL);
    check("T3 Self-inverse", read64(), 0xDEADBEEFCAFEBABEULL);

    /* T4: Identity */
    accum64(0);
    check("T4 Identity", read64(), 0xDEADBEEFCAFEBABEULL);

    /* T5: Commutativity */
    load64(0, 0);
    accum64(0xAAAAAAAAAAAAAAAAULL);
    accum64(0x5555555555555555ULL);
    uint64_t ab = read64();
    load64(0, 0);
    accum64(0x5555555555555555ULL);
    accum64(0xAAAAAAAAAAAAAAAAULL);
    uint64_t ba = read64();
    check("T5 Commutativity", ab, ba);

    /* T6: Multi-address */
    load64(0, 0x1111111111111111ULL);
    load64(1, 0x2222222222222222ULL);
    check("T6a Addr 1", read64(), 0x2222222222222222ULL);
    load64(0, 0x1111111111111111ULL);
    check("T6b Addr 0", read64(), 0x1111111111111111ULL);

    /* T7: STATUS */
    load64(0, 0x42);
    uint32_t sw = atomik_status();
    check("T7 STATUS", sw & 0xFFFF, 0x0101);

    /* T8: SWAP */
    load64(5, 0xAAAA0000BBBB0000ULL);
    accum64(0x0000FFFF0000FFFFULL);
    uint64_t expected = 0xAAAA0000BBBB0000ULL ^ 0x0000FFFF0000FFFFULL;
    atomik_swap(5);
    check("T8 SWAP", read64(), expected);
    check("T9 SWAP-clr", read64(), expected);

    putstr("\n");
    /* Print summary */
    char pbuf[4], fbuf[4];
    pbuf[0] = '0' + (pass_count / 10); pbuf[1] = '0' + (pass_count % 10); pbuf[2] = 0;
    fbuf[0] = '0' + (fail_count / 10); fbuf[1] = '0' + (fail_count % 10); fbuf[2] = 0;
    putstr(pbuf); putstr(" PASS, ");
    putstr(fbuf); putstr(" FAIL\n");
    if (fail_count == 0) putstr("ALL TESTS PASSED\n");

    sys3(SYS_close, fd, 0, 0);
    sys3(SYS_exit, fail_count ? 1 : 0, 0, 0);
}
