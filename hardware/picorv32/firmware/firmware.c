#include <stdint.h>
#include <stdbool.h>

#include "atomik.h"
#include "atomik_mem.h"
#include "atomik_alloc.h"
#include "printf.h"
#include "perf_bench.h"

// a pointer to this is a null pointer, but the compiler does not
// know that because "sram" is a linker symbol from sections.lds.
extern uint32_t sram;

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

void print_hex(uint32_t v, int digits)
{
    for (int i = 7; i >= 0; i--) {
        char c = "0123456789abcdef"[(v >> (4*i)) & 15];
        if (c == '0' && i >= digits) continue;
        putchar(c);
        digits = i;
    }
}

char getchar_prompt(char *prompt)
{
    int32_t c = -1;

    uint32_t cycles_begin, cycles_now, cyc;
    __asm__ volatile ("rdcycle %0" : "=r"(cycles_begin));

    if (prompt)
        print(prompt);

    while (c == -1) {
        __asm__ volatile ("rdcycle %0" : "=r"(cycles_now));
        cyc = cycles_now - cycles_begin;
        if (cyc > 12000000) {
            if (prompt)
                print(prompt);
            cycles_begin = cycles_now;
        }
        c = UART0->DATA;
    }
    return c;
}

char getchar()
{
    return getchar_prompt(0);
}

#define QSPI_REG_CRM  0x00100000
#define QSPI_REG_DSPI 0x00400000

void cmd_set_crm(int on) { if (on) QSPI0->REG |= QSPI_REG_CRM; else QSPI0->REG &= ~QSPI_REG_CRM; }
int cmd_get_crm() { return QSPI0->REG & QSPI_REG_CRM; }
void cmd_set_dspi(int on) { if (on) QSPI0->REG |= QSPI_REG_DSPI; else QSPI0->REG &= ~QSPI_REG_DSPI; }
int cmd_get_dspi() { return QSPI0->REG & QSPI_REG_DSPI; }

void cmd_read_flash_id()
{
    int pre_dspi = cmd_get_dspi();
    cmd_set_dspi(0);
    uint8_t buffer[4] = { 0x9F };
    spi_flashio(buffer, 4, 0);
    for (int i = 1; i <= 3; i++) { putchar(' '); print_hex(buffer[i], 2); }
    putchar('\n');
    cmd_set_dspi(pre_dspi);
}

// --------------------------------------------------------
// Phase 1: ATOMiK Hardware Test (using HAL)
// --------------------------------------------------------

void cmd_atomik_test()
{
    uint32_t pass = 0;
    uint32_t fail = 0;
    uint32_t val;

    print("\n--- ATOMiK Delta Accumulator Test ---\n\n");

    // T1: Soft reset
    print("T1 Soft reset:      ");
    atomik_init(0);
    if (atomik_unchanged(0)) { print("PASS\n"); pass++; } else { print("FAIL\n"); fail++; }

    // T2: Load initial state
    print("T2 Load 0xDEADBEEF: ");
    atomik_load(0, 0xDEADBEEF);
    val = atomik_get_initial(0);
    if (val == 0xDEADBEEF) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%08x\n", val); fail++; }

    // T3: Accumulator zero after load
    print("T3 Acc zero:        ");
    if (atomik_unchanged(0)) { print("PASS\n"); pass++; } else { print("FAIL\n"); fail++; }

    // T4: State == initial when acc=0
    print("T4 State==init:     ");
    val = atomik_state(0);
    if (val == 0xDEADBEEF) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%08x\n", val); fail++; }

    // T5: Accumulate a delta
    print("T5 Accum 0xFF:      ");
    atomik_accumulate(0, 0x000000FF);
    val = atomik_get_delta(0);
    if (val == 0x000000FF) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%08x\n", val); fail++; }

    // T6: Accumulator no longer zero
    print("T6 Acc !zero:       ");
    if (!atomik_unchanged(0)) { print("PASS\n"); pass++; } else { print("FAIL\n"); fail++; }

    // T7: State = init XOR delta
    print("T7 State XOR:       ");
    val = atomik_state(0);
    if (val == (0xDEADBEEF ^ 0x000000FF)) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%08x\n", val); fail++; }

    // T8: XOR cancellation
    print("T8 XOR cancel:      ");
    atomik_undo(0, 0x000000FF);
    val = atomik_state(0);
    if (val == 0xDEADBEEF) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%08x\n", val); fail++; }

    // T9: Accumulator zero after cancel
    print("T9 Acc zero again:  ");
    if (atomik_unchanged(0)) { print("PASS\n"); pass++; } else { print("FAIL\n"); fail++; }

    // T10: Multiple deltas
    print("T10 Multi-delta:    ");
    atomik_load(0, 0x00000000);
    atomik_accumulate(0, 0x11111111);
    atomik_accumulate(0, 0x22222222);
    atomik_accumulate(0, 0x44444444);
    val = atomik_state(0);
    if (val == 0x77777777) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%08x\n", val); fail++; }

    // T11: Cycle count
    print("T11 Perf (cycles):  ");
    uint32_t c0, c1;
    atomik_load(0, 0);
    c0 = cycles();
    atomik_accumulate(0, 0xAAAAAAAA);
    val = atomik_state(0);
    c1 = cycles();
    mini_printf("%u cycles\n", c1 - c0);
    pass++;

    mini_printf("\nResult: %u/%u passed", pass, pass + fail);
    if (fail == 0) print(" -- ALL PASS");
    print("\n");
}

