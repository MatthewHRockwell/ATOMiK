// ATOMiK v3 SoC Firmware (RV64I)
// UART menu + ATOMiK custom instruction tests

#include <stdint.h>
#include <stdbool.h>
#include "atomik_v3.h"

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

#define FLASHIO_ENTRY_ADDR ((void *)0x80000054)

void (*spi_flashio)(uint8_t *pdata, int length, int wren) = FLASHIO_ENTRY_ADDR;

int putchar(int c)
{
    if (c == '\n')
        UART0->DATA = '\r';
    UART0->DATA = c;
    return c;
}

void print(const char *p)
{
    while (*p)
        putchar(*(p++));
}

// Print 64-bit hex (16 digits max)
void print_hex64(uint64_t v, int digits)
{
    for (int i = 15; i >= 0; i--) {
        char c = "0123456789abcdef"[(v >> (4*i)) & 15];
        if (c == '0' && i >= digits) continue;
        putchar(c);
        digits = i;
    }
}

// Print 32-bit hex (8 digits max)
void print_hex(uint32_t v, int digits)
{
    for (int i = 7; i >= 0; i--) {
        char c = "0123456789abcdef"[(v >> (4*i)) & 15];
        if (c == '0' && i >= digits) continue;
        putchar(c);
        digits = i;
    }
}

// Print decimal (no multiply — RV64I has no M extension)
void print_dec(uint32_t v)
{
    if (v >= 100) {
        print(">=100");
        return;
    }

    if      (v >= 90) { putchar('9'); v -= 90; }
    else if (v >= 80) { putchar('8'); v -= 80; }
    else if (v >= 70) { putchar('7'); v -= 70; }
    else if (v >= 60) { putchar('6'); v -= 60; }
    else if (v >= 50) { putchar('5'); v -= 50; }
    else if (v >= 40) { putchar('4'); v -= 40; }
    else if (v >= 30) { putchar('3'); v -= 30; }
    else if (v >= 20) { putchar('2'); v -= 20; }
    else if (v >= 10) { putchar('1'); v -= 10; }

    if      (v >= 9) { putchar('9'); v -= 9; }
    else if (v >= 8) { putchar('8'); v -= 8; }
    else if (v >= 7) { putchar('7'); v -= 7; }
    else if (v >= 6) { putchar('6'); v -= 6; }
    else if (v >= 5) { putchar('5'); v -= 5; }
    else if (v >= 4) { putchar('4'); v -= 4; }
    else if (v >= 3) { putchar('3'); v -= 3; }
    else if (v >= 2) { putchar('2'); v -= 2; }
    else if (v >= 1) { putchar('1'); v -= 1; }
    else putchar('0');
}

char getchar_prompt(char *prompt)
{
    int32_t c = -1;

    uint64_t cycles_begin, cycles_now, cycles;
    __asm__ volatile ("rdcycle %0" : "=r"(cycles_begin));

    if (prompt)
        print(prompt);

    while (c == -1) {
        __asm__ volatile ("rdcycle %0" : "=r"(cycles_now));
        cycles = cycles_now - cycles_begin;
        if (cycles > 12000000) {
            if (prompt)
                print(prompt);
            cycles_begin = cycles_now;
        }
        c = (int32_t)UART0->DATA;
    }
    return (char)c;
}

char getchar()
{
    return getchar_prompt(0);
}

#define QSPI_REG_CRM  0x00100000
#define QSPI_REG_DSPI 0x00400000

void cmd_set_crm(int on)
{
    if (on)
        QSPI0->REG |= QSPI_REG_CRM;
    else
        QSPI0->REG &= ~QSPI_REG_CRM;
}

int cmd_get_crm() {
    return QSPI0->REG & QSPI_REG_CRM;
}

void cmd_set_dspi(int on)
{
    if (on)
        QSPI0->REG |= QSPI_REG_DSPI;
    else
        QSPI0->REG &= ~QSPI_REG_DSPI;
}

int cmd_get_dspi() {
    return QSPI0->REG & QSPI_REG_DSPI;
}

void cmd_read_flash_id()
{
    int pre_dspi = cmd_get_dspi();
    cmd_set_dspi(0);

    uint8_t buffer[4] = { 0x9F, /* zeros */ };
    spi_flashio(buffer, 4, 0);

    for (int i = 1; i <= 3; i++) {
        putchar(' ');
        print_hex(buffer[i], 2);
    }
    putchar('\n');

    cmd_set_dspi(pre_dspi);
}

// --------------------------------------------------------
// ATOMiK v3 Custom Instruction Tests
// --------------------------------------------------------

