/* Minimal /dev/mem mmap diagnostic — no libc */
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

#define SYS_openat     56
#define SYS_exit_group 94
#define SYS_mmap2      222
#define SYS_msync      227
#define SYS_write      64
#define AT_FDCWD       -100
#define O_RDWR         2
#define O_SYNC         0x101000

void _start(void) {
    /* Step marker: write 0xAA to LED CSR offset at DDR start to show we started */
    /* (Won't work from userspace, but the syscall writes below will) */

    /* Open /dev/mem */
    int fd = (int)syscall4(SYS_openat, AT_FDCWD, (long)"/dev/mem", O_RDWR | O_SYNC, 0);
    if (fd < 0) {
        /* Signal: write 0xE1 (error 1) to stdout */
        char c = '1';
        syscall3(SYS_write, 1, (long)&c, 1);
        syscall1(SYS_exit_group, 1);
        __builtin_unreachable();
    }

    /* Map DDR at 0x5FF00000 (reserved, no-map in DTB) */
    void *p = (void *)syscall6(SYS_mmap2,
        0, 0x1000, 1|2 /*PROT_READ|PROT_WRITE*/, 1 /*MAP_SHARED*/, fd,
        0x5FF00000UL / 4096);
    if ((long)p < 0 || p == (void *)-1) {
        char c = '2';
        syscall3(SYS_write, 1, (long)&c, 1);
        syscall1(SYS_exit_group, 2);
        __builtin_unreachable();
    }

    /* Write known values */
    volatile unsigned int *r = (volatile unsigned int *)p;
    r[0] = 0x41544F4D;  /* "ATOM" */
    r[1] = 0xDEADBEEF;
    r[2] = 0xCAFEBABE;
    r[3] = 0x12345678;

    /* Fence + msync to flush to physical DDR */
    asm volatile("fence rw, rw" ::: "memory");
    syscall3(SYS_msync, (long)p, 0x1000, 4 /*MS_SYNC*/);
    asm volatile("fence rw, rw" ::: "memory");

    /* Signal success */
    char c = '0';
    syscall3(SYS_write, 1, (long)&c, 1);
    syscall1(SYS_exit_group, 0);
    __builtin_unreachable();
}
