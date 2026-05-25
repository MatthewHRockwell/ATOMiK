/* uart.c — LiteX BIOS UART backend, redirected to Zynq PS UART0.
 *
 * This LiteX SoC instance (HamGeek RK-ZYNQ7020-F + ATOMiK) builds with a
 * LiteX UART module pinned to PL pad V12 on the 40-pin expansion header,
 * which is not wired to a host-visible USB-serial bridge on this board.
 * The Zynq PS exposes UART0 on MIO10/11 which IS wired to a dedicated
 * FT232 chip → /dev/ttyUSB1 on the development host.
 *
 * The FSBL initializes PS UART0 before programming the PL bitstream, so
 * by the time NaxRiscv runs the LiteX BIOS, UART0 controller is alive at
 * 115200 8N1. From NaxRiscv, the PS UART0 register window is reachable
 * through the LiteX SoC's ps_iop bridge: ARM 0xE0000000 == Nax 0x80000000.
 *
 * Everything from BIOS printf() lands in PS UART0 TXFIFO and exits the
 * board through the proper UART path. The LiteX UART CSR (at
 * CSR_UART_BASE) is no longer used. It still exists in the synthesized
 * bitstream but is unwired from anything host-visible; a future bitstream
 * regen should drop it from the SoC entirely (remove `serial` from
 * hamgeek_rk7020f.py platform).
 */
#include "uart.h"

#include <irq.h>
#include <generated/csr.h>

#ifdef CSR_UART_BASE

/* PS UART0 register view from NaxRiscv via ps_iop window. */
#define PS_UART0_BASE   0x80000000UL
#define PS_UART0_CR     (PS_UART0_BASE + 0x00UL)
#define PS_UART0_SR     (PS_UART0_BASE + 0x2CUL)   /* bit 1 = REMPTY, bit 3 = TXEMPTY, bit 4 = TXFULL */
#define PS_UART0_FIFO   (PS_UART0_BASE + 0x30UL)

#define PS_UART_SR_REMPTY   (1u << 1)
#define PS_UART_SR_TXEMPTY  (1u << 3)
#define PS_UART_SR_TXFULL   (1u << 4)

static inline volatile unsigned int *_r(unsigned long a) {
    return (volatile unsigned int *)a;
}

void uart_init(void)
{
    /* No-op. FSBL configures UART0 controller + MIO10/11 pinmux before
     * PCAP, and the configuration survives across PL bring-up. */
}

void uart_isr(void)
{
    /* No interrupts for PS UART0 in this build — BIOS uses polled I/O. */
}

void uart_write(char c)
{
    /* Block while TX FIFO is full. SR @0xE000002C bit 4 = TXFULL. */
    while ((*_r(PS_UART0_SR) & PS_UART_SR_TXFULL) != 0u) { }
    *_r(PS_UART0_FIFO) = (unsigned int)(unsigned char)c;
}

char uart_read(void)
{
    while ((*_r(PS_UART0_SR) & PS_UART_SR_REMPTY) != 0u) { }
    return (char)(*_r(PS_UART0_FIFO) & 0xFFu);
}

int uart_read_nonblock(void)
{
    return (*_r(PS_UART0_SR) & PS_UART_SR_REMPTY) == 0u;
}

void uart_sync(void)
{
    /* Drain TX FIFO. SR bit 3 = TXEMPTY when fully drained. */
    while ((*_r(PS_UART0_SR) & PS_UART_SR_TXEMPTY) == 0u) { }
}

#endif /* CSR_UART_BASE */