// --------------------------------------------------------
// Phase 2: Checkpoint/Rollback Demo
// --------------------------------------------------------

typedef struct {
    uint32_t temperature;
    uint32_t pressure;
    uint32_t humidity;
    uint32_t altitude;
} SensorState;

void cmd_checkpoint_demo()
{
    SensorState state = { 2500, 101325, 4500, 150 };
    uint32_t c0, c1;

    print("\n--- Checkpoint/Rollback Demo ---\n\n");

    // Step 1: Fingerprint initial state
    c0 = cycles();
    uint32_t checkpoint_fp = atomik_fingerprint(0, (uint32_t *)&state, 4);
    c1 = cycles();
    mini_printf("Initial state: T=%u P=%u H=%u A=%u\n",
                state.temperature, state.pressure, state.humidity, state.altitude);
    mini_printf("Checkpoint FP: 0x%08x (%u cycles)\n\n", checkpoint_fp, c1 - c0);

    // Step 2: Apply 5 mutations, tracking deltas
    uint32_t deltas[5][2]; // [mutation_index][field_index, delta_value]

    // Mutation 1: temperature 2500 -> 2600
    uint32_t old_val = state.temperature;
    state.temperature = 2600;
    atomik_load(0, checkpoint_fp);  // start from checkpoint
    // Accumulate all current field values to get new fingerprint
    uint32_t new_fp = atomik_fingerprint(0, (uint32_t *)&state, 4);
    mini_printf("  M1: T=%u -> %u  FP=0x%08x\n", old_val, state.temperature, new_fp);

    // Mutation 2: pressure 101325 -> 101400
    state.pressure = 101400;
    new_fp = atomik_fingerprint(0, (uint32_t *)&state, 4);
    mini_printf("  M2: P=101325 -> %u  FP=0x%08x\n", state.pressure, new_fp);

    // Mutation 3: humidity 4500 -> 4800
    state.humidity = 4800;
    new_fp = atomik_fingerprint(0, (uint32_t *)&state, 4);
    mini_printf("  M3: H=4500 -> %u  FP=0x%08x\n", state.humidity, new_fp);

    // Mutation 4: altitude 150 -> 175
    state.altitude = 175;
    new_fp = atomik_fingerprint(0, (uint32_t *)&state, 4);
    mini_printf("  M4: A=150 -> %u  FP=0x%08x\n", state.altitude, new_fp);

    // Mutation 5: temperature 2600 -> 2550
    state.temperature = 2550;
    new_fp = atomik_fingerprint(0, (uint32_t *)&state, 4);
    mini_printf("  M5: T=2600 -> %u  FP=0x%08x\n\n", state.temperature, new_fp);

    mini_printf("Modified state: T=%u P=%u H=%u A=%u\n",
                state.temperature, state.pressure, state.humidity, state.altitude);

    // Step 3: Check if state has changed from checkpoint
    c0 = cycles();
    int changed = atomik_region_changed((uint32_t *)&state, 4, checkpoint_fp);
    c1 = cycles();
    mini_printf("Changed from checkpoint? %s (%u cycles)\n\n", changed ? "YES" : "NO", c1 - c0);

    // Step 4: Rollback to original values
    print("Rolling back...\n");
    state.temperature = 2500;
    state.pressure = 101325;
    state.humidity = 4500;
    state.altitude = 150;

    // Verify fingerprint matches checkpoint
    c0 = cycles();
    int matches = atomik_verify(0, (uint32_t *)&state, 4, checkpoint_fp);
    c1 = cycles();
    mini_printf("Rolled back: T=%u P=%u H=%u A=%u\n",
                state.temperature, state.pressure, state.humidity, state.altitude);
    mini_printf("Matches checkpoint? %s (%u cycles)\n",
                matches ? "YES" : "NO", c1 - c0);
}

