// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vmanual_uart_tx.h for the primary calling header

#include "Vmanual_uart_tx__pch.h"
#include "Vmanual_uart_tx___024root.h"

VL_ATTR_COLD void Vmanual_uart_tx___024root___eval_static(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_static\n"); );
}

VL_ATTR_COLD void Vmanual_uart_tx___024root___eval_initial__TOP(Vmanual_uart_tx___024root* vlSelf);

VL_ATTR_COLD void Vmanual_uart_tx___024root___eval_initial(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_initial\n"); );
    // Body
    Vmanual_uart_tx___024root___eval_initial__TOP(vlSelf);
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = vlSelf->clk;
}

VL_ATTR_COLD void Vmanual_uart_tx___024root___eval_initial__TOP(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_initial__TOP\n"); );
    // Body
    vlSelf->reg_dat_do = 0xffffffffU;
}

VL_ATTR_COLD void Vmanual_uart_tx___024root___eval_final(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_final\n"); );
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vmanual_uart_tx___024root___dump_triggers__stl(Vmanual_uart_tx___024root* vlSelf);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vmanual_uart_tx___024root___eval_phase__stl(Vmanual_uart_tx___024root* vlSelf);

VL_ATTR_COLD void Vmanual_uart_tx___024root___eval_settle(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_settle\n"); );
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
            Vmanual_uart_tx___024root___dump_triggers__stl(vlSelf);
#endif
            VL_FATAL_MT("../soc/manual_uart_tx.v", 15, "", "Settle region did not converge.");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
        __VstlContinue = 0U;
        if (Vmanual_uart_tx___024root___eval_phase__stl(vlSelf)) {
            __VstlContinue = 1U;
        }
        vlSelf->__VstlFirstIteration = 0U;
    }
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vmanual_uart_tx___024root___dump_triggers__stl(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VstlTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vmanual_uart_tx___024root___stl_sequent__TOP__0(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___stl_sequent__TOP__0\n"); );
    // Body
    vlSelf->ser_tx = vlSelf->manual_uart_tx__DOT__tx_shift_reg;
    vlSelf->reg_div_do = vlSelf->manual_uart_tx__DOT__cfg_divider;
    vlSelf->reg_dat_wait = vlSelf->manual_uart_tx__DOT__tx_active;
}

VL_ATTR_COLD void Vmanual_uart_tx___024root___eval_stl(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_stl\n"); );
    // Body
    if ((1ULL & vlSelf->__VstlTriggered.word(0U))) {
        Vmanual_uart_tx___024root___stl_sequent__TOP__0(vlSelf);
    }
}

VL_ATTR_COLD void Vmanual_uart_tx___024root___eval_triggers__stl(Vmanual_uart_tx___024root* vlSelf);

VL_ATTR_COLD bool Vmanual_uart_tx___024root___eval_phase__stl(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_phase__stl\n"); );
    // Init
    CData/*0:0*/ __VstlExecute;
    // Body
    Vmanual_uart_tx___024root___eval_triggers__stl(vlSelf);
    __VstlExecute = vlSelf->__VstlTriggered.any();
    if (__VstlExecute) {
        Vmanual_uart_tx___024root___eval_stl(vlSelf);
    }
    return (__VstlExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vmanual_uart_tx___024root___dump_triggers__act(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___dump_triggers__act\n"); );
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
VL_ATTR_COLD void Vmanual_uart_tx___024root___dump_triggers__nba(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___dump_triggers__nba\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VnbaTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 0 is active: @(posedge clk)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vmanual_uart_tx___024root___ctor_var_reset(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___ctor_var_reset\n"); );
    // Body
    vlSelf->clk = VL_RAND_RESET_I(1);
    vlSelf->resetn = VL_RAND_RESET_I(1);
    vlSelf->ser_tx = VL_RAND_RESET_I(1);
    vlSelf->ser_rx = VL_RAND_RESET_I(1);
    vlSelf->reg_div_we = VL_RAND_RESET_I(4);
    vlSelf->reg_div_di = VL_RAND_RESET_I(32);
    vlSelf->reg_div_do = VL_RAND_RESET_I(32);
    vlSelf->reg_dat_we = VL_RAND_RESET_I(1);
    vlSelf->reg_dat_re = VL_RAND_RESET_I(1);
    vlSelf->reg_dat_di = VL_RAND_RESET_I(32);
    vlSelf->reg_dat_do = VL_RAND_RESET_I(32);
    vlSelf->reg_dat_wait = VL_RAND_RESET_I(1);
    vlSelf->manual_uart_tx__DOT__cfg_divider = VL_RAND_RESET_I(32);
    vlSelf->manual_uart_tx__DOT__bit_counter = VL_RAND_RESET_I(32);
    vlSelf->manual_uart_tx__DOT__bit_index = VL_RAND_RESET_I(4);
    vlSelf->manual_uart_tx__DOT__tx_data = VL_RAND_RESET_I(8);
    vlSelf->manual_uart_tx__DOT__tx_active = VL_RAND_RESET_I(1);
    vlSelf->manual_uart_tx__DOT__tx_shift_reg = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = VL_RAND_RESET_I(1);
}