void cmd_atomik_test()
{
    uint32_t pass = 0;
    uint32_t fail = 0;
    uint64_t val;

    print("\n--- ATOMiK v3 Custom Instruction Tests ---\n\n");

    // Test 1: Load initial state
    print("T1  Load 0xDEADBEEF: ");
    atomik_load(0, 0xDEADBEEFULL);
    val = atomik_read(0);
    if ((val & 0xFFFFFFFF) == 0xDEADBEEF) { print("PASS\n"); pass++; }
    else { print("FAIL got 0x"); print_hex64(val, 16); putchar('\n'); fail++; }

    // Test 2: Read state = initial (accumulator is zero)
    print("T2  State==init:     ");
    val = atomik_read(0);
    if ((val & 0xFFFFFFFF) == 0xDEADBEEF) { print("PASS\n"); pass++; }
    else { print("FAIL got 0x"); print_hex64(val, 16); putchar('\n'); fail++; }

    // Test 3: Accumulate a delta
    print("T3  Accum 0xFF:      ");
    atomik_accum(0x000000FFULL);
    val = atomik_read(0);
    if ((val & 0xFFFFFFFF) == (0xDEADBEEF ^ 0xFF)) { print("PASS\n"); pass++; }
    else { print("FAIL got 0x"); print_hex64(val, 16); putchar('\n'); fail++; }

    // Test 4: XOR cancel (accumulate same delta again)
    print("T4  XOR cancel:      ");
    atomik_accum(0x000000FFULL);
    val = atomik_read(0);
    if ((val & 0xFFFFFFFF) == 0xDEADBEEF) { print("PASS\n"); pass++; }
    else { print("FAIL got 0x"); print_hex64(val, 16); putchar('\n'); fail++; }

    // Test 5: Multiple deltas
    print("T5  Multi-delta:     ");
    atomik_load(0, 0x00000000ULL);
    atomik_accum(0x11111111ULL);
    atomik_accum(0x22222222ULL);
    atomik_accum(0x44444444ULL);
    val = atomik_read(0);
    if ((val & 0xFFFFFFFF) == 0x77777777) { print("PASS\n"); pass++; }
    else { print("FAIL got 0x"); print_hex64(val, 16); putchar('\n'); fail++; }

    // Test 6: 64-bit delta (upper bits)
    print("T6  64-bit delta:    ");
    atomik_load(0, 0ULL);
    atomik_accum(0xCAFEBABE12345678ULL);
    val = atomik_read(0);
    if (val == 0xCAFEBABE12345678ULL) { print("PASS\n"); pass++; }
    else { print("FAIL got 0x"); print_hex64(val, 16); putchar('\n'); fail++; }

    // Test 7: Swap reference state
    print("T7  Swap ref:        ");
    atomik_load(0, 0xAAAAAAAAULL);
    atomik_accum(0x55555555ULL);
    val = atomik_swap(0);  // Returns old current_state
    if ((val & 0xFFFFFFFF) == (0xAAAAAAAA ^ 0x55555555)) { print("PASS\n"); pass++; }
    else { print("FAIL got 0x"); print_hex64(val, 16); putchar('\n'); fail++; }

    // Test 8: After swap, accumulator should be zero → state = new reference
    print("T8  Post-swap state: ");
    val = atomik_read(0);
    if ((val & 0xFFFFFFFF) == (0xAAAAAAAA ^ 0x55555555)) { print("PASS\n"); pass++; }
    else { print("FAIL got 0x"); print_hex64(val, 16); putchar('\n'); fail++; }

    // Test 9: Cycle count for accumulate
    print("T9  Perf (cycles):   ");
    uint64_t c0, c1;
    atomik_load(0, 0);
    __asm__ volatile ("rdcycle %0" : "=r"(c0));
    atomik_accum(0xAAAAAAAAULL);
    val = atomik_read(0);
    __asm__ volatile ("rdcycle %0" : "=r"(c1));
    print_hex((uint32_t)(c1 - c0), 8);
    print(" cycles\n");
    pass++;

    // Summary
    print("\nResult: ");
    print_dec(pass);
    print("/");
    print_dec(pass + fail);
    print(" passed");
    if (fail == 0) print(" -- ALL PASS");
    print("\n");
}

// --------------------------------------------------------
// Benchmark
// --------------------------------------------------------

uint32_t cmd_benchmark(bool verbose, uint32_t *instns_p)
{
    uint8_t data[256];
    uint32_t *words = (void*)data;

    uint32_t x32 = 314159265;

    uint64_t cycles_begin, cycles_end;
    uint64_t instns_begin, instns_end;
    __asm__ volatile ("rdcycle %0" : "=r"(cycles_begin));
    __asm__ volatile ("rdinstret %0" : "=r"(instns_begin));

    for (int i = 0; i < 20; i++) {
        for (int k = 0; k < 256; k++) {
            x32 ^= x32 << 13;
            x32 ^= x32 >> 17;
            x32 ^= x32 << 5;
            data[k] = x32;
        }

        for (int k = 0, p = 0; k < 256; k++) {
            if (data[k])
                data[p++] = k;
        }

        for (int k = 0, p = 0; k < 64; k++) {
            x32 = x32 ^ words[k];
        }
    }

    __asm__ volatile ("rdcycle %0" : "=r"(cycles_end));
    __asm__ volatile ("rdinstret %0" : "=r"(instns_end));

    if (verbose) {
        print("Cycles: 0x");
        print_hex((uint32_t)(cycles_end - cycles_begin), 8);
        putchar('\n');

        print("Instns: 0x");
        print_hex((uint32_t)(instns_end - instns_begin), 8);
        putchar('\n');

        print("Chksum: 0x");
        print_hex(x32, 8);
        putchar('\n');
    }

    if (instns_p)
        *instns_p = (uint32_t)(instns_end - instns_begin);

    return (uint32_t)(cycles_end - cycles_begin);
}

