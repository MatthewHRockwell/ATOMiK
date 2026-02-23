// ISP Flasher for ATOMiK v3 SoC (RV64I)
// Identical logic to v2 — all MMIO is 32-bit (uses lw/sw via volatile uint32_t*)

// Compile-time flag for hardware bringup mode
// When enabled: bypasses ISP/flash, continuously spams UART + toggles GPIO
#define BRINGUP_MODE 1

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    volatile uint32_t DATA;
    volatile uint32_t CLKDIV;
} PICOUART;

typedef struct {
    volatile uint32_t OUT;
    volatile uint32_t IN;
    volatile uint32_t OE;
} PICOGPIO;

typedef struct {
    union {
        volatile uint32_t REG;
        volatile uint16_t IOW;
        struct {
            volatile uint8_t IO;
            volatile uint8_t OE;
            volatile uint8_t CFG;
            volatile uint8_t EN;
        };
    };
} PICOQSPI;

#define QSPI0 ((PICOQSPI*)0x81000000)
#define GPIO0 ((PICOGPIO*)0x82000000)
#define UART0 ((PICOUART*)0x83000000)

// --------------------------------------------------------

#define QSPI_IO_CSb     0x20
#define QSPI_IO_CLK     0x10
#define QSPI_IO_MOSI    0x01
#define QSPI_IO_MISO    0x02

#define QSPI_OE_MOSI    0x0100

#define QSPI_EN_ENABLE  0x80

#define QSPI_FLASH_RDSR     0x05
#define QSPI_FLASH_WREN     0x06
#define QSPI_FLASH_SE       0x20
#define QSPI_FLASH_PP       0x02
#define QSPI_FLASHSR_WIP    0x01

#define FLASHIO_REQWREN 0x01

static inline uint8_t uart_getchar() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
    } while (rdata < 0);
    return (uint8_t)rdata;
}

static inline void uart_putchar(uint8_t wdata) {
    UART0->DATA = wdata;
}

static inline uint8_t spi_trbyte(uint8_t txdata) {
    uint8_t spi_io;
    for (int i = 0; i < 8; i++) {
        spi_io = (txdata >> 7) & QSPI_IO_MOSI;
        QSPI0->IO = spi_io;
        spi_io |= QSPI_IO_CLK;
        QSPI0->IO = spi_io;
        txdata = (txdata << 1) | ((QSPI0->IO & QSPI_IO_MISO) >> 1);
    }
    return txdata;
}

void spi_flashio(uint8_t *pdata, int length, int wren) {
    // Set CS high, IO0 is output
    QSPI0->IOW = QSPI_OE_MOSI | QSPI_IO_CSb;

    // Enable Manual SPI Ctrl
    QSPI0->EN = 0;

    // Send WREN cmd when requested
    if (wren) {
        QSPI0->IO = 0;
        spi_trbyte(QSPI_FLASH_WREN);
        QSPI0->IO = QSPI_IO_CSb;
    }

    // Perform actual data RW
    QSPI0->IO = 0;
    while (length) {
        *pdata = spi_trbyte(*pdata);
        pdata++;
        length--;
    }
    QSPI0->IO = QSPI_IO_CSb;

    // Check WIP/BUSY bit when WREN issued
    if (wren) {
        uint8_t res;
        do {
            QSPI0->IO = 0;
            spi_trbyte(QSPI_FLASH_RDSR);
            res = spi_trbyte(0x00);
            QSPI0->IO = QSPI_IO_CSb;
        } while(res & QSPI_FLASHSR_WIP);
    }

    // Return to XIP mode
    QSPI0->EN = QSPI_EN_ENABLE;
}

typedef struct {
    uint8_t instr;
    // In transmit sequence, addr[0] -> 23:16 / addr[1] -> 15:8 / addr[2] -> 7:0
    uint8_t addr[3];
    uint8_t data_buf[256];
} FLASH_BUF;

// Test function to verify jump mechanism
void test_loop() {
    while (1) {
        uart_putchar('T');
        uart_putchar('E');
        uart_putchar('S');
        uart_putchar('T');
        uart_putchar('\n');
        for (volatile int i = 0; i < 100000; i++);
    }
}

#define FW_WAIT_MAXCNT  5000000  // ~370ms timeout at 13.5 MHz (longer for ISP)
#define CLK_FREQ        13500000  // Crystal direct ÷2 (bypassing PLL)
#define UART_BAUD       115200

