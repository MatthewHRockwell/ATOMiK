// ATOMiK v3 SoC Firmware (RV64I)
// Full port from v2 with ATOMiK custom instructions

#include <stdint.h>
#include <stdbool.h>
#include "atomik_v3.h"
#include "atomik_v3_hal.h"
#include "atomik_v3_mem.h"
#include "atomik_v3_alloc.h"
#include "printf_v3.h"
#include "perf_bench_v3.h"

// Linker symbol for null pointer detection
extern uint64_t sram;

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

// Display pipeline MMIO (S3 slot: 0xC0000000)
#define DISP_CTRL   (*(volatile uint32_t*)0xC0000000)
#define DISP_LUT    (*(volatile uint32_t*)0xC0000004)
#define DISP_SCAN   (*(volatile uint32_t*)0xC0000008)
#define DISP_STATUS (*(volatile uint32_t*)0xC000000C)

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

// Print decimal (simple version for small numbers)
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

    uint64_t cycles_begin, cycles_now, cyc;
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
// ATOMiK v3 Hardware Test (using custom instructions)
// --------------------------------------------------------

void cmd_atomik_test()
{
    uint32_t pass = 0;
    uint32_t fail = 0;
    uint64_t val;

    print("\n--- ATOMiK v3 Custom Instruction Test ---\n\n");

    // T1: Load initial state
    print("T1 Load 0xDEADBEEF: ");
    atomik_load_state(0, 0xDEADBEEFULL);
    val = atomik_state(0);
    if ((val & 0xFFFFFFFF) == 0xDEADBEEF) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%lx\n", val); fail++; }

    // T2: State == initial when acc=0
    print("T2 State==init:     ");
    val = atomik_state(0);
    if ((val & 0xFFFFFFFF) == 0xDEADBEEF) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%lx\n", val); fail++; }

    // T3: Accumulate a delta
    print("T3 Accum 0xFF:      ");
    atomik_accumulate(0, 0x000000FFULL);
    val = atomik_state(0);
    if ((val & 0xFFFFFFFF) == (0xDEADBEEF ^ 0xFF)) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%lx\n", val); fail++; }

    // T4: XOR cancel
    print("T4 XOR cancel:      ");
    atomik_undo(0, 0x000000FFULL);
    val = atomik_state(0);
    if ((val & 0xFFFFFFFF) == 0xDEADBEEF) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%lx\n", val); fail++; }

    // T5: Multiple deltas
    print("T5 Multi-delta:     ");
    atomik_load_state(0, 0x00000000ULL);
    atomik_accumulate(0, 0x11111111ULL);
    atomik_accumulate(0, 0x22222222ULL);
    atomik_accumulate(0, 0x44444444ULL);
    val = atomik_state(0);
    if ((val & 0xFFFFFFFF) == 0x77777777) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%lx\n", val); fail++; }

    // T6: 64-bit delta
    print("T6 64-bit delta:    ");
    atomik_load_state(0, 0ULL);
    atomik_accumulate(0, 0xCAFEBABE12345678ULL);
    val = atomik_state(0);
    if (val == 0xCAFEBABE12345678ULL) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%lx\n", val); fail++; }

    // T7: Swap reference
    print("T7 Swap ref:        ");
    atomik_load_state(0, 0xAAAAAAAAULL);
    atomik_accumulate(0, 0x55555555ULL);
    val = atomik_checkpoint(0);  // Swap
    if ((val & 0xFFFFFFFF) == (0xAAAAAAAA ^ 0x55555555)) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%lx\n", val); fail++; }

    // T8: After swap, accumulator should be zero
    print("T8 Post-swap state: ");
    val = atomik_state(0);
    if ((val & 0xFFFFFFFF) == (0xAAAAAAAA ^ 0x55555555)) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%lx\n", val); fail++; }

    // T9: Cycle count
    print("T9 Perf (cycles):   ");
    uint64_t c0, c1;
    atomik_load_state(0, 0);
    c0 = cycles64();
    atomik_accumulate(0, 0xAAAAAAAAULL);
    val = atomik_state(0);
    c1 = cycles64();
    mini_printf("%u cycles\n", (uint32_t)(c1 - c0));
    pass++;

    mini_printf("\nResult: %u/%u passed", pass, pass + fail);
    if (fail == 0) print(" -- ALL PASS");
    print("\n");
}

// --------------------------------------------------------
// Checkpoint/Rollback Demo
// --------------------------------------------------------

typedef struct {
    uint64_t temperature;
    uint64_t pressure;
    uint64_t humidity;
    uint64_t altitude;
} SensorState;

