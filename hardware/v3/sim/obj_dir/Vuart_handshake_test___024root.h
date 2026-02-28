// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vuart_handshake_test.h for the primary calling header

#ifndef VERILATED_VUART_HANDSHAKE_TEST___024ROOT_H_
#define VERILATED_VUART_HANDSHAKE_TEST___024ROOT_H_  // guard

#include "verilated.h"


class Vuart_handshake_test__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vuart_handshake_test___024root final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_OUT8(uart_tx,0,0);
    CData/*7:0*/ uart_handshake_test__DOT__reset_cnt;
    CData/*0:0*/ uart_handshake_test__DOT__resetn;
    CData/*3:0*/ uart_handshake_test__DOT__state;
    CData/*0:0*/ uart_handshake_test__DOT__reg_dat_wait;
    CData/*3:0*/ uart_handshake_test__DOT__reg_div_we_r;
    CData/*0:0*/ uart_handshake_test__DOT__reg_dat_we_r;
    CData/*3:0*/ uart_handshake_test__DOT__u_uart__DOT__recv_state;
    CData/*7:0*/ uart_handshake_test__DOT__u_uart__DOT__recv_pattern;
    CData/*7:0*/ uart_handshake_test__DOT__u_uart__DOT__recv_buf_data;
    CData/*0:0*/ uart_handshake_test__DOT__u_uart__DOT__recv_buf_valid;
    CData/*3:0*/ uart_handshake_test__DOT__u_uart__DOT__send_bitcnt;
    CData/*0:0*/ uart_handshake_test__DOT__u_uart__DOT__send_dummy;
    CData/*0:0*/ __VstlFirstIteration;
    CData/*0:0*/ __Vtrigprevexpr___TOP__clk__0;
    CData/*0:0*/ __VactContinue;
    SData/*9:0*/ uart_handshake_test__DOT__u_uart__DOT__send_pattern;
    IData/*31:0*/ uart_handshake_test__DOT__counter;
    IData/*31:0*/ uart_handshake_test__DOT__reg_div_di_r;
    IData/*31:0*/ uart_handshake_test__DOT__reg_dat_di_r;
    IData/*31:0*/ uart_handshake_test__DOT__u_uart__DOT__cfg_divider;
    IData/*31:0*/ uart_handshake_test__DOT__u_uart__DOT__recv_divcnt;
    IData/*31:0*/ uart_handshake_test__DOT__u_uart__DOT__send_divcnt;
    IData/*31:0*/ __VactIterCount;
    VlUnpacked<CData/*0:0*/, 2> __Vm_traceActivity;
    VlTriggerVec<1> __VstlTriggered;
    VlTriggerVec<1> __VactTriggered;
    VlTriggerVec<1> __VnbaTriggered;

    // INTERNAL VARIABLES
    Vuart_handshake_test__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vuart_handshake_test___024root(Vuart_handshake_test__Syms* symsp, const char* v__name);
    ~Vuart_handshake_test___024root();
    VL_UNCOPYABLE(Vuart_handshake_test___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
