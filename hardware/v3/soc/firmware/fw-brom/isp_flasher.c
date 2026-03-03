// ISP Flasher for ATOMiK v3 SoC (RV64I)
// Identical logic to v2 — all MMIO is 32-bit (uses lw/sw via volatile uint32_t*)

// BISECTION MODE: Systematically test CPU features to isolate bug
// Controlled by Makefile defines:
//   -DBISECT_STEP1: Function call test
//   -DBISECT_STEP2: Stack frame test
//   -DBISECT_STEP3: Load test
//   -DBRINGUP_MODE: Simple loop (known working)

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

#define CLK_FREQ        21600000  // 21.6 MHz (108 MHz / 5) — V3-020 fix
#define UART_BAUD       115200

static inline void uart_putchar(uint8_t wdata) {
    UART0->DATA = wdata;
}

static inline void delay(int cycles) {
    for (volatile int i = 0; i < cycles; i++);
}

// --------------------------------------------------------
// BISECTION STEP 1: Single function call
// --------------------------------------------------------

#ifdef BISECT_STEP1

// Test: Can we call a function and return?
// noinline forces actual JAL/JALR instead of inlining
void __attribute__((noinline)) print_F(void) {
    uart_putchar('F');
}

int main() {
    // Initialize UART
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Infinite loop: call function, delay, repeat
    while (1) {
        print_F();  // Function call test
        delay(100000);
    }
}

// --------------------------------------------------------
// BISECTION STEP 2: Stack frame test
// --------------------------------------------------------

#elif defined(BISECT_STEP2)

int main() {
    // Initialize UART
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Create local array to force stack frame
    uint8_t local_array[16];

    // Write pattern to array
    for (int i = 0; i < 16; i++) {
        local_array[i] = 'S';
    }

    // Read back and print
    while (1) {
        uart_putchar(local_array[0]);
        delay(100000);
    }
}

// --------------------------------------------------------
// BISECTION STEP 3: Load test (polling UART status)
// --------------------------------------------------------

#elif defined(BISECT_STEP3)

int main() {
    // Initialize UART
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    while (1) {
        // Read UART DATA register (polling)
        int32_t rdata = (int32_t)UART0->DATA;

        // Print result-coded byte
        if (rdata < 0) {
            uart_putchar('N');  // Negative = no data
        } else {
            uart_putchar('D');  // Non-negative = data available
        }
        delay(100000);
    }
}

// --------------------------------------------------------
// BRINGUP MODE: Simple loop (known working baseline)
// --------------------------------------------------------

#elif defined(BRINGUP_MODE)

int main() {
    // Initialize UART
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Configure GPIO for LED heartbeat
    GPIO0->OE = 0x01;
    GPIO0->OUT = 0x00;

    // Simple spam loop
    uint8_t led_state = 0;
    while (1) {
        UART0->DATA = 'T';
        delay(100000);

        led_state ^= 1;
        GPIO0->OUT = led_state;
    }
}

// --------------------------------------------------------
// BISECTION STEP 4: Timeout loop (like ISP but simpler)
// --------------------------------------------------------

#elif defined(BISECT_STEP4)

#define FW_WAIT_MAXCNT 5000000  // ~185ms at 27 MHz

int main() {
    volatile int waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Timeout loop (like ISP flasher)
    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        // Don't actually check UART - just loop
    }

    // If we reach here, timeout expired
    uart_putchar('J');
    uart_putchar('U');
    uart_putchar('M');
    uart_putchar('P');
    uart_putchar('!');

    // Infinite loop to prevent further execution
    while (1) {
        delay(1000000);
    }
}

// --------------------------------------------------------
// BISECTION STEP 5: Jump to flash (critical test)
// --------------------------------------------------------

#elif defined(BISECT_STEP5)

#define FW_WAIT_MAXCNT 5000000

int main() {
    volatile int waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Timeout loop
    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        // Skip ISP handshake for now
    }

    // Print JUMP message
    uart_putchar('J');
    uart_putchar('U');
    uart_putchar('M');
    uart_putchar('P');
    uart_putchar('!');

    delay(100000);  // Let UART finish

    // Jump to flash entry point
    asm volatile("li t0, 0; jr t0" ::: "t0");

    // Should never reach here
    while (1) {
        uart_putchar('E');  // Error - jump failed
        delay(100000);
    }
}