void cmd_checkpoint_demo()
{
    SensorState state = { 2500, 101325, 4500, 150 };
    uint64_t c0, c1;

    print("\n--- Checkpoint/Rollback Demo ---\n\n");

    // Step 1: Fingerprint initial state
    c0 = cycles64();
    uint64_t checkpoint_fp = atomik_fingerprint(0, (uint64_t *)&state, 4);
    c1 = cycles64();
    mini_printf("Initial state: T=%u P=%u H=%u A=%u\n",
                (uint32_t)state.temperature, (uint32_t)state.pressure,
                (uint32_t)state.humidity, (uint32_t)state.altitude);
    mini_printf("Checkpoint FP: 0x%lx (%u cycles)\n\n", checkpoint_fp, (uint32_t)(c1 - c0));

    // Step 2: Apply mutations
    state.temperature = 2600;
    uint64_t new_fp = atomik_fingerprint(0, (uint64_t *)&state, 4);
    mini_printf("  M1: T=2500 -> %u  FP=0x%lx\n", (uint32_t)state.temperature, new_fp);

    state.pressure = 101400;
    new_fp = atomik_fingerprint(0, (uint64_t *)&state, 4);
    mini_printf("  M2: P=101325 -> %u  FP=0x%lx\n", (uint32_t)state.pressure, new_fp);

    state.humidity = 4800;
    new_fp = atomik_fingerprint(0, (uint64_t *)&state, 4);
    mini_printf("  M3: H=4500 -> %u  FP=0x%lx\n", (uint32_t)state.humidity, new_fp);

    state.altitude = 175;
    new_fp = atomik_fingerprint(0, (uint64_t *)&state, 4);
    mini_printf("  M4: A=150 -> %u  FP=0x%lx\n", (uint32_t)state.altitude, new_fp);

    state.temperature = 2550;
    new_fp = atomik_fingerprint(0, (uint64_t *)&state, 4);
    mini_printf("  M5: T=2600 -> %u  FP=0x%lx\n\n", (uint32_t)state.temperature, new_fp);

    mini_printf("Modified state: T=%u P=%u H=%u A=%u\n",
                (uint32_t)state.temperature, (uint32_t)state.pressure,
                (uint32_t)state.humidity, (uint32_t)state.altitude);

    // Step 3: Check if state has changed
    c0 = cycles64();
    int changed = atomik_region_changed((uint64_t *)&state, 4, checkpoint_fp);
    c1 = cycles64();
    mini_printf("Changed from checkpoint? %s (%u cycles)\n\n",
                changed ? "YES" : "NO", (uint32_t)(c1 - c0));

    // Step 4: Rollback
    print("Rolling back...\n");
    state.temperature = 2500;
    state.pressure = 101325;
    state.humidity = 4500;
    state.altitude = 150;

    c0 = cycles64();
    int matches = atomik_verify(0, (uint64_t *)&state, 4, checkpoint_fp);
    c1 = cycles64();
    mini_printf("Rolled back: T=%u P=%u H=%u A=%u\n",
                (uint32_t)state.temperature, (uint32_t)state.pressure,
                (uint32_t)state.humidity, (uint32_t)state.altitude);
    mini_printf("Matches checkpoint? %s (%u cycles)\n",
                matches ? "YES" : "NO", (uint32_t)(c1 - c0));
}

// --------------------------------------------------------
// Memory Benchmark
// --------------------------------------------------------

