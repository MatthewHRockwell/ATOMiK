// Minimal test firmware to verify flash boot works
#include <stdint.h>

typedef struct {
    volatile uint32_t DATA;
    volatile uint32_t CLKDIV;
} PICOUART;

#define UART0 ((PICOUART*)0x83000000)
#define CLK_FREQ 27000000
#define UART_BAUD 115200

void putchar(int c) {
    UART0->DATA = c;
}

void print(const char *s) {
    while (*s) putchar(*s++);
}

void main() {
    // Configure UART
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;

    // Wait for UART to settle
    for (volatile int i = 0; i < 2000; i++);

    // Print test message repeatedly
    while (1) {
        print("\n*** FLASH BOOT WORKS! ***\n");
        print("Press X to run ATOMiK tests\n");

        // Wait a bit
        for (volatile int i = 0; i < 1000000; i++);
    }
}
