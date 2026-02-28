// =============================================================================
// ATOMiK v3 SoC Bringup Test
//
// Verifies minimal CPU liveness with Boot ROM in bringup mode.
// Expected behavior:
//   1. PC boots at 0x80000000 (Boot ROM base)
//   2. UART continuously spams 'T' characters
//   3. GPIO[0] toggles as heartbeat
//
// This isolates basic CPU/clock/reset/UART functionality from firmware complexity.
// =============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Vsim_soc_top.h"
#include "verilated.h"
#ifdef TRACE
#include "verilated_fst_c.h"
#endif

// ---------------------------------------------------------------------------
// UART TX Monitor (simplified for bringup mode)
// ---------------------------------------------------------------------------
class UartMonitor {
public:
    int baud_divisor;  // clk cycles per bit
    int t_count = 0;   // Count of 'T' characters received
    int char_count = 0;
    uint64_t last_char_cycle = 0;

    UartMonitor(int clk_freq, int baud) {
        baud_divisor = clk_freq / baud;
    }

    void tick(int tx_val, uint64_t cycle) {
        switch (state) {
        case IDLE:
            if (tx_val == 0) {
                // Start bit detected
                state = RECEIVING;
                bit_count = 0;
                shift_reg = 0;
                cycle_count = baud_divisor + baud_divisor / 2; // sample mid-bit
            }
            break;

        case RECEIVING:
            if (--cycle_count <= 0) {
                shift_reg |= (tx_val << bit_count);
                bit_count++;
                cycle_count = baud_divisor;

                if (bit_count == 8) {
                    // Got a byte
                    char c = (char)(shift_reg & 0xFF);
                    char_count++;
                    if (c == 'T') {
                        t_count++;
                        printf("T");
                        fflush(stdout);
                    } else {
                        printf("[0x%02X]", (unsigned char)c);
                        fflush(stdout);
                    }

                    last_char_cycle = cycle;
                    state = STOP;
                    cycle_count = baud_divisor; // wait for stop bit
                }
            }
            break;

        case STOP:
            if (--cycle_count <= 0) {
                state = IDLE;
            }
            break;
        }
    }

private:
    enum { IDLE, RECEIVING, STOP } state = IDLE;
    int bit_count = 0;
    int cycle_count = 0;
    int shift_reg = 0;
};

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);

    // Bringup mode expects 13.5 MHz clock
    int CLK_FREQ = 13500000;
    int UART_BAUD = 115200;
    uint64_t max_cycles = 50000000;  // 50M cycles @ 13.5 MHz ≈ 3.7s
    bool trace_en = false;

    // Parse args
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--trace") == 0)
            trace_en = true;
        else if (strcmp(argv[i], "--timeout") == 0 && i+1 < argc)
            max_cycles = strtoull(argv[++i], NULL, 0);
    }

    printf("=== ATOMiK v3 SoC Bringup Test ===\n");
    printf("Clock:   %d Hz\n", CLK_FREQ);
    printf("UART:    %d baud\n", UART_BAUD);
    printf("Timeout: %lu cycles (%.3f ms)\n\n", max_cycles,
           (double)max_cycles / CLK_FREQ * 1000);

    // Create DUT
    Vsim_soc_top *top = new Vsim_soc_top;

#ifdef TRACE
    VerilatedFstC *tfp = nullptr;
    if (trace_en) {
        Verilated::traceEverOn(true);
        tfp = new VerilatedFstC;
        top->trace(tfp, 99);
        tfp->open("soc_bringup.fst");
        printf("Tracing enabled: soc_bringup.fst\n\n");
    }
#endif

    UartMonitor uart_mon(CLK_FREQ, UART_BAUD);

    // Reset sequence
    top->clk = 0;
    top->rst_n = 0;
    top->ser_rx = 1;  // Idle high

    printf("Holding reset for 256 cycles...\n");
    for (int i = 0; i < 512; i++) {
        top->clk = !top->clk;
        top->eval();
#ifdef TRACE
        if (tfp) tfp->dump(i);
#endif
    }
    top->rst_n = 1;
    printf("Reset released. CPU should boot from 0x80000000...\n\n");

    // Run simulation
    printf("--- UART Output (expecting 'T' spam) ---\n");
    uint64_t cycle = 0;
    uint64_t sim_time = 512;
    int last_gpio = -1;
    int gpio_toggle_count = 0;

    while (cycle < max_cycles && !Verilated::gotFinish()) {
        // Rising edge
        top->clk = 1;
        top->eval();
#ifdef TRACE
        if (tfp) tfp->dump(sim_time++);
#endif

        uart_mon.tick(top->ser_tx, cycle);

        // Monitor GPIO[0] toggle
        int gpio = top->gpio_out & 1;
        if (last_gpio >= 0 && gpio != last_gpio) {
            gpio_toggle_count++;
            if (gpio_toggle_count <= 5) {
                printf(" [GPIO%d]", gpio);
                fflush(stdout);
            }
        }
        last_gpio = gpio;

        // Check for success: at least 3 'T' characters received
        if (uart_mon.t_count >= 3) {
            printf("\n\n*** SUCCESS: Received %d 'T' characters! ***\n", uart_mon.t_count);
            printf("CPU is alive and UART is functional.\n");
            break;
        }

        // Falling edge
        top->clk = 0;
        top->eval();
#ifdef TRACE
        if (tfp) tfp->dump(sim_time++);
#endif

        cycle++;
    }

    printf("\n--- End UART Output ---\n\n");

    // Results
    printf("=== Test Results ===\n");
    printf("Cycles simulated:  %lu (%.3f ms)\n", cycle, (double)cycle / CLK_FREQ * 1000);
    printf("UART characters:   %d total, %d 'T' chars\n", uart_mon.char_count, uart_mon.t_count);
    printf("GPIO toggles:      %d\n", gpio_toggle_count);

    int result = 0;
    if (uart_mon.t_count >= 3) {
        printf("\nPASS: CPU boots, UART transmits correctly\n");
    } else if (uart_mon.char_count > 0) {
        printf("\nFAIL: UART active but not sending 'T' (got garbage)\n");
        printf("      This suggests UART works but Boot ROM code is wrong\n");
        result = 1;
    } else {
        printf("\nFAIL: No UART output detected\n");
        printf("      Possible causes:\n");
        printf("      - CPU not fetching (PC stuck, clock stopped, reset stuck)\n");
        printf("      - Boot ROM not mapped at 0x80000000\n");
        printf("      - UART peripheral not functional\n");
        printf("      - UART baud rate mismatch (try different divider)\n");
        result = 1;
    }

    if (gpio_toggle_count > 0) {
        printf("      + GPIO toggle detected (CPU is executing code!)\n");
    } else {
        printf("      - No GPIO toggle (CPU may not be executing)\n");
    }

#ifdef TRACE
    if (tfp) { tfp->close(); delete tfp; }
#endif
    delete top;

    printf("\n");
    return result;
}