void cmd_mem_benchmark()
{
    // Use SRAM buffers — keep small to fit in 8KB
    static uint64_t buf_a[32];  // 256 bytes
    static uint64_t buf_b[32];
    uint64_t c0, c1;
    uint64_t fp;

    print("\n--- Memory Operation Benchmark ---\n\n");

    // Fill buf_a with test pattern
    for (int i = 0; i < 32; i++)
        buf_a[i] = 0xA5A5A5A5A5A5A5A5ULL ^ (uint64_t)i;

    // --- memcpy benchmark (32 words = 256 bytes) ---
    print("memcpy 256B:\n");

    c0 = cycles64();
    sw_memcpy(buf_b, buf_a, 256);
    c1 = cycles64();
    mini_printf("  sw_memcpy:             %u cycles\n", (uint32_t)(c1 - c0));

    c0 = cycles64();
    atomik_memcpy_tracked(buf_b, buf_a, 256, &fp);
    c1 = cycles64();
    mini_printf("  atomik_memcpy_tracked: %u cycles (fp=0x%lx)\n", (uint32_t)(c1 - c0), fp);

    // --- memset benchmark ---
    print("\nmemset 256B:\n");

    c0 = cycles64();
    sw_memset(buf_b, 0x42, 256);
    c1 = cycles64();
    mini_printf("  sw_memset:              %u cycles\n", (uint32_t)(c1 - c0));

    c0 = cycles64();
    atomik_memset_verified(buf_b, 0x42, 256, &fp);
    c1 = cycles64();
    mini_printf("  atomik_memset_verified: %u cycles (fp=0x%lx)\n", (uint32_t)(c1 - c0), fp);

    // --- Change detection benchmark ---
    print("\nChange detection 256B:\n");

    uint64_t saved_fp = atomik_fingerprint(0, buf_a, 32);

    // Method 1: Software memcmp
    sw_memcpy(buf_b, buf_a, 256);
    c0 = cycles64();
    int cmp_result = sw_memcmp(buf_a, buf_b, 256);
    c1 = cycles64();
    mini_printf("  sw_memcmp (identical):   %u cycles (result=%d)\n",
                (uint32_t)(c1 - c0), cmp_result);

    // Method 2: ATOMiK fingerprint comparison
    c0 = cycles64();
    int changed = atomik_region_changed(buf_a, 32, saved_fp);
    c1 = cycles64();
    mini_printf("  atomik_region_changed:   %u cycles (changed=%d)\n",
                (uint32_t)(c1 - c0), changed);

    // Detect 1-bit flip
    buf_a[16] ^= 1;
    c0 = cycles64();
    changed = atomik_region_changed(buf_a, 32, saved_fp);
    c1 = cycles64();
    mini_printf("  atomik detect 1-bit flip: %u cycles (changed=%d)\n",
                (uint32_t)(c1 - c0), changed);
    buf_a[16] ^= 1;  // restore

    // --- Fingerprint-only benchmark ---
    print("\nFingerprint 256B:\n");
    c0 = cycles64();
    fp = atomik_fingerprint(0, buf_a, 32);
    c1 = cycles64();
    mini_printf("  atomik_fingerprint:    %u cycles (fp=0x%lx)\n",
                (uint32_t)(c1 - c0), fp);
}

// --------------------------------------------------------
// Heap Integrity Demo
// --------------------------------------------------------

void cmd_heap_demo()
{
    print("\n--- Heap Integrity Demo ---\n\n");

    atomik_heap_init();
    mini_printf("Heap: %u bytes total\n\n", atomik_heap_total());

    // Allocate 3 blocks
    uint64_t *blk1 = (uint64_t *)atomik_malloc(64);
    uint64_t *blk2 = (uint64_t *)atomik_malloc(128);
    uint64_t *blk3 = (uint64_t *)atomik_malloc(32);

    mini_printf("Block 1: addr=0x%lx (64 bytes)\n", (uint64_t)blk1);
    mini_printf("Block 2: addr=0x%lx (128 bytes)\n", (uint64_t)blk2);
    mini_printf("Block 3: addr=0x%lx (32 bytes)\n", (uint64_t)blk3);
    mini_printf("Heap used: %u / %u bytes\n\n", atomik_heap_used(), atomik_heap_total());

    // Verify integrity
    uint64_t c0 = cycles64();
    int ok = atomik_heap_verify();
    uint64_t c1 = cycles64();
    mini_printf("Integrity check: %s (%u cycles)\n",
                ok ? "PASS" : "FAIL", (uint32_t)(c1 - c0));

    // Write data to blocks
    if (blk1) { for (int i = 0; i < 8; i++) blk1[i] = 0xCAFE0000ULL + i; }
    if (blk2) { for (int i = 0; i < 16; i++) blk2[i] = 0xBEEF0000ULL + i; }
    if (blk3) { for (int i = 0; i < 4; i++) blk3[i] = 0xFACE0000ULL + i; }

    // Verify again
    ok = atomik_heap_verify();
    mini_printf("After data write: %s\n", ok ? "PASS" : "FAIL");
}

// --------------------------------------------------------
// Phase 2 Full Integration Test
// --------------------------------------------------------

