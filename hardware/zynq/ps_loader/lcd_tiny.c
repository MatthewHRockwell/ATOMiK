/* Minimal LCD splash — no libc, raw syscalls only.
 * ST7789V 320x172 via GPIO bitbang SPI.
 * Pins: SDA=U19, SCL=V18, DC=W13, CS=AA13, RST=AA18, LED=Y13
 * Build: riscv64-linux-gnu-gcc -Os -nostdlib -static -o lcd_tiny lcd_tiny.c
 */
#include <sys/mman.h>

/* Syscall wrappers */
static long syscall3(long n, long a, long b, long c) {
    register long a0 asm("a0") = a;
    register long a1 asm("a1") = b;
    register long a2 asm("a2") = c;
    register long nr asm("a7") = n;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(nr) : "memory");
    return a0;
}
static long syscall6(long n, long a, long b, long c, long d, long e, long f) {
    register long a0 asm("a0") = a;
    register long a1 asm("a1") = b;
    register long a2 asm("a2") = c;
    register long a3 asm("a3") = d;
    register long a4 asm("a4") = e;
    register long a5 asm("a5") = f;
    register long nr asm("a7") = n;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(nr) : "memory");
    return a0;
}
#define SYS_write  64
#define SYS_openat 56
#define SYS_mmap   222
#define SYS_exit   93
#define SYS_nanosleep 101
#define AT_FDCWD   -100

static void msg(const char *s) {
    int n = 0;
    while (s[n]) n++;
    syscall3(SYS_write, 1, (long)s, n);
}

static void msleep(int ms) {
    struct { long sec; long nsec; } ts = { ms / 1000, (ms % 1000) * 1000000L };
    syscall3(SYS_nanosleep, (long)&ts, 0, 0);
}

/* CSR addresses (zlcd_ prefix, sorted after uart) */
#define CSR_BASE  0xF0000000UL
#define CSR_SIZE  0x00100000UL
#define LCD_CLK   0xF0003000UL
#define LCD_CS    0xF0003800UL
#define LCD_DC    0xF0004000UL
#define LCD_LED   0xF0004800UL
#define LCD_MOSI  0xF0005000UL
#define LCD_RST   0xF0005800UL

static volatile unsigned int *csr;

static void wr(unsigned long a, unsigned int v) { csr[(a - CSR_BASE) / 4] = v; }
static void pin_mosi(int v) { wr(LCD_MOSI, v & 1); }
static void pin_clk(int v)  { wr(LCD_CLK, v & 1); }
static void pin_cs(int v)   { wr(LCD_CS, v & 1); }
static void pin_dc(int v)   { wr(LCD_DC, v & 1); }
static void pin_rst(int v)  { wr(LCD_RST, v & 1); }
static void pin_led(int v)  { wr(LCD_LED, v & 1); }

static void spi_byte(unsigned char val) {
    for (int i = 7; i >= 0; i--) {
        pin_mosi((val >> i) & 1);
        pin_clk(0);
        pin_clk(1);
    }
}

static void lcd_cmd(unsigned char cmd) {
    pin_dc(0); pin_cs(0);
    spi_byte(cmd);
    pin_cs(1);
}

static void lcd_data8(unsigned char val) {
    pin_dc(1); pin_cs(0);
    spi_byte(val);
    pin_cs(1);
}

static void lcd_fill(int x0, int y0, int w, int h, unsigned short c) {
    /* Column address set */
    lcd_cmd(0x2A);
    pin_dc(1); pin_cs(0);
    spi_byte(x0 >> 8); spi_byte(x0);
    spi_byte((x0+w-1) >> 8); spi_byte(x0+w-1);
    pin_cs(1);
    /* Row address set (with 34 offset for 172-row panel) */
    lcd_cmd(0x2B);
    pin_dc(1); pin_cs(0);
    spi_byte((y0+34) >> 8); spi_byte(y0+34);
    spi_byte((y0+h-1+34) >> 8); spi_byte(y0+h-1+34);
    pin_cs(1);
    /* Memory write */
    lcd_cmd(0x2C);
    pin_dc(1); pin_cs(0);
    for (int i = 0; i < w * h; i++) {
        spi_byte(c >> 8);
        spi_byte(c & 0xFF);
    }
    pin_cs(1);
}

void _start(void) {
    int fd = syscall3(SYS_openat, AT_FDCWD, (long)"/dev/mem", 2 /* O_RDWR */);
    if (fd < 0) { msg("open fail\n"); syscall3(SYS_exit, 1, 0, 0); }

    csr = (volatile unsigned int *)syscall6(SYS_mmap,
        0, CSR_SIZE, 3 /* PROT_READ|WRITE */, 1 /* MAP_SHARED */, fd, CSR_BASE);
    if ((long)csr < 0) { msg("mmap fail\n"); syscall3(SYS_exit, 1, 0, 0); }

    msg("LED on\n");
    pin_led(1);

    msg("Reset\n");
    pin_rst(1); pin_cs(1);
    msleep(50);
    pin_rst(0);
    msleep(50);
    pin_rst(1);
    msleep(150);

    msg("Init\n");
    lcd_cmd(0x11); /* Sleep out */
    msleep(120);
    lcd_cmd(0x36); lcd_data8(0x60); /* Rotation 270 */
    lcd_cmd(0x3A); lcd_data8(0x05); /* 16-bit color */
    lcd_cmd(0x21); /* Inversion on */
    lcd_cmd(0x13); /* Normal mode */
    lcd_cmd(0x29); /* Display on */
    msleep(50);

    /* ATOMiK blue: rgb(0, 180, 255) = RGB565 0x05BF */
    unsigned short blue = (0 >> 3 << 11) | (180 >> 2 << 5) | (255 >> 3);
    /* Dark background */
    unsigned short dark = (0 >> 3 << 11) | (0 >> 2 << 5) | (40 >> 3);
    /* White */
    unsigned short white = 0xFFFF;

    msg("Fill\n");
    lcd_fill(0, 0, 320, 172, dark);

    msg("Bars\n");
    /* Top accent bar */
    lcd_fill(0, 0, 320, 4, blue);
    /* Center block */
    lcd_fill(80, 60, 160, 50, blue);
    /* Bottom bar */
    lcd_fill(0, 168, 320, 4, blue);

    msg("DONE\n");
    syscall3(SYS_exit, 0, 0, 0);
}
