// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vmanual_uart_tx.h for the primary calling header

#include "Vmanual_uart_tx__pch.h"
#include "Vmanual_uart_tx__Syms.h"
#include "Vmanual_uart_tx___024root.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Vmanual_uart_tx___024root___dump_triggers__stl(Vmanual_uart_tx___024root* vlSelf);
#endif  // VL_DEBUG

VL_ATTR_COLD void Vmanual_uart_tx___024root___eval_triggers__stl(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_triggers__stl\n"); );
    // Body
    vlSelf->__VstlTriggered.set(0U, (IData)(vlSelf->__VstlFirstIteration));
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vmanual_uart_tx___024root___dump_triggers__stl(vlSelf);
    }
#endif
}
