// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vmanual_uart_tx.h for the primary calling header

#include "Vmanual_uart_tx__pch.h"
#include "Vmanual_uart_tx__Syms.h"
#include "Vmanual_uart_tx___024root.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Vmanual_uart_tx___024root___dump_triggers__act(Vmanual_uart_tx___024root* vlSelf);
#endif  // VL_DEBUG

void Vmanual_uart_tx___024root___eval_triggers__act(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_triggers__act\n"); );
    // Body
    vlSelf->__VactTriggered.set(0U, ((IData)(vlSelf->clk) 
                                     & (~ (IData)(vlSelf->__Vtrigprevexpr___TOP__clk__0))));
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = vlSelf->clk;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vmanual_uart_tx___024root___dump_triggers__act(vlSelf);
    }
#endif
}
