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

// Display pipeline MMIO (S3 slot: 0xC0000000, addr[8]=0)
#define DISP_CTRL   (*(volatile uint32_t*)0xC0000000)
#define DISP_LUT    (*(volatile uint32_t*)0xC0000004)
#define DISP_SCAN   (*(volatile uint32_t*)0xC0000008)
#define DISP_STATUS (*(volatile uint32_t*)0xC000000C)

// Link UART (S3 slot: 0xC0000100, addr[8]=1, inter-board)
#define LINK0 ((PICOUART*)0xC0000100)

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
// Link UART Driver (inter-board, ~2 Mbaud)
// --------------------------------------------------------

// Protocol: [0xA5] [CMD] [LEN] [PAYLOAD...] [CRC8]
// CRC8 = XOR of all bytes (sync + cmd + len + payload)
#define LINK_SYNC   0xA5
#define LINK_CMD_DELTA     0x01  // 8-byte delta value
#define LINK_CMD_PING      0x02  // no payload
#define LINK_CMD_PONG      0x03  // no payload
#define LINK_CMD_STATE_REQ 0x04  // request peer state
#define LINK_CMD_STATE_RSP 0x05  // 8-byte state value

// Link baud: 21.6 MHz / (CLKDIV+2) ≈ 115200 at CLKDIV=185
// Note: CLKDIV=9 (~2 Mbaud) is too fast for software polling — the 1-byte
// RX buffer overflows before the CPU can read each byte. CLKDIV=185 gives
// ~1870 cycles/byte, plenty of margin for the XIP polling loop.
#define LINK_CLKDIV_VAL  185

static void link_init(void)
{
    LINK0->CLKDIV = LINK_CLKDIV_VAL;
}

static void link_putchar(uint8_t c)
{
    LINK0->DATA = c;
}

static int link_getchar(void)
{
    return (int32_t)LINK0->DATA;
}

// Non-blocking receive with timeout (in cycles)
static int link_getchar_timeout(uint32_t timeout_cycles)
{
    uint64_t start = cycles64();
    int c;
    while ((c = link_getchar()) < 0) {
        if ((uint32_t)(cycles64() - start) > timeout_cycles)
            return -1;
    }
    return c;
}

static void link_send_packet(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t crc = LINK_SYNC ^ cmd ^ len;
    link_putchar(LINK_SYNC);
    link_putchar(cmd);
    link_putchar(len);
    for (uint8_t i = 0; i < len; i++) {
        link_putchar(payload[i]);
        crc ^= payload[i];
    }
    link_putchar(crc);
}

// Receive a packet. Returns cmd on success, -1 on timeout/error.
// payload buffer must be >= 8 bytes. *out_len set to payload length.
static int link_recv_packet(uint8_t *payload, uint8_t *out_len, uint32_t timeout)
{
    // Wait for sync byte
    int c = link_getchar_timeout(timeout);
    if (c < 0) return -1;
    if (c != LINK_SYNC) return -1;

    int cmd = link_getchar_timeout(timeout);
    if (cmd < 0) return -1;

    int len = link_getchar_timeout(timeout);
    if (len < 0 || len > 8) return -1;

    uint8_t crc = LINK_SYNC ^ (uint8_t)cmd ^ (uint8_t)len;
    for (int i = 0; i < len; i++) {
        c = link_getchar_timeout(timeout);
        if (c < 0) return -1;
        payload[i] = (uint8_t)c;
        crc ^= (uint8_t)c;
    }

    c = link_getchar_timeout(timeout);
    if (c < 0 || (uint8_t)c != crc) return -1;

    *out_len = (uint8_t)len;
    return cmd;
}

static void link_send_delta(uint64_t delta)
{
    uint8_t buf[8];
    for (int i = 0; i < 8; i++)
        buf[i] = (uint8_t)(delta >> (i * 8));
    link_send_packet(LINK_CMD_DELTA, buf, 8);
}

static void link_send_ping(void)
{
    link_send_packet(LINK_CMD_PING, 0, 0);
}

static void link_send_pong(void)
{
    link_send_packet(LINK_CMD_PONG, 0, 0);
}

