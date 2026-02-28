// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vuart_handshake_test.h for the primary calling header

#include "Vuart_handshake_test__pch.h"
#include "Vuart_handshake_test__Syms.h"
#include "Vuart_handshake_test___024root.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Vuart_handshake_test___024root___dump_triggers__stl(Vuart_handshake_test___024root* vlSelf);
#endif  // VL_DEBUG

VL_ATTR_COLD void Vuart_handshake_test___024root___eval_triggers__stl(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_triggers__stl\n"); );
    // Body
    vlSelf->__VstlTriggered.set(0U, (IData)(vlSelf->__VstlFirstIteration));
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vuart_handshake_test___024root___dump_triggers__stl(vlSelf);
    }
#endif
}
