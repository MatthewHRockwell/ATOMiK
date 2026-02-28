// GPIO debug firmware - trace execution with LEDs
// LEDs will show binary pattern indicating execution progress

#include <stdint.h>

typedef struct {
    volatile uint32_t OUT;
    volatile uint32_t IN;
    volatile uint32_t OE;
} PICOGPIO;

typedef struct {
    volatile uint32_t DATA;
    volatile uint32_t CLKDIV;
} PICOUART;

#define GPIO0 ((PICOGPIO*)0x82000000)
#define UART0 ((PICOUART*)0x83000000)
#define CLK_FREQ 13500000
#define UART_BAUD 115200

// Pattern codes:
// 0x01 = entered main()
// 0x02 = configured GPIO
// 0x03 = configured UART
// 0x04 = sent first character
// 0x3F = running main loop

void main() {
    // Signal: entered main()
    GPIO0->OE = 0x3F;
    GPIO0->OUT = 0x01;

    for (volatile int i = 0; i < 100000; i++);

    // Signal: GPIO configured
    GPIO0->OUT = 0x02;

    for (volatile int i = 0; i < 100000; i++);

    // Set UART baud rate
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;

    // Wait for UART idle bits
    for (volatile int i = 0; i < 2000; i++);

    // Signal: UART configured
    GPIO0->OUT = 0x03;

    for (volatile int i = 0; i < 100000; i++);

    // Send first character
    UART0->DATA = 'G';

    for (volatile int i = 0; i < 100000; i++);

    // Signal: sent first character
    GPIO0->OUT = 0x04;

    for (volatile int i = 0; i < 100000; i++);

    // Signal: entering main loop
    GPIO0->OUT = 0x3F;

    // Main loop: continuously send 'G' and toggle LEDs
    while (1) {
        UART0->DATA = 'G';
        GPIO0->OUT = 0x3F;
        for (volatile int i = 0; i < 50000; i++);

        GPIO0->OUT = 0x00;
        for (volatile int i = 0; i < 50000; i++);
    }
}