// --------------------------------------------------------
// BISECTION STEP 6: Minimal ISP with handshake only
// --------------------------------------------------------

#elif defined(BISECT_STEP6)

#define FW_WAIT_MAXCNT 5000000

static inline uint8_t uart_getchar_blocking() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
    } while (rdata < 0);
    return (uint8_t)rdata;
}

int main() {
    volatile int waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // ISP handshake loop
    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        int32_t rdata = (int32_t)UART0->DATA;
        if (rdata == 0x55) {
            uart_putchar(0x56);  // ACK
            break;
        }
    }

    // If ISP detected, enter simple echo loop for testing
    if (waitcnt < FW_WAIT_MAXCNT) {
        uart_putchar('I');  // ISP mode
        uart_putchar('S');
        uart_putchar('P');

        while (1) {
            uint8_t cmd = uart_getchar_blocking();
            uart_putchar(cmd);  // Echo back
        }
    }

    // Timeout - jump to flash
    uart_putchar('J');
    uart_putchar('U');
    uart_putchar('M');
    uart_putchar('P');
    uart_putchar('!');

    delay(100000);

    asm volatile("li t0, 0; jr t0" ::: "t0");

    while (1);
}

// --------------------------------------------------------
// BISECTION STEP 7: Isolate UART polling issue
// --------------------------------------------------------

#elif defined(BISECT_STEP7)

int main() {
    volatile int waitcnt;
    volatile int32_t dummy;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Loop that reads UART but doesn't use the value
    for (waitcnt = 0; waitcnt < 10000; waitcnt++) {
        dummy = (int32_t)UART0->DATA;  // Read UART in loop
    }

    // If we get here, print success
    uart_putchar('O');
    uart_putchar('K');

    while (1) {
        delay(1000000);
    }
}

// --------------------------------------------------------
// BISECTION STEP 8: Very few UART reads to test LSU fix
// --------------------------------------------------------

#elif defined(BISECT_STEP8)

int main() {
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Just 10 reads instead of 10,000
    for (int i = 0; i < 10; i++) {
        volatile int32_t dummy = (int32_t)UART0->DATA;
    }

    // Print success
    uart_putchar('O');
    uart_putchar('K');
    uart_putchar('!');

    while (1) delay(1000000);
}

// --------------------------------------------------------
// ISP STAGE 1: Minimal handshake (banner + echo, no flash)
// --------------------------------------------------------

#elif defined(ISP_STAGE1)

#define FW_WAIT_MAXCNT 5000000

static inline uint8_t uart_getchar_blocking() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
    } while (rdata < 0);
    return (uint8_t)rdata;
}

int main() {
    volatile int waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Print ISP banner
    uart_putchar('I');
    uart_putchar('S');
    uart_putchar('P');
    uart_putchar('\n');

    // ISP handshake loop
    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        int32_t rdata = (int32_t)UART0->DATA;
        if (rdata == 0x55) {
            uart_putchar(0x56);  // ACK

            // Echo 3 bytes to confirm handshake
            for (int i = 0; i < 3; i++) {
                uint8_t byte = uart_getchar_blocking();
                uart_putchar(byte);
            }

            // Stay in echo loop (no flash ops yet)
            while (1) {
                uint8_t cmd = uart_getchar_blocking();
                uart_putchar(cmd);
            }
        }
    }

    // Timeout - should jump to flash (Stage 2)
    uart_putchar('T');
    uart_putchar('O');
    uart_putchar('\n');

    while (1) delay(1000000);
}

// --------------------------------------------------------
// ISP STAGE 2: Complete timeout path (handshake + jump to flash)
// --------------------------------------------------------

#elif defined(ISP_STAGE2)

#define FW_WAIT_MAXCNT 100000  // Reduced for debugging (~5ms at 21.6MHz)

static inline uint8_t uart_getchar_blocking() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
    } while (rdata < 0);
    return (uint8_t)rdata;
}

