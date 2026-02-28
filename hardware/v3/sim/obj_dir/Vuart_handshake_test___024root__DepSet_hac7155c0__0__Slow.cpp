// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vuart_handshake_test.h for the primary calling header

#include "Vuart_handshake_test__pch.h"
#include "Vuart_handshake_test___024root.h"

VL_ATTR_COLD void Vuart_handshake_test___024root___eval_static__TOP(Vuart_handshake_test___024root* vlSelf);

VL_ATTR_COLD void Vuart_handshake_test___024root___eval_static(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_static\n"); );
    // Body
    Vuart_handshake_test___024root___eval_static__TOP(vlSelf);
    vlSelf->__Vm_traceActivity[1U] = 1U;
    vlSelf->__Vm_traceActivity[0U] = 1U;
}

VL_ATTR_COLD void Vuart_handshake_test___024root___eval_static__TOP(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_static__TOP\n"); );
    // Body
    vlSelf->uart_handshake_test__DOT__reset_cnt = 0U;
    vlSelf->uart_handshake_test__DOT__state = 0U;
    vlSelf->uart_handshake_test__DOT__counter = 0U;
    vlSelf->uart_handshake_test__DOT__reg_div_we_r = 0U;
    vlSelf->uart_handshake_test__DOT__reg_div_di_r = 0U;
    vlSelf->uart_handshake_test__DOT__reg_dat_we_r = 0U;
    vlSelf->uart_handshake_test__DOT__reg_dat_di_r = 0U;
}

VL_ATTR_COLD void Vuart_handshake_test___024root___eval_initial(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_initial\n"); );
    // Body
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = vlSelf->clk;
}

VL_ATTR_COLD void Vuart_handshake_test___024root___eval_final(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_final\n"); );
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vuart_handshake_test___024root___dump_triggers__stl(Vuart_handshake_test___024root* vlSelf);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vuart_handshake_test___024root___eval_phase__stl(Vuart_handshake_test___024root* vlSelf);

VL_ATTR_COLD void Vuart_handshake_test___024root___eval_settle(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_settle\n"); );
    // Init
    IData/*31:0*/ __VstlIterCount;
    CData/*0:0*/ __VstlContinue;
    // Body
    __VstlIterCount = 0U;
    vlSelf->__VstlFirstIteration = 1U;
    __VstlContinue = 1U;
    while (__VstlContinue) {
        if (VL_UNLIKELY((0x64U < __VstlIterCount))) {
#ifdef VL_DEBUG
            Vuart_handshake_test___024root___dump_triggers__stl(vlSelf);
#endif
            VL_FATAL_MT("../test/uart_handshake_test.v", 3, "", "Settle region did not converge.");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
        __VstlContinue = 0U;
        if (Vuart_handshake_test___024root___eval_phase__stl(vlSelf)) {
            __VstlContinue = 1U;
        }
        vlSelf->__VstlFirstIteration = 0U;
    }
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vuart_handshake_test___024root___dump_triggers__stl(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VstlTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vuart_handshake_test___024root___stl_sequent__TOP__0(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___stl_sequent__TOP__0\n"); );
    // Body
    vlSelf->uart_tx = (1U & (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_pattern));
    vlSelf->uart_handshake_test__DOT__resetn = (0xffU 
                                                == (IData)(vlSelf->uart_handshake_test__DOT__reset_cnt));
    vlSelf->uart_handshake_test__DOT__reg_dat_wait 
        = ((IData)(vlSelf->uart_handshake_test__DOT__reg_dat_we_r) 
           & ((0U != (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt)) 
              | (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_dummy)));
}

VL_ATTR_COLD void Vuart_handshake_test___024root___eval_stl(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_stl\n"); );
    // Body
    if ((1ULL & vlSelf->__VstlTriggered.word(0U))) {
        Vuart_handshake_test___024root___stl_sequent__TOP__0(vlSelf);
        vlSelf->__Vm_traceActivity[1U] = 1U;
        vlSelf->__Vm_traceActivity[0U] = 1U;
    }
}

VL_ATTR_COLD void Vuart_handshake_test___024root___eval_triggers__stl(Vuart_handshake_test___024root* vlSelf);

VL_ATTR_COLD bool Vuart_handshake_test___024root___eval_phase__stl(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_phase__stl\n"); );
    // Init
    CData/*0:0*/ __VstlExecute;
    // Body
    Vuart_handshake_test___024root___eval_triggers__stl(vlSelf);
    __VstlExecute = vlSelf->__VstlTriggered.any();
    if (__VstlExecute) {
        Vuart_handshake_test___024root___eval_stl(vlSelf);
    }
    return (__VstlExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vuart_handshake_test___024root___dump_triggers__act(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VactTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 0 is active: @(posedge clk)\n");
    }
}
#endif  // VL_DEBUG

#ifdef VL_DEBUG
VL_ATTR_COLD void Vuart_handshake_test___024root___dump_triggers__nba(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___dump_triggers__nba\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VnbaTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 0 is active: @(posedge clk)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vuart_handshake_test___024root___ctor_var_reset(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___ctor_var_reset\n"); );
    // Body
    vlSelf->clk = VL_RAND_RESET_I(1);
    vlSelf->uart_tx = VL_RAND_RESET_I(1);
    vlSelf->uart_handshake_test__DOT__reset_cnt = VL_RAND_RESET_I(8);
    vlSelf->uart_handshake_test__DOT__resetn = VL_RAND_RESET_I(1);
    vlSelf->uart_handshake_test__DOT__state = VL_RAND_RESET_I(4);
    vlSelf->uart_handshake_test__DOT__counter = VL_RAND_RESET_I(32);
    vlSelf->uart_handshake_test__DOT__reg_dat_wait = VL_RAND_RESET_I(1);
    vlSelf->uart_handshake_test__DOT__reg_div_we_r = VL_RAND_RESET_I(4);
    vlSelf->uart_handshake_test__DOT__reg_div_di_r = VL_RAND_RESET_I(32);
    vlSelf->uart_handshake_test__DOT__reg_dat_we_r = VL_RAND_RESET_I(1);
    vlSelf->uart_handshake_test__DOT__reg_dat_di_r = VL_RAND_RESET_I(32);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider = VL_RAND_RESET_I(32);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_state = VL_RAND_RESET_I(4);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_divcnt = VL_RAND_RESET_I(32);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_pattern = VL_RAND_RESET_I(8);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_data = VL_RAND_RESET_I(8);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_valid = VL_RAND_RESET_I(1);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_pattern = VL_RAND_RESET_I(10);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt = VL_RAND_RESET_I(4);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_divcnt = VL_RAND_RESET_I(32);
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_dummy = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->__Vm_traceActivity[__Vi0] = 0;
    }
}