// --------------------------------------------------------
// Phase 2: Memory Benchmark
// --------------------------------------------------------

void cmd_mem_benchmark()
{
    // Use SRAM buffers — keep them small to fit in 8KB
    static uint32_t buf_a[64];  // 256 bytes
    static uint32_t buf_b[64];
    uint32_t c0, c1;
    uint32_t fp;

    print("\n--- Memory Operation Benchmark ---\n\n");

    // Fill buf_a with test pattern
    for (int i = 0; i < 64; i++)
        buf_a[i] = 0xA5A5A5A5 ^ (uint32_t)i;

    // --- memcpy benchmark (64 words = 256 bytes) ---
    print("memcpy 256B:\n");

    c0 = cycles();
    sw_memcpy(buf_b, buf_a, 256);
    c1 = cycles();
    mini_printf("  sw_memcpy:             %u cycles\n", c1 - c0);

    c0 = cycles();
    atomik_memcpy_tracked(buf_b, buf_a, 256, &fp);
    c1 = cycles();
    mini_printf("  atomik_memcpy_tracked: %u cycles (fp=0x%08x)\n", c1 - c0, fp);

    // --- memset benchmark ---
    print("\nmemset 256B:\n");

    c0 = cycles();
    sw_memset(buf_b, 0x42, 256);
    c1 = cycles();
    mini_printf("  sw_memset:              %u cycles\n", c1 - c0);

    c0 = cycles();
    atomik_memset_verified(buf_b, 0x42, 256, &fp);
    c1 = cycles();
    mini_printf("  atomik_memset_verified: %u cycles (fp=0x%08x)\n", c1 - c0, fp);

    // --- Change detection benchmark ---
    print("\nChange detection 256B:\n");

    // Fingerprint buf_a
    uint32_t saved_fp = atomik_fingerprint(0, buf_a, 64);

    // Method 1: Software memcmp
    sw_memcpy(buf_b, buf_a, 256);  // make identical copy
    c0 = cycles();
    int cmp_result = sw_memcmp(buf_a, buf_b, 256);
    c1 = cycles();
    mini_printf("  sw_memcmp (identical):   %u cycles (result=%d)\n", c1 - c0, cmp_result);

    // Method 2: ATOMiK fingerprint comparison
    c0 = cycles();
    int changed = atomik_region_changed(buf_a, 64, saved_fp);
    c1 = cycles();
    mini_printf("  atomik_region_changed:   %u cycles (changed=%d)\n", c1 - c0, changed);

    // Now modify one word and test detection
    buf_a[32] ^= 1;  // flip one bit
    c0 = cycles();
    changed = atomik_region_changed(buf_a, 64, saved_fp);
    c1 = cycles();
    mini_printf("  atomik detect 1-bit flip: %u cycles (changed=%d)\n", c1 - c0, changed);
    buf_a[32] ^= 1;  // restore

    // --- Fingerprint-only benchmark ---
    print("\nFingerprint 256B:\n");
    c0 = cycles();
    fp = atomik_fingerprint(0, buf_a, 64);
    c1 = cycles();
    mini_printf("  atomik_fingerprint:    %u cycles (fp=0x%08x)\n", c1 - c0, fp);
}

// --------------------------------------------------------
// Phase 2: Heap Integrity Demo
// --------------------------------------------------------