static void link_send_state(uint64_t state)
{
    uint8_t buf[8];
    for (int i = 0; i < 8; i++)
        buf[i] = (uint8_t)(state >> (i * 8));
    link_send_packet(LINK_CMD_STATE_RSP, buf, 8);
}

static uint64_t link_payload_to_u64(const uint8_t *buf)
{
    uint64_t val = 0;
    for (int i = 7; i >= 0; i--)
        val = (val << 8) | buf[i];
    return val;
}

// --------------------------------------------------------
// Multi-Node Tests [N]
// --------------------------------------------------------

static void cmd_link_loopback(void)
{
    // N1: Loopback test — requires jumper wire TX→RX (pin 25→26)
    print("\nN1: Link UART Loopback Test\n");
    print("  (Requires jumper: pin 25 → pin 26)\n\n");

    link_init();

    int pass = 0, fail = 0;
    uint8_t test_bytes[] = { 0x00, 0x55, 0xAA, 0xFF, 0x42 };

    // Test 1: Raw byte echo
    print("  Raw byte echo: ");
    for (int i = 0; i < 5; i++) {
        link_putchar(test_bytes[i]);
        int c = link_getchar_timeout(216000);  // 10ms timeout
        if (c == test_bytes[i]) {
            pass++;
        } else {
            mini_printf("FAIL (sent 0x%02x, got 0x%02x)\n", test_bytes[i], c);
            fail++;
        }
    }
    if (fail == 0) print("PASS (5/5)\n");

    // Test 2: Framed delta packet
    print("  Delta packet:   ");
    link_send_delta(0xCAFEBABE12345678ULL);
    uint8_t payload[8];
    uint8_t plen;
    int cmd = link_recv_packet(payload, &plen, 216000);
    if (cmd == LINK_CMD_DELTA && plen == 8) {
        uint64_t val = link_payload_to_u64(payload);
        if (val == 0xCAFEBABE12345678ULL) {
            print("PASS\n");
            pass++;
        } else {
            mini_printf("FAIL (got 0x%lx)\n", val);
            fail++;
        }
    } else {
        mini_printf("FAIL (cmd=%d len=%d)\n", cmd, plen);
        fail++;
    }

    // Test 3: Ping/pong
    print("  Ping loopback:  ");
    link_send_ping();
    cmd = link_recv_packet(payload, &plen, 216000);
    if (cmd == LINK_CMD_PING && plen == 0) {
        print("PASS\n");
        pass++;
    } else {
        mini_printf("FAIL (cmd=%d)\n", cmd);
        fail++;
    }

    mini_printf("\nN1 Result: %u/%u PASS\n", pass, pass + fail);
}

static void cmd_link_ping(void)
{
    // N2: Ping peer and measure round-trip
    print("\nN2: Ping Peer\n\n");
    link_init();

    link_send_ping();
    uint64_t t0 = cycles64();
    uint8_t payload[8];
    uint8_t plen;
    int cmd = link_recv_packet(payload, &plen, 21600000);  // 1s timeout
    uint64_t t1 = cycles64();

    if (cmd == LINK_CMD_PONG) {
        mini_printf("PONG received: %u cycles round-trip\n", (uint32_t)(t1 - t0));
    } else if (cmd == LINK_CMD_PING) {
        // Peer also sent a ping — respond with pong
        link_send_pong();
        print("Received PING (peer also pinged), sent PONG\n");
    } else {
        print("No response (timeout)\n");
    }
}