int main()
{
#ifdef BRINGUP_MODE
    // =========================================================================
    // BRINGUP MODE: Minimal test to prove CPU + UART + GPIO liveness
    // =========================================================================
    // Goal: Isolate "is hardware alive?" from ISP/flash complexity

    // Configure UART baud rate
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;  // Try formula 1: clk/baud - 2

    // Wait for UART to settle (15 idle bits = ~1740 cycles)
    for (volatile int i = 0; i < 2000; i++);

    // Configure GPIO[0] as output for LED heartbeat
    GPIO0->OE = 0x01;
    GPIO0->OUT = 0x00;

    // Dual liveness indicators:
    // 1. UART: Continuous 'T' spam (proves: CPU fetch, ROM mapping, UART TX, baud)
    // 2. GPIO: Toggle heartbeat (proves: CPU alive independent of UART)

    uint8_t led_state = 0;
    while (1) {
        // UART spam: 'T' character
        UART0->DATA = 'T';

        // Delay ~100k cycles at 13.5 MHz = ~7.4ms
        // Gives ~135 Hz UART spam, ~67.5 Hz LED toggle
        for (volatile int i = 0; i < 100000; i++);

        // Toggle GPIO[0] LED
        led_state ^= 1;
        GPIO0->OUT = led_state;
    }
    // Never returns

#else
    // =========================================================================
    // PRODUCTION MODE: ISP flasher with flash boot fallback
    // =========================================================================
    FLASH_BUF flash_buffer;
    uint8_t instr;
    int buflen;
    int waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;

    // CRITICAL: Wait for UART to finish sending 15 idle bits after CLKDIV write
    // This takes ~1740 cycles (15 bits * 116 cycles/bit at 115200 baud, 13.5 MHz clock)
    for (waitcnt = 0; waitcnt < 2000; waitcnt++);

    // Flush UART RX noise from FPGA reconfiguration glitches.
    // Without this, pin noise during SRAM bitstream load can appear
    // as 0x55 and prematurely enter ISP mode.
    for (waitcnt = 0; waitcnt < 200000; waitcnt++) {
        (void)UART0->DATA;
    }

    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        if (UART0->DATA == 0x55) {
            uart_putchar(0x56);
            break;
        }
    }

    if (waitcnt == FW_WAIT_MAXCNT) {
        // Phase 3B: Jump to flash firmware at 0x00000000
        // Diagnostic: read first word from SPI flash XIP and print it
        uart_putchar('J');  // 'J' = Jumping to flash
        volatile uint32_t *fp = (volatile uint32_t *)0x00000004;
        uint32_t w = *fp;
        for (int i = 7; i >= 0; i--) {
            uint8_t n = (w >> (4*i)) & 0xF;
            uart_putchar(n < 10 ? '0' + n : 'a' + n - 10);
        }
        uart_putchar('!');

        // Jump to flash entry point (0x00000000)
        // Use function pointer to avoid compiler optimizations
        void (*flash_entry)(void) = (void (*)(void))0x00000000;
        flash_entry();

        // Should never reach here
        while (1) {
            uart_putchar('E');  // 'E' = Error (jump failed)
            uart_putchar('R');
            uart_putchar('R');
            uart_putchar('\n');
            for (volatile int i = 0; i < 100000; i++);
        }
    }

    while (1) {
        instr = uart_getchar();

        switch(instr) {
        case 0x55:
            // ISP Flasher ACK
            uart_putchar(0x56);
            break;

        case 0x10:
            // ISP Flasher WBUF (Write Pagebuf)
            uart_putchar(0x11);

            buflen = uart_getchar() + 1;

            uint8_t chksum = 0;
            for (int i = 0; i < buflen; i++) {
                uint8_t rdata = uart_getchar();
                flash_buffer.data_buf[i] = rdata;
                chksum += rdata;
            }
            uart_putchar(chksum);
            break;

        case 0x30:
            // ISP Flasher ESEC (Erase Sector)
            uart_putchar(0x31);

            flash_buffer.instr = QSPI_FLASH_SE;
            flash_buffer.addr[0] = uart_getchar();
            flash_buffer.addr[1] = uart_getchar();
            flash_buffer.addr[2] = uart_getchar();

            if (buflen) {
                spi_flashio( (void *)&flash_buffer, 4, FLASHIO_REQWREN);
            }

            uart_putchar(0x32);
            break;

        case 0x40:
            // ISP Flasher WPAG (Write Page)
            uart_putchar(0x41);

            flash_buffer.instr = QSPI_FLASH_PP;
            flash_buffer.addr[0] = uart_getchar();
            flash_buffer.addr[1] = uart_getchar();
            flash_buffer.addr[2] = uart_getchar();

            if (buflen) {
                spi_flashio( (void *)&flash_buffer, 4+buflen, FLASHIO_REQWREN);
            }

            uart_putchar(0x42);
            break;

        case 0xF0:
            // ISP Flasher RST
            uart_putchar(0xF1);

            // Jump to reset vector (use asm to avoid compiler UB issues)
            __asm__ volatile ("li t0, 0x80000000; jr t0" ::: "t0");
            break;
        default:
            break;
        }
    }
#endif  // BRINGUP_MODE
}