void cmd_benchmark_all()
{
    uint32_t instns = 0;

    print("default        ");
    cmd_set_dspi(0);
    cmd_set_crm(0);
    print(": ");
    print_hex(cmd_benchmark(false, &instns), 8);
    putchar('\n');

    print("dspi-0         ");
    cmd_set_dspi(1);
    print(": ");
    print_hex(cmd_benchmark(false, &instns), 8);
    putchar('\n');

    print("dspi-crm-0     ");
    cmd_set_crm(1);
    print(": ");
    print_hex(cmd_benchmark(false, &instns), 8);
    putchar('\n');

    print("instns         : ");
    print_hex(instns, 8);
    putchar('\n');
}

// --------------------------------------------------------

volatile int i;

#define CLK_FREQ        25175000
#define UART_BAUD       115200

void main()
{
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;

    GPIO0->OE = 0x3F;
    GPIO0->OUT = 0x3F;

    cmd_set_crm(1);
    cmd_set_dspi(1);

    print("\n");
    print("    _  _____ ___  __  __ _ _  __        ____\n");
    print("   / \\|_   _/ _ \\|  \\/  (_) |/ /  ____ |___ \\\n");
    print("  / _ \\ | || | | | |\\/| | | ' /  |____| __) |\n");
    print(" / ___ \\| || |_| | |  | | | . \\         |__ <\n");
    print("/_/   \\_\\_| \\___/|_|  |_|_|_|\\_\\       |___/\n");
    print("\n");
    print("  RV64I + ATOMiK Custom Instructions\n");
    print("  Tang Nano 9K @ 25.2 MHz\n");
    print("\n");

    for (i = 0; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x01;
    for (i = 0; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x02;
    for (i = 0; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x04;
    for (i = 0; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x08;
    for (i = 0; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x10;
    for (i = 0; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x20;
    for (i = 0; i < 10000; i++);
    GPIO0->OUT = 0x3F;
    for (i = 0; i < 10000; i++);
    GPIO0->OUT = 0x00;
    for (i = 0; i < 10000; i++);
    GPIO0->OUT = 0x3F;
    for (i = 0; i < 10000; i++);

    while (1)
    {
        print("\n");
        print("Select an action:\n");
        print("\n");
        print("   [1] Toggle led 1\n");
        print("   [2] Toggle led 2\n");
        print("   [3] Toggle led 3\n");
        print("   [4] Toggle led 4\n");
        print("   [5] Toggle led 5\n");
        print("   [6] Toggle led 6\n");
        print("   [F] Get flash mode\n");
        print("   [I] Read SPI flash ID\n");
        print("   [S] Set Single SPI mode\n");
        print("   [D] Set DSPI mode\n");
        print("   [C] Set DSPI+CRM mode\n");
        print("   [B] Run simplistic benchmark\n");
        print("   [A] Benchmark all configs\n");
        print("   [X] ATOMiK v3 test\n");

        for (int rep = 10; rep > 0; rep--)
        {
            print("\n");
            print("IO State: ");
            print_hex(GPIO0->IN, 8);
            print("\n\n");

            print("Command> ");
            char cmd = getchar();
            if (cmd > 32 && cmd < 127)
                putchar(cmd);
            print("\n");

            switch (cmd)
            {
            case 'F': case 'f':
                print("\nSPI State:\n  DSPI ");
                print(cmd_get_dspi() ? "ON\n" : "OFF\n");
                print("  CRM  ");
                print(cmd_get_crm() ? "ON\n" : "OFF\n");
                break;

            case 'I': case 'i':
                cmd_read_flash_id();
                break;

            case 'S': case 's':
                cmd_set_dspi(0);
                cmd_set_crm(0);
                break;

            case 'D': case 'd':
                cmd_set_crm(0);
                cmd_set_dspi(1);
                break;

            case 'C': case 'c':
                cmd_set_crm(1);
                cmd_set_dspi(1);
                break;

            case 'B': case 'b':
                cmd_benchmark(1, 0);
                break;

            case 'A': case 'a':
                cmd_benchmark_all();
                break;

            case '1': GPIO0->OUT ^= 0x00000001; break;
            case '2': GPIO0->OUT ^= 0x00000002; break;
            case '3': GPIO0->OUT ^= 0x00000004; break;
            case '4': GPIO0->OUT ^= 0x00000008; break;
            case '5': GPIO0->OUT ^= 0x00000010; break;
            case '6': GPIO0->OUT ^= 0x00000020; break;

            case 'X': case 'x':
                cmd_atomik_test();
                break;

            default:
                continue;
            }
        }
    }
}
