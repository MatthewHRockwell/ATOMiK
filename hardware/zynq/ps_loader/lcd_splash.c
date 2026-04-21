/*
 * ATOMiK SPI LCD Splash — ST7789V 320×172 via /dev/spidev + GPIO
 *
 * Drives the on-board 1.47" LCD through the PS SPI0 controller.
 * Uses spidev for data transfer and sysfs GPIO for DC/RST pins.
 *
 * ST7789V config (from factory device tree):
 *   SPI0 CS0, mode 3 (CPOL=1 CPHA=1), 32 MHz, 8-bit
 *   DC = GPIO 59 (EMIO[5]), RST = GPIO 61 (EMIO[7])
 *   172×320 native, rotated 270° → 320×172 landscape
 *
 * Build:
 *     riscv64-linux-gnu-gcc -O2 -static lcd_splash.c -o lcd_splash
 * ========================================================================= */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

/* Display dimensions (after 270° rotation) */
#define LCD_W  320
#define LCD_H  172

/* GPIO pins (Zynq PS GPIO numbering) */
#define GPIO_DC   59   /* EMIO[5] — Data/Command */
#define GPIO_RST  61   /* EMIO[7] — Reset (active-low) */

/* Colors (RGB565) */
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_BLUE    0x44BF   /* ATOMiK blue ~#4488FF in RGB565 */
#define C_DKBLUE  0x1083   /* dark navy ~#080C14 */
#define C_GREEN   0x27E9   /* #44FF88 approx */
#define C_GRAY    0xA514

static int spi_fd;

/* ── GPIO helpers ────────────────────────────────────────────────────── */
static void gpio_export(int pin) {
    char buf[64];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) return;
    int n = snprintf(buf, sizeof(buf), "%d", pin);
    write(fd, buf, n);
    close(fd);
    usleep(100000); /* wait for sysfs to create the node */
}

static void gpio_direction(int pin, const char *dir) {
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    int fd = open(path, O_WRONLY);
    if (fd < 0) return;
    write(fd, dir, strlen(dir));
    close(fd);
}

static void gpio_set(int pin, int val) {
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    int fd = open(path, O_WRONLY);
    if (fd < 0) return;
    write(fd, val ? "1" : "0", 1);
    close(fd);
}

/* ── SPI helpers ─────────────────────────────────────────────────────── */
static void spi_write(const uint8_t *data, int len) {
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)data,
        .len = len,
        .speed_hz = 32000000,
        .bits_per_word = 8,
    };
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

static void lcd_cmd(uint8_t cmd) {
    gpio_set(GPIO_DC, 0);  /* command mode */
    spi_write(&cmd, 1);
}

static void lcd_data(const uint8_t *data, int len) {
    gpio_set(GPIO_DC, 1);  /* data mode */
    spi_write(data, len);
}

static void lcd_data8(uint8_t val) {
    gpio_set(GPIO_DC, 1);
    spi_write(&val, 1);
}

/* ── ST7789V init sequence ───────────────────────────────────────────── */
static void lcd_reset(void) {
    gpio_set(GPIO_RST, 1);
    usleep(50000);
    gpio_set(GPIO_RST, 0);
    usleep(50000);
    gpio_set(GPIO_RST, 1);
    usleep(150000);
}

static void lcd_init(void) {
    lcd_reset();

    lcd_cmd(0x11);  /* Sleep out */
    usleep(120000);

    lcd_cmd(0x36);  /* Memory access control — rotation 270° */
    lcd_data8(0x60); /* MY=0, MX=1, MV=1 → 270° landscape */

    lcd_cmd(0x3A);  /* Pixel format */
    lcd_data8(0x05); /* 16-bit RGB565 */

    lcd_cmd(0x21);  /* Inversion on (ST7789V needs this for correct colors) */

    lcd_cmd(0x13);  /* Normal display mode */
    lcd_cmd(0x29);  /* Display on */
    usleep(50000);
}

/* ── Drawing primitives ──────────────────────────────────────────────── */
static void lcd_set_window(int x0, int y0, int x1, int y1) {
    /* ST7789V column/row offsets depend on rotation and panel size.
     * For 172×320 panel with MADCTL=0x60 (270° rotation):
     * X maps to rows (offset may be needed), Y maps to columns.
     * Factory image uses offsets — try 34 for the 172-wide dimension. */
    uint8_t ca[4] = {(x0 >> 8), x0 & 0xFF, (x1 >> 8), x1 & 0xFF};
    uint8_t ra[4] = {((y0 + 34) >> 8), (y0 + 34) & 0xFF,
                     ((y1 + 34) >> 8), (y1 + 34) & 0xFF};

    lcd_cmd(0x2A);  /* Column address set */
    lcd_data(ca, 4);
    lcd_cmd(0x2B);  /* Row address set */
    lcd_data(ra, 4);
    lcd_cmd(0x2C);  /* Memory write */
}