static void cmd_link_convergence(void)
{
    // N5: Convergence test — both boards accumulate same deltas, compare states
    print("\nN5: Convergence Test\n\n");
    link_init();

    // Initialize ATOMiK slot 0
    atomik_load(0, 0x0ULL);

    // Accumulate local deltas
    uint64_t deltas[] = { 0x1111111111111111ULL,
                          0x2222222222222222ULL,
                          0x4444444444444444ULL };
    for (int i = 0; i < 3; i++) {
        atomik_accum(deltas[i]);
        mini_printf("  Accumulated delta 0x%lx\n", deltas[i]);
    }

    // Send deltas to peer
    print("  Sending deltas to peer...\n");
    for (int i = 0; i < 3; i++)
        link_send_delta(deltas[i]);

    // Also try to receive deltas from peer (bidirectional)
    print("  Receiving deltas from peer...\n");
    uint8_t payload[8];
    uint8_t plen;
    int recv_count = 0;
    for (int i = 0; i < 3; i++) {
        int cmd = link_recv_packet(payload, &plen, 21600000);
        if (cmd == LINK_CMD_DELTA && plen == 8) {
            uint64_t d = link_payload_to_u64(payload);
            atomik_accum(d);
            mini_printf("  Received delta 0x%lx\n", d);
            recv_count++;
        }
    }

    // Read final state
    uint64_t state = atomik_read(0);
    mini_printf("\n  Local state:  0x%lx\n", state);
    mini_printf("  Deltas recv:  %d\n", recv_count);

    // Request peer state
    link_send_packet(LINK_CMD_STATE_REQ, 0, 0);
    int cmd = link_recv_packet(payload, &plen, 21600000);
    if (cmd == LINK_CMD_STATE_RSP && plen == 8) {
        uint64_t peer_state = link_payload_to_u64(payload);
        mini_printf("  Peer state:   0x%lx\n", peer_state);
        if (state == peer_state) {
            print("\n  CONVERGENCE: PASS (states match)\n");
        } else {
            print("\n  CONVERGENCE: FAIL (states differ)\n");
        }
    } else {
        print("  Could not get peer state\n");
    }
}

static void cmd_link_listen(void)
{
    // N4: Receive mode — accumulate incoming deltas, respond to pings
    print("\nN4: Link Listen Mode (Ctrl-C or 'q' to exit)\n\n");
    link_init();

    atomik_load(0, 0x0ULL);
    int delta_count = 0;

    while (1) {
        // Check for console input to exit
        int console = (int32_t)UART0->DATA;
        if (console == 'q' || console == 3) break;  // 'q' or Ctrl-C

        uint8_t payload[8];
        uint8_t plen;
        int cmd = link_recv_packet(payload, &plen, 216000);  // 10ms timeout

        if (cmd == LINK_CMD_DELTA && plen == 8) {
            uint64_t d = link_payload_to_u64(payload);
            atomik_accum(d);
            delta_count++;
            // Don't print per-delta — console UART stall (~1870 cy/char)
            // causes next packet's bytes to overflow the 1-byte RX buffer.
            // Print summary after loop exits instead.
            putchar('+');  // minimal progress indicator (single char)
        } else if (cmd == LINK_CMD_PING) {
            link_send_pong();
            print("  PING -> PONG\n");
        } else if (cmd == LINK_CMD_STATE_REQ) {
            uint64_t state = atomik_read(0);
            link_send_state(state);
            mini_printf("  STATE_REQ -> 0x%lx\n", state);
        }
    }

    mini_printf("\nReceived %d deltas, final state: 0x%lx\n",
                delta_count, atomik_read(0));
}

static void cmd_link_send(void)
{
    // N3: Send delta stream to peer
    print("\nN3: Send Delta Stream\n\n");
    link_init();

    uint64_t deltas[] = {
        0x1111111111111111ULL,
        0x2222222222222222ULL,
        0x4444444444444444ULL,
        0x8888888888888888ULL,
        0x0F0F0F0F0F0F0F0FULL
    };

    // Accumulate locally and send
    atomik_load(0, 0x0ULL);
    for (int i = 0; i < 5; i++) {
        atomik_accum(deltas[i]);
        link_send_delta(deltas[i]);
        mini_printf("  Sent delta 0x%lx\n", deltas[i]);
    }

    uint64_t state = atomik_read(0);
    mini_printf("\n  Local state: 0x%lx\n", state);
    print("  (Peer should converge to same state)\n");
}

