// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table internal header
//
// Internal details; most calling programs do not need this header,
// unless using verilator public meta comments.

#ifndef VERILATED_VMANUAL_UART_TX__SYMS_H_
#define VERILATED_VMANUAL_UART_TX__SYMS_H_  // guard

#include "verilated.h"

// INCLUDE MODEL CLASS

#include "Vmanual_uart_tx.h"

// INCLUDE MODULE CLASSES
#include "Vmanual_uart_tx___024root.h"

// SYMS CLASS (contains all model state)
class alignas(VL_CACHE_LINE_BYTES)Vmanual_uart_tx__Syms final : public VerilatedSyms {
  public:
    // INTERNAL STATE
    Vmanual_uart_tx* const __Vm_modelp;
    VlDeleter __Vm_deleter;
    bool __Vm_didInit = false;

    // MODULE INSTANCE STATE
    Vmanual_uart_tx___024root      TOP;

    // CONSTRUCTORS
    Vmanual_uart_tx__Syms(VerilatedContext* contextp, const char* namep, Vmanual_uart_tx* modelp);
    ~Vmanual_uart_tx__Syms();

    // METHODS
    const char* name() { return TOP.name(); }
};

#endif  // guard
