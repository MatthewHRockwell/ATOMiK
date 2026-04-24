/*
 * LCD Pin Scanner — tries all DC/CS combinations with SPI init.
 * Uses 3 confirmed pins (SDA=U19, SCL=V18, RST=AA18) fixed.
 * Scans remaining Bank 33 pins as DC and CS candidates.
 * When the right pair is found, the LCD will light up.
 *
 * Build: riscv64-linux-gnu-gcc -O2 -static lcd_pin_scan.c -o lcd_pin_scan
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define CSR_BASE 0xF0000000UL
#define CSR_SIZE 0x00100000UL

/* CSR addresses from the build:
 * lcd_clk:  0xF0002000
 * lcd_mosi: 0xF0002800
 * lcd_probe:0xF0003000 (34 bits for candidate pins)
 * lcd_rst:  0xF0003800
 * VTG:      0xF0004000  DMA: 0xF0004800 (if HDMI variant)
 */
#define CSR_CLK   0xF0002000UL
#define CSR_MOSI  0xF0002800UL
#define CSR_PROBE 0xF0003000UL
#define CSR_RST   0xF0003800UL

static volatile uint32_t *csr;

static void wr(unsigned long addr, uint32_t v) { csr[(addr-CSR_BASE)/4] = v; }

/* Probe pins bit assignment (same as platform definition):
 * 0=U14 1=U20 2=U21 3=U22 4=V14 5=V19 6=V20 7=V22
 * 8=W13 9=W15 10=W20 11=W21 12=W22 13=Y13 14=Y14 15=Y15
 * 16=Y19 17=Y20 18=Y21 19=T21 20=T22 21=AA13 22=AA14 23=AA16
 * 24=AA19 25=AA21 26=AA22 27=AB14 28=AB15 29=AB16 30=AB19
 * 31=AB20 32=AB21 33=AB22
 */
static const char *pin_names[] = {
    "U14","U20","U21","U22","V14","V19","V20","V22",
    "W13","W15","W20","W21","W22","Y13","Y14","Y15",
    "Y19","Y20","Y21","T21","T22","AA13","AA14","AA16",
    "AA19","AA21","AA22","AB14","AB15","AB16","AB19",
    "AB20","AB21","AB22"
};
#define N_PINS 34

static uint32_t probe_val;

static void set_probe(uint32_t v) {
    probe_val = v;
    wr(CSR_PROBE, v & 0xFFFFFFFF);
    /* If >32 bits, write upper word */
    if (N_PINS > 32)
        wr(CSR_PROBE + 4, (v >> 32) & 0xFFFFFFFF);
}

static void pin_clk(int v) { wr(CSR_CLK, v & 1); }
static void pin_mosi(int v) { wr(CSR_MOSI, v & 1); }
static void pin_rst(int v) { wr(CSR_RST, v & 1); }

/* Set DC via probe GPIO (specific bit) */
static void pin_dc(int dc_bit, int v) {
    if (v) probe_val |= (1UL << dc_bit);
    else   probe_val &= ~(1UL << dc_bit);
    set_probe(probe_val);
}

/* Set CS via probe GPIO (specific bit) */
static void pin_cs(int cs_bit, int v) {
    if (v) probe_val |= (1UL << cs_bit);
    else   probe_val &= ~(1UL << cs_bit);
    set_probe(probe_val);
}

static void spi_byte(uint8_t val) {
    for (int i = 7; i >= 0; i--) {
        pin_mosi((val >> i) & 1);
        pin_clk(0);
        pin_clk(1);
    }
    pin_clk(0);
}

static int try_lcd_init(int dc_bit, int cs_bit) {
    /* Reset */
    pin_rst(1);
    set_probe((1UL << cs_bit)); /* CS high, DC low */
    usleep(10000);
    pin_rst(0);
    usleep(10000);
    pin_rst(1);
    usleep(50000);

    /* Sleep out (0x11) */
    pin_dc(dc_bit, 0); pin_cs(cs_bit, 0);
    spi_byte(0x11);
    pin_cs(cs_bit, 1);
    usleep(50000);

    /* MADCTL (0x36, 0x60) */
    pin_dc(dc_bit, 0); pin_cs(cs_bit, 0);
    spi_byte(0x36);
    pin_cs(cs_bit, 1);
    pin_dc(dc_bit, 1); pin_cs(cs_bit, 0);
    spi_byte(0x60);
    pin_cs(cs_bit, 1);

    /* Pixel format (0x3A, 0x05) */
    pin_dc(dc_bit, 0); pin_cs(cs_bit, 0);
    spi_byte(0x3A);
    pin_cs(cs_bit, 1);
    pin_dc(dc_bit, 1); pin_cs(cs_bit, 0);
    spi_byte(0x05);
    pin_cs(cs_bit, 1);

    /* Inversion on (0x21) */
    pin_dc(dc_bit, 0); pin_cs(cs_bit, 0);
    spi_byte(0x21);
    pin_cs(cs_bit, 1);

    /* Display on (0x29) */
    pin_dc(dc_bit, 0); pin_cs(cs_bit, 0);
    spi_byte(0x29);
    pin_cs(cs_bit, 1);
    usleep(20000);

    /* Fill screen with red (just first few lines to be quick) */
    /* Set column address */
    pin_dc(dc_bit, 0); pin_cs(cs_bit, 0);
    spi_byte(0x2A);
    pin_cs(cs_bit, 1);
    pin_dc(dc_bit, 1); pin_cs(cs_bit, 0);
    spi_byte(0); spi_byte(0); spi_byte(1); spi_byte(63);
    pin_cs(cs_bit, 1);

    /* Set row address (with offset 34) */
    pin_dc(dc_bit, 0); pin_cs(cs_bit, 0);
    spi_byte(0x2B);
    pin_cs(cs_bit, 1);
    pin_dc(dc_bit, 1); pin_cs(cs_bit, 0);
    spi_byte(0); spi_byte(34); spi_byte(0); spi_byte(34+9);
    pin_cs(cs_bit, 1);

    /* Memory write */
    pin_dc(dc_bit, 0); pin_cs(cs_bit, 0);
    spi_byte(0x2C);
    pin_cs(cs_bit, 1);
    pin_dc(dc_bit, 1); pin_cs(cs_bit, 0);
    for (int i = 0; i < 320; i++) { /* 320 red pixels */
        spi_byte(0xF8); spi_byte(0x00); /* red in RGB565 */
    }
    pin_cs(cs_bit, 1);

    return 0;
}

int main(void) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("/dev/mem"); return 1; }
    csr = mmap(NULL, CSR_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, CSR_BASE);
    if (csr == MAP_FAILED) { perror("mmap"); return 1; }

    printf("LCD Pin Scanner: testing %d x %d = %d combinations\n",
           N_PINS, N_PINS-1, N_PINS*(N_PINS-1));
    printf("SDA=U19 SCL=V18 RST=AA18 (fixed)\n");
    printf("Watch the LCD — it will light up when the right DC/CS pair is found.\n\n");

    int count = 0;
    for (int dc = 0; dc < N_PINS; dc++) {
        for (int cs = 0; cs < N_PINS; cs++) {
            if (dc == cs) continue;
            count++;
            if (count % 34 == 1)
                printf("Testing DC=%s CS=%s (#%d/%d)\n",
                       pin_names[dc], pin_names[cs], count, N_PINS*(N_PINS-1));

            try_lcd_init(dc, cs);
            usleep(50000); /* brief pause to see if LCD responds */
        }
    }

    printf("\nScan complete. %d combinations tested.\n", count);
    munmap((void*)csr, CSR_SIZE);
    close(fd);
    return 0;
}
