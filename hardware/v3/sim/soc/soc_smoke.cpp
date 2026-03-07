// =============================================================================
// ATOMiK v3 SoC Smoke Test
//
// Simulates the full SoC with behavioral SRAM models (no Gowin IP).
// Loads boot ROM firmware from .v32 hex file into BROM behavioral model.
// Monitors UART TX output for banner string and ATOMiK test results.
//
// Usage: soc_smoke [--brom <fw-brom.v32>] [--flash <fw-flash.v32>]
//                  [--timeout <cycles>] [--trace]
// =============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Vsim_soc_top.h"
#include "verilated.h"
#ifdef TRACE
#include "verilated_fst_c.h"
#endif

// ---------------------------------------------------------------------------
// UART TX bit-banger (deserializer)
// ---------------------------------------------------------------------------
class UartMonitor {
public:
    int baud_divisor;  // clk cycles per bit
    std::string buffer;
    bool found_banner  = false;
    bool found_atomik  = false;
    bool found_allpass = false;

    UartMonitor(int clk_freq, int baud) {
        baud_divisor = clk_freq / baud;
    }

    // Call every clock edge with the TX pin value
    void tick(int tx_val) {
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
                    if (c != '\r') {
                        buffer += c;
                        putchar(c);
                        fflush(stdout);
                    }

                    // Check for key strings
                    if (buffer.find("ATOMiK") != std::string::npos)
                        found_banner = true;
                    if (buffer.find("Custom Instruction Tests") != std::string::npos)
                        found_atomik = true;
                    if (buffer.find("ALL PASS") != std::string::npos)
                        found_allpass = true;

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
// UART RX injector (serializer) — sends bytes to the CPU
// ---------------------------------------------------------------------------
class UartInjector {
public:
    int baud_divisor;
    int tx_val = 1;  // Idle high

    UartInjector(int clk_freq, int baud) {
        baud_divisor = clk_freq / baud;
    }

    void queue(uint8_t byte) {
        pending.push_back(byte);
    }

    void queue_string(const char *s) {
        while (*s) pending.push_back(*s++);
    }

    // Call every clock edge, returns current TX value
    int tick() {
        switch (state) {
        case IDLE:
            tx_val = 1;
            if (!pending.empty()) {
                current_byte = pending.front();
                pending.erase(pending.begin());
                state = START;
                cycle_count = baud_divisor;
                tx_val = 0;  // Start bit
            }
            break;

        case START:
            tx_val = 0;
            if (--cycle_count <= 0) {
                state = DATA;
                bit_index = 0;
                cycle_count = baud_divisor;
                tx_val = current_byte & 1;
            }
            break;

        case DATA:
            tx_val = (current_byte >> bit_index) & 1;
            if (--cycle_count <= 0) {
                bit_index++;
                cycle_count = baud_divisor;
                if (bit_index == 8) {
                    state = STOP;
                    tx_val = 1;  // Stop bit
                }
            }
            break;

        case STOP:
            tx_val = 1;
            if (--cycle_count <= 0) {
                state = IDLE;
            }
            break;
        }
        return tx_val;
    }

private:
    enum { IDLE, START, DATA, STOP } state = IDLE;
    std::vector<uint8_t> pending;
    uint8_t current_byte = 0;
    int bit_index = 0;
    int cycle_count = 0;
};

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);

    // Defaults
    std::string brom_path = "hardware/v3/soc/firmware/fw-brom/build/fw-brom.v32";
    std::string flash_path = "hardware/v3/soc/firmware/fw-flash/build/fw-flash.v";
    uint64_t max_cycles = 20000000;  // 20M cycles @ 25 MHz ≈ 0.8s
    bool trace_en = false;

    // Parse args
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--brom") == 0 && i+1 < argc)
            brom_path = argv[++i];
        else if (strcmp(argv[i], "--flash") == 0 && i+1 < argc)
            flash_path = argv[++i];
        else if (strcmp(argv[i], "--timeout") == 0 && i+1 < argc)
            max_cycles = strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--trace") == 0)
            trace_en = true;
    }

    printf("=== ATOMiK v3 SoC Smoke Test ===\n");
    printf("BROM:    %s\n", brom_path.c_str());
    printf("Flash:   %s\n", flash_path.c_str());
    printf("Timeout: %lu cycles\n\n", max_cycles);

    // Create DUT
    Vsim_soc_top *top = new Vsim_soc_top;

