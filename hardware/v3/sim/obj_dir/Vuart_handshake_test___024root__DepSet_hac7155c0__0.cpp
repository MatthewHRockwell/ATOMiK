// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vuart_handshake_test.h for the primary calling header

#include "Vuart_handshake_test__pch.h"
#include "Vuart_handshake_test___024root.h"

void Vuart_handshake_test___024root___eval_act(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_act\n"); );
}

VL_INLINE_OPT void Vuart_handshake_test___024root___nba_sequent__TOP__0(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___nba_sequent__TOP__0\n"); );
    // Init
    CData/*7:0*/ __Vdly__uart_handshake_test__DOT__reset_cnt;
    __Vdly__uart_handshake_test__DOT__reset_cnt = 0;
    IData/*31:0*/ __Vdly__uart_handshake_test__DOT__counter;
    __Vdly__uart_handshake_test__DOT__counter = 0;
    CData/*3:0*/ __Vdly__uart_handshake_test__DOT__state;
    __Vdly__uart_handshake_test__DOT__state = 0;
    IData/*31:0*/ __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_divcnt;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_divcnt = 0;
    CData/*3:0*/ __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_state;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_state = 0;
    CData/*0:0*/ __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_dummy;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_dummy = 0;
    IData/*31:0*/ __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_divcnt;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_divcnt = 0;
    SData/*9:0*/ __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_pattern;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_pattern = 0;
    CData/*3:0*/ __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_bitcnt;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_bitcnt = 0;
    // Body
    __Vdly__uart_handshake_test__DOT__reset_cnt = vlSelf->uart_handshake_test__DOT__reset_cnt;
    __Vdly__uart_handshake_test__DOT__state = vlSelf->uart_handshake_test__DOT__state;
    __Vdly__uart_handshake_test__DOT__counter = vlSelf->uart_handshake_test__DOT__counter;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_state 
        = vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_state;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_divcnt 
        = vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_divcnt;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_divcnt 
        = vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_divcnt;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_pattern 
        = vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_pattern;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_bitcnt 
        = vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt;
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_dummy 
        = vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_dummy;
    if ((1U & (~ (IData)(vlSelf->uart_handshake_test__DOT__resetn)))) {
        __Vdly__uart_handshake_test__DOT__reset_cnt 
            = (0xffU & ((IData)(1U) + (IData)(vlSelf->uart_handshake_test__DOT__reset_cnt)));
    }
    if ((0U != (IData)(vlSelf->uart_handshake_test__DOT__reg_div_we_r))) {
        __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_dummy = 1U;
    }
    __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_divcnt 
        = ((IData)(1U) + vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_divcnt);
    if ((0xffU == (IData)(vlSelf->uart_handshake_test__DOT__reset_cnt))) {
        if (((IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_dummy) 
             & (~ (IData)((0U != (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt)))))) {
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_pattern = 0x3ffU;
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_bitcnt = 0xfU;
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_divcnt = 0U;
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_dummy = 0U;
        } else if (((IData)(vlSelf->uart_handshake_test__DOT__reg_dat_we_r) 
                    & (~ (IData)((0U != (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt)))))) {
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_pattern 
                = (0x200U | (0x1feU & (vlSelf->uart_handshake_test__DOT__reg_dat_di_r 
                                       << 1U)));
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_bitcnt = 0xaU;
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_divcnt = 0U;
        } else if (((vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_divcnt 
                     > vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider) 
                    & (0U != (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt)))) {
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_pattern 
                = (0x200U | (0x1ffU & ((IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_pattern) 
                                       >> 1U)));
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_bitcnt 
                = (0xfU & ((IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt) 
                           - (IData)(1U)));
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_divcnt = 0U;
        }
        __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_divcnt 
            = ((IData)(1U) + vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_divcnt);
        if ((0U == (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_state))) {
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_divcnt = 0U;
        } else if ((1U == (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_state))) {
            if ((VL_SHIFTL_III(32,32,32, vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_divcnt, 1U) 
                 > vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider)) {
                __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_state = 2U;
                __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_divcnt = 0U;
            }
        } else if ((0xaU == (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_state))) {
            if ((vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_divcnt 
                 > vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider)) {
                vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_data 
                    = vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_pattern;
                vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_valid = 1U;
                __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_state = 0U;
            }
        } else if ((vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_divcnt 
                    > vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider)) {
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_state 
                = (0xfU & ((IData)(1U) + (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_state)));
            vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_pattern 
                = (0x80U | (0x7fU & ((IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_pattern) 
                                     >> 1U)));
            __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_divcnt = 0U;
        }
        if ((1U & (IData)(vlSelf->uart_handshake_test__DOT__reg_div_we_r))) {
            vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider 
                = ((0xffffff00U & vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider) 
                   | (0xffU & vlSelf->uart_handshake_test__DOT__reg_div_di_r));
        }
        if ((2U & (IData)(vlSelf->uart_handshake_test__DOT__reg_div_we_r))) {
            vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider 
                = ((0xffff00ffU & vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider) 
                   | (0xff00U & vlSelf->uart_handshake_test__DOT__reg_div_di_r));
        }
        if ((4U & (IData)(vlSelf->uart_handshake_test__DOT__reg_div_we_r))) {
            vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider 
                = ((0xff00ffffU & vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider) 
                   | (0xff0000U & vlSelf->uart_handshake_test__DOT__reg_div_di_r));
        }
        if ((8U & (IData)(vlSelf->uart_handshake_test__DOT__reg_div_we_r))) {
            vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider 
                = ((0xffffffU & vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider) 
                   | (0xff000000U & vlSelf->uart_handshake_test__DOT__reg_div_di_r));
        }
        __Vdly__uart_handshake_test__DOT__counter = 
            ((IData)(1U) + vlSelf->uart_handshake_test__DOT__counter);
        if (((((((((0U == (IData)(vlSelf->uart_handshake_test__DOT__state)) 
                   | (1U == (IData)(vlSelf->uart_handshake_test__DOT__state))) 
                  | (2U == (IData)(vlSelf->uart_handshake_test__DOT__state))) 
                 | (3U == (IData)(vlSelf->uart_handshake_test__DOT__state))) 
                | (4U == (IData)(vlSelf->uart_handshake_test__DOT__state))) 
               | (5U == (IData)(vlSelf->uart_handshake_test__DOT__state))) 
              | (6U == (IData)(vlSelf->uart_handshake_test__DOT__state))) 
             | (7U == (IData)(vlSelf->uart_handshake_test__DOT__state)))) {
            if ((0U == (IData)(vlSelf->uart_handshake_test__DOT__state))) {
                vlSelf->uart_handshake_test__DOT__reg_div_we_r = 0U;
                vlSelf->uart_handshake_test__DOT__reg_dat_we_r = 0U;
                if ((0x3e8U < vlSelf->uart_handshake_test__DOT__counter)) {
                    __Vdly__uart_handshake_test__DOT__state = 1U;
                    __Vdly__uart_handshake_test__DOT__counter = 0U;
                }
            } else if ((1U == (IData)(vlSelf->uart_handshake_test__DOT__state))) {
                vlSelf->uart_handshake_test__DOT__reg_div_we_r = 0xfU;
                vlSelf->uart_handshake_test__DOT__reg_div_di_r = 0xe8U;
                __Vdly__uart_handshake_test__DOT__state = 2U;
            } else if ((2U == (IData)(vlSelf->uart_handshake_test__DOT__state))) {
                vlSelf->uart_handshake_test__DOT__reg_div_we_r = 0U;
                __Vdly__uart_handshake_test__DOT__counter = 0U;
                __Vdly__uart_handshake_test__DOT__state = 3U;
            } else if ((3U == (IData)(vlSelf->uart_handshake_test__DOT__state))) {
                if ((0xfa0U < vlSelf->uart_handshake_test__DOT__counter)) {
                    __Vdly__uart_handshake_test__DOT__state = 4U;
                    __Vdly__uart_handshake_test__DOT__counter = 0U;
                }
            } else if ((4U == (IData)(vlSelf->uart_handshake_test__DOT__state))) {
                vlSelf->uart_handshake_test__DOT__reg_dat_we_r = 1U;
                vlSelf->uart_handshake_test__DOT__reg_dat_di_r = 0x55U;
                __Vdly__uart_handshake_test__DOT__state = 5U;
            } else if ((5U == (IData)(vlSelf->uart_handshake_test__DOT__state))) {
                if (vlSelf->uart_handshake_test__DOT__reg_dat_wait) {
                    vlSelf->uart_handshake_test__DOT__reg_dat_we_r = 1U;
                } else {
                    vlSelf->uart_handshake_test__DOT__reg_dat_we_r = 1U;
                    __Vdly__uart_handshake_test__DOT__state = 6U;
                }
            } else if ((6U == (IData)(vlSelf->uart_handshake_test__DOT__state))) {
                if (vlSelf->uart_handshake_test__DOT__reg_dat_wait) {
                    vlSelf->uart_handshake_test__DOT__reg_dat_we_r = 0U;
                    __Vdly__uart_handshake_test__DOT__state = 7U;
                    __Vdly__uart_handshake_test__DOT__counter = 0U;
                } else {
                    vlSelf->uart_handshake_test__DOT__reg_dat_we_r = 1U;
                }
            } else if ((0x1388U < vlSelf->uart_handshake_test__DOT__counter)) {
                __Vdly__uart_handshake_test__DOT__state = 4U;
                __Vdly__uart_handshake_test__DOT__counter = 0U;
            }
        } else {
            __Vdly__uart_handshake_test__DOT__state = 0U;
        }
    } else {
        __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_pattern = 0x3ffU;
        __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_bitcnt = 0U;
        __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_divcnt = 0U;
        __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_dummy = 1U;
        vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_pattern = 0U;
        __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_state = 0U;
        __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_divcnt = 0U;
        vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_data = 0U;
        vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_valid = 0U;
        vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider = 1U;
        __Vdly__uart_handshake_test__DOT__state = 0U;
        __Vdly__uart_handshake_test__DOT__counter = 0U;
        vlSelf->uart_handshake_test__DOT__reg_div_we_r = 0U;
        vlSelf->uart_handshake_test__DOT__reg_dat_we_r = 0U;
    }
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_divcnt 
        = __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_divcnt;
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_pattern 
        = __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_pattern;
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_dummy 
        = __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_dummy;
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt 
        = __Vdly__uart_handshake_test__DOT__u_uart__DOT__send_bitcnt;
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_divcnt 
        = __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_divcnt;
    vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_state 
        = __Vdly__uart_handshake_test__DOT__u_uart__DOT__recv_state;
    vlSelf->uart_tx = (1U & (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_pattern));
    vlSelf->uart_handshake_test__DOT__counter = __Vdly__uart_handshake_test__DOT__counter;
    vlSelf->uart_handshake_test__DOT__state = __Vdly__uart_handshake_test__DOT__state;
    vlSelf->uart_handshake_test__DOT__reset_cnt = __Vdly__uart_handshake_test__DOT__reset_cnt;
    vlSelf->uart_handshake_test__DOT__reg_dat_wait 
        = ((IData)(vlSelf->uart_handshake_test__DOT__reg_dat_we_r) 
           & ((0U != (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt)) 
              | (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_dummy)));
    vlSelf->uart_handshake_test__DOT__resetn = (0xffU 
                                                == (IData)(vlSelf->uart_handshake_test__DOT__reset_cnt));
}