int main() {
    volatile uint32_t waitcnt;  // Changed to uint32_t and volatile

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Print ISP banner
    uart_putchar('I');
    uart_putchar('S');
    uart_putchar('P');
    uart_putchar('\n');

    // ISP handshake loop with heartbeat
    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        // Compiler barrier to prevent loop optimization
        asm volatile("" ::: "memory");

        // Heartbeat dot every 16K iterations (sparse to avoid UART flood)
        if ((waitcnt & 0x3FFF) == 0) {
            uart_putchar('.');
        }

        int32_t rdata = (int32_t)UART0->DATA;
        if (rdata == 0x55) {
            uart_putchar(0x56);  // ACK

            // Echo 3 bytes to confirm handshake
            for (int i = 0; i < 3; i++) {
                uint8_t byte = uart_getchar_blocking();
                uart_putchar(byte);
            }

            // Stay in echo loop (no flash ops yet)
            while (1) {
                uint8_t cmd = uart_getchar_blocking();
                uart_putchar(cmd);
            }
        }
    }

    // Timeout - print exit marker
    uart_putchar('E');  // Exit marker (proves loop completed)
    uart_putchar('\n');

    // Jump message
    uart_putchar('J');
    uart_putchar('U');
    uart_putchar('M');
    uart_putchar('P');
    uart_putchar('!');
    uart_putchar('\n');

    delay(100000);  // Let UART finish transmitting

    // Jump to flash entry point (0x00000000)
    asm volatile("li t0, 0; jr t0" ::: "t0");

    // Should never reach here - flash firmware should take over
    while (1) {
        uart_putchar('E');  // Error - jump failed
        uart_putchar('R');
        uart_putchar('R');
        uart_putchar('\n');
        delay(1000000);
    }
}

// --------------------------------------------------------
// ISP STAGE 3A: Command parser only (no buffers)
// --------------------------------------------------------

#elif defined(ISP_STAGE3A)

#define FW_WAIT_MAXCNT 5000000

static inline uint8_t uart_getchar_blocking() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
    } while (rdata < 0);
    return (uint8_t)rdata;
}

int main() {
    uint8_t instr;
    volatile uint32_t waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // Print SP value for diagnostics
    register uint64_t sp_val asm("sp");
    uart_putchar('S');
    uart_putchar('P');
    uart_putchar(':');
    uart_putchar('0' + ((sp_val >> 28) & 0xF));  // Simple hex digit
    uart_putchar('\n');

    // ISP handshake with timeout
    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        int32_t rdata = (int32_t)UART0->DATA;
        if (rdata == 0x55) {
            uart_putchar(0x56);
            break;
        }
    }

    if (waitcnt == FW_WAIT_MAXCNT) {
        uart_putchar('J');
        uart_putchar('U');
        uart_putchar('M');
        uart_putchar('P');
        uart_putchar('!');
        uart_putchar('\n');
        delay(100000);
        asm volatile("li t0, 0; jr t0" ::: "t0");
    }

    // Command parser (echo command IDs only, no operations)
    while (1) {
        instr = uart_getchar_blocking();

        switch(instr) {
        case 0x55:
            uart_putchar(0x56);
            break;
        case 0x10:  // WBUF
            uart_putchar('W');
            uart_putchar('B');
            break;
        case 0x30:  // ESEC
            uart_putchar('E');
            uart_putchar('S');
            break;
        case 0x40:  // WPAG
            uart_putchar('W');
            uart_putchar('P');
            break;
        case 0xF0:  // RST
            uart_putchar('R');
            uart_putchar('S');
            break;
        default:
            uart_putchar('?');
            break;
        }
    }
}

// --------------------------------------------------------
// ISP STAGE 3B: Add ESEC (erase sector) operation
// --------------------------------------------------------

#elif defined(ISP_STAGE3B)

#define FW_WAIT_MAXCNT 5000000

#define QSPI_IO_CSb     0x20
#define QSPI_IO_CLK     0x10
#define QSPI_IO_MOSI    0x01
#define QSPI_IO_MISO    0x02
#define QSPI_OE_MOSI    0x0100
#define QSPI_EN_ENABLE  0x80
#define QSPI_FLASH_RDSR     0x05
#define QSPI_FLASH_WREN     0x06
#define QSPI_FLASH_SE       0x20
#define QSPI_FLASHSR_WIP    0x01
#define FLASHIO_REQWREN 0x01

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
    QSPI0->IOW = QSPI_OE_MOSI | QSPI_IO_CSb;
    QSPI0->EN = 0;

    if (wren) {
        QSPI0->IO = 0;
        spi_trbyte(QSPI_FLASH_WREN);
        QSPI0->IO = QSPI_IO_CSb;
    }

    QSPI0->IO = 0;
    while (length) {
        *pdata = spi_trbyte(*pdata);
        pdata++;
        length--;
    }
    QSPI0->IO = QSPI_IO_CSb;

    if (wren) {
        uint8_t res;
        do {
            QSPI0->IO = 0;
            spi_trbyte(QSPI_FLASH_RDSR);
            res = spi_trbyte(0x00);
            QSPI0->IO = QSPI_IO_CSb;
        } while(res & QSPI_FLASHSR_WIP);
    }

    QSPI0->EN = QSPI_EN_ENABLE;
}