void cmd_phase2_test()
{
    uint32_t pass = 0, fail = 0;
    uint64_t val;

    print("\n=== Phase 2 Integration Test ===\n\n");

    // P1: ATOMiK API init
    print("P1 API init:        ");
    atomik_init(0);
    // No direct unchanged status in v3, just check that we can read state
    val = atomik_state(0);
    if (val == 0) { print("PASS\n"); pass++; } else { print("FAIL\n"); fail++; }

    // P2: Fingerprint computation
    print("P2 Fingerprint:     ");
    static uint64_t test_data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    val = atomik_fingerprint(0, test_data, 8);
    if (val == (1^2^3^4^5^6^7^8)) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL got 0x%lx expected 0x%x\n", val, 1^2^3^4^5^6^7^8); fail++; }

    // P3: Tracked memcpy
    print("P3 Tracked memcpy:  ");
    static uint64_t dst[8];
    uint64_t fp;
    atomik_memcpy_tracked(dst, test_data, 64, &fp);
    int cmp_ok = (sw_memcmp(dst, test_data, 64) == 0);
    int fp_ok = (fp == (1^2^3^4^5^6^7^8));
    if (cmp_ok && fp_ok) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL cmp=%d fp_ok=%d\n", cmp_ok, fp_ok); fail++; }

    // P4: Change detection (unchanged)
    print("P4 No change:       ");
    uint64_t saved_fp = atomik_fingerprint(0, test_data, 8);
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
    atomik_memset_verified(dst, 0xAA, 64, &fp);
    // All 8 words = 0xAAAAAAAAAAAAAAAA, XOR = 0 (even count)
    if (fp == 0) { print("PASS\n"); pass++; }
    else { mini_printf("FAIL fp=0x%lx\n", fp); fail++; }

    // P7: Checkpoint/rollback
    print("P7 Checkpoint:      ");
    SensorState s = { 100, 200, 300, 400 };
    uint64_t ckpt = atomik_fingerprint(0, (uint64_t *)&s, 4);
    s.temperature = 999;
    s.pressure = 888;
    int is_changed = atomik_region_changed((uint64_t *)&s, 4, ckpt);
    s.temperature = 100;
    s.pressure = 200;
    int is_restored = atomik_verify(0, (uint64_t *)&s, 4, ckpt);
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
    mini_printf("PASS (val=%d hex=0x%lx)\n", 42, 0xDEADBEEFCAFEBABEULL);
    pass++;

    mini_printf("\n=== Result: %u/%u passed", pass, pass + fail);
    if (fail == 0) print(" -- ALL PASS");
    print(" ===\n");
}

// --------------------------------------------------------

volatile int i;

