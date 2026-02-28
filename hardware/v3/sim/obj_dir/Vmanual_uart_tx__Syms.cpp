// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table implementation internals

#include "Vmanual_uart_tx__pch.h"
#include "Vmanual_uart_tx.h"
#include "Vmanual_uart_tx___024root.h"

// FUNCTIONS
Vmanual_uart_tx__Syms::~Vmanual_uart_tx__Syms()
{
}

Vmanual_uart_tx__Syms::Vmanual_uart_tx__Syms(VerilatedContext* contextp, const char* namep, Vmanual_uart_tx* modelp)
    : VerilatedSyms{contextp}
    // Setup internal state of the Syms class
    , __Vm_modelp{modelp}
    // Setup module instances
    , TOP{this, namep}
{
        // Check resources
        Verilated::stackCheck(28);
    // Configure time unit / time precision
    _vm_contextp__->timeunit(-12);
    _vm_contextp__->timeprecision(-12);
    // Setup each module's pointers to their submodules
    // Setup each module's pointer back to symbol table (for public functions)
    TOP.__Vconfigure(true);
}