static void lcd_fill(int x, int y, int w, int h, uint16_t color) {
    lcd_set_window(x, y, x + w - 1, y + h - 1);
    /* Send pixel data in chunks */
    uint8_t buf[640]; /* 320 pixels per chunk */
    for (int i = 0; i < 640; i += 2) {
        buf[i]     = color >> 8;
        buf[i + 1] = color & 0xFF;
    }
    int total = w * h;
    gpio_set(GPIO_DC, 1);
    while (total > 0) {
        int chunk = total > 320 ? 320 : total;
        spi_write(buf, chunk * 2);
        total -= chunk;
    }
}

/* Minimal 5×7 font for splash text (uppercase + digits + space) */
static const uint8_t font5x7[][5] = {
    [' '-' '] = {0x00,0x00,0x00,0x00,0x00},
    ['!'-' '] = {0x00,0x00,0x5F,0x00,0x00},
    ['('-' '] = {0x00,0x1C,0x22,0x41,0x00},
    [')'-' '] = {0x00,0x41,0x22,0x1C,0x00},
    ['+'-' '] = {0x08,0x08,0x3E,0x08,0x08},
    ['-'-' '] = {0x08,0x08,0x08,0x08,0x08},
    ['.'-' '] = {0x00,0x60,0x60,0x00,0x00},
    ['0'-' '] = {0x3E,0x51,0x49,0x45,0x3E},
    ['1'-' '] = {0x00,0x42,0x7F,0x40,0x00},
    ['2'-' '] = {0x42,0x61,0x51,0x49,0x46},
    ['3'-' '] = {0x21,0x41,0x45,0x4B,0x31},
    ['4'-' '] = {0x18,0x14,0x12,0x7F,0x10},
    ['5'-' '] = {0x27,0x45,0x45,0x45,0x39},
    ['6'-' '] = {0x3C,0x4A,0x49,0x49,0x30},
    ['7'-' '] = {0x01,0x71,0x09,0x05,0x03},
    ['8'-' '] = {0x36,0x49,0x49,0x49,0x36},
    ['9'-' '] = {0x06,0x49,0x49,0x29,0x1E},
    ['='-' '] = {0x14,0x14,0x14,0x14,0x14},
    ['A'-' '] = {0x7E,0x11,0x11,0x11,0x7E},
    ['B'-' '] = {0x7F,0x49,0x49,0x49,0x36},
    ['C'-' '] = {0x3E,0x41,0x41,0x41,0x22},
    ['D'-' '] = {0x7F,0x41,0x41,0x22,0x1C},
    ['E'-' '] = {0x7F,0x49,0x49,0x49,0x41},
    ['F'-' '] = {0x7F,0x09,0x09,0x09,0x01},
    ['G'-' '] = {0x3E,0x41,0x49,0x49,0x7A},
    ['H'-' '] = {0x7F,0x08,0x08,0x08,0x7F},
    ['I'-' '] = {0x00,0x41,0x7F,0x41,0x00},
    ['K'-' '] = {0x7F,0x08,0x14,0x22,0x41},
    ['L'-' '] = {0x7F,0x40,0x40,0x40,0x40},
    ['M'-' '] = {0x7F,0x02,0x0C,0x02,0x7F},
    ['N'-' '] = {0x7F,0x04,0x08,0x10,0x7F},
    ['O'-' '] = {0x3E,0x41,0x41,0x41,0x3E},
    ['P'-' '] = {0x7F,0x09,0x09,0x09,0x06},
    ['R'-' '] = {0x7F,0x09,0x19,0x29,0x46},
    ['S'-' '] = {0x46,0x49,0x49,0x49,0x31},
    ['T'-' '] = {0x01,0x01,0x7F,0x01,0x01},
    ['U'-' '] = {0x3F,0x40,0x40,0x40,0x3F},
    ['V'-' '] = {0x1F,0x20,0x40,0x20,0x1F},
    ['X'-' '] = {0x63,0x14,0x08,0x14,0x63},
    ['Z'-' '] = {0x61,0x51,0x49,0x45,0x43},
    ['a'-' '] = {0x20,0x54,0x54,0x54,0x78},
    ['c'-' '] = {0x38,0x44,0x44,0x44,0x20},
    ['d'-' '] = {0x38,0x44,0x44,0x48,0x7F},
    ['e'-' '] = {0x38,0x54,0x54,0x54,0x18},
    ['i'-' '] = {0x00,0x44,0x7D,0x40,0x00},
    ['l'-' '] = {0x00,0x41,0x7F,0x40,0x00},
    ['m'-' '] = {0x7C,0x04,0x18,0x04,0x78},
    ['n'-' '] = {0x7C,0x08,0x04,0x04,0x78},
    ['o'-' '] = {0x38,0x44,0x44,0x44,0x38},
    ['r'-' '] = {0x7C,0x08,0x04,0x04,0x08},
    ['s'-' '] = {0x48,0x54,0x54,0x54,0x20},
    ['t'-' '] = {0x04,0x3F,0x44,0x40,0x20},
    ['u'-' '] = {0x3C,0x40,0x40,0x20,0x7C},
    ['x'-' '] = {0x44,0x28,0x10,0x28,0x44},
    ['y'-' '] = {0x0C,0x50,0x50,0x50,0x3C},
};

