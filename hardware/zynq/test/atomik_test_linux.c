/*
 * atomik_test_linux.c — ATOMiK MMIO smoke test from Linux userspace
 *
 * Self-contained: no libc, uses raw Linux syscalls.
 * Results written to DDR at 0x50000000 (readable via JTAG).
 * Console output is best-effort (SBI relay is slow on VexRiscv SMP).
 *
 * Build:
 *   riscv64-unknown-elf-gcc -march=rv32ima -mabi=ilp32 -nostdlib -static \
 *     -O2 -fno-builtin -o atomik_test atomik_test_linux.c
 *
 * Results format at DDR 0x50000000 (= PS DDR 0x10100000):
 *   +0x00  magic       0x41544F4D  ("ATOM")
 *   +0x04  step        Last completed step (0-99)
 *   +0x08  pass_count  Number of PASS
 *   +0x0C  fail_count  Number of FAIL
 *   +0x10  error_code  0=OK, 1=open fail, 2=DDR mmap fail, 3=CSR mmap fail
 *   +0x14  test_data[0..N]  Per-test: {got, expected} pairs
 */

/* --- Syscall wrappers --- */

static long syscall1(long n, long a) {
    register long a7 asm("a7") = n;
    register long a0 asm("a0") = a;
    asm volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
    return a0;
}
static long syscall3(long n, long a, long b, long c) {
    register long a7 asm("a7") = n;
    register long a0 asm("a0") = a;
    register long a1 asm("a1") = b;
    register long a2 asm("a2") = c;
    asm volatile("ecall" : "+r"(a0) : "r"(a7), "r"(a1), "r"(a2) : "memory");
    return a0;
}
static long syscall4(long n, long a, long b, long c, long d) {
    register long a7 asm("a7") = n;
    register long a0 asm("a0") = a;
    register long a1 asm("a1") = b;
    register long a2 asm("a2") = c;
    register long a3 asm("a3") = d;
    asm volatile("ecall" : "+r"(a0) : "r"(a7), "r"(a1), "r"(a2), "r"(a3) : "memory");
    return a0;
}
static long syscall6(long n, long a, long b, long c, long d, long e, long f) {
    register long a7 asm("a7") = n;
    register long a0 asm("a0") = a;
    register long a1 asm("a1") = b;
    register long a2 asm("a2") = c;
    register long a3 asm("a3") = d;
    register long a4 asm("a4") = e;
    register long a5 asm("a5") = f;
    asm volatile("ecall" : "+r"(a0)
                 : "r"(a7), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5)
                 : "memory");
    return a0;
}

#define SYS_write       64
#define SYS_openat      56
#define SYS_close       57
#define SYS_exit_group  94
#define SYS_mmap2       222
#define SYS_msync       227

#define MS_SYNC         4

#define AT_FDCWD        -100
#define O_RDWR          2
#define O_SYNC          0x101000
#define PROT_READ       1
#define PROT_WRITE      2
#define MAP_SHARED      1
#define MAP_FAILED      ((void *)-1)
#define PAGE_SIZE       4096

static void sys_exit(int code) { syscall1(SYS_exit_group, code); __builtin_unreachable(); }
static void sys_write_fd(int fd, const void *buf, int len) { syscall3(SYS_write, fd, (long)buf, len); }
static int sys_openat(const char *path, int flags) { return syscall4(SYS_openat, AT_FDCWD, (long)path, flags, 0); }
static int sys_close(int fd) { return syscall1(SYS_close, fd); }
static void *sys_mmap(unsigned long addr, unsigned long len, int prot, int flags, int fd, unsigned long pgoff) {
    return (void *)syscall6(SYS_mmap2, addr, len, prot, flags, fd, pgoff);
}

/* --- Output (best-effort via SBI relay) --- */

static int __attribute__((noinline)) slen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}
static void emit(const char *s) { sys_write_fd(1, s, slen(s)); }

/* --- Result buffer in DDR --- */

#define RESULT_ADDR     0x5FF00000UL  /* VexRiscv address (reserved in DTB) */
#define RESULT_SIZE     0x1000
#define RESULT_MAGIC    0x41544F4D    /* "ATOM" */

struct result_buf {
    unsigned int magic;
    unsigned int step;
    unsigned int pass_count;
    unsigned int fail_count;
    unsigned int error_code;
    unsigned int test_pairs[64];  /* {got, expected} × 32 tests */
};

static volatile struct result_buf *res;
static int pair_idx;

static void res_step(unsigned int s) { if (res) res->step = s; }
static void res_error(unsigned int e) { if (res) res->error_code = e; }

/* --- ATOMiK registers --- */

#define ATOMIK_BASE     0xF0000000UL
#define ATOMIK_SIZE     0x1000

#define REG_LOAD_ADDR     0x00
#define REG_LOAD_DATA_LO  0x04
#define REG_LOAD_DATA_HI  0x08
#define REG_ACCUM_LO      0x0C
#define REG_ACCUM_HI      0x10
#define REG_SWAP_ADDR     0x14
#define REG_CONFIG         0x18
#define REG_STATE_LO       0x1C
#define REG_STATE_HI       0x20
#define REG_STATUS         0x24