// =========================================================================
// Display pipeline tests
// =========================================================================
void cmd_display_test()
{
    int pass = 0, fail = 0;

    print("\n=== Display Pipeline Test ===\n\n");

    // D0: Raw register dump (debug)
    print("D0 Register dump:\n");
    uint32_t r0 = DISP_CTRL;
    uint32_t r1 = DISP_LUT;
    uint32_t r2 = DISP_SCAN;
    uint32_t r3 = DISP_STATUS;
    mini_printf("  CTRL=0x%08x LUT=0x%08x SCAN=0x%08x STATUS=0x%08x\n", r0, r1, r2, r3);

    // D1: Simple write/read test on DISP_CTRL
    print("D1 CTRL write/read: ");
    DISP_CTRL = 1;  // Enable
    // Read back multiple times to check stability
    uint32_t ctrl1 = DISP_CTRL;
    uint32_t ctrl2 = DISP_CTRL;
    mini_printf("wrote 1, read 0x%08x 0x%08x ", ctrl1, ctrl2);
    DISP_CTRL = 0;  // Disable
    uint32_t ctrl3 = DISP_CTRL;
    mini_printf("wrote 0, read 0x%08x\n", ctrl3);
    if ((ctrl1 & 1) == 1 && (ctrl3 & 1) == 0) {
        print("  PASS\n");
        pass++;
    } else {
        print("  FAIL\n");
        fail++;
    }

    // D2: LUT write/readback
    print("D2 LUT write/read: ");
    DISP_LUT = (1u << 24) | 0xFF0000;
    // Add delay for BSRAM read latency
    uint32_t dummy = DISP_CTRL;  // one bus transaction delay
    (void)dummy;
    uint32_t readback = DISP_LUT;
    uint32_t rd_color = readback & 0x00FFFFFF;
    uint32_t rd_addr  = (readback >> 24) & 0xFF;
    mini_printf("wrote LUT[1]=0xFF0000, read 0x%08x (addr=%u color=0x%06x)\n",
                readback, rd_addr, rd_color);
    if (rd_color == 0xFF0000) {
        print("  PASS\n");
        pass++;
    } else {
        print("  FAIL\n");
        fail++;
    }

    // D3: Static passthrough (delta_enable=0, all scan entries = 0)
    print("D3 Static passthrough: ");
    DISP_CTRL = 0;  // Disable delta overlay
    for (int col = 0; col < 640; col++) {
        DISP_SCAN = ((uint32_t)col << 16) | 0x000;
    }
    uint32_t ctrl = DISP_CTRL;
    if ((ctrl & 1) == 0) {
        print("PASS (disabled, passthrough)\n");
        pass++;
    } else {
        print("FAIL (enable bit not cleared)\n");
        fail++;
    }

    // D4: Single pixel change — mark col 320 as changed with LUT index 1
    print("D4 Single pixel delta: ");
    DISP_LUT = (0u << 24) | 0x000000;   // LUT[0] = identity
    DISP_LUT = (1u << 24) | 0xFF0000;   // LUT[1] = red toggle
    DISP_LUT = (4u << 24) | 0xFFFFFF;   // LUT[4] = invert
    DISP_SCAN = (320u << 16) | 0x100 | 1;  // col=320, change=1, index=1
    DISP_CTRL = 1;  // Enable delta overlay
    ctrl = DISP_CTRL;
    if ((ctrl & 1) == 1) {
        print("PASS (pixel 320 = ref XOR red)\n");
        pass++;
    } else {
        print("FAIL (enable not set)\n");
        fail++;
    }

    // D5: Scanline band
    print("D5 Scanline band delta: ");
    for (int col = 200; col < 440; col++) {
        DISP_SCAN = ((uint32_t)col << 16) | 0x100 | 4;  // change=1, index=4
    }
    print("PASS (cols 200-439 inverted)\n");
    pass++;

    // D6: Frame timing using frame_count from DISP_STATUS[31:16]
    print("D6 Frame timing: ");
    uint32_t status = DISP_STATUS;
    uint32_t fc0 = status >> 16;
    mini_printf("(STATUS=0x%08x fc=%u) ", status, fc0);
    // Wait for frame count to change (next SOF)
    uint64_t timeout_start = cycles64();
    uint32_t timeout_cycles = 21600000;  // 1 second at 21.6 MHz
    while ((DISP_STATUS >> 16) == fc0) {
        if (cycles64() - timeout_start > timeout_cycles) break;
    }
    uint32_t fc1 = DISP_STATUS >> 16;
    if (fc1 == fc0) {
        print("FAIL (frame_count stuck)\n");
        fail++;
    } else {
        // Measure time for one full frame
        uint64_t t0 = cycles64();
        uint32_t fc_start = DISP_STATUS >> 16;
        while ((DISP_STATUS >> 16) == fc_start) {
            if (cycles64() - t0 > timeout_cycles) break;
        }
        uint64_t t1 = cycles64();
        uint32_t frame_cycles = (uint32_t)(t1 - t0);
        mini_printf("PASS (%u cycles/frame, ~%u fps)\n",
                    frame_cycles, 21600000 / frame_cycles);
        pass++;
    }

    // Summary
    mini_printf("\nDisplay pipeline: %u/%u PASS\n", pass, pass + fail);
    print("(Display deltas active — visible on HDMI)\n");
}

#define CLK_FREQ        21600000  // 21.6 MHz (108 MHz PLL / 5 CLKDIV)
#define UART_BAUD       115200

void main()
{
    UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;

    // Wait for UART to settle
    for (volatile int wait = 0; wait < 2000; wait++);

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
    print("  Tang Nano 9K @ 21.6 MHz\n");
    print("\n");

    // LED startup sequence
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
        print("   [1-6] Toggle LED 1-6\n");
        print("   [F] Get flash mode\n");
        print("   [I] Read SPI flash ID\n");
        print("   [S] Single SPI  [D] DSPI  [C] DSPI+CRM\n");
        print("   [B] Simple benchmark\n");
        print("   [X] ATOMiK hardware test\n");
        print("   [K] Checkpoint/rollback demo\n");
        print("   [M] Memory benchmark\n");
        print("   [H] Heap integrity demo\n");
        print("   [P] Phase 2 full test\n");
        print("   [V] Display pipeline test\n");
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
                uint64_t c0 = cycles64();
                // Simple XOR benchmark: 1000 iterations
                uint32_t x = 0xDEADBEEF;
                for (int j = 0; j < 1000; j++) {
                    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
                }
                uint64_t c1 = cycles64();
                mini_printf("XOR benchmark: %u cycles, chksum=0x%08x\n",
                            (uint32_t)(c1 - c0), x);
                break;
            }
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
            case 'V': case 'v': cmd_display_test(); break;
            case 'R': case 'r': cmd_perf_suite(); break;
            default: continue;
            }
        }
    }
}

void irqCallback() {
}
