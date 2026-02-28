// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vmanual_uart_tx.h for the primary calling header

#include "Vmanual_uart_tx__pch.h"
#include "Vmanual_uart_tx___024root.h"

void Vmanual_uart_tx___024root___eval_act(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_act\n"); );
}

VL_INLINE_OPT void Vmanual_uart_tx___024root___nba_sequent__TOP__0(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___nba_sequent__TOP__0\n"); );
    // Init
    IData/*31:0*/ __Vdly__manual_uart_tx__DOT__bit_counter;
    __Vdly__manual_uart_tx__DOT__bit_counter = 0;
    CData/*3:0*/ __Vdly__manual_uart_tx__DOT__bit_index;
    __Vdly__manual_uart_tx__DOT__bit_index = 0;
    // Body
    __Vdly__manual_uart_tx__DOT__bit_index = vlSelf->manual_uart_tx__DOT__bit_index;
    __Vdly__manual_uart_tx__DOT__bit_counter = vlSelf->manual_uart_tx__DOT__bit_counter;
    if (vlSelf->resetn) {
        if (vlSelf->manual_uart_tx__DOT__tx_active) {
            if ((vlSelf->manual_uart_tx__DOT__bit_counter 
                 >= vlSelf->manual_uart_tx__DOT__cfg_divider)) {
                __Vdly__manual_uart_tx__DOT__bit_counter = 0U;
                if ((0xaU > (IData)(vlSelf->manual_uart_tx__DOT__bit_index))) {
                    __Vdly__manual_uart_tx__DOT__bit_index 
                        = (0xfU & ((IData)(1U) + (IData)(vlSelf->manual_uart_tx__DOT__bit_index)));
                    vlSelf->manual_uart_tx__DOT__tx_shift_reg 
                        = ((1U & (~ ((((((((0U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index)) 
                                           | (1U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))) 
                                          | (2U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))) 
                                         | (3U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))) 
                                        | (4U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))) 
                                       | (5U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))) 
                                      | (6U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))) 
                                     | (7U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))))) 
                           || (1U & ((0U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))
                                      ? (IData)(vlSelf->manual_uart_tx__DOT__tx_data)
                                      : ((1U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))
                                          ? ((IData)(vlSelf->manual_uart_tx__DOT__tx_data) 
                                             >> 1U)
                                          : ((2U == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))
                                              ? ((IData)(vlSelf->manual_uart_tx__DOT__tx_data) 
                                                 >> 2U)
                                              : ((3U 
                                                  == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))
                                                  ? 
                                                 ((IData)(vlSelf->manual_uart_tx__DOT__tx_data) 
                                                  >> 3U)
                                                  : 
                                                 ((4U 
                                                   == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))
                                                   ? 
                                                  ((IData)(vlSelf->manual_uart_tx__DOT__tx_data) 
                                                   >> 4U)
                                                   : 
                                                  ((5U 
                                                    == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))
                                                    ? 
                                                   ((IData)(vlSelf->manual_uart_tx__DOT__tx_data) 
                                                    >> 5U)
                                                    : 
                                                   ((6U 
                                                     == (IData)(vlSelf->manual_uart_tx__DOT__bit_index))
                                                     ? 
                                                    ((IData)(vlSelf->manual_uart_tx__DOT__tx_data) 
                                                     >> 6U)
                                                     : 
                                                    ((IData)(vlSelf->manual_uart_tx__DOT__tx_data) 
                                                     >> 7U))))))))));
                } else {
                    vlSelf->manual_uart_tx__DOT__tx_active = 0U;
                    __Vdly__manual_uart_tx__DOT__bit_index = 0U;
                    vlSelf->manual_uart_tx__DOT__tx_shift_reg = 1U;
                }
            } else {
                __Vdly__manual_uart_tx__DOT__bit_counter 
                    = ((IData)(1U) + vlSelf->manual_uart_tx__DOT__bit_counter);
            }
        } else {
            vlSelf->manual_uart_tx__DOT__tx_shift_reg = 1U;
            if (vlSelf->reg_dat_we) {
                __Vdly__manual_uart_tx__DOT__bit_index = 0U;
                vlSelf->manual_uart_tx__DOT__tx_data 
                    = (0xffU & vlSelf->reg_dat_di);
                vlSelf->manual_uart_tx__DOT__tx_active = 1U;
                __Vdly__manual_uart_tx__DOT__bit_counter = 0U;
                vlSelf->manual_uart_tx__DOT__tx_shift_reg = 0U;
            }
        }
        if ((1U & (IData)(vlSelf->reg_div_we))) {
            vlSelf->manual_uart_tx__DOT__cfg_divider 
                = ((0xffffff00U & vlSelf->manual_uart_tx__DOT__cfg_divider) 
                   | (0xffU & vlSelf->reg_div_di));
        }
        if ((2U & (IData)(vlSelf->reg_div_we))) {
            vlSelf->manual_uart_tx__DOT__cfg_divider 
                = ((0xffff00ffU & vlSelf->manual_uart_tx__DOT__cfg_divider) 
                   | (0xff00U & vlSelf->reg_div_di));
        }
        if ((4U & (IData)(vlSelf->reg_div_we))) {
            vlSelf->manual_uart_tx__DOT__cfg_divider 
                = ((0xff00ffffU & vlSelf->manual_uart_tx__DOT__cfg_divider) 
                   | (0xff0000U & vlSelf->reg_div_di));
        }
        if ((8U & (IData)(vlSelf->reg_div_we))) {
            vlSelf->manual_uart_tx__DOT__cfg_divider 
                = ((0xffffffU & vlSelf->manual_uart_tx__DOT__cfg_divider) 
                   | (0xff000000U & vlSelf->reg_div_di));
        }
    } else {
        __Vdly__manual_uart_tx__DOT__bit_index = 0U;
        __Vdly__manual_uart_tx__DOT__bit_counter = 0U;
        vlSelf->manual_uart_tx__DOT__tx_data = 0U;
        vlSelf->manual_uart_tx__DOT__tx_active = 0U;
        vlSelf->manual_uart_tx__DOT__tx_shift_reg = 1U;
        vlSelf->manual_uart_tx__DOT__cfg_divider = 1U;
    }
    vlSelf->manual_uart_tx__DOT__bit_counter = __Vdly__manual_uart_tx__DOT__bit_counter;
    vlSelf->manual_uart_tx__DOT__bit_index = __Vdly__manual_uart_tx__DOT__bit_index;
    vlSelf->ser_tx = vlSelf->manual_uart_tx__DOT__tx_shift_reg;
    vlSelf->reg_dat_wait = vlSelf->manual_uart_tx__DOT__tx_active;
    vlSelf->reg_div_do = vlSelf->manual_uart_tx__DOT__cfg_divider;
}