static inline uint8_t uart_getchar_blocking() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
    } while (rdata < 0);
    return (uint8_t)rdata;
}

int main() {
    uint8_t instr;
    uint8_t flash_cmd[4];  // Small buffer for erase command
    volatile uint32_t waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        int32_t rdata = (int32_t)UART0->DATA;
        if (rdata == 0x55) {
            uart_putchar(0x56);
            break;
        }
    }

    if (waitcnt == FW_WAIT_MAXCNT) {
        uart_putchar('J');
        uart_putchar('U');
        uart_putchar('M');
        uart_putchar('P');
        uart_putchar('!');
        uart_putchar('\n');
        delay(100000);
        asm volatile("li t0, 0; jr t0" ::: "t0");
    }

    while (1) {
        instr = uart_getchar_blocking();

        switch(instr) {
        case 0x55:
            uart_putchar(0x56);
            break;

        case 0x30:  // ESEC (Erase Sector)
            uart_putchar(0x31);
            flash_cmd[0] = QSPI_FLASH_SE;
            flash_cmd[1] = uart_getchar_blocking();  // addr[2]
            flash_cmd[2] = uart_getchar_blocking();  // addr[1]
            flash_cmd[3] = uart_getchar_blocking();  // addr[0]
            spi_flashio(flash_cmd, 4, FLASHIO_REQWREN);
            uart_putchar(0x32);
            break;

        case 0xF0:  // RST
            uart_putchar(0xF1);
            void (*rst_vec)(void) = (void (*)(void))0x80000000;
            rst_vec();
            break;

        default:
            break;
        }
    }
}

// --------------------------------------------------------
// ISP STAGE 3C: Add WBUF (small buffer for incremental testing)
// --------------------------------------------------------

#elif defined(ISP_STAGE3C)

#define FW_WAIT_MAXCNT 5000000
#define WBUF_SIZE 32  // Small buffer for incremental testing (vs 256 in full version)

#define QSPI_IO_CSb     0x20
#define QSPI_IO_CLK     0x10
#define QSPI_IO_MOSI    0x01
#define QSPI_IO_MISO    0x02
#define QSPI_OE_MOSI    0x0100
#define QSPI_EN_ENABLE  0x80
#define QSPI_FLASH_RDSR     0x05
#define QSPI_FLASH_WREN     0x06
#define QSPI_FLASH_SE       0x20
#define QSPI_FLASHSR_WIP    0x01
#define FLASHIO_REQWREN 0x01

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
    QSPI0->IOW = QSPI_OE_MOSI | QSPI_IO_CSb;
    QSPI0->EN = 0;

    if (wren) {
        QSPI0->IO = 0;
        spi_trbyte(QSPI_FLASH_WREN);
        QSPI0->IO = QSPI_IO_CSb;
    }

    QSPI0->IO = 0;
    while (length) {
        *pdata = spi_trbyte(*pdata);
        pdata++;
        length--;
    }
    QSPI0->IO = QSPI_IO_CSb;

    if (wren) {
        uint8_t res;
        do {
            QSPI0->IO = 0;
            spi_trbyte(QSPI_FLASH_RDSR);
            res = spi_trbyte(0x00);
            QSPI0->IO = QSPI_IO_CSb;
        } while(res & QSPI_FLASHSR_WIP);
    }

    QSPI0->EN = QSPI_EN_ENABLE;
}

static inline uint8_t uart_getchar_blocking() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
    } while (rdata < 0);
    return (uint8_t)rdata;
}