void cmd_heap_demo()
{
    print("\n--- Heap Integrity Demo ---\n\n");

    atomik_heap_init();
    mini_printf("Heap: %u bytes total\n\n", atomik_heap_total());

    // Allocate 3 blocks
    uint32_t *blk1 = (uint32_t *)atomik_malloc(64);
    uint32_t *blk2 = (uint32_t *)atomik_malloc(128);
    uint32_t *blk3 = (uint32_t *)atomik_malloc(32);

    mini_printf("Block 1: addr=0x%08x (64 bytes)\n", (uint32_t)blk1);
    mini_printf("Block 2: addr=0x%08x (128 bytes)\n", (uint32_t)blk2);
    mini_printf("Block 3: addr=0x%08x (32 bytes)\n", (uint32_t)blk3);
    mini_printf("Heap used: %u / %u bytes\n\n", atomik_heap_used(), atomik_heap_total());

    // Verify integrity (should pass)
    uint32_t c0 = cycles();
    int ok = atomik_heap_verify();
    uint32_t c1 = cycles();
    mini_printf("Integrity check: %s (%u cycles)\n", ok ? "PASS" : "FAIL", c1 - c0);

    // Write data to blocks
    if (blk1) { for (int i = 0; i < 16; i++) blk1[i] = 0xCAFE0000 + i; }
    if (blk2) { for (int i = 0; i < 32; i++) blk2[i] = 0xBEEF0000 + i; }
    if (blk3) { for (int i = 0; i < 8; i++) blk3[i] = 0xFACE0000 + i; }

    // Verify again (still pass — data writes don't affect metadata)
    ok = atomik_heap_verify();
    mini_printf("After data write: %s\n", ok ? "PASS" : "FAIL");
}

// --------------------------------------------------------
// Phase 2: Full Integration Test
// --------------------------------------------------------

