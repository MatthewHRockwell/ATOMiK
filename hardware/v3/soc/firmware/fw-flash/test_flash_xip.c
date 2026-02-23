// Phase 3B: Flash XIP Validation Test
// Minimal firmware to prove CPU can execute from SPI flash

#include <stdint.h>

typedef struct {
    volatile uint32_t DATA;
    volatile uint32_t CLKDIV;
} PICOUART;

#define UART0 ((PICOUART*)0x83000000)

#define CLK_FREQ   13500000  // Crystal ÷2 (13.5 MHz)
#define UART_BAUD  115200

static inline void uart_putchar(uint8_t c) {
    UART0->DATA = c;
}

void print(const char *s) {
    while (*s) {
        if (*s == '\n')
            uart_putchar('\r');
        uart_putchar(*s++);
    }
}

void print_hex32(uint32_t v) {
    for (int i = 7; i >= 0; i--) {
        uint8_t n = (v >> (4*i)) & 0xF;
        uart_putchar(n < 10 ? '0' + n : 'a' + n - 10);
    }
}

int main() {
    // UART already initialized by Boot ROM, but set again for safety
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;

    // Wait for UART to stabilize
    for (volatile int i = 0; i < 2000; i++);

    // Banner - proves we're executing from flash at 0x00000000
    print("\n\n");
    print("=== ATOMiK v3 Flash XIP Test ===\n");
    print("Phase 3B: SPI Flash Execute-In-Place\n");
    print("Running from: 0x00000000 (SPI Flash)\n");

    // Read and display Program Counter to prove we're in flash address space
    uint64_t pc;
    __asm__ volatile ("auipc %0, 0" : "=r"(pc));
    print("Current PC: 0x");
    print_hex32((uint32_t)pc);
    print("\n");

    // Continuous heartbeat to show firmware is running
    print("\nFlash XIP Working! (Heartbeat below)\n");

    uint32_t counter = 0;
    while (1) {
        print("FLASH-XIP-OK [");
        print_hex32(counter++);
        print("]\n");

        // Delay ~1 second (13.5 MHz / 13500000 = 1 Hz)
        for (volatile int i = 0; i < 1350000; i++);
    }

    return 0;
}