int main() {
    uint8_t instr;
    uint8_t flash_cmd[4];
    uint8_t write_buf[WBUF_SIZE];  // Small write buffer
    int buflen = 0;
    volatile uint32_t waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // ISP handshake with timeout
    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        int32_t rdata = (int32_t)UART0->DATA;
        if (rdata == 0x55) {
            uart_putchar(0x56);
            break;
        }
    }

    // Timeout - jump to flash
    if (waitcnt == FW_WAIT_MAXCNT) {
        uart_putchar('J');
        uart_putchar('U');
        uart_putchar('M');
        uart_putchar('P');
        uart_putchar('!');
        uart_putchar('\n');
        delay(100000);
        asm volatile("li t0, 0; jr t0" ::: "t0");
    }

    // ISP command loop
    while (1) {
        instr = uart_getchar_blocking();

        switch(instr) {
        case 0x55:
            // ISP handshake ACK
            uart_putchar(0x56);
            break;

        case 0x10:
            // ISP WBUF (Write Page Buffer)
            // Host:  0x10 len dat0-datn
            // Reply:      0x11           chksum
            uart_putchar(0x11);

            buflen = uart_getchar_blocking() + 1;

            // Limit to buffer size
            if (buflen > WBUF_SIZE) {
                buflen = WBUF_SIZE;
            }

            uint8_t chksum = 0;
            for (int i = 0; i < buflen; i++) {
                uint8_t rdata = uart_getchar_blocking();
                write_buf[i] = rdata;
                chksum += rdata;
            }
            uart_putchar(chksum);
            break;

        case 0x30:
            // ISP ESEC (Erase Sector)
            uart_putchar(0x31);
            flash_cmd[0] = QSPI_FLASH_SE;
            flash_cmd[1] = uart_getchar_blocking();
            flash_cmd[2] = uart_getchar_blocking();
            flash_cmd[3] = uart_getchar_blocking();
            spi_flashio(flash_cmd, 4, FLASHIO_REQWREN);
            uart_putchar(0x32);
            break;

        case 0xF0:
            // ISP RST (Reset/Jump to BROM)
            uart_putchar(0xF1);
            void (*rst_vec)(void) = (void (*)(void))0x80000000;
            rst_vec();
            break;

        default:
            // Unknown command - ignore
            break;
        }
    }
}

// --------------------------------------------------------
// ISP STAGE 3D: Add WPAG (page program) - complete flash programming
// --------------------------------------------------------

#elif defined(ISP_STAGE3D)

#define FW_WAIT_MAXCNT 5000000
#define WBUF_SIZE 32

#define QSPI_IO_CSb     0x20
#define QSPI_IO_CLK     0x10
#define QSPI_IO_MOSI    0x01
#define QSPI_IO_MISO    0x02
#define QSPI_OE_MOSI    0x0100
#define QSPI_EN_ENABLE  0x80
#define QSPI_FLASH_RDSR     0x05
#define QSPI_FLASH_WREN     0x06
#define QSPI_FLASH_SE       0x20
#define QSPI_FLASH_PP       0x02  // Page Program
#define QSPI_FLASHSR_WIP    0x01
#define FLASHIO_REQWREN 0x01

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
    QSPI0->IOW = QSPI_OE_MOSI | QSPI_IO_CSb;
    QSPI0->EN = 0;

    if (wren) {
        QSPI0->IO = 0;
        spi_trbyte(QSPI_FLASH_WREN);
        QSPI0->IO = QSPI_IO_CSb;
    }

    QSPI0->IO = 0;
    while (length) {
        *pdata = spi_trbyte(*pdata);
        pdata++;
        length--;
    }
    QSPI0->IO = QSPI_IO_CSb;

    if (wren) {
        uint8_t res;
        do {
            QSPI0->IO = 0;
            spi_trbyte(QSPI_FLASH_RDSR);
            res = spi_trbyte(0x00);
            QSPI0->IO = QSPI_IO_CSb;
        } while(res & QSPI_FLASHSR_WIP);
    }

    QSPI0->EN = QSPI_EN_ENABLE;
}

static inline uint8_t uart_getchar_blocking() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
    } while (rdata < 0);
    return (uint8_t)rdata;
}