void Vuart_handshake_test___024root___eval_nba(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_nba\n"); );
    // Body
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        Vuart_handshake_test___024root___nba_sequent__TOP__0(vlSelf);
        vlSelf->__Vm_traceActivity[1U] = 1U;
    }
}

void Vuart_handshake_test___024root___eval_triggers__act(Vuart_handshake_test___024root* vlSelf);

bool Vuart_handshake_test___024root___eval_phase__act(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_phase__act\n"); );
    // Init
    VlTriggerVec<1> __VpreTriggered;
    CData/*0:0*/ __VactExecute;
    // Body
    Vuart_handshake_test___024root___eval_triggers__act(vlSelf);
    __VactExecute = vlSelf->__VactTriggered.any();
    if (__VactExecute) {
        __VpreTriggered.andNot(vlSelf->__VactTriggered, vlSelf->__VnbaTriggered);
        vlSelf->__VnbaTriggered.thisOr(vlSelf->__VactTriggered);
        Vuart_handshake_test___024root___eval_act(vlSelf);
    }
    return (__VactExecute);
}

bool Vuart_handshake_test___024root___eval_phase__nba(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_phase__nba\n"); );
    // Init
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = vlSelf->__VnbaTriggered.any();
    if (__VnbaExecute) {
        Vuart_handshake_test___024root___eval_nba(vlSelf);
        vlSelf->__VnbaTriggered.clear();
    }
    return (__VnbaExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vuart_handshake_test___024root___dump_triggers__nba(Vuart_handshake_test___024root* vlSelf);
#endif  // VL_DEBUG
#ifdef VL_DEBUG
VL_ATTR_COLD void Vuart_handshake_test___024root___dump_triggers__act(Vuart_handshake_test___024root* vlSelf);
#endif  // VL_DEBUG

void Vuart_handshake_test___024root___eval(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval\n"); );
    // Init
    IData/*31:0*/ __VnbaIterCount;
    CData/*0:0*/ __VnbaContinue;
    // Body
    __VnbaIterCount = 0U;
    __VnbaContinue = 1U;
    while (__VnbaContinue) {
        if (VL_UNLIKELY((0x64U < __VnbaIterCount))) {
#ifdef VL_DEBUG
            Vuart_handshake_test___024root___dump_triggers__nba(vlSelf);
#endif
            VL_FATAL_MT("../test/uart_handshake_test.v", 3, "", "NBA region did not converge.");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        __VnbaContinue = 0U;
        vlSelf->__VactIterCount = 0U;
        vlSelf->__VactContinue = 1U;
        while (vlSelf->__VactContinue) {
            if (VL_UNLIKELY((0x64U < vlSelf->__VactIterCount))) {
#ifdef VL_DEBUG
                Vuart_handshake_test___024root___dump_triggers__act(vlSelf);
#endif
                VL_FATAL_MT("../test/uart_handshake_test.v", 3, "", "Active region did not converge.");
            }
            vlSelf->__VactIterCount = ((IData)(1U) 
                                       + vlSelf->__VactIterCount);
            vlSelf->__VactContinue = 0U;
            if (Vuart_handshake_test___024root___eval_phase__act(vlSelf)) {
                vlSelf->__VactContinue = 1U;
            }
        }
        if (Vuart_handshake_test___024root___eval_phase__nba(vlSelf)) {
            __VnbaContinue = 1U;
        }
    }
}

#ifdef VL_DEBUG
void Vuart_handshake_test___024root___eval_debug_assertions(Vuart_handshake_test___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root___eval_debug_assertions\n"); );
    // Body
    if (VL_UNLIKELY((vlSelf->clk & 0xfeU))) {
        Verilated::overWidthError("clk");}
}
#endif  // VL_DEBUG
