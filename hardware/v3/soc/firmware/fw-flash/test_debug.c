// Minimal debug firmware for v3 SoC
// Tests UART output with correct clock frequency and timing

#include <stdint.h>

typedef struct {
    volatile uint32_t DATA;
    volatile uint32_t CLKDIV;
} PICOUART;

#define UART0 ((PICOUART*)0x83000000)
#define CLK_FREQ 13500000
#define UART_BAUD 115200

void main() {
    // Set UART baud rate
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;

    // Wait for UART to finish 15 idle bits (~1740 cycles)
    for (volatile int i = 0; i < 2000; i++);

    // Send 'D' for Debug continuously
    while (1) {
        UART0->DATA = 'D';
        for (volatile int i = 0; i < 50000; i++);
    }
}
