/*
 * LCD CS-only Scanner — 4 pins confirmed, scan CS across Bank 33.
 * SDA=U19, SCL=V18, DC=V19, RST=AA18 (dedicated GPIOs)
 * CS candidates: 33 remaining Bank 33 pins via lcd_probe GPIO
 *
 * Build: riscv64-linux-gnu-gcc -O2 -static lcd_cs_scan.c -o lcd_cs_scan
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define CSR_BASE  0xF0000000UL
#define CSR_SIZE  0x00100000UL
#define CSR_CLK   0xF0002000UL
#define CSR_DC    0xF0002800UL
#define CSR_MOSI  0xF0003000UL
#define CSR_PROBE 0xF0003800UL
#define CSR_RST   0xF0004000UL

static volatile uint32_t *csr;
static void wr(unsigned long a, uint32_t v) { csr[(a-CSR_BASE)/4] = v; }

static void pin_clk(int v)  { wr(CSR_CLK, v & 1); }
static void pin_dc(int v)   { wr(CSR_DC, v & 1); }
static void pin_mosi(int v) { wr(CSR_MOSI, v & 1); }
static void pin_rst(int v)  { wr(CSR_RST, v & 1); }

/* CS via probe GPIO — set ONE bit low for active-low CS */
static void pin_cs(int bit, int active) {
    if (active)
        wr(CSR_PROBE, ~(1U << bit));  /* all high except this bit */
    else
        wr(CSR_PROBE, 0xFFFFFFFF);    /* all high = deselected */
}

/* SPI Mode 0: CLK idle LOW, data sampled on rising edge */
static void spi_byte(uint8_t val) {
    for (int i = 7; i >= 0; i--) {
        pin_mosi((val >> i) & 1);
        pin_clk(0);
        pin_clk(1);
    }
    pin_clk(0);
}

static void lcd_cmd(int cs, uint8_t cmd) {
    pin_dc(0); pin_cs(cs, 1);
    spi_byte(cmd);
    pin_cs(cs, 0);
}

static void lcd_data(int cs, uint8_t val) {
    pin_dc(1); pin_cs(cs, 1);
    spi_byte(val);
    pin_cs(cs, 0);
}

static void try_cs(int cs_bit, const char *name) {
    /* Reset */
    wr(CSR_PROBE, 0xFFFFFFFF); /* all CS high */
    pin_rst(1); usleep(10000);
    pin_rst(0); usleep(10000);
    pin_rst(1); usleep(50000);

    /* Sleep out */
    lcd_cmd(cs_bit, 0x11);
    usleep(50000);

    /* MADCTL */
    lcd_cmd(cs_bit, 0x36);
    lcd_data(cs_bit, 0x60);

    /* Pixel format RGB565 */
    lcd_cmd(cs_bit, 0x3A);
    lcd_data(cs_bit, 0x05);

    /* Inversion on */
    lcd_cmd(cs_bit, 0x21);

    /* Normal mode */
    lcd_cmd(cs_bit, 0x13);

    /* Display on */
    lcd_cmd(cs_bit, 0x29);
    usleep(20000);

    /* Fill a small area with red */
    lcd_cmd(cs_bit, 0x2A);
    pin_dc(1); pin_cs(cs_bit, 1);
    spi_byte(0); spi_byte(0); spi_byte(0); spi_byte(63);
    pin_cs(cs_bit, 0);

    lcd_cmd(cs_bit, 0x2B);
    pin_dc(1); pin_cs(cs_bit, 1);
    spi_byte(0); spi_byte(34); spi_byte(0); spi_byte(43);
    pin_cs(cs_bit, 0);

    lcd_cmd(cs_bit, 0x2C);
    pin_dc(1); pin_cs(cs_bit, 1);
    for (int i = 0; i < 640; i++) {
        spi_byte(0xF8); spi_byte(0x00);
    }
    pin_cs(cs_bit, 0);
}

int main(void) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("/dev/mem"); return 1; }
    csr = mmap(NULL, CSR_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, CSR_BASE);
    if (csr == MAP_FAILED) { perror("mmap"); return 1; }

    const char *pins[] = {
        "U14","U20","U21","U22","V14","V20","V22",
        "W13","W15","W20","W21","W22",
        "Y13","Y14","Y15","Y19","Y20","Y21",
        "T21","T22",
        "AA13","AA14","AA16","AA19","AA21","AA22",
        "AB14","AB15","AB16","AB19","AB20","AB21","AB22"
    };
    int n = 33;

    printf("LCD CS Scanner: SDA=U19 SCL=V18 DC=V19 RST=AA18\n");
    printf("Testing %d CS candidates. Watch the LCD!\n\n", n);

    for (int i = 0; i < n; i++) {
        printf("CS=%s (bit %d)\n", pins[i], i);
        try_cs(i, pins[i]);
        usleep(200000); /* 200ms pause to observe */
    }

    printf("\nScan complete.\n");
    munmap((void*)csr, CSR_SIZE);
    close(fd);
    return 0;
}
