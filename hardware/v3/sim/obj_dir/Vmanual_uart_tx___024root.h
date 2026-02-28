// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vmanual_uart_tx.h for the primary calling header

#ifndef VERILATED_VMANUAL_UART_TX___024ROOT_H_
#define VERILATED_VMANUAL_UART_TX___024ROOT_H_  // guard

#include "verilated.h"


class Vmanual_uart_tx__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vmanual_uart_tx___024root final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(resetn,0,0);
    VL_OUT8(ser_tx,0,0);
    VL_IN8(ser_rx,0,0);
    VL_IN8(reg_div_we,3,0);
    VL_IN8(reg_dat_we,0,0);
    VL_IN8(reg_dat_re,0,0);
    VL_OUT8(reg_dat_wait,0,0);
    CData/*3:0*/ manual_uart_tx__DOT__bit_index;
    CData/*7:0*/ manual_uart_tx__DOT__tx_data;
    CData/*0:0*/ manual_uart_tx__DOT__tx_active;
    CData/*0:0*/ manual_uart_tx__DOT__tx_shift_reg;
    CData/*0:0*/ __VstlFirstIteration;
    CData/*0:0*/ __Vtrigprevexpr___TOP__clk__0;
    CData/*0:0*/ __VactContinue;
    VL_IN(reg_div_di,31,0);
    VL_OUT(reg_div_do,31,0);
    VL_IN(reg_dat_di,31,0);
    VL_OUT(reg_dat_do,31,0);
    IData/*31:0*/ manual_uart_tx__DOT__cfg_divider;
    IData/*31:0*/ manual_uart_tx__DOT__bit_counter;
    IData/*31:0*/ __VactIterCount;
    VlTriggerVec<1> __VstlTriggered;
    VlTriggerVec<1> __VactTriggered;
    VlTriggerVec<1> __VnbaTriggered;

    // INTERNAL VARIABLES
    Vmanual_uart_tx__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vmanual_uart_tx___024root(Vmanual_uart_tx__Syms* symsp, const char* v__name);
    ~Vmanual_uart_tx___024root();
    VL_UNCOPYABLE(Vmanual_uart_tx___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