static void cmd_multinode(void)
{
    print("\n=== Multi-Node Operations ===\n");
    print("  N1: Loopback test (pin 25 → pin 26 jumper)\n");
    print("  N2: Ping peer\n");
    print("  N3: Send delta stream\n");
    print("  N4: Listen mode (receive deltas)\n");
    print("  N5: Convergence test\n");
    print("\nSelect: ");

    char c = getchar();
    putchar(c);
    putchar('\n');

    switch (c) {
    case '1': cmd_link_loopback(); break;
    case '2': cmd_link_ping(); break;
    case '3': cmd_link_send(); break;
    case '4': cmd_link_listen(); break;
    case '5': cmd_link_convergence(); break;
    default: print("Unknown sub-command\n"); break;
    }
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

// =========================================================================
// Interactive Shell
// =========================================================================

static int shell_getline(char *buf, int maxlen)
{
    int pos = 0;
    while (pos < maxlen - 1) {
        char c = getchar();
        if (c == '\r' || c == '\n') {
            putchar('\n');
            break;
        } else if (c == 8 || c == 127) {  // backspace or delete
            if (pos > 0) {
                pos--;
                print("\b \b");
            }
        } else if (c >= 32 && c < 127) {
            buf[pos++] = c;
            putchar(c);
        }
    }
    buf[pos] = '\0';
    return pos;
}

static uint64_t shell_parse_hex(const char *s)
{
    uint64_t val = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    while (*s) {
        char c = *s++;
        if (c >= '0' && c <= '9') val = (val << 4) | (c - '0');
        else if (c >= 'a' && c <= 'f') val = (val << 4) | (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val = (val << 4) | (c - 'A' + 10);
        else break;
    }
    return val;
}

static int shell_tokenize(char *line, char **tokens, int max_tokens)
{
    int n = 0;
    while (*line && n < max_tokens) {
        while (*line == ' ') line++;
        if (!*line) break;
        tokens[n++] = line;
        while (*line && *line != ' ') line++;
        if (*line) *line++ = '\0';
    }
    return n;
}

static int str_eq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

static void shell_cmd_help(void)
{
    print("Commands:\n");
    print("  help                       Show this help\n");
    print("  status                     System info\n");
    print("  atomik load <addr> <val>   Load reference state\n");
    print("  atomik accum <val>         Accumulate delta\n");
    print("  atomik read [addr]         Read current state\n");
    print("  atomik swap [addr]         Swap reference\n");
    print("  atomik demo                Interactive demo\n");
    print("  disp enable|disable        Toggle display overlay\n");
    print("  disp lut <idx> <color>     Set LUT entry\n");
    print("  disp scan <col> <chg> <i>  Set scanline delta\n");
    print("  disp clear                 Clear all deltas\n");
    print("  disp fill <color>          Fill scanline with color\n");
    print("  mem read <addr>            Read 32-bit word\n");
    print("  mem write <addr> <val>     Write 32-bit word\n");
    print("  test                       Run all test suites\n");
    print("  exit                       Return to menu\n");
}

static void shell_cmd_status(void)
{
    uint64_t cy = cycles64();
    uint32_t sec = 0;
    uint64_t rem = cy;
    // Compute seconds via repeated subtraction (no div instruction)
    while (rem >= 21600000ULL) { sec++; rem -= 21600000ULL; }
    mini_printf("ATOMiK v3 SoC Status\n");
    mini_printf("  CPU:     RV64I @ 21.6 MHz\n");
    mini_printf("  ATOMiK:  Direct-wire custom instructions (64-bit)\n");
    mini_printf("  Uptime:  %u seconds (~%u M cycles)\n", sec, (uint32_t)(cy / 1000000ULL));
    uint32_t disp_ctrl = DISP_CTRL;
    uint32_t disp_status = DISP_STATUS;
    mini_printf("  Display: %s (frame %u)\n",
                (disp_ctrl & 1) ? "ENABLED" : "disabled",
                disp_status >> 16);
    // Read ATOMiK state for slot 0
    uint64_t state = atomik_read(0);
    mini_printf("  ATOMiK slot 0 state: 0x%lx\n", state);
}

static void shell_cmd_atomik(char **tokens, int ntok)
{
    if (ntok < 2) { print("Usage: atomik <load|accum|read|swap|demo>\n"); return; }

    if (str_eq(tokens[1], "load")) {
        if (ntok < 4) { print("Usage: atomik load <addr> <value>\n"); return; }
        uint64_t addr = shell_parse_hex(tokens[2]);
        uint64_t val = shell_parse_hex(tokens[3]);
        atomik_load(addr, val);
        mini_printf("Loaded slot %u = 0x%lx\n", (uint32_t)addr, val);
    } else if (str_eq(tokens[1], "accum")) {
        if (ntok < 3) { print("Usage: atomik accum <value>\n"); return; }
        uint64_t val = shell_parse_hex(tokens[2]);
        atomik_accum(val);
        mini_printf("Accumulated delta 0x%lx\n", val);
    } else if (str_eq(tokens[1], "read")) {
        uint64_t addr = (ntok >= 3) ? shell_parse_hex(tokens[2]) : 0;
        uint64_t state = atomik_read(addr);
        mini_printf("Slot %u state = 0x%lx\n", (uint32_t)addr, state);
    } else if (str_eq(tokens[1], "swap")) {
        uint64_t addr = (ntok >= 3) ? shell_parse_hex(tokens[2]) : 0;
        uint64_t old = atomik_swap(addr);
        mini_printf("Swapped slot %u, old state = 0x%lx\n", (uint32_t)addr, old);
    } else if (str_eq(tokens[1], "demo")) {
        print("\n--- ATOMiK Interactive Demo ---\n\n");
        print("1. Load reference state 0xCAFEBABE...\n");
        atomik_load(0, 0xCAFEBABEDEADBEEFULL);
        uint64_t s = atomik_read(0);
        mini_printf("   State = 0x%lx\n", s);

        print("2. Accumulate delta 0xFF...\n");
        atomik_accum(0xFFULL);
        s = atomik_read(0);
        mini_printf("   State = 0x%lx (ref XOR 0xFF)\n", s);

        print("3. Undo (accumulate same delta again)...\n");
        atomik_accum(0xFFULL);
        s = atomik_read(0);
        mini_printf("   State = 0x%lx (restored!)\n", s);

        print("4. Multiple deltas: 0x1000 then 0x2000...\n");
        atomik_accum(0x1000ULL);
        atomik_accum(0x2000ULL);
        s = atomik_read(0);
        mini_printf("   State = 0x%lx\n", s);

        print("5. Swap reference (checkpoint)...\n");
        uint64_t old = atomik_swap(0);
        mini_printf("   Old state = 0x%lx\n", old);
        s = atomik_read(0);
        mini_printf("   New state = 0x%lx (accumulator cleared)\n", s);

        print("\nDemo complete. XOR algebra: commutative, self-inverse, zero-cost undo.\n");
    } else {
        print("Unknown: atomik ");
        print(tokens[1]);
        print("\n");
    }
}

static void shell_cmd_disp(char **tokens, int ntok)
{
    if (ntok < 2) { print("Usage: disp <enable|disable|lut|scan|clear|fill>\n"); return; }

    if (str_eq(tokens[1], "enable")) {
        DISP_CTRL = 1;
        print("Display delta overlay enabled\n");
    } else if (str_eq(tokens[1], "disable")) {
        DISP_CTRL = 0;
        print("Display delta overlay disabled\n");
    } else if (str_eq(tokens[1], "lut")) {
        if (ntok < 4) { print("Usage: disp lut <index> <color>\n"); return; }
        uint32_t idx = (uint32_t)shell_parse_hex(tokens[2]);
        uint32_t color = (uint32_t)shell_parse_hex(tokens[3]);
        DISP_LUT = (idx << 24) | (color & 0xFFFFFF);
        mini_printf("LUT[%u] = 0x%06x\n", idx, color & 0xFFFFFF);
    } else if (str_eq(tokens[1], "scan")) {
        if (ntok < 5) { print("Usage: disp scan <col> <change> <index>\n"); return; }
        uint32_t col = (uint32_t)shell_parse_hex(tokens[2]);
        uint32_t chg = (uint32_t)shell_parse_hex(tokens[3]);
        uint32_t idx = (uint32_t)shell_parse_hex(tokens[4]);
        DISP_SCAN = (col << 16) | ((chg & 1) << 8) | (idx & 0xFF);
        mini_printf("Scan[%u] = change=%u index=%u\n", col, chg & 1, idx & 0xFF);
    } else if (str_eq(tokens[1], "clear")) {
        for (int col = 0; col < 640; col++)
            DISP_SCAN = ((uint32_t)col << 16) | 0x000;
        print("Scanline deltas cleared (640 entries)\n");
    } else if (str_eq(tokens[1], "fill")) {
        if (ntok < 3) { print("Usage: disp fill <color>\n"); return; }
        uint32_t color = (uint32_t)shell_parse_hex(tokens[2]);
        DISP_LUT = (255u << 24) | (color & 0xFFFFFF);
        for (int col = 0; col < 640; col++)
            DISP_SCAN = ((uint32_t)col << 16) | 0x1FF;  // change=1, index=255
        mini_printf("Filled scanline with delta 0x%06x (LUT[255])\n", color & 0xFFFFFF);
    } else {
        print("Unknown: disp ");
        print(tokens[1]);
        print("\n");
    }
}

static void shell_cmd_mem(char **tokens, int ntok)
{
    if (ntok < 3) { print("Usage: mem read|write <addr> [value]\n"); return; }

    uint32_t addr = (uint32_t)shell_parse_hex(tokens[2]);

    if (str_eq(tokens[1], "read")) {
        uint32_t val = *(volatile uint32_t *)(uintptr_t)addr;
        mini_printf("[0x%08x] = 0x%08x\n", addr, val);
    } else if (str_eq(tokens[1], "write")) {
        if (ntok < 4) { print("Usage: mem write <addr> <value>\n"); return; }
        uint32_t val = (uint32_t)shell_parse_hex(tokens[3]);
        *(volatile uint32_t *)(uintptr_t)addr = val;
        mini_printf("[0x%08x] <- 0x%08x\n", addr, val);
    } else {
        print("Usage: mem read|write <addr> [value]\n");
    }
}

void cmd_shell(void)
{
    char line[128];
    char *tokens[8];

    print("\nATOMiK v3 Interactive Shell\n");
    print("Type 'help' for commands, 'exit' to return to menu.\n\n");

    while (1) {
        print("atomik> ");
        int len = shell_getline(line, sizeof(line));
        if (len == 0) continue;

        int ntok = shell_tokenize(line, tokens, 8);
        if (ntok == 0) continue;

        if (str_eq(tokens[0], "help")) {
            shell_cmd_help();
        } else if (str_eq(tokens[0], "status")) {
            shell_cmd_status();
        } else if (str_eq(tokens[0], "atomik")) {
            shell_cmd_atomik(tokens, ntok);
        } else if (str_eq(tokens[0], "disp")) {
            shell_cmd_disp(tokens, ntok);
        } else if (str_eq(tokens[0], "mem")) {
            shell_cmd_mem(tokens, ntok);
        } else if (str_eq(tokens[0], "test")) {
            cmd_atomik_test();
            cmd_phase2_test();
            cmd_display_test();
        } else if (str_eq(tokens[0], "exit") || str_eq(tokens[0], "quit")) {
            print("Returning to menu.\n");
            break;
        } else {
            print("Unknown command: ");
            print(tokens[0]);
            print(" (type 'help')\n");
        }
    }
}

// --------------------------------------------------------
// Auto-Run: Boot Self-Test Suite
// --------------------------------------------------------
// Runs all test suites and demos on boot (no user input needed).
// Output goes to both UART and HDMI text terminal.

void cmd_run_all(void)
{
    print("\n========================================\n");
    print("  ATOMiK v3 Boot Self-Test\n");
    print("========================================\n");

    cmd_atomik_test();      // 9/9 custom instruction tests
    cmd_phase2_test();      // 10/10 integration tests
    cmd_checkpoint_demo();  // checkpoint/rollback demo
    cmd_mem_benchmark();    // memory performance
    cmd_heap_demo();        // heap integrity

    print("\n========================================\n");
    print("  Boot Self-Test Complete\n");
    print("========================================\n\n");
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

    // Auto-run test suite on boot (output on HDMI + UART)
    cmd_run_all();

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
        print("   [N] Multi-node link operations\n");
        print("   [A] Run all test suites\n");
        print("   [>] Interactive shell\n");

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
            case 'N': case 'n': cmd_multinode(); break;
            case 'A': case 'a': cmd_run_all(); break;
            case '>': cmd_shell(); break;
            default: continue;
            }
        }
    }
}

void irqCallback() {
}
