// Verilator testbench for uart_handshake_test
// Monitors internal simpleuart signals to debug why UART doesn't transmit

#include "Vuart_handshake_test.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include <iostream>
#include <iomanip>

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vuart_handshake_test* top = new Vuart_handshake_test;

    // Enable waveform tracing
    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("uart_debug.vcd");

    unsigned long time = 0;
    top->clk = 0;

    std::cout << "Starting UART debug simulation..." << std::endl;
    std::cout << "Looking for: cfg_divider write, send_pattern loads, uart_tx activity" << std::endl;
    std::cout << std::endl;

    // Run for 1 million cycles (enough to send several bytes at 27 MHz / 232 baud)
    while (time < 1000000 && !Verilated::gotFinish()) {
        // Toggle clock
        top->clk = !top->clk;
        top->eval();
        tfp->dump(time);

        // Monitor key signals on rising edges
        if (top->clk == 1) {
            // Access internal signals (will need to expose these in uart_handshake_test.v)
            // For now, just monitor uart_tx
            static int uart_tx_prev = 1;
            if (top->uart_tx != uart_tx_prev) {
                std::cout << "Time " << std::setw(8) << time
                          << ": uart_tx changed: " << uart_tx_prev
                          << " -> " << (int)top->uart_tx << std::endl;
                uart_tx_prev = top->uart_tx;
            }

            // Print status every 10000 cycles
            if ((time % 20000) == 0) {
                std::cout << "Time " << std::setw(8) << time
                          << ": uart_tx = " << (int)top->uart_tx << std::endl;
            }
        }

        time++;
    }

    std::cout << std::endl;
    std::cout << "Simulation complete. Check uart_debug.vcd for waveforms." << std::endl;
    std::cout << "Key signals to inspect:" << std::endl;
    std::cout << "  - u_uart.cfg_divider (should be 232)" << std::endl;
    std::cout << "  - u_uart.send_pattern (should load 0x155 = {1, 0x55, 0})" << std::endl;
    std::cout << "  - u_uart.send_bitcnt (should count 10 -> 0)" << std::endl;
    std::cout << "  - u_uart.send_divcnt (should count 0 -> 232)" << std::endl;
    std::cout << "  - u_uart.send_dummy (should be 1 after CLKDIV write, then 0)" << std::endl;
    std::cout << "  - uart_tx (should toggle during transmission)" << std::endl;

    tfp->close();
    delete top;
    return 0;
}