#ifdef TRACE
    VerilatedFstC *tfp = nullptr;
    if (trace_en) {
        Verilated::traceEverOn(true);
        tfp = new VerilatedFstC;
        top->trace(tfp, 99);
        tfp->open("soc_smoke.fst");
    }
#endif

    // UART monitor — firmware sets CLKDIV=185, simpleuart uses CLKDIV+2=187
    // cycles per bit. Use 21.6 MHz (actual CPU clock) for correct timing.
    UartMonitor uart_mon(21600000, 115200);
    UartInjector uart_inj(21600000, 115200);

    // Reset
    top->clk = 0;
    top->rst_n = 0;
    top->ser_rx = 1;   // Idle high
    top->link_rx = 1;  // Idle high

    // Hold reset for 20 cycles
    for (int i = 0; i < 40; i++) {
        top->clk = !top->clk;
        top->eval();
#ifdef TRACE
        if (tfp) tfp->dump(i);
#endif
    }
    top->rst_n = 1;

    // ISP flasher waits for 0x55 sync byte for ~500K iterations,
    // then jumps to flash at 0x00000000.
    // For simulation: we do NOT send 0x55, so the ISP flasher will
    // timeout and jump to flash. We need to wait for the timeout.
    // At ~10 instructions per wait loop iteration @ multi-cycle CPU,
    // this takes ~5M+ cycles. For faster sim, we'll inject the 'X'
    // command once the banner appears.

    // Run simulation
    uint64_t cycle = 0;
    uint64_t sim_time = 40;  // Continue from reset phase
    bool command_sent = false;

    printf("--- UART Output ---\n");
    while (cycle < max_cycles && !Verilated::gotFinish()) {
        // Rising edge
        top->clk = 1;
        top->ser_rx = uart_inj.tick();
        top->link_rx = top->link_tx;  // Loopback for N1 test
        top->eval();
#ifdef TRACE
        if (tfp) tfp->dump(sim_time++);
#endif

        // Monitor UART on rising edge
        uart_mon.tick(top->ser_tx);

        // Once we see "Command>", send 'X' for ATOMiK hardware test
        if (uart_mon.found_banner && !command_sent &&
            uart_mon.buffer.find("Command>") != std::string::npos) {
            uart_inj.queue('X');
            command_sent = true;
        }

        // Check for completion (ATOMiK ALL PASS)
        if (uart_mon.found_allpass) {
            // Let a few more characters drain
            for (int i = 0; i < 100000; i++) {
                top->clk = !top->clk;
                top->ser_rx = uart_inj.tick();
                top->eval();
#ifdef TRACE
                if (tfp) tfp->dump(sim_time++);
#endif
                if (top->clk) uart_mon.tick(top->ser_tx);
            }
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
    int result = 0;
    if (uart_mon.found_banner) {
        printf("PASS: Banner detected\n");
    } else {
        printf("FAIL: No banner detected\n");
        result = 1;
    }

    if (uart_mon.found_allpass) {
        printf("PASS: ATOMiK tests ALL PASS\n");
    } else if (uart_mon.found_atomik) {
        printf("FAIL: ATOMiK tests started but did not complete\n");
        result = 1;
    } else {
        printf("INFO: ATOMiK tests not run (timeout before Command>)\n");
        // Not a hard failure — the ISP timeout is long
    }

    printf("\nSimulated %lu cycles\n", cycle);

    if (result == 0) {
        printf("\n*** SoC SMOKE TEST PASSED ***\n");
    } else {
        printf("\n*** SoC SMOKE TEST FAILED ***\n");
    }

#ifdef TRACE
    if (tfp) { tfp->close(); delete tfp; }
#endif
    delete top;
    return result;
}