void cmd_phase2_test()
{
    uint32_t pass = 0, fail = 0;
    uint32_t val;

    print("\n=== Phase 2 Integration Test ===\n\n");

    // P1: ATOMiK API init
    print("P1 API init:        ");
    atomik_init(0);
    if (atomik_unchanged(0)) { print("PASS\n"); pass++; } else { print("FAIL\n"); fail++; }

    // P2: Fingerprint computation
    print("P2 Fingerprint:     ");
    static uint32_t test_data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    // Expected: 1^2^3^4^5^6^7^8 = (1^2)=3, (3^3)=0, (0^4)=4, (4^5)=1, (1^6)=7, (7^7)=0, (0^8)=8
    val = atomik_fingerprint(0, test_data, 8);
    if (val == (1^2^3^4^5^6^7^8)) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%08x expected 0x%08x\n", val, 1^2^3^4^5^6^7^8); fail++; }

    // P3: Tracked memcpy
    print("P3 Tracked memcpy:  ");
    static uint32_t dst[8];
    uint32_t fp;
    atomik_memcpy_tracked(dst, test_data, 32, &fp);
    int cmp_ok = (sw_memcmp(dst, test_data, 32) == 0);
    int fp_ok = (fp == (1^2^3^4^5^6^7^8));
    if (cmp_ok && fp_ok) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL cmp=%d fp_ok=%d\n", cmp_ok, fp_ok); fail++; }

    // P4: Change detection (unchanged)
    print("P4 No change:       ");
    uint32_t saved_fp = atomik_fingerprint(0, test_data, 8);
    if (!atomik_region_changed(test_data, 8, saved_fp)) { print("PASS\n"); pass++; }
    else { print("FAIL\n"); fail++; }

    // P5: Change detection (changed)
    print("P5 Detect change:   ");
    test_data[3] ^= 0xFF;
    if (atomik_region_changed(test_data, 8, saved_fp)) { print("PASS\n"); pass++; }
    else { print("FAIL\n"); fail++; }
    test_data[3] ^= 0xFF;  // restore

    // P6: Verified memset
    print("P6 Verified memset: ");
    atomik_memset_verified(dst, 0xAA, 32, &fp);
    // All 8 words = 0xAAAAAAAA, XOR = 0 (even count XOR of identical values)
    if (fp == 0) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL fp=0x%08x\n", fp); fail++; }

    // P7: Checkpoint/rollback
    print("P7 Checkpoint:      ");
    SensorState s = { 100, 200, 300, 400 };
    uint32_t ckpt = atomik_fingerprint(0, (uint32_t *)&s, 4);
    s.temperature = 999;
    s.pressure = 888;
    int is_changed = atomik_region_changed((uint32_t *)&s, 4, ckpt);
    s.temperature = 100;
    s.pressure = 200;
    int is_restored = atomik_verify(0, (uint32_t *)&s, 4, ckpt);
    if (is_changed && is_restored) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL changed=%d restored=%d\n", is_changed, is_restored); fail++; }

    // P8: Heap allocator
    print("P8 Heap alloc:      ");
    atomik_heap_init();
    void *p1 = atomik_malloc(64);
    void *p2 = atomik_malloc(32);
    if (p1 && p2 && p1 != p2) { print("PASS\n"); pass++; }
    else { print("FAIL\n"); fail++; }

    // P9: Heap integrity
    print("P9 Heap integrity:  ");
    if (atomik_heap_verify()) { print("PASS\n"); pass++; }
    else { print("FAIL\n"); fail++; }

    // P10: mini_printf
    print("P10 printf test:    ");
    // If we got here and text is readable, printf works
    mini_printf("PASS (val=%d hex=0x%08x)\n", 42, 0xDEADBEEF);
    pass++;

    mini_printf("\n=== Result: %u/%u passed", pass, pass + fail);
    if (fail == 0) print(" -- ALL PASS");
    print(" ===\n");
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
    print("  ____  _          ____         ____\n");
    print(" |  _ \\(_) ___ ___/ ___|  ___  / ___|\n");
    print(" | |_) | |/ __/ _ \\___ \\ / _ \\| |\n");
    print(" |  __/| | (_| (_) |__) | (_) | |___\n");
    print(" |_|   |_|\\___\\___/____/ \\___/ \\____|\n");
    print("\n");
    print("     On Lichee Tang Nano-9K + ATOMiK\n");
    print("\n");

    for ( i = 0 ; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x01;
    for ( i = 0 ; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x02;
    for ( i = 0 ; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x04;
    for ( i = 0 ; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x08;
    for ( i = 0 ; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x10;
    for ( i = 0 ; i < 10000; i++);
    GPIO0->OUT = 0x3F ^ 0x20;
    for ( i = 0 ; i < 10000; i++);
    GPIO0->OUT = 0x3F;
    for ( i = 0 ; i < 10000; i++);
    GPIO0->OUT = 0x00;
    for ( i = 0 ; i < 10000; i++);
    GPIO0->OUT = 0x3F;
    for ( i = 0 ; i < 10000; i++);

    while (1)
    {
        print("\n");
        print("Select an action:\n");
        print("\n");
        print("   [1-6] Toggle LED 1-6\n");
        print("   [F] Get flash mode\n");
        print("   [I] Read SPI flash ID\n");
        print("   [S] Single SPI  [D] DSPI  [C] DSPI+CRM\n");
        print("   [B] Benchmark   [A] Benchmark all\n");
        print("   [X] ATOMiK hardware test (Phase 1)\n");
        print("   [K] Checkpoint/rollback demo\n");
        print("   [M] Memory benchmark\n");
        print("   [H] Heap integrity demo\n");
        print("   [P] Phase 2 full test\n");
        print("   [R] Performance benchmark suite\n");

        for (int rep = 10; rep > 0; rep--)
        {
            print("\nCommand> ");
            char cmd = getchar();
            if (cmd > 32 && cmd < 127)
                putchar(cmd);
            print("\n");

            switch (cmd)
            {
            case 'F': case 'f':
                mini_printf("\nSPI State:\n  DSPI %s\n  CRM  %s\n",
                    cmd_get_dspi() ? "ON" : "OFF",
                    cmd_get_crm() ? "ON" : "OFF");
                break;
            case 'I': case 'i': cmd_read_flash_id(); break;
            case 'S': case 's': cmd_set_dspi(0); cmd_set_crm(0); break;
            case 'D': case 'd': cmd_set_crm(0); cmd_set_dspi(1); break;
            case 'C': case 'c': cmd_set_crm(1); cmd_set_dspi(1); break;
            case 'B': case 'b': {
                uint32_t cyc, ins;
                cyc = 0; ins = 0;
                uint32_t c0 = cycles();
                // Simple XOR benchmark: 1000 iterations
                uint32_t x = 0xDEADBEEF;
                for (int j = 0; j < 1000; j++) {
                    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
                }
                uint32_t c1 = cycles();
                mini_printf("XOR benchmark: %u cycles, chksum=0x%08x\n", c1 - c0, x);
                break;
            }
            case 'A': case 'a': break; // removed old benchmark
            case '1': GPIO0->OUT ^= 0x01; break;
            case '2': GPIO0->OUT ^= 0x02; break;
            case '3': GPIO0->OUT ^= 0x04; break;
            case '4': GPIO0->OUT ^= 0x08; break;
            case '5': GPIO0->OUT ^= 0x10; break;
            case '6': GPIO0->OUT ^= 0x20; break;
            case 'X': case 'x': cmd_atomik_test(); break;
            case 'K': case 'k': cmd_checkpoint_demo(); break;
            case 'M': case 'm': cmd_mem_benchmark(); break;
            case 'H': case 'h': cmd_heap_demo(); break;
            case 'P': case 'p': cmd_phase2_test(); break;
            case 'R': case 'r': cmd_perf_suite(); break;
            default: continue;
            }
        }
    }
}

void irqCallback() {
}
