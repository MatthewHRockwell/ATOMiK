/*
 * ATOMiK SPI LCD Splash — ST7789V 320×172 via 6 separate GPIO CSRs
 *
 * Pins from RK-ZYNQ7020-F Schematics.pdf page 5, Bank 33:
 *   SDA=U19, SCL=V18, DC=W13, CS=AA13, RST=AA18, LED=Y13
 *
 * CSR map (alphabetical, from csr.h):
 *   lcd_clk=0xF0001000, lcd_cs=0xF0001800, lcd_dc=0xF0002000,
 *   lcd_led=0xF0002800, lcd_mosi=0xF0003000, lcd_rst=0xF0003800
 *
 * Build: riscv64-linux-gnu-gcc -O2 -static lcd_splash.c -o lcd_splash
 * ========================================================================= */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define LCD_W 320
#define LCD_H 172

#define CSR_BASE      0xF0000000UL
#define CSR_SIZE      0x00100000UL
#define CSR_LCD_CLK   0xF0003000UL  /* zlcd_clk  */
#define CSR_LCD_CS    0xF0003800UL  /* zlcd_cs   */
#define CSR_LCD_DC    0xF0004000UL  /* zlcd_dc   */
#define CSR_LCD_LED   0xF0004800UL  /* zlcd_led  */
#define CSR_LCD_MOSI  0xF0005000UL  /* zlcd_mosi */
#define CSR_LCD_RST   0xF0005800UL  /* zlcd_rst  */

static volatile uint32_t *csr;

static inline void csr_wr(unsigned long addr, uint32_t val) {
    csr[(addr - CSR_BASE) / 4] = val;
}

static void pin_mosi(int v) { csr_wr(CSR_LCD_MOSI, v & 1); }
static void pin_clk(int v)  { csr_wr(CSR_LCD_CLK, v & 1); }
static void pin_cs(int v)   { csr_wr(CSR_LCD_CS, v & 1); }
static void pin_dc(int v)   { csr_wr(CSR_LCD_DC, v & 1); }
static void pin_rst(int v)  { csr_wr(CSR_LCD_RST, v & 1); }
static void pin_led(int v)  { csr_wr(CSR_LCD_LED, v & 1); }

static void spi_byte(uint8_t val) {
    for (int i = 7; i >= 0; i--) {
        pin_mosi((val >> i) & 1);
        pin_clk(0);
        pin_clk(1);
    }
    pin_clk(0);
}

static void lcd_cmd(uint8_t cmd) {
    pin_dc(0); pin_cs(0);
    spi_byte(cmd);
    pin_cs(1);
}

static void lcd_data8(uint8_t val) {
    pin_dc(1); pin_cs(0);
    spi_byte(val);
    pin_cs(1);
}

static void lcd_reset(void) {
    pin_rst(1); pin_cs(1); usleep(50000);
    pin_rst(0); usleep(50000);
    pin_rst(1); usleep(150000);
}

static void lcd_init(void) {
    lcd_reset();
    lcd_cmd(0x11); usleep(120000);
    lcd_cmd(0x36); lcd_data8(0x60);
    lcd_cmd(0x3A); lcd_data8(0x05);
    lcd_cmd(0x21);
    lcd_cmd(0x13);
    lcd_cmd(0x29); usleep(50000);
}

static void lcd_set_window(int x0, int y0, int x1, int y1) {
    lcd_cmd(0x2A);
    pin_dc(1); pin_cs(0);
    spi_byte(x0>>8); spi_byte(x0); spi_byte(x1>>8); spi_byte(x1);
    pin_cs(1);
    lcd_cmd(0x2B);
    pin_dc(1); pin_cs(0);
    spi_byte((y0+34)>>8); spi_byte(y0+34); spi_byte((y1+34)>>8); spi_byte(y1+34);
    pin_cs(1);
    lcd_cmd(0x2C);
}

static void lcd_fill(int x, int y, int w, int h, uint16_t c) {
    lcd_set_window(x, y, x+w-1, y+h-1);
    pin_dc(1); pin_cs(0);
    for (int i = 0; i < w*h; i++) { spi_byte(c>>8); spi_byte(c&0xFF); }
    pin_cs(1);
}

static uint16_t rgb565(int r, int g, int b) {
    return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);
}

int main(void) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("/dev/mem"); return 1; }
    csr = mmap(NULL, CSR_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, CSR_BASE);
    if (csr == MAP_FAILED) { perror("mmap"); return 1; }

    printf("Backlight ON...\n");
    pin_led(1);

    printf("Init ST7789V...\n");
    lcd_init();

    /* ATOMiK boot splash: dark blue background with white text */
    uint16_t bg = rgb565(0, 0, 40);
    uint16_t fg = rgb565(255, 255, 255);
    uint16_t accent = rgb565(0, 180, 255);

    printf("Drawing splash...\n");
    lcd_fill(0, 0, LCD_W, LCD_H, bg);

    /* Top accent bar */
    lcd_fill(0, 0, LCD_W, 3, accent);

    /* Center block — ATOMiK blue */
    lcd_fill(80, 50, 160, 40, accent);

    /* Bottom accent bar */
    lcd_fill(0, LCD_H - 3, LCD_W, 3, accent);

    printf("Done — LCD splash displayed\n");

    munmap((void*)csr, CSR_SIZE);
    close(fd);
    return 0;
}