static void lcd_char(int x, int y, char ch, uint16_t fg, uint16_t bg, int scale) {
    int idx = ch - ' ';
    if (idx < 0 || idx >= (int)(sizeof(font5x7)/5)) return;
    for (int col = 0; col < 5; col++) {
        uint8_t bits = font5x7[idx][col];
        for (int row = 0; row < 7; row++) {
            uint16_t c = (bits & (1 << row)) ? fg : bg;
            if (scale == 1) {
                lcd_fill(x + col, y + row, 1, 1, c);
            } else {
                lcd_fill(x + col * scale, y + row * scale, scale, scale, c);
            }
        }
    }
}

static void lcd_string(int x, int y, const char *s, uint16_t fg, uint16_t bg, int scale) {
    while (*s) {
        lcd_char(x, y, *s, fg, bg, scale);
        x += 6 * scale;
        s++;
    }
}

static void lcd_center(int y, const char *s, uint16_t fg, uint16_t bg, int scale) {
    int len = strlen(s);
    int x = (LCD_W - len * 6 * scale) / 2;
    lcd_string(x, y, s, fg, bg, scale);
}

/* ── Splash screen ───────────────────────────────────────────────────── */
static void draw_splash(void) {
    /* Background */
    lcd_fill(0, 0, LCD_W, LCD_H, C_DKBLUE);

    /* Title */
    lcd_center(20, "ATOMiK", C_WHITE, C_DKBLUE, 3);

    /* Subtitle */
    lcd_center(52, "Delta-State Engine", C_BLUE, C_DKBLUE, 1);

    /* Horizontal rule */
    lcd_fill(LCD_W / 2 - 80, 66, 160, 1, C_GRAY);

    /* System info */
    lcd_center(75, "NaxRiscv RV64GC", C_GRAY, C_DKBLUE, 1);
    lcd_center(87, "100 MHz XC7Z020", C_GRAY, C_DKBLUE, 1);

    /* Formula */
    lcd_center(105, "state = init XOR acc", C_GREEN, C_DKBLUE, 1);

    /* Bottom line */
    lcd_center(130, "108 Lean4 theorems", C_GRAY, C_DKBLUE, 1);
    lcd_center(142, "353 SDK tests PASS", C_GRAY, C_DKBLUE, 1);
    lcd_center(156, "Hardware validated", C_GRAY, C_DKBLUE, 1);
}

int main(void) {
    /* Setup GPIO */
    gpio_export(GPIO_DC);
    gpio_export(GPIO_RST);
    gpio_direction(GPIO_DC, "out");
    gpio_direction(GPIO_RST, "out");

    /* Open SPI device */
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        perror("open spidev0.0");
        return 1;
    }

    /* Configure SPI: mode 3, 32 MHz, 8-bit */
    uint8_t mode = SPI_MODE_3;
    uint8_t bits = 8;
    uint32_t speed = 32000000;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    printf("Initializing ST7789V LCD...\n");
    lcd_init();

    printf("Drawing ATOMiK splash...\n");
    draw_splash();

    printf("LCD splash complete (320x172)\n");

    close(spi_fd);
    return 0;
}
