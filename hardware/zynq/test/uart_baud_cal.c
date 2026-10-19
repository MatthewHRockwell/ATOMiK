/*
 * Baud rate calibration: continuous 0x55 stream, no pauses.
 * Host measures throughput → baud = throughput_bytes * 10 bits/byte / seconds
 * Then ref_clk = baud * (BAUDGEN * (BAUDDIV + 1))
 * Then PS_CLK = ref_clk * UART_CLK_DIV / IO_PLL_FDIV
 */

#define UART0_CR   (*(volatile unsigned int *)0xE0000000)
#define UART0_MR   (*(volatile unsigned int *)0xE0000004)
#define UART0_SR   (*(volatile unsigned int *)0xE000002C)
#define UART0_FIFO (*(volatile unsigned int *)0xE0000030)
#define UART0_BDIV (*(volatile unsigned int *)0xE0000018) /* BAUDGEN */
#define UART0_CD   (*(volatile unsigned int *)0xE0000034) /* BAUDDIV */

void main(void) {
    /* Reset */
    UART0_CR = 0x03;
    for (volatile int i = 0; i < 10000; i++);

    /* Known-working dividers: BAUDGEN=13, BAUDDIV=62 */
    UART0_BDIV = 13;  /* 0x18 = BAUDGEN */
    UART0_CD = 62;    /* 0x34 = BAUDDIV */

    /* 8N1 normal */
    UART0_MR = 0x00000020;

    /* Enable TX+RX */
    UART0_CR = 0x114; /* STOPBRK | TX_EN | RX_EN */
    for (volatile int i = 0; i < 10000; i++);

    /* Blast 0x55 forever — tight loop, TXFULL wait only */
    while (1) {
        while (UART0_SR & 0x10); /* wait TXFULL clear */
        UART0_FIFO = 0x55;
    }
}
