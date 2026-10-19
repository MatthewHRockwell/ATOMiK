/*
 * Bare-metal UART0 sender for Zynq-7020 Cortex-A9
 * Loaded to OCM via xsdb, runs standalone — no xsdb polling artifacts
 *
 * Cadence UART CR bits (from Linux xilinx_uartps driver):
 *   0x001 RXRST    0x002 TXRST    0x004 RX_EN   0x008 RX_DIS
 *   0x010 TX_EN    0x020 TX_DIS   0x040 RSTTO   0x080 STARTBRK
 *   0x100 STOPBRK
 *
 * SR bits:
 *   0x002 RXEMPTY  0x008 TXEMPTY  0x010 TXFULL  0x800 TACTIVE
 */

#define UART0_CR   (*(volatile unsigned int *)0xE0000000)
#define UART0_MR   (*(volatile unsigned int *)0xE0000004)
#define UART0_SR   (*(volatile unsigned int *)0xE000002C)
#define UART0_FIFO (*(volatile unsigned int *)0xE0000030)
#define UART0_BDIV (*(volatile unsigned int *)0xE0000018)
#define UART0_CD   (*(volatile unsigned int *)0xE0000034)

#define CR_RXRST    0x001
#define CR_TXRST    0x002
#define CR_RX_EN    0x004
#define CR_TX_EN    0x010
#define CR_STOPBRK  0x100

#define SR_TXFULL   0x010
#define SR_TACTIVE  0x800

static void uart_putc(char c) {
    while (UART0_SR & SR_TXFULL);
    UART0_FIFO = (unsigned int)c;
}

static void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

static void delay(volatile int n) {
    while (n-- > 0);
}

void main(void) {
    /* Reset TX and RX paths */
    UART0_CR = CR_TXRST | CR_RXRST;
    delay(10000);

    /* PS_CLK=32.214MHz, IO_PLL=966.4MHz, ref=96.64MHz
     * 115200 baud: 96642000 / (7 * 120) = 115050 (0.13% error)
     * BAUDGEN(0x18)=7, BAUDDIV(0x34)=119
     */
    UART0_BDIV = 7;    /* register 0x18 = BAUDGEN */
    UART0_CD = 119;    /* register 0x34 = BAUDDIV */

    /* 8N1 normal mode */
    UART0_MR = 0x00000020;

    /* Enable TX + RX */
    UART0_CR = CR_STOPBRK | CR_TX_EN | CR_RX_EN;
    delay(10000);

    /* Blast forever */
    while (1) {
        uart_puts("ATOMIK\r\n");

        for (int i = 0; i < 256; i++)
            uart_putc(0x55);

        uart_puts("\r\n");

        /* Brief pause between bursts */
        delay(200000);
    }
}
