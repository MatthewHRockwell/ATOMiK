// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table internal header
//
// Internal details; most calling programs do not need this header,
// unless using verilator public meta comments.

#ifndef VERILATED_VSIM_SOC_TOP__SYMS_H_
#define VERILATED_VSIM_SOC_TOP__SYMS_H_  // guard

#include "verilated.h"

// INCLUDE MODEL CLASS

#include "Vsim_soc_top.h"

// INCLUDE MODULE CLASSES
#include "Vsim_soc_top___024root.h"

// SYMS CLASS (contains all model state)
class alignas(VL_CACHE_LINE_BYTES)Vsim_soc_top__Syms final : public VerilatedSyms {
  public:
    // INTERNAL STATE
    Vsim_soc_top* const __Vm_modelp;
    VlDeleter __Vm_deleter;
    bool __Vm_didInit = false;

    // MODULE INSTANCE STATE
    Vsim_soc_top___024root         TOP;

    // CONSTRUCTORS
    Vsim_soc_top__Syms(VerilatedContext* contextp, const char* namep, Vsim_soc_top* modelp);
    ~Vsim_soc_top__Syms();

    // METHODS
    const char* name() { return TOP.name(); }
};

#endif  // guard
