// Quick Verilator test for manual_uart_tx module
#include "Vmanual_uart_tx.h"
#include "verilated.h"
#include <iostream>

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vmanual_uart_tx* uart = new Vmanual_uart_tx;

    unsigned long time = 0;
    uart->clk = 0;
    uart->resetn = 0;
    uart->reg_div_we = 0;
    uart->reg_dat_we = 0;

    // Reset for 10 cycles
    for (int i = 0; i < 20; i++) {
        uart->clk = !uart->clk;
        uart->eval();
        time++;
    }
    uart->resetn = 1;

    // Write baud divider (115 for 13.5 MHz / 115200)
    uart->clk = 1; uart->eval(); time++;
    uart->clk = 0; uart->eval(); time++;
    uart->reg_div_we = 0xF;
    uart->reg_div_di = 115;
    uart->clk = 1; uart->eval(); time++;
    uart->clk = 0; uart->eval(); time++;
    uart->reg_div_we = 0;

    std::cout << "Divider written: " << uart->reg_div_do << std::endl;

    // Write data byte (0x55)
    uart->reg_dat_we = 1;
    uart->reg_dat_di = 0x55;
    uart->clk = 1; uart->eval(); time++;
    uart->clk = 0; uart->eval(); time++;
    uart->reg_dat_we = 0;

    std::cout << "Data written, transmitting..." << std::endl;
    std::cout << "reg_dat_wait: " << (int)uart->reg_dat_wait << std::endl;

    // Run for enough cycles to transmit one byte
    // 10 bits * 116 cycles/bit = 1160 cycles
    int last_tx = uart->ser_tx;
    int transitions = 0;
    for (int i = 0; i < 2000; i++) {
        uart->clk = !uart->clk;
        uart->eval();

        if (uart->clk && uart->ser_tx != last_tx) {
            std::cout << "Time " << time << ": ser_tx = " << (int)uart->ser_tx << std::endl;
            last_tx = uart->ser_tx;
            transitions++;
        }
        time++;
    }

    std::cout << "\nTest complete!" << std::endl;
    std::cout << "TX transitions: " << transitions << std::endl;
    std::cout << "Expected: ~10 (start + 8 data bits + stop)" << std::endl;

    if (transitions >= 8 && transitions <= 12) {
        std::cout << "✓ PASS - UART TX is working!" << std::endl;
        delete uart;
        return 0;
    } else {
        std::cout << "✗ FAIL - Unexpected number of transitions" << std::endl;
        delete uart;
        return 1;
    }
}