int main() {
    uint8_t instr;
    uint8_t page_buf[4 + WBUF_SIZE];  // cmd(1) + addr(3) + data(WBUF_SIZE)
    int buflen = 0;
    volatile uint32_t waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // ISP handshake with timeout
    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        int32_t rdata = (int32_t)UART0->DATA;
        if (rdata == 0x55) {
            uart_putchar(0x56);
            break;
        }
    }

    // Timeout - jump to flash
    if (waitcnt == FW_WAIT_MAXCNT) {
        uart_putchar('J');
        uart_putchar('U');
        uart_putchar('M');
        uart_putchar('P');
        uart_putchar('!');
        uart_putchar('\n');
        delay(100000);
        asm volatile("li t0, 0; jr t0" ::: "t0");
    }

    // ISP command loop
    while (1) {
        instr = uart_getchar_blocking();

        switch(instr) {
        case 0x55:
            uart_putchar(0x56);
            break;

        case 0x10:
            // ISP WBUF (Write Page Buffer)
            // Host:  0x10 len dat0-datn
            // Reply:      0x11           chksum
            uart_putchar(0x11);

            buflen = uart_getchar_blocking() + 1;
            if (buflen > WBUF_SIZE) {
                buflen = WBUF_SIZE;
            }

            uint8_t chksum = 0;
            for (int i = 0; i < buflen; i++) {
                uint8_t rdata = uart_getchar_blocking();
                page_buf[4 + i] = rdata;  // Store after cmd+addr
                chksum += rdata;
            }
            uart_putchar(chksum);
            break;

        case 0x30:
            // ISP ESEC (Erase Sector)
            uart_putchar(0x31);
            page_buf[0] = QSPI_FLASH_SE;
            page_buf[1] = uart_getchar_blocking();
            page_buf[2] = uart_getchar_blocking();
            page_buf[3] = uart_getchar_blocking();
            spi_flashio(page_buf, 4, FLASHIO_REQWREN);
            uart_putchar(0x32);
            break;

        case 0x40:
            // ISP WPAG (Write/Program Page)
            // Host:  0x40 addr2-0
            // Reply:      0x41    [program] 0x42
            uart_putchar(0x41);

            page_buf[0] = QSPI_FLASH_PP;
            page_buf[1] = uart_getchar_blocking();  // addr[2]
            page_buf[2] = uart_getchar_blocking();  // addr[1]
            page_buf[3] = uart_getchar_blocking();  // addr[0]

            // Program page with buffered data
            if (buflen > 0) {
                spi_flashio(page_buf, 4 + buflen, FLASHIO_REQWREN);
            }

            uart_putchar(0x42);
            break;

        case 0xF0:
            // ISP RST
            uart_putchar(0xF1);
            void (*rst_vec)(void) = (void (*)(void))0x80000000;
            rst_vec();
            break;

        default:
            break;
        }
    }
}

// --------------------------------------------------------
// ISP STAGE 3: Full flash programming protocol
// --------------------------------------------------------

#elif defined(ISP_STAGE3)

#define FW_WAIT_MAXCNT 5000000  // ~16s wall time (volatile loop, ~3 insn/iter at 21.6 MHz)
#define WBUF_SIZE 256  // Full page buffer (production)

// SPI Flash bit definitions
#define QSPI_IO_CSb     0x20
#define QSPI_IO_CLK     0x10
#define QSPI_IO_MOSI    0x01
#define QSPI_IO_MISO    0x02
#define QSPI_OE_MOSI    0x0100
#define QSPI_EN_ENABLE  0x80

// SPI Flash commands
#define QSPI_FLASH_RDSR     0x05  // Read Status Register
#define QSPI_FLASH_WREN     0x06  // Write Enable
#define QSPI_FLASH_SE       0x20  // Sector Erase (4KB)
#define QSPI_FLASH_PP       0x02  // Page Program
#define QSPI_FLASHSR_WIP    0x01  // Write In Progress bit

#define FLASHIO_REQWREN 0x01

// SPI transfer single byte
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

// SPI flash I/O with optional write enable
void spi_flashio(uint8_t *pdata, int length, int wren) {
    // Set CS high, IO0 is output
    QSPI0->IOW = QSPI_OE_MOSI | QSPI_IO_CSb;

    // Disable XIP mode (enter manual SPI control)
    QSPI0->EN = 0;

    // Send WREN command if requested
    if (wren) {
        QSPI0->IO = 0;
        spi_trbyte(QSPI_FLASH_WREN);
        QSPI0->IO = QSPI_IO_CSb;
    }

    // Perform actual data transfer
    QSPI0->IO = 0;
    while (length) {
        *pdata = spi_trbyte(*pdata);
        pdata++;
        length--;
    }
    QSPI0->IO = QSPI_IO_CSb;

    // Wait for write completion if WREN was issued
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
    uint8_t addr[3];  // addr[0]=bits[23:16], addr[1]=bits[15:8], addr[2]=bits[7:0]
    uint8_t data_buf[256];
} FLASH_BUF;

