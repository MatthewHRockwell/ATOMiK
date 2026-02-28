// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Tracing implementation internals
#include "verilated_vcd_c.h"
#include "Vuart_handshake_test__Syms.h"


void Vuart_handshake_test___024root__trace_chg_0_sub_0(Vuart_handshake_test___024root* vlSelf, VerilatedVcd::Buffer* bufp);

void Vuart_handshake_test___024root__trace_chg_0(void* voidSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_chg_0\n"); );
    // Init
    Vuart_handshake_test___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vuart_handshake_test___024root*>(voidSelf);
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;
    // Body
    Vuart_handshake_test___024root__trace_chg_0_sub_0((&vlSymsp->TOP), bufp);
}

void Vuart_handshake_test___024root__trace_chg_0_sub_0(Vuart_handshake_test___024root* vlSelf, VerilatedVcd::Buffer* bufp) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_chg_0_sub_0\n"); );
    // Init
    uint32_t* const oldp VL_ATTR_UNUSED = bufp->oldp(vlSymsp->__Vm_baseCode + 1);
    // Body
    if (VL_UNLIKELY(vlSelf->__Vm_traceActivity[1U])) {
        bufp->chgCData(oldp+0,(vlSelf->uart_handshake_test__DOT__reset_cnt),8);
        bufp->chgBit(oldp+1,((0xffU == (IData)(vlSelf->uart_handshake_test__DOT__reset_cnt))));
        bufp->chgCData(oldp+2,(vlSelf->uart_handshake_test__DOT__state),4);
        bufp->chgIData(oldp+3,(vlSelf->uart_handshake_test__DOT__counter),32);
        bufp->chgIData(oldp+4,(((IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_valid)
                                 ? (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_data)
                                 : 0xffffffffU)),32);
        bufp->chgIData(oldp+5,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider),32);
        bufp->chgBit(oldp+6,(vlSelf->uart_handshake_test__DOT__reg_dat_wait));
        bufp->chgCData(oldp+7,(vlSelf->uart_handshake_test__DOT__reg_div_we_r),4);
        bufp->chgIData(oldp+8,(vlSelf->uart_handshake_test__DOT__reg_div_di_r),32);
        bufp->chgBit(oldp+9,(vlSelf->uart_handshake_test__DOT__reg_dat_we_r));
        bufp->chgIData(oldp+10,(vlSelf->uart_handshake_test__DOT__reg_dat_di_r),32);
        bufp->chgCData(oldp+11,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_state),4);
        bufp->chgIData(oldp+12,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_divcnt),32);
        bufp->chgCData(oldp+13,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_pattern),8);
        bufp->chgCData(oldp+14,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_data),8);
        bufp->chgBit(oldp+15,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_valid));
        bufp->chgSData(oldp+16,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_pattern),10);
        bufp->chgCData(oldp+17,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt),4);
        bufp->chgIData(oldp+18,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_divcnt),32);
        bufp->chgBit(oldp+19,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_dummy));
    }
    bufp->chgBit(oldp+20,(vlSelf->clk));
    bufp->chgBit(oldp+21,(vlSelf->uart_tx));
}

void Vuart_handshake_test___024root__trace_cleanup(void* voidSelf, VerilatedVcd* /*unused*/) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_cleanup\n"); );
    // Init
    Vuart_handshake_test___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vuart_handshake_test___024root*>(voidSelf);
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    // Body
    vlSymsp->__Vm_activity = false;
    vlSymsp->TOP.__Vm_traceActivity[0U] = 0U;
    vlSymsp->TOP.__Vm_traceActivity[1U] = 0U;
}