static volatile unsigned int *csr;

static unsigned int csr_rd(int off) { return csr[off / 4]; }
static void csr_wr(int off, unsigned int v) { csr[off / 4] = v; }

static void atomik_load(int addr, unsigned int lo, unsigned int hi) {
    csr_wr(REG_LOAD_ADDR, addr);
    csr_wr(REG_LOAD_DATA_LO, lo);
    csr_wr(REG_LOAD_DATA_HI, hi);
    /* Bus round-trip: HI write triggers LOAD on Wishbone.
     * A fence alone is insufficient — need a read to drain the bus pipeline
     * before STATE_LO/HI return valid data (especially on address change). */
    asm volatile("fence iorw, iorw" ::: "memory");
    (void)csr_rd(REG_STATUS);
}

static void atomik_accum(unsigned int lo, unsigned int hi) {
    csr_wr(REG_ACCUM_LO, lo);
    csr_wr(REG_ACCUM_HI, hi);
    asm volatile("fence iorw, iorw" ::: "memory");
}

static void check(unsigned int got, unsigned int expect) {
    if (res) {
        res->test_pairs[pair_idx++] = got;
        res->test_pairs[pair_idx++] = expect;
        if (got == expect) res->pass_count++;
        else               res->fail_count++;
    }
}

/* --- Main --- */

void _start(void) {
    int fd;

    emit("ATOMiK test start\n");

    /* Open /dev/mem */
    fd = sys_openat("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        emit("E:open\n");
        sys_exit(1);
    }

    /* Map DDR result buffer */
    res = sys_mmap(0, RESULT_SIZE, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, RESULT_ADDR / PAGE_SIZE);
    if (res == MAP_FAILED) {
        emit("E:ddr_mmap\n");
        sys_close(fd);
        sys_exit(2);
    }

    /* Initialize result buffer */
    res->magic = RESULT_MAGIC;
    res->step = 0;
    res->pass_count = 0;
    res->fail_count = 0;
    res->error_code = 0;
    pair_idx = 0;

    emit("DDR OK\n");
    res_step(1);

    /* Map ATOMiK CSRs */
    csr = sys_mmap(0, ATOMIK_SIZE, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, ATOMIK_BASE / PAGE_SIZE);
    if (csr == MAP_FAILED) {
        emit("E:csr_mmap\n");
        res_error(3);
        res_step(99);
        sys_close(fd);
        sys_exit(3);
    }

    emit("CSR OK\n");
    res_step(2);

    /* T1: Status register */
    {
        unsigned int s = csr_rd(REG_STATUS);
        check((s >> 16) & 0xFF, 2);  /* version */
        check((s >> 8) & 0xFF, 1);   /* n_banks */
    }
    res_step(10);

    /* T2: Config */
    check(csr_rd(REG_CONFIG) & 1, 1);
    res_step(20);

    /* T3: LOAD */
    atomik_load(0, 0xDEADBEEF, 0xCAFEBABE);
    check(csr_rd(REG_STATE_LO), 0xDEADBEEF);
    check(csr_rd(REG_STATE_HI), 0xCAFEBABE);
    res_step(30);

    /* T4: ACCUM */
    atomik_accum(0x11111111, 0x00000000);
    check(csr_rd(REG_STATE_LO), 0xDEADBEEF ^ 0x11111111);
    check(csr_rd(REG_STATE_HI), 0xCAFEBABE);
    res_step(40);

    /* T5: XOR self-inverse */
    atomik_accum(0x11111111, 0x00000000);
    check(csr_rd(REG_STATE_LO), 0xDEADBEEF);
    check(csr_rd(REG_STATE_HI), 0xCAFEBABE);
    res_step(50);

    /* T6: Identity */
    atomik_accum(0x00000000, 0x00000000);
    check(csr_rd(REG_STATE_LO), 0xDEADBEEF);
    check(csr_rd(REG_STATE_HI), 0xCAFEBABE);
    res_step(60);

    /* T7: Commutativity */
    atomik_load(0, 0x00000000, 0x00000000);
    atomik_accum(0xAAAAAAAA, 0x55555555);
    atomik_accum(0x55555555, 0xAAAAAAAA);
    check(csr_rd(REG_STATE_LO), 0xFFFFFFFF);
    check(csr_rd(REG_STATE_HI), 0xFFFFFFFF);
    res_step(70);

    /* T8: acc_zero */
    check(csr_rd(REG_STATUS) & 1, 0);
    res_step(80);

    /* T9: Different address */
    atomik_load(42, 0x9ABCDEF0, 0x12345678);
    check(csr_rd(REG_STATE_LO), 0x9ABCDEF0);
    check(csr_rd(REG_STATE_HI), 0x12345678);
    res_step(90);

    /* Done */
    emit("DONE\n");
    res_step(99);

    /* Flush result buffer to physical DDR (for JTAG readback) */
    syscall3(SYS_msync, (long)res, RESULT_SIZE, MS_SYNC);
    /* Also try fence to ensure stores complete */
    asm volatile("fence rw, rw" ::: "memory");

    sys_close(fd);
    sys_exit(res->fail_count ? 1 : 0);
}