// --- Flash header + CRC gate helpers ---

static inline uint32_t get_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t crc32_update(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xEDB88320;
        else
            crc >>= 1;
    }
    return crc;
}

// Read N bytes from flash at 24-bit address into flash_buffer.data_buf
static void flash_read_bytes(FLASH_BUF *buf, uint32_t addr, int len) {
    buf->instr = 0x03;  // SPI Flash Read command
    buf->addr[0] = (addr >> 16) & 0xFF;
    buf->addr[1] = (addr >> 8) & 0xFF;
    buf->addr[2] = addr & 0xFF;
    for (int i = 0; i < len; i++)
        buf->data_buf[i] = 0;
    spi_flashio((uint8_t *)buf, 4 + len, 0);
}

#define FLASH_HDR_MAGIC 0x4D4F5441  // "ATOM" little-endian
#define FLASH_HDR_SIZE  16
#define FLASH_MAX_IMAGE 0x100000    // 1MB max

// --- End CRC gate helpers ---

static inline uint8_t uart_getchar_blocking() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
    } while (rdata < 0);
    return (uint8_t)rdata;
}

int main() {
    FLASH_BUF flash_buffer;  // Back to stack (will increase stack size in linker)
    uint8_t instr;
    int buflen = 0;
    volatile uint32_t waitcnt;

    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
    delay(2000);

    // ISP handshake with timeout
    for (waitcnt = 0; waitcnt < FW_WAIT_MAXCNT; waitcnt++) {
        int32_t rdata = (int32_t)UART0->DATA;
        if (rdata == 0x55) {
            uart_putchar(0x56);  // ACK
            break;
        }
    }

    // Timeout - validate flash header + CRC before jumping
    if (waitcnt == FW_WAIT_MAXCNT) {
        // Read 16-byte header from flash offset 0x00
        flash_read_bytes(&flash_buffer, 0x000000, FLASH_HDR_SIZE);

        uint32_t magic     = get_u32_le(&flash_buffer.data_buf[0]);
        uint32_t image_len = get_u32_le(&flash_buffer.data_buf[4]);
        uint32_t exp_crc   = get_u32_le(&flash_buffer.data_buf[8]);
        uint32_t entry     = get_u32_le(&flash_buffer.data_buf[12]);

        // Validate magic
        if (magic != FLASH_HDR_MAGIC) {
            uart_putchar('N'); uart_putchar('O'); uart_putchar('I');
            uart_putchar('M'); uart_putchar('G'); uart_putchar('\n');
            goto stay_in_isp;
        }

        // Validate entry point (must be 0x10 = right after header)
        if (entry != FLASH_HDR_SIZE) {
            uart_putchar('B'); uart_putchar('A'); uart_putchar('D');
            uart_putchar('E'); uart_putchar('P'); uart_putchar('\n');
            goto stay_in_isp;
        }

        // Sanity check image length (min 4 bytes, max flash size minus header)
        if (image_len < 4 || image_len > FLASH_MAX_IMAGE) {
            uart_putchar('B'); uart_putchar('A'); uart_putchar('D');
            uart_putchar('S'); uart_putchar('Z'); uart_putchar('\n');
            goto stay_in_isp;
        }

        // Compute CRC32 over payload (flash offset 0x10, length image_len)
        uint32_t crc = 0xFFFFFFFF;
        uint32_t remaining = image_len;
        uint32_t addr = FLASH_HDR_SIZE;
        while (remaining > 0) {
            uint32_t chunk = (remaining > 256) ? 256 : remaining;
            flash_read_bytes(&flash_buffer, addr, (int)chunk);
            for (uint32_t i = 0; i < chunk; i++)
                crc = crc32_update(crc, flash_buffer.data_buf[i]);
            addr += chunk;
            remaining -= chunk;
        }
        crc ^= 0xFFFFFFFF;

        if (crc != exp_crc) {
            uart_putchar('B'); uart_putchar('A'); uart_putchar('D');
            uart_putchar('C'); uart_putchar('R'); uart_putchar('C');
            uart_putchar('\n');
            goto stay_in_isp;
        }

        uart_putchar('J'); uart_putchar('U'); uart_putchar('M');
        uart_putchar('P'); uart_putchar('!'); uart_putchar('\n');
        delay(100000);  // Let UART finish transmitting

        // Jump to entry point from header
        // MUST use inline asm: GCC treats low addresses as null pointer UB
        asm volatile("mv t0, %0; jr t0" :: "r"((uint64_t)entry) : "t0");
    }

    stay_in_isp:

    // ISP command loop
    while (1) {
        instr = uart_getchar_blocking();

        switch(instr) {
        case 0x55:
            // ISP handshake ACK
            uart_putchar(0x56);
            break;

        case 0x10:
            // ISP WBUF (Write Page Buffer)
            // Host:  0x10 len dat0-datn
            // Reply:      0x11           chksum
            uart_putchar(0x11);

            buflen = uart_getchar_blocking() + 1;

            uint8_t chksum = 0;
            for (int i = 0; i < buflen; i++) {
                uint8_t rdata = uart_getchar_blocking();
                flash_buffer.data_buf[i] = rdata;
                chksum += rdata;
            }
            uart_putchar(chksum);
            break;

        case 0x30:
            // ISP ESEC (Erase Sector)
            // Host:  0x30 addr2-0
            // Reply:      0x31    [erase] 0x32
            uart_putchar(0x31);

            flash_buffer.instr = QSPI_FLASH_SE;
            flash_buffer.addr[0] = uart_getchar_blocking();
            flash_buffer.addr[1] = uart_getchar_blocking();
            flash_buffer.addr[2] = uart_getchar_blocking();

            // Erase 4KB sector (unconditional — erase doesn't use data buffer)
            spi_flashio((uint8_t *)&flash_buffer, 4, FLASHIO_REQWREN);

            uart_putchar(0x32);
            break;

        case 0x40:
            // ISP WPAG (Write/Program Page)
            // Host:  0x40 addr2-0
            // Reply: 0x4F (NACK if no buffer) or 0x41 [program] 0x42
            flash_buffer.instr = QSPI_FLASH_PP;
            flash_buffer.addr[0] = uart_getchar_blocking();
            flash_buffer.addr[1] = uart_getchar_blocking();
            flash_buffer.addr[2] = uart_getchar_blocking();

            if (buflen == 0) {
                uart_putchar(0x4F);  // NACK: no buffer data loaded
                break;
            }

            uart_putchar(0x41);
            spi_flashio((uint8_t *)&flash_buffer, 4 + buflen, FLASHIO_REQWREN);
            uart_putchar(0x42);
            break;

        case 0x50: {
            // ISP RDBK (Read Back Flash)
            // Host:  0x50 addr2 addr1 addr0 len   (len = actual_length - 1)
            // Reply: 0x51 data0..dataN chksum
            uart_putchar(0x51);

            flash_buffer.instr = 0x03;  // SPI Flash Read command
            flash_buffer.addr[0] = uart_getchar_blocking();
            flash_buffer.addr[1] = uart_getchar_blocking();
            flash_buffer.addr[2] = uart_getchar_blocking();
            int rb_len = uart_getchar_blocking() + 1;

            // Clear read buffer
            for (int i = 0; i < rb_len; i++)
                flash_buffer.data_buf[i] = 0;

            // SPI read (no WREN needed for reads)
            spi_flashio((uint8_t *)&flash_buffer, 4 + rb_len, 0);

            // Send data and checksum
            uint8_t rb_chksum = 0;
            for (int i = 0; i < rb_len; i++) {
                uart_putchar(flash_buffer.data_buf[i]);
                rb_chksum += flash_buffer.data_buf[i];
            }
            uart_putchar(rb_chksum);
            break;
        }

        case 0xF0:
            // ISP RST (Reset/Jump to BROM)
            // Host:  0xF0
            // Reply:      0xF1
            uart_putchar(0xF1);

            // Jump back to BROM reset vector
            void (*rst_vec)(void) = (void (*)(void))0x80000000;
            rst_vec();
            break;

        default:
            // Unknown command NACK
            uart_putchar(0xFF);
            break;
        }
    }
}

// --------------------------------------------------------
// FULL ISP MODE: (Disabled until staged bringup complete)
// --------------------------------------------------------

#else

#error "Please define BISECT_STEP1-8, BRINGUP_MODE, ISP_STAGE1, ISP_STAGE2, or ISP_STAGE3"

#endif

