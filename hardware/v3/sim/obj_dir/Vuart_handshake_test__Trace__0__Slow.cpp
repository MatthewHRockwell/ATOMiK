// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Tracing implementation internals
#include "verilated_vcd_c.h"
#include "Vuart_handshake_test__Syms.h"


VL_ATTR_COLD void Vuart_handshake_test___024root__trace_init_sub__TOP__0(Vuart_handshake_test___024root* vlSelf, VerilatedVcd* tracep) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_init_sub__TOP__0\n"); );
    // Init
    const int c = vlSymsp->__Vm_baseCode;
    // Body
    tracep->declBit(c+21,0,"clk",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+22,0,"uart_tx",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->pushPrefix("uart_handshake_test", VerilatedTracePrefixType::SCOPE_MODULE);
    tracep->declBit(c+21,0,"clk",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+22,0,"uart_tx",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+1,0,"reset_cnt",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 7,0);
    tracep->declBit(c+2,0,"resetn",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+3,0,"state",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 3,0);
    tracep->declBus(c+4,0,"counter",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBus(c+5,0,"reg_dat_do",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBus(c+6,0,"reg_div_do",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBit(c+7,0,"reg_dat_wait",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+8,0,"reg_div_we_r",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 3,0);
    tracep->declBus(c+9,0,"reg_div_di_r",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBit(c+10,0,"reg_dat_we_r",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+11,0,"reg_dat_di_r",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->pushPrefix("u_uart", VerilatedTracePrefixType::SCOPE_MODULE);
    tracep->declBit(c+21,0,"clk",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+2,0,"resetn",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+22,0,"ser_tx",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+23,0,"ser_rx",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+8,0,"reg_div_we",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 3,0);
    tracep->declBus(c+9,0,"reg_div_di",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBus(c+6,0,"reg_div_do",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBit(c+10,0,"reg_dat_we",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+24,0,"reg_dat_re",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+11,0,"reg_dat_di",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBus(c+5,0,"reg_dat_do",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBit(c+7,0,"reg_dat_wait",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+6,0,"cfg_divider",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBus(c+12,0,"recv_state",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 3,0);
    tracep->declBus(c+13,0,"recv_divcnt",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBus(c+14,0,"recv_pattern",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 7,0);
    tracep->declBus(c+15,0,"recv_buf_data",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 7,0);
    tracep->declBit(c+16,0,"recv_buf_valid",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+17,0,"send_pattern",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 9,0);
    tracep->declBus(c+18,0,"send_bitcnt",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 3,0);
    tracep->declBus(c+19,0,"send_divcnt",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBit(c+20,0,"send_dummy",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->popPrefix();
    tracep->popPrefix();
}

VL_ATTR_COLD void Vuart_handshake_test___024root__trace_init_top(Vuart_handshake_test___024root* vlSelf, VerilatedVcd* tracep) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_init_top\n"); );
    // Body
    Vuart_handshake_test___024root__trace_init_sub__TOP__0(vlSelf, tracep);
}

VL_ATTR_COLD void Vuart_handshake_test___024root__trace_const_0(void* voidSelf, VerilatedVcd::Buffer* bufp);
VL_ATTR_COLD void Vuart_handshake_test___024root__trace_full_0(void* voidSelf, VerilatedVcd::Buffer* bufp);
void Vuart_handshake_test___024root__trace_chg_0(void* voidSelf, VerilatedVcd::Buffer* bufp);
void Vuart_handshake_test___024root__trace_cleanup(void* voidSelf, VerilatedVcd* /*unused*/);

VL_ATTR_COLD void Vuart_handshake_test___024root__trace_register(Vuart_handshake_test___024root* vlSelf, VerilatedVcd* tracep) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_register\n"); );
    // Body
    tracep->addConstCb(&Vuart_handshake_test___024root__trace_const_0, 0U, vlSelf);
    tracep->addFullCb(&Vuart_handshake_test___024root__trace_full_0, 0U, vlSelf);
    tracep->addChgCb(&Vuart_handshake_test___024root__trace_chg_0, 0U, vlSelf);
    tracep->addCleanupCb(&Vuart_handshake_test___024root__trace_cleanup, vlSelf);
}

VL_ATTR_COLD void Vuart_handshake_test___024root__trace_const_0_sub_0(Vuart_handshake_test___024root* vlSelf, VerilatedVcd::Buffer* bufp);

VL_ATTR_COLD void Vuart_handshake_test___024root__trace_const_0(void* voidSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_const_0\n"); );
    // Init
    Vuart_handshake_test___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vuart_handshake_test___024root*>(voidSelf);
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    // Body
    Vuart_handshake_test___024root__trace_const_0_sub_0((&vlSymsp->TOP), bufp);
}

VL_ATTR_COLD void Vuart_handshake_test___024root__trace_const_0_sub_0(Vuart_handshake_test___024root* vlSelf, VerilatedVcd::Buffer* bufp) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_const_0_sub_0\n"); );
    // Init
    uint32_t* const oldp VL_ATTR_UNUSED = bufp->oldp(vlSymsp->__Vm_baseCode);
    // Body
    bufp->fullBit(oldp+23,(1U));
    bufp->fullBit(oldp+24,(0U));
}

VL_ATTR_COLD void Vuart_handshake_test___024root__trace_full_0_sub_0(Vuart_handshake_test___024root* vlSelf, VerilatedVcd::Buffer* bufp);

VL_ATTR_COLD void Vuart_handshake_test___024root__trace_full_0(void* voidSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_full_0\n"); );
    // Init
    Vuart_handshake_test___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vuart_handshake_test___024root*>(voidSelf);
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    // Body
    Vuart_handshake_test___024root__trace_full_0_sub_0((&vlSymsp->TOP), bufp);
}

VL_ATTR_COLD void Vuart_handshake_test___024root__trace_full_0_sub_0(Vuart_handshake_test___024root* vlSelf, VerilatedVcd::Buffer* bufp) {
    (void)vlSelf;  // Prevent unused variable warning
    Vuart_handshake_test__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vuart_handshake_test___024root__trace_full_0_sub_0\n"); );
    // Init
    uint32_t* const oldp VL_ATTR_UNUSED = bufp->oldp(vlSymsp->__Vm_baseCode);
    // Body
    bufp->fullCData(oldp+1,(vlSelf->uart_handshake_test__DOT__reset_cnt),8);
    bufp->fullBit(oldp+2,((0xffU == (IData)(vlSelf->uart_handshake_test__DOT__reset_cnt))));
    bufp->fullCData(oldp+3,(vlSelf->uart_handshake_test__DOT__state),4);
    bufp->fullIData(oldp+4,(vlSelf->uart_handshake_test__DOT__counter),32);
    bufp->fullIData(oldp+5,(((IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_valid)
                              ? (IData)(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_data)
                              : 0xffffffffU)),32);
    bufp->fullIData(oldp+6,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__cfg_divider),32);
    bufp->fullBit(oldp+7,(vlSelf->uart_handshake_test__DOT__reg_dat_wait));
    bufp->fullCData(oldp+8,(vlSelf->uart_handshake_test__DOT__reg_div_we_r),4);
    bufp->fullIData(oldp+9,(vlSelf->uart_handshake_test__DOT__reg_div_di_r),32);
    bufp->fullBit(oldp+10,(vlSelf->uart_handshake_test__DOT__reg_dat_we_r));
    bufp->fullIData(oldp+11,(vlSelf->uart_handshake_test__DOT__reg_dat_di_r),32);
    bufp->fullCData(oldp+12,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_state),4);
    bufp->fullIData(oldp+13,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_divcnt),32);
    bufp->fullCData(oldp+14,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_pattern),8);
    bufp->fullCData(oldp+15,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_data),8);
    bufp->fullBit(oldp+16,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__recv_buf_valid));
    bufp->fullSData(oldp+17,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_pattern),10);
    bufp->fullCData(oldp+18,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_bitcnt),4);
    bufp->fullIData(oldp+19,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_divcnt),32);
    bufp->fullBit(oldp+20,(vlSelf->uart_handshake_test__DOT__u_uart__DOT__send_dummy));
    bufp->fullBit(oldp+21,(vlSelf->clk));
    bufp->fullBit(oldp+22,(vlSelf->uart_tx));
}