void Vmanual_uart_tx___024root___eval_nba(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_nba\n"); );
    // Body
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        Vmanual_uart_tx___024root___nba_sequent__TOP__0(vlSelf);
    }
}

void Vmanual_uart_tx___024root___eval_triggers__act(Vmanual_uart_tx___024root* vlSelf);

bool Vmanual_uart_tx___024root___eval_phase__act(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_phase__act\n"); );
    // Init
    VlTriggerVec<1> __VpreTriggered;
    CData/*0:0*/ __VactExecute;
    // Body
    Vmanual_uart_tx___024root___eval_triggers__act(vlSelf);
    __VactExecute = vlSelf->__VactTriggered.any();
    if (__VactExecute) {
        __VpreTriggered.andNot(vlSelf->__VactTriggered, vlSelf->__VnbaTriggered);
        vlSelf->__VnbaTriggered.thisOr(vlSelf->__VactTriggered);
        Vmanual_uart_tx___024root___eval_act(vlSelf);
    }
    return (__VactExecute);
}

bool Vmanual_uart_tx___024root___eval_phase__nba(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_phase__nba\n"); );
    // Init
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = vlSelf->__VnbaTriggered.any();
    if (__VnbaExecute) {
        Vmanual_uart_tx___024root___eval_nba(vlSelf);
        vlSelf->__VnbaTriggered.clear();
    }
    return (__VnbaExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vmanual_uart_tx___024root___dump_triggers__nba(Vmanual_uart_tx___024root* vlSelf);
#endif  // VL_DEBUG
#ifdef VL_DEBUG
VL_ATTR_COLD void Vmanual_uart_tx___024root___dump_triggers__act(Vmanual_uart_tx___024root* vlSelf);
#endif  // VL_DEBUG

void Vmanual_uart_tx___024root___eval(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval\n"); );
    // Init
    IData/*31:0*/ __VnbaIterCount;
    CData/*0:0*/ __VnbaContinue;
    // Body
    __VnbaIterCount = 0U;
    __VnbaContinue = 1U;
    while (__VnbaContinue) {
        if (VL_UNLIKELY((0x64U < __VnbaIterCount))) {
#ifdef VL_DEBUG
            Vmanual_uart_tx___024root___dump_triggers__nba(vlSelf);
#endif
            VL_FATAL_MT("../soc/manual_uart_tx.v", 15, "", "NBA region did not converge.");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        __VnbaContinue = 0U;
        vlSelf->__VactIterCount = 0U;
        vlSelf->__VactContinue = 1U;
        while (vlSelf->__VactContinue) {
            if (VL_UNLIKELY((0x64U < vlSelf->__VactIterCount))) {
#ifdef VL_DEBUG
                Vmanual_uart_tx___024root___dump_triggers__act(vlSelf);
#endif
                VL_FATAL_MT("../soc/manual_uart_tx.v", 15, "", "Active region did not converge.");
            }
            vlSelf->__VactIterCount = ((IData)(1U) 
                                       + vlSelf->__VactIterCount);
            vlSelf->__VactContinue = 0U;
            if (Vmanual_uart_tx___024root___eval_phase__act(vlSelf)) {
                vlSelf->__VactContinue = 1U;
            }
        }
        if (Vmanual_uart_tx___024root___eval_phase__nba(vlSelf)) {
            __VnbaContinue = 1U;
        }
    }
}

#ifdef VL_DEBUG
void Vmanual_uart_tx___024root___eval_debug_assertions(Vmanual_uart_tx___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vmanual_uart_tx__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vmanual_uart_tx___024root___eval_debug_assertions\n"); );
    // Body
    if (VL_UNLIKELY((vlSelf->clk & 0xfeU))) {
        Verilated::overWidthError("clk");}
    if (VL_UNLIKELY((vlSelf->resetn & 0xfeU))) {
        Verilated::overWidthError("resetn");}
    if (VL_UNLIKELY((vlSelf->ser_rx & 0xfeU))) {
        Verilated::overWidthError("ser_rx");}
    if (VL_UNLIKELY((vlSelf->reg_div_we & 0xf0U))) {
        Verilated::overWidthError("reg_div_we");}
    if (VL_UNLIKELY((vlSelf->reg_dat_we & 0xfeU))) {
        Verilated::overWidthError("reg_dat_we");}
    if (VL_UNLIKELY((vlSelf->reg_dat_re & 0xfeU))) {
        Verilated::overWidthError("reg_dat_re");}
}
#endif  // VL_DEBUG
