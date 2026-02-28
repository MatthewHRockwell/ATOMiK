// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vsim_soc_top.h for the primary calling header

#include "Vsim_soc_top__pch.h"
#include "Vsim_soc_top___024root.h"

VL_ATTR_COLD void Vsim_soc_top___024root___eval_static(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_static\n"); );
}

VL_ATTR_COLD void Vsim_soc_top___024root___eval_initial__TOP(Vsim_soc_top___024root* vlSelf);

VL_ATTR_COLD void Vsim_soc_top___024root___eval_initial(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_initial\n"); );
    // Body
    Vsim_soc_top___024root___eval_initial__TOP(vlSelf);
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = vlSelf->clk;
}

VL_ATTR_COLD void Vsim_soc_top___024root___eval_initial__TOP(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_initial__TOP\n"); );
    // Init
    IData/*31:0*/ sim_soc_top__DOT__unnamedblk1__DOT__i;
    sim_soc_top__DOT__unnamedblk1__DOT__i = 0;
    IData/*31:0*/ sim_soc_top__DOT__unnamedblk2__DOT__i;
    sim_soc_top__DOT__unnamedblk2__DOT__i = 0;
    IData/*31:0*/ sim_soc_top__DOT__unnamedblk3__DOT__i;
    sim_soc_top__DOT__unnamedblk3__DOT__i = 0;
    IData/*31:0*/ sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__i;
    sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__i = 0;
    VlWide<4>/*127:0*/ __Vtemp_2;
    VlWide<4>/*127:0*/ __Vtemp_5;
    // Body
    sim_soc_top__DOT__unnamedblk1__DOT__i = 0U;
    while (VL_GTS_III(32, 0x4000U, sim_soc_top__DOT__unnamedblk1__DOT__i)) {
        vlSelf->sim_soc_top__DOT__flash_mem[(0x3fffU 
                                             & sim_soc_top__DOT__unnamedblk1__DOT__i)] = 0U;
        sim_soc_top__DOT__unnamedblk1__DOT__i = ((IData)(1U) 
                                                 + sim_soc_top__DOT__unnamedblk1__DOT__i);
    }
    __Vtemp_2[0U] = 0x65783332U;
    __Vtemp_2[1U] = 0x73682e68U;
    __Vtemp_2[2U] = 0x2d666c61U;
    __Vtemp_2[3U] = 0x6677U;
    VL_READMEM_N(true, 32, 16384, 0, VL_CVT_PACK_STR_NW(4, __Vtemp_2)
                 ,  &(vlSelf->sim_soc_top__DOT__flash_mem)
                 , 0, ~0ULL);
    sim_soc_top__DOT__unnamedblk2__DOT__i = 0U;
    while (VL_GTS_III(32, 0x800U, sim_soc_top__DOT__unnamedblk2__DOT__i)) {
        vlSelf->sim_soc_top__DOT__sram_mem[(0x7ffU 
                                            & sim_soc_top__DOT__unnamedblk2__DOT__i)] = 0U;
        sim_soc_top__DOT__unnamedblk2__DOT__i = ((IData)(1U) 
                                                 + sim_soc_top__DOT__unnamedblk2__DOT__i);
    }
    sim_soc_top__DOT__unnamedblk3__DOT__i = 0U;
    while (VL_GTS_III(32, 0x800U, sim_soc_top__DOT__unnamedblk3__DOT__i)) {
        vlSelf->sim_soc_top__DOT__brom_mem[(0x7ffU 
                                            & sim_soc_top__DOT__unnamedblk3__DOT__i)] = 0U;
        sim_soc_top__DOT__unnamedblk3__DOT__i = ((IData)(1U) 
                                                 + sim_soc_top__DOT__unnamedblk3__DOT__i);
    }
    __Vtemp_5[0U] = 0x65783332U;
    __Vtemp_5[1U] = 0x6f6d2e68U;
    __Vtemp_5[2U] = 0x772d6272U;
    __Vtemp_5[3U] = 0x66U;
    VL_READMEM_N(true, 32, 2048, 0, VL_CVT_PACK_STR_NW(4, __Vtemp_5)
                 ,  &(vlSelf->sim_soc_top__DOT__brom_mem)
                 , 0, ~0ULL);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[1U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[1U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[2U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[2U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[3U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[3U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[4U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[4U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[5U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[5U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[6U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[6U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[7U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[7U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[8U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[8U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[9U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[9U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0xaU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0xaU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0xbU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0xbU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0xcU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0xcU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0xdU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0xdU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0xeU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0xeU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0xfU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0xfU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x10U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x10U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x11U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x11U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x12U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x12U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x13U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x13U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x14U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x14U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x15U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x15U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x16U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x16U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x17U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x17U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x18U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x18U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x19U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x19U] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x1aU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x1aU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x1bU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x1bU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x1cU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x1cU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x1dU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x1dU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x1eU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x1eU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[0x1fU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[0x1fU] = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs1_data_reg = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs2_data_reg = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator = 0ULL;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__active_addr = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_read_reg = 0ULL;
    sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__i = 0U;
    while (VL_GTS_III(32, 0x100U, sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__i)) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table[(0xffU 
                                                                          & sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__i)] = 0ULL;
        sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__i 
            = ((IData)(1U) + sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__i);
    }
}

VL_ATTR_COLD void Vsim_soc_top___024root___eval_final(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_final\n"); );
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vsim_soc_top___024root___dump_triggers__stl(Vsim_soc_top___024root* vlSelf);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vsim_soc_top___024root___eval_phase__stl(Vsim_soc_top___024root* vlSelf);

VL_ATTR_COLD void Vsim_soc_top___024root___eval_settle(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_settle\n"); );
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
            Vsim_soc_top___024root___dump_triggers__stl(vlSelf);
#endif
            VL_FATAL_MT("sim_soc_top.v", 21, "", "Settle region did not converge.");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
        __VstlContinue = 0U;
        if (Vsim_soc_top___024root___eval_phase__stl(vlSelf)) {
            __VstlContinue = 1U;
        }
        vlSelf->__VstlFirstIteration = 0U;
    }
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vsim_soc_top___024root___dump_triggers__stl(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VstlTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

extern const VlUnpacked<CData/*2:0*/, 128> Vsim_soc_top__ConstPool__TABLE_h1cbca306_0;
extern const VlUnpacked<CData/*1:0*/, 32> Vsim_soc_top__ConstPool__TABLE_hdcc6d3b5_0;

VL_ATTR_COLD void Vsim_soc_top___024root___stl_sequent__TOP__0(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___stl_sequent__TOP__0\n"); );
    // Init
    CData/*0:0*/ sim_soc_top__DOT__mem_ready;
    sim_soc_top__DOT__mem_ready = 0;
    CData/*0:0*/ sim_soc_top__DOT__spicfg_sel;
    sim_soc_top__DOT__spicfg_sel = 0;
    CData/*0:0*/ sim_soc_top__DOT__uart_sel;
    sim_soc_top__DOT__uart_sel = 0;
    CData/*0:0*/ sim_soc_top__DOT____Vcellinp__u_uart__mem_s_valid;
    sim_soc_top__DOT____Vcellinp__u_uart__mem_s_valid = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__fetch_start;
    sim_soc_top__DOT__u_cpu__DOT__fetch_start = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_imm;
    sim_soc_top__DOT__u_cpu__DOT__dec_imm = 0;
    CData/*4:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_alu_op;
    sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm;
    sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_word_op;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_word_op = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_lui;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_lui = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_auipc;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_auipc = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_jal;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_jal = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_jalr;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_jalr = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_branch;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_branch = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_load;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_load = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_reg;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_reg = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_imm;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_imm = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_fence;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_fence = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__alu_operand_a;
    sim_soc_top__DOT__u_cpu__DOT__alu_operand_a = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__alu_operand_b;
    sim_soc_top__DOT__u_cpu__DOT__alu_operand_b = 0;
    IData/*31:0*/ sim_soc_top__DOT__u_cpu__DOT__lsu_bus_addr;
    sim_soc_top__DOT__u_cpu__DOT__lsu_bus_addr = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__csr_rdata;
    sim_soc_top__DOT__u_cpu__DOT__csr_rdata = 0;
    CData/*1:0*/ sim_soc_top__DOT__u_cpu__DOT__pc_src;
    sim_soc_top__DOT__u_cpu__DOT__pc_src = 0;
    CData/*2:0*/ sim_soc_top__DOT__u_cpu__DOT__wb_src;
    sim_soc_top__DOT__u_cpu__DOT__wb_src = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__csr_wdata_mux;
    sim_soc_top__DOT__u_cpu__DOT__csr_wdata_mux = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_i;
    sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_i = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_u;
    sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_u = 0;
    CData/*5:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shamt;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shamt = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__fill;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__fill = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s5;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s5 = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s4;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s4 = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s3;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s3 = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s2;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s2 = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s1;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s1 = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_result;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_result = 0;
    QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__alu_raw;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__alu_raw = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc3b440c0__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc3b440c0__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc0736949__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc0736949__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc186871b__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc186871b__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hca551dc0__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hca551dc0__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1866147__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1866147__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hdfb8392e__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hdfb8392e__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc198d0b4__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc198d0b4__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4cf6b8c3__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4cf6b8c3__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1242405__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1242405__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hff142eb0__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hff142eb0__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf2a1867c__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf2a1867c__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hfff7f07b__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hfff7f07b__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he25a5f9b__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he25a5f9b__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h10b9c95c__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h10b9c95c__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h11680cc4__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h11680cc4__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hef246f29__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hef246f29__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc2d88330__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc2d88330__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf4e5cffb__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf4e5cffb__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0965b10f__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0965b10f__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0b737f26__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0b737f26__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4483ee30__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4483ee30__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h40d56b5d__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h40d56b5d__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h3bd48df5__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h3bd48df5__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0dfe3846__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0dfe3846__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed14d33c__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed14d33c__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h311a5427__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h311a5427__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he18f614b__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he18f614b__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc5dcee02__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc5dcee02__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc50b0c2f__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc50b0c2f__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he9945308__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he9945308__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he970a5a8__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he970a5a8__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed411969__0;
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed411969__0 = 0;
    IData/*31:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata;
    sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_mret;
    sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_mret = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_csr_op;
    sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_csr_op = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgExtracted_hc8f4ae46__0;
    sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgExtracted_hc8f4ae46__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgTmp_h1487415d__0;
    sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgTmp_h1487415d__0 = 0;
    CData/*0:0*/ sim_soc_top__DOT__u_uart__DOT__reg_div_sel;
    sim_soc_top__DOT__u_uart__DOT__reg_div_sel = 0;
    CData/*4:0*/ __Vtableidx1;
    __Vtableidx1 = 0;
    CData/*6:0*/ __Vtableidx2;
    __Vtableidx2 = 0;
    // Body
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc_wen = 0U;
    vlSelf->ser_tx = vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_shift_reg;
    vlSelf->gpio_out = (0x7fU & vlSelf->sim_soc_top__DOT__gpio_out_r);
    if ((0U == (0x1fU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                         >> 0xfU)))) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT____Vcellinp__u_atomik__addr_in = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data = 0ULL;
    } else {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT____Vcellinp__u_atomik__addr_in 
            = (0xffU & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs1_data_reg));
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data 
            = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs1_data_reg;
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata = 0U;
    sim_soc_top__DOT__u_cpu__DOT__fetch_start = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb = 0U;
    sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata 
        = VL_SHIFTR_III(32,32,32, vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata, 
                        VL_SHIFTL_III(32,32,32, (3U 
                                                 & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr)), 3U));
    sim_soc_top__DOT__u_cpu__DOT__csr_rdata = ((0x300U 
                                                == 
                                                (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                 >> 0x14U))
                                                ? (QData)((IData)(
                                                                  (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpp) 
                                                                    << 0xbU) 
                                                                   | (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpie) 
                                                                       << 7U) 
                                                                      | ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie) 
                                                                         << 3U)))))
                                                : (
                                                   (0x301U 
                                                    == 
                                                    (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                     >> 0x14U))
                                                    ? 0x8000000000000100ULL
                                                    : 
                                                   ((0x305U 
                                                     == 
                                                     (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                      >> 0x14U))
                                                     ? vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mtvec
                                                     : 
                                                    ((0x340U 
                                                      == 
                                                      (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                       >> 0x14U))
                                                      ? vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mscratch
                                                      : 
                                                     ((0x341U 
                                                       == 
                                                       (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                        >> 0x14U))
                                                       ? vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mepc
                                                       : 
                                                      ((0x342U 
                                                        == 
                                                        (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                         >> 0x14U))
                                                        ? vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mcause
                                                        : 0ULL))))));
    sim_soc_top__DOT__u_cpu__DOT__dec_is_fence = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0 = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_imm = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_reg = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_jalr = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_lui = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_valid = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_system = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 0U;
    sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_u 
        = (((QData)((IData)((- (IData)((vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                        >> 0x1fU))))) 
            << 0x20U) | (QData)((IData)((0xfffff000U 
                                         & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))));
    sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_i 
        = (((- (QData)((IData)((vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                >> 0x1fU)))) << 0xcU) 
           | (QData)((IData)((vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 0x14U))));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data 
        = ((0U == (0x1fU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                            >> 0x14U))) ? 0ULL : vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs2_data_reg);
    sim_soc_top__DOT__u_cpu__DOT__lsu_bus_addr = 0U;
    if ((1U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state))) {
        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_is_store) {
            if ((4U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                    = (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata);
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb = 0xfU;
            } else if ((2U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                    = ((1U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))
                        ? (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata)
                        : (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata));
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb = 0xfU;
            } else if ((1U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                    = (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata) 
                        << 0x10U) | (0xffffU & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata)));
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb 
                    = (0xfU & ((IData)(3U) << (3U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr))));
            } else {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                    = (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata) 
                        << 0x18U) | ((0xff0000U & ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata) 
                                                   << 0x10U)) 
                                     | ((0xff00U & 
                                         ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata) 
                                          << 8U)) | 
                                        (0xffU & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata)))));
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb 
                    = (0xfU & ((IData)(1U) << (3U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr))));
            }
        }
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_valid = 1U;
        sim_soc_top__DOT__u_cpu__DOT__lsu_bus_addr 
            = (0xfffffffcU & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr));
    } else if ((2U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state))) {
        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_is_store) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                = (IData)((vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata 
                           >> 0x20U));
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb = 0xfU;
        }
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_valid = 1U;
        sim_soc_top__DOT__u_cpu__DOT__lsu_bus_addr 
            = (0xfffffffcU & ((IData)(4U) + (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr)));
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_store = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_load = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_auipc = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_branch = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_jal = 0U;
    sim_soc_top__DOT__u_cpu__DOT__dec_is_word_op = 0U;
    if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                  >> 6U)))) {
        if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                      >> 5U)))) {
            if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                          >> 4U)))) {
                if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_fence = 1U;
                            }
                        }
                    }
                    if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                  >> 2U)))) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0 = 1U;
                            }
                        }
                    }
                }
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 3U)))) {
                    if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                  >> 2U)))) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_load = 1U;
                            }
                        }
                    }
                }
            }
            if ((0x10U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                  >> 2U)))) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_imm = 1U;
                            }
                        }
                    }
                } else if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                     >> 2U)))) {
                    if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_imm = 1U;
                        }
                    }
                }
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 3U)))) {
                    if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_auipc = 1U;
                            }
                        }
                    }
                }
            }
        }
        if ((0x20U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((0x10U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                  >> 2U)))) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_reg = 1U;
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_word_op = 1U;
                            }
                        }
                    }
                } else if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                     >> 2U)))) {
                    if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_reg = 1U;
                        }
                    }
                }
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 3U)))) {
                    if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_lui = 1U;
                            }
                        }
                    }
                }
            }
            if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                          >> 4U)))) {
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 3U)))) {
                    if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                  >> 2U)))) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_store = 1U;
                            }
                        }
                    }
                }
            }
        } else if ((0x10U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 2U)))) {
                    if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            sim_soc_top__DOT__u_cpu__DOT__dec_is_word_op = 1U;
                        }
                    }
                }
            }
        }
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_load_en 
        = ((2U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state)) 
           & ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0) 
              & (0U == (0x7000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_swap_en 
        = ((2U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state)) 
           & ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0) 
              & (0x3000U == (0x7000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))));
    if ((0x40U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
        if ((0x20U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((0x10U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                        vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                    }
                } else {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 3U)))) {
                    if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                  >> 2U)))) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_system = 1U;
                            }
                        }
                    }
                }
            } else if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                            vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                        }
                    } else {
                        vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                    }
                } else {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
            } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                        vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                    }
                } else {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
            } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
            } else {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
            }
            if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                          >> 4U)))) {
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 3U)))) {
                    if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_jalr = 1U;
                            }
                        }
                    }
                }
            }
        } else {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
        }
    } else if ((0x20U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
        if ((0x10U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((0U != (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                          >> 0xcU)))) {
                            if ((1U != (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                              >> 0xcU)))) {
                                if ((5U != (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                  >> 0xcU)))) {
                                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                                }
                            }
                        }
                    } else {
                        vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                    }
                } else {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
            } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                        vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                    }
                } else {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
            } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
            } else {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
            }
        } else if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
        } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
        } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
            }
        } else {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
        }
    } else if ((0x10U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
        if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
            } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((0U != (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                      >> 0xcU)))) {
                        if ((1U != (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                          >> 0xcU)))) {
                            if ((5U != (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                              >> 0xcU)))) {
                                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                            }
                        }
                    }
                } else {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
            } else {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
            }
        } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
            } else {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
            }
        } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
            }
        } else {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
        }
    } else if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
        if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
                }
            } else {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
            }
        } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
            }
        } else {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
        }
    } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
    } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
        if ((1U & (~ vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
        }
    } else {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = 1U;
    }
    sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_csr_op 
        = ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_system) 
           & (0U != (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                           >> 0xcU))));
    sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgTmp_h1487415d__0 
        = ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_system) 
           & (0U == (0x7000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)));
    sim_soc_top__DOT__u_cpu__DOT__dec_imm = 0ULL;
    if ((0x40U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
        if ((0x20U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                          >> 4U)))) {
                if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 1U;
                                sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0U;
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_jal = 1U;
                            }
                        }
                    }
                } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 1U;
                            sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0U;
                        }
                    }
                } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 1U;
                        sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0U;
                    }
                }
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 3U)))) {
                    if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                  >> 2U)))) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_is_branch = 1U;
                            }
                        }
                    }
                }
            }
            if ((0x10U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 3U)))) {
                    if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                  >> 2U)))) {
                        if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                                    = sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_i;
                            }
                        }
                    }
                }
            } else if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                                = (((- (QData)((IData)(
                                                       (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                        >> 0x1fU)))) 
                                    << 0x15U) | (QData)((IData)(
                                                                ((0x100000U 
                                                                  & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                                     >> 0xbU)) 
                                                                 | ((0xff000U 
                                                                     & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr) 
                                                                    | ((0x800U 
                                                                        & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                                           >> 9U)) 
                                                                       | (0x7feU 
                                                                          & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                                             >> 0x14U))))))));
                        }
                    }
                }
            } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                            = sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_i;
                    }
                }
            } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                        = (((- (QData)((IData)((vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                >> 0x1fU)))) 
                            << 0xdU) | (QData)((IData)(
                                                       ((0x1000U 
                                                         & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                            >> 0x13U)) 
                                                        | ((0x800U 
                                                            & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                               << 4U)) 
                                                           | ((0x7e0U 
                                                               & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                                  >> 0x14U)) 
                                                              | (0x1eU 
                                                                 & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                                    >> 7U))))))));
                }
            }
        }
    } else if ((0x20U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
        if ((0x10U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                          >> 3U)))) {
                if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 1U;
                            sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                                = sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_u;
                        }
                    }
                }
            }
            if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                              >> 2U)))) {
                    if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                            if ((0U == (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                              >> 0xcU)))) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_alu_op 
                                    = ((0x40000000U 
                                        & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                        ? 1U : 0U);
                            } else if ((1U == (7U & 
                                               (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                >> 0xcU)))) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 2U;
                            } else if ((5U == (7U & 
                                               (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                >> 0xcU)))) {
                                sim_soc_top__DOT__u_cpu__DOT__dec_alu_op 
                                    = ((0x40000000U 
                                        & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                        ? 7U : 6U);
                            }
                        }
                    }
                }
            } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0xaU;
                    }
                }
            } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    sim_soc_top__DOT__u_cpu__DOT__dec_alu_op 
                        = ((0x4000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                            ? ((0x2000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                ? ((0x1000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                    ? 9U : 8U) : ((0x1000U 
                                                   & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                                   ? 
                                                  ((0x40000000U 
                                                    & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                                    ? 7U
                                                    : 6U)
                                                   : 5U))
                            : ((0x2000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                ? ((0x1000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                    ? 4U : 3U) : ((0x1000U 
                                                   & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                                   ? 2U
                                                   : 
                                                  ((0x40000000U 
                                                    & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                                    ? 1U
                                                    : 0U))));
                }
            }
        } else if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                             >> 3U)))) {
            if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                          >> 2U)))) {
                if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 1U;
                        sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0U;
                        sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                            = (((- (QData)((IData)(
                                                   (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                    >> 0x1fU)))) 
                                << 0xcU) | (QData)((IData)(
                                                           ((0xfe0U 
                                                             & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                                >> 0x14U)) 
                                                            | (0x1fU 
                                                               & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                                  >> 7U))))));
                    }
                }
            }
        }
    } else if ((0x10U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
        if ((8U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                          >> 2U)))) {
                if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                        sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 1U;
                        if ((0U == (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                          >> 0xcU)))) {
                            sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0U;
                        } else if ((1U == (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                 >> 0xcU)))) {
                            sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 2U;
                        } else if ((5U == (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                 >> 0xcU)))) {
                            sim_soc_top__DOT__u_cpu__DOT__dec_alu_op 
                                = ((0x40000000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                    ? 7U : 6U);
                        }
                        sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                            = sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_i;
                    }
                }
            }
        } else if ((4U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 1U;
                    sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0U;
                    sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                        = sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_u;
                }
            }
        } else if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
            if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 1U;
                sim_soc_top__DOT__u_cpu__DOT__dec_alu_op 
                    = ((0x4000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                        ? ((0x2000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                            ? ((0x1000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                ? 9U : 8U) : ((0x1000U 
                                               & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                               ? ((0x40000000U 
                                                   & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                                   ? 7U
                                                   : 6U)
                                               : 5U))
                        : ((0x2000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                            ? ((0x1000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                ? 4U : 3U) : ((0x1000U 
                                               & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                               ? 2U
                                               : 0U)));
                sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                    = sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_i;
            }
        }
    } else if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                         >> 3U)))) {
        if ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                      >> 2U)))) {
            if ((2U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                if ((1U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)) {
                    sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm = 1U;
                    sim_soc_top__DOT__u_cpu__DOT__dec_alu_op = 0U;
                    sim_soc_top__DOT__u_cpu__DOT__dec_imm 
                        = sim_soc_top__DOT__u_cpu__DOT__u_decode__DOT__imm_i;
                }
            }
        }
    }
    __Vtableidx2 = (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__fetch_done) 
                     << 6U) | (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_store) 
                                << 5U) | (((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_load) 
                                           << 4U) | 
                                          (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_done) 
                                            << 3U) 
                                           | (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state)))));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state_next 
        = Vsim_soc_top__ConstPool__TABLE_h1cbca306_0
        [__Vtableidx2];
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_start = 0U;
    if ((1U & (~ ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state) 
                  >> 2U)))) {
        if ((1U & (~ ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state) 
                      >> 1U)))) {
            if ((1U & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state)))) {
                sim_soc_top__DOT__u_cpu__DOT__fetch_start 
                    = (1U & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__fetch_done)));
            }
        }
        if ((2U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state))) {
            if ((1U & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state)))) {
                if (((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_load) 
                     | (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_store))) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_start = 1U;
                }
            }
        }
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_has_bus 
        = ((3U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state)) 
           | ((2U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state)) 
              & ((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_load) 
                 | (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_store))));
    sim_soc_top__DOT__u_cpu__DOT__csr_wdata_mux = (
                                                   (0x4000U 
                                                    & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                                    ? (QData)((IData)(
                                                                      (0x1fU 
                                                                       & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                                          >> 0xfU))))
                                                    : vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data);
    sim_soc_top__DOT__u_cpu__DOT__alu_operand_a = (
                                                   ((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_auipc) 
                                                    | ((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_branch) 
                                                       | (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_jal)))
                                                    ? vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc
                                                    : vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__csr_wen = 0U;
    sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgExtracted_hc8f4ae46__0 
        = ((IData)(sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgTmp_h1487415d__0) 
           & ((0U == (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                      >> 0x14U)) | (1U == (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                           >> 0x14U))));
    sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_mret 
        = ((IData)(sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgTmp_h1487415d__0) 
           & (0x302U == (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                         >> 0x14U)));
    sim_soc_top__DOT__u_cpu__DOT__alu_operand_b = ((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_src_b_imm)
                                                    ? sim_soc_top__DOT__u_cpu__DOT__dec_imm
                                                    : vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data);
    if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_has_bus) {
        vlSelf->sim_soc_top__DOT__mem_wstrb = vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb;
        vlSelf->sim_soc_top__DOT__mem_valid = vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_valid;
        vlSelf->sim_soc_top__DOT__mem_addr = sim_soc_top__DOT__u_cpu__DOT__lsu_bus_addr;
    } else {
        vlSelf->sim_soc_top__DOT__mem_wstrb = 0U;
        vlSelf->sim_soc_top__DOT__mem_valid = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate;
        vlSelf->sim_soc_top__DOT__mem_addr = (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc);
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val 
        = ((0x2000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
            ? ((0x1000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                ? (sim_soc_top__DOT__u_cpu__DOT__csr_rdata 
                   & (~ sim_soc_top__DOT__u_cpu__DOT__csr_wdata_mux))
                : (sim_soc_top__DOT__u_cpu__DOT__csr_rdata 
                   | sim_soc_top__DOT__u_cpu__DOT__csr_wdata_mux))
            : ((0x1000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                ? sim_soc_top__DOT__u_cpu__DOT__csr_wdata_mux
                : sim_soc_top__DOT__u_cpu__DOT__csr_rdata));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__regfile_wen = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_enter = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_return = 0U;
    sim_soc_top__DOT__u_cpu__DOT__pc_src = 0U;
    sim_soc_top__DOT__u_cpu__DOT__wb_src = 0U;
    if ((4U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state))) {
        if ((1U & (~ ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state) 
                      >> 1U)))) {
            if ((1U & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state)))) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc_wen = 1U;
                if (((((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_lui) 
                       | (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_auipc)) 
                      | (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_reg)) 
                     | (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_imm))) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__regfile_wen = 1U;
                    sim_soc_top__DOT__u_cpu__DOT__wb_src = 0U;
                } else if (sim_soc_top__DOT__u_cpu__DOT__dec_is_load) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__regfile_wen = 1U;
                    sim_soc_top__DOT__u_cpu__DOT__wb_src = 1U;
                } else if ((1U & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_store)))) {
                    if (sim_soc_top__DOT__u_cpu__DOT__dec_is_jal) {
                        vlSelf->sim_soc_top__DOT__u_cpu__DOT__regfile_wen = 1U;
                        sim_soc_top__DOT__u_cpu__DOT__wb_src = 2U;
                    } else if (sim_soc_top__DOT__u_cpu__DOT__dec_is_jalr) {
                        vlSelf->sim_soc_top__DOT__u_cpu__DOT__regfile_wen = 1U;
                        sim_soc_top__DOT__u_cpu__DOT__wb_src = 2U;
                    } else if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_branch)))) {
                        if (sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_csr_op) {
                            vlSelf->sim_soc_top__DOT__u_cpu__DOT__regfile_wen = 1U;
                            sim_soc_top__DOT__u_cpu__DOT__wb_src = 3U;
                        } else if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgExtracted_hc8f4ae46__0)))) {
                            if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_mret)))) {
                                if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0) {
                                    if (((2U == (7U 
                                                 & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                    >> 0xcU))) 
                                         | (3U == (7U 
                                                   & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                      >> 0xcU))))) {
                                        vlSelf->sim_soc_top__DOT__u_cpu__DOT__regfile_wen = 1U;
                                        sim_soc_top__DOT__u_cpu__DOT__wb_src = 4U;
                                    }
                                }
                            }
                        }
                    }
                }
                sim_soc_top__DOT__u_cpu__DOT__pc_src = 0U;
                if ((1U & (~ ((((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_lui) 
                                | (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_auipc)) 
                               | (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_reg)) 
                              | (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_alu_imm))))) {
                    if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_load)))) {
                        if ((1U & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_store)))) {
                            if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_jal)))) {
                                if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_jalr)))) {
                                    if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_branch)))) {
                                        if (sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_csr_op) {
                                            vlSelf->sim_soc_top__DOT__u_cpu__DOT__csr_wen = 1U;
                                        }
                                        if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_csr_op)))) {
                                            if (sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgExtracted_hc8f4ae46__0) {
                                                vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_enter = 1U;
                                            } else if (
                                                       (1U 
                                                        & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_mret)))) {
                                                if (
                                                    (1U 
                                                     & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0)))) {
                                                    if (
                                                        (1U 
                                                         & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_fence)))) {
                                                        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal) {
                                                            vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_enter = 1U;
                                                        }
                                                    }
                                                }
                                            }
                                            if ((1U 
                                                 & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgExtracted_hc8f4ae46__0)))) {
                                                if (sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_mret) {
                                                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_return = 1U;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if (sim_soc_top__DOT__u_cpu__DOT__dec_is_jal) {
                                sim_soc_top__DOT__u_cpu__DOT__pc_src = 1U;
                            } else if (sim_soc_top__DOT__u_cpu__DOT__dec_is_jalr) {
                                sim_soc_top__DOT__u_cpu__DOT__pc_src = 2U;
                            } else if (sim_soc_top__DOT__u_cpu__DOT__dec_is_branch) {
                                if (((0x4000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                      ? ((0x2000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                          ? ((0x1000U 
                                              & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                              ? (vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data 
                                                 >= vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data)
                                              : (vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data 
                                                 < vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data))
                                          : ((0x1000U 
                                              & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                              ? VL_GTES_IQQ(64, vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data, vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data)
                                              : VL_LTS_IQQ(64, vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data, vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data)))
                                      : ((1U & (~ (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                                   >> 0xdU))) 
                                         && ((0x1000U 
                                              & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr)
                                              ? (vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data 
                                                 != vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data)
                                              : (vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data 
                                                 == vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data))))) {
                                    sim_soc_top__DOT__u_cpu__DOT__pc_src = 1U;
                                }
                            } else if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_csr_op)))) {
                                if (sim_soc_top__DOT__u_cpu__DOT__u_control__DOT____VdfgExtracted_hc8f4ae46__0) {
                                    sim_soc_top__DOT__u_cpu__DOT__pc_src = 3U;
                                } else if (sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__is_mret) {
                                    sim_soc_top__DOT__u_cpu__DOT__pc_src = 3U;
                                } else if ((1U & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0)))) {
                                    if ((1U & (~ (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_fence)))) {
                                        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal) {
                                            sim_soc_top__DOT__u_cpu__DOT__pc_src = 3U;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    vlSelf->sim_soc_top__DOT__brom_sel = (IData)((0x80000000U 
                                                  == 
                                                  (0xcf000000U 
                                                   & vlSelf->sim_soc_top__DOT__mem_addr)));
    vlSelf->sim_soc_top__DOT__gpio_sel = (IData)((0x82000000U 
                                                  == 
                                                  (0xcf000000U 
                                                   & vlSelf->sim_soc_top__DOT__mem_addr)));
    sim_soc_top__DOT__spicfg_sel = (IData)((0x81000000U 
                                            == (0xcf000000U 
                                                & vlSelf->sim_soc_top__DOT__mem_addr)));
    sim_soc_top__DOT__uart_sel = (IData)((0x83000000U 
                                          == (0xcf000000U 
                                              & vlSelf->sim_soc_top__DOT__mem_addr)));
    if (sim_soc_top__DOT__u_cpu__DOT__dec_is_word_op) {
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
            = (QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__alu_operand_a));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
            = (((QData)((IData)((- (IData)((1U & (IData)(
                                                         (sim_soc_top__DOT__u_cpu__DOT__alu_operand_a 
                                                          >> 0x1fU))))))) 
                << 0x20U) | (QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__alu_operand_a)));
    } else {
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
            = sim_soc_top__DOT__u_cpu__DOT__alu_operand_a;
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
            = sim_soc_top__DOT__u_cpu__DOT__alu_operand_a;
    }
    if ((2U == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))) {
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc0736949__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 1U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc186871b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 2U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hca551dc0__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 3U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1866147__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 4U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hdfb8392e__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 5U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc198d0b4__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 6U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4cf6b8c3__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 7U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1242405__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 8U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hff142eb0__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 9U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf2a1867c__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0xaU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hfff7f07b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0xbU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he25a5f9b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0xcU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h10b9c95c__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0xdU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h11680cc4__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0xeU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hef246f29__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0xfU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc2d88330__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x10U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf4e5cffb__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x11U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0965b10f__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x12U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0b737f26__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x13U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4483ee30__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x14U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h40d56b5d__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x15U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h3bd48df5__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x16U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0dfe3846__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x17U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed14d33c__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x18U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h311a5427__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x19U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he18f614b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x1aU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc5dcee02__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x1bU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc50b0c2f__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x1cU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he9945308__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x1dU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he970a5a8__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x1eU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed411969__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x1fU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc3b440c0__0 
            = (1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base));
    } else if ((7U == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))) {
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc0736949__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x3eU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc186871b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x3dU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hca551dc0__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x3cU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1866147__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x3bU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hdfb8392e__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x3aU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc198d0b4__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x39U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4cf6b8c3__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x38U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1242405__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x37U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hff142eb0__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x36U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf2a1867c__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x35U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hfff7f07b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x34U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he25a5f9b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x33U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h10b9c95c__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x32U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h11680cc4__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x31U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hef246f29__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x30U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc2d88330__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x2fU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf4e5cffb__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x2eU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0965b10f__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x2dU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0b737f26__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x2cU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4483ee30__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x2bU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h40d56b5d__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x2aU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h3bd48df5__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x29U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0dfe3846__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x28U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed14d33c__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x27U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h311a5427__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x26U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he18f614b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x25U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc5dcee02__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x24U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc50b0c2f__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x23U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he9945308__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x22U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he970a5a8__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x21U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed411969__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x20U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc3b440c0__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                             >> 0x3fU)));
    } else {
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc0736949__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x3eU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc186871b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x3dU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hca551dc0__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x3cU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1866147__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x3bU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hdfb8392e__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x3aU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc198d0b4__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x39U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4cf6b8c3__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x38U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1242405__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x37U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hff142eb0__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x36U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf2a1867c__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x35U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hfff7f07b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x34U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he25a5f9b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x33U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h10b9c95c__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x32U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h11680cc4__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x31U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hef246f29__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x30U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc2d88330__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x2fU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf4e5cffb__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x2eU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0965b10f__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x2dU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0b737f26__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x2cU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4483ee30__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x2bU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h40d56b5d__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x2aU)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h3bd48df5__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x29U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0dfe3846__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x28U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed14d33c__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x27U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h311a5427__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x26U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he18f614b__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x25U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc5dcee02__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x24U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc50b0c2f__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x23U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he9945308__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x22U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he970a5a8__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x21U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed411969__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x20U)));
        sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc3b440c0__0 
            = (1U & (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                             >> 0x3fU)));
    }
    sim_soc_top__DOT____Vcellinp__u_uart__mem_s_valid 
        = ((IData)(vlSelf->sim_soc_top__DOT__mem_valid) 
           & (IData)(sim_soc_top__DOT__uart_sel));
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__fill 
        = ((7U == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op)) 
           & (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc3b440c0__0));
    vlSelf->sim_soc_top__DOT__u_uart__DOT__reg_dat_sel 
        = ((~ (vlSelf->sim_soc_top__DOT__mem_addr >> 2U)) 
           & (IData)(sim_soc_top__DOT____Vcellinp__u_uart__mem_s_valid));
    sim_soc_top__DOT__u_uart__DOT__reg_div_sel = ((IData)(sim_soc_top__DOT____Vcellinp__u_uart__mem_s_valid) 
                                                  & (vlSelf->sim_soc_top__DOT__mem_addr 
                                                     >> 2U));
    if ((0U == (vlSelf->sim_soc_top__DOT__mem_addr 
                >> 0x1eU))) {
        vlSelf->sim_soc_top__DOT__mem_rdata = vlSelf->sim_soc_top__DOT__flash_rdata;
        sim_soc_top__DOT__mem_ready = vlSelf->sim_soc_top__DOT__flash_ready;
    } else if ((1U == (vlSelf->sim_soc_top__DOT__mem_addr 
                       >> 0x1eU))) {
        vlSelf->sim_soc_top__DOT__mem_rdata = vlSelf->sim_soc_top__DOT__sram_rdata;
        sim_soc_top__DOT__mem_ready = vlSelf->sim_soc_top__DOT__sram_ready;
    } else if (vlSelf->sim_soc_top__DOT__brom_sel) {
        vlSelf->sim_soc_top__DOT__mem_rdata = vlSelf->sim_soc_top__DOT__brom_rdata;
        sim_soc_top__DOT__mem_ready = vlSelf->sim_soc_top__DOT__brom_ready;
    } else if (sim_soc_top__DOT__spicfg_sel) {
        vlSelf->sim_soc_top__DOT__mem_rdata = 0U;
        sim_soc_top__DOT__mem_ready = ((IData)(vlSelf->sim_soc_top__DOT__mem_valid) 
                                       & (IData)(sim_soc_top__DOT__spicfg_sel));
    } else if (vlSelf->sim_soc_top__DOT__gpio_sel) {
        vlSelf->sim_soc_top__DOT__mem_rdata = vlSelf->sim_soc_top__DOT__gpio_rdata;
        sim_soc_top__DOT__mem_ready = vlSelf->sim_soc_top__DOT__gpio_ready;
    } else if (sim_soc_top__DOT__uart_sel) {
        vlSelf->sim_soc_top__DOT__mem_rdata = ((4U 
                                                & vlSelf->sim_soc_top__DOT__mem_addr)
                                                ? vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider
                                                : 0xffffffffU);
        sim_soc_top__DOT__mem_ready = ((IData)(sim_soc_top__DOT__u_uart__DOT__reg_div_sel) 
                                       | ((~ (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_active)) 
                                          & (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__reg_dat_sel)));
    } else {
        vlSelf->sim_soc_top__DOT__mem_rdata = 0U;
        sim_soc_top__DOT__mem_ready = (IData)(((0xc0000000U 
                                                == 
                                                (0xc0000000U 
                                                 & vlSelf->sim_soc_top__DOT__mem_addr)) 
                                               & (IData)(vlSelf->sim_soc_top__DOT__mem_valid)));
    }
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shamt 
        = (0x3fU & ((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_word_op)
                     ? (0x1fU & (IData)(sim_soc_top__DOT__u_cpu__DOT__alu_operand_b))
                     : (IData)(sim_soc_top__DOT__u_cpu__DOT__alu_operand_b)));
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s5 = 
        ((0x20U & (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shamt))
          ? (((QData)((IData)((- (IData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__fill))))) 
              << 0x20U) | (QData)((IData)((((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc3b440c0__0) 
                                            << 0x1fU) 
                                           | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc0736949__0) 
                                               << 0x1eU) 
                                              | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc186871b__0) 
                                                  << 0x1dU) 
                                                 | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hca551dc0__0) 
                                                     << 0x1cU) 
                                                    | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1866147__0) 
                                                        << 0x1bU) 
                                                       | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hdfb8392e__0) 
                                                           << 0x1aU) 
                                                          | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc198d0b4__0) 
                                                              << 0x19U) 
                                                             | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4cf6b8c3__0) 
                                                                 << 0x18U) 
                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1242405__0) 
                                                                    << 0x17U) 
                                                                   | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hff142eb0__0) 
                                                                       << 0x16U) 
                                                                      | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf2a1867c__0) 
                                                                          << 0x15U) 
                                                                         | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hfff7f07b__0) 
                                                                             << 0x14U) 
                                                                            | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he25a5f9b__0) 
                                                                                << 0x13U) 
                                                                               | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h10b9c95c__0) 
                                                                                << 0x12U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h11680cc4__0) 
                                                                                << 0x11U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hef246f29__0) 
                                                                                << 0x10U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc2d88330__0) 
                                                                                << 0xfU) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf4e5cffb__0) 
                                                                                << 0xeU) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0965b10f__0) 
                                                                                << 0xdU) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0b737f26__0) 
                                                                                << 0xcU) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4483ee30__0) 
                                                                                << 0xbU) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h40d56b5d__0) 
                                                                                << 0xaU) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h3bd48df5__0) 
                                                                                << 9U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0dfe3846__0) 
                                                                                << 8U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed14d33c__0) 
                                                                                << 7U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h311a5427__0) 
                                                                                << 6U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he18f614b__0) 
                                                                                << 5U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc5dcee02__0) 
                                                                                << 4U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc50b0c2f__0) 
                                                                                << 3U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he9945308__0) 
                                                                                << 2U) 
                                                                                | (((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he970a5a8__0) 
                                                                                << 1U) 
                                                                                | (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed411969__0)))))))))))))))))))))))))))))))))))
          : (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc3b440c0__0)) 
              << 0x3fU) | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc0736949__0)) 
                            << 0x3eU) | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc186871b__0)) 
                                          << 0x3dU) 
                                         | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hca551dc0__0)) 
                                             << 0x3cU) 
                                            | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1866147__0)) 
                                                << 0x3bU) 
                                               | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hdfb8392e__0)) 
                                                   << 0x3aU) 
                                                  | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc198d0b4__0)) 
                                                      << 0x39U) 
                                                     | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4cf6b8c3__0)) 
                                                         << 0x38U) 
                                                        | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf1242405__0)) 
                                                            << 0x37U) 
                                                           | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hff142eb0__0)) 
                                                               << 0x36U) 
                                                              | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf2a1867c__0)) 
                                                                  << 0x35U) 
                                                                 | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hfff7f07b__0)) 
                                                                     << 0x34U) 
                                                                    | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he25a5f9b__0)) 
                                                                        << 0x33U) 
                                                                       | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h10b9c95c__0)) 
                                                                           << 0x32U) 
                                                                          | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h11680cc4__0)) 
                                                                              << 0x31U) 
                                                                             | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hef246f29__0)) 
                                                                                << 0x30U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc2d88330__0)) 
                                                                                << 0x2fU) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hf4e5cffb__0)) 
                                                                                << 0x2eU) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0965b10f__0)) 
                                                                                << 0x2dU) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0b737f26__0)) 
                                                                                << 0x2cU) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h4483ee30__0)) 
                                                                                << 0x2bU) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h40d56b5d__0)) 
                                                                                << 0x2aU) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h3bd48df5__0)) 
                                                                                << 0x29U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h0dfe3846__0)) 
                                                                                << 0x28U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed14d33c__0)) 
                                                                                << 0x27U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_h311a5427__0)) 
                                                                                << 0x26U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he18f614b__0)) 
                                                                                << 0x25U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc5dcee02__0)) 
                                                                                << 0x24U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hc50b0c2f__0)) 
                                                                                << 0x23U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he9945308__0)) 
                                                                                << 0x22U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_he970a5a8__0)) 
                                                                                << 0x21U) 
                                                                                | (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT____VdfgTmp_hed411969__0)) 
                                                                                << 0x20U) 
                                                                                | (QData)((IData)(
                                                                                ((((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x20U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x1fU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x1fU)))) 
                                                                                << 0x1fU) 
                                                                                | ((0x40000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x21U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x1eU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x1eU)))) 
                                                                                << 0x1eU)) 
                                                                                | ((0x20000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x22U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x1dU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x1dU)))) 
                                                                                << 0x1dU)) 
                                                                                | ((0x10000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x23U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x1cU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x1cU)))) 
                                                                                << 0x1cU)) 
                                                                                | ((0x8000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x24U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x1bU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x1bU)))) 
                                                                                << 0x1bU)) 
                                                                                | ((0x4000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x25U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x1aU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x1aU)))) 
                                                                                << 0x1aU)) 
                                                                                | ((0x2000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x26U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x19U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x19U)))) 
                                                                                << 0x19U)) 
                                                                                | ((0x1000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x27U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x18U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x18U)))) 
                                                                                << 0x18U)) 
                                                                                | ((0x800000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x28U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x17U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x17U)))) 
                                                                                << 0x17U)) 
                                                                                | ((0x400000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x29U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x16U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x16U)))) 
                                                                                << 0x16U)) 
                                                                                | ((0x200000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x2aU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x15U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x15U)))) 
                                                                                << 0x15U)) 
                                                                                | ((0x100000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x2bU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x14U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x14U)))) 
                                                                                << 0x14U)) 
                                                                                | ((0x80000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x2cU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x13U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x13U)))) 
                                                                                << 0x13U)) 
                                                                                | ((0x40000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x2dU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x12U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x12U)))) 
                                                                                << 0x12U)) 
                                                                                | ((0x20000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x2eU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x11U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x11U)))) 
                                                                                << 0x11U)) 
                                                                                | ((0x10000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x2fU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0x10U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x10U)))) 
                                                                                << 0x10U)) 
                                                                                | ((0x8000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x30U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0xfU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0xfU)))) 
                                                                                << 0xfU)) 
                                                                                | ((0x4000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x31U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0xeU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0xeU)))) 
                                                                                << 0xeU)) 
                                                                                | ((0x2000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x32U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0xdU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0xdU)))) 
                                                                                << 0xdU)) 
                                                                                | ((0x1000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x33U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0xcU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0xcU)))) 
                                                                                << 0xcU)) 
                                                                                | ((0x800U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x34U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0xbU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0xbU)))) 
                                                                                << 0xbU)) 
                                                                                | ((0x400U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x35U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 0xaU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0xaU)))) 
                                                                                << 0xaU)) 
                                                                                | ((0x200U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x36U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 9U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 9U)))) 
                                                                                << 9U)) 
                                                                                | ((0x100U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x37U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 8U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 8U)))) 
                                                                                << 8U)) 
                                                                                | ((0x80U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x38U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 7U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 7U)))) 
                                                                                << 7U)) 
                                                                                | ((0x40U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x39U))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 6U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 6U)))) 
                                                                                << 6U)) 
                                                                                | ((0x20U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x3aU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 5U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 5U)))) 
                                                                                << 5U)) 
                                                                                | ((0x10U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x3bU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 4U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 4U)))) 
                                                                                << 4U)) 
                                                                                | ((8U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x3cU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 3U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 3U)))) 
                                                                                << 3U)) 
                                                                                | ((4U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x3dU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 2U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 2U)))) 
                                                                                << 2U)) 
                                                                                | ((2U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x3eU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input 
                                                                                >> 1U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 1U)))) 
                                                                                << 1U)) 
                                                                                | (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base 
                                                                                >> 0x3fU))
                                                                                 : 
                                                                                ((7U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__sra_input)
                                                                                 : (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_base))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))));
    vlSelf->sim_soc_top__DOT__u_uart__DOT____Vcellinp__u_manual_uart_tx__reg_div_we 
        = ((- (IData)((IData)(sim_soc_top__DOT__u_uart__DOT__reg_div_sel))) 
           & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb));
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s4 = 
        ((0x10U & (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shamt))
          ? (((QData)((IData)((0xffffU & (- (IData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__fill)))))) 
              << 0x30U) | (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s5 
                           >> 0x10U)) : sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s5);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__fetch_bus_ready 
        = ((~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_has_bus)) 
           & (IData)(sim_soc_top__DOT__mem_ready));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_ready 
        = ((IData)(sim_soc_top__DOT__mem_ready) & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_has_bus));
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s3 = 
        ((8U & (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shamt))
          ? (((QData)((IData)((0xffU & (- (IData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__fill)))))) 
              << 0x38U) | (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s4 
                           >> 8U)) : sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s4);
    if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate_next = 1U;
        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate) {
            if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__fetch_bus_ready) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate_next = 0U;
            }
        } else {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate_next = 0U;
        }
    } else {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate_next = 0U;
        if (sim_soc_top__DOT__u_cpu__DOT__fetch_start) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate_next = 1U;
        }
    }
    __Vtableidx1 = (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_start) 
                     << 4U) | (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_needs_two) 
                                << 3U) | (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_ready) 
                                           << 2U) | (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state))));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state_next 
        = Vsim_soc_top__ConstPool__TABLE_hdcc6d3b5_0
        [__Vtableidx1];
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s2 = 
        ((4U & (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shamt))
          ? (((QData)((IData)((0xfU & (- (IData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__fill)))))) 
              << 0x3cU) | (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s3 
                           >> 4U)) : sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s3);
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s1 = 
        ((2U & (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shamt))
          ? (((QData)((IData)((3U & (- (IData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__fill)))))) 
              << 0x3eU) | (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s2 
                           >> 2U)) : sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s2);
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 = 
        ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shamt))
          ? (((QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__fill)) 
              << 0x3fU) | (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s1 
                           >> 1U)) : sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s1);
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_result 
        = (((QData)((IData)((1U & ((2U == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                    ? (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0)
                                    : (IData)((sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                               >> 0x3fU)))))) 
            << 0x3fU) | (((QData)((IData)((1U & ((2U 
                                                  == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                  ? (IData)(
                                                            (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                             >> 1U))
                                                  : (IData)(
                                                            (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                             >> 0x3eU)))))) 
                          << 0x3eU) | (((QData)((IData)(
                                                        (1U 
                                                         & ((2U 
                                                             == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                             ? (IData)(
                                                                       (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                        >> 2U))
                                                             : (IData)(
                                                                       (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                        >> 0x3dU)))))) 
                                        << 0x3dU) | 
                                       (((QData)((IData)(
                                                         (1U 
                                                          & ((2U 
                                                              == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                              ? (IData)(
                                                                        (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                         >> 3U))
                                                              : (IData)(
                                                                        (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                         >> 0x3cU)))))) 
                                         << 0x3cU) 
                                        | (((QData)((IData)(
                                                            (1U 
                                                             & ((2U 
                                                                 == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                 ? (IData)(
                                                                           (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                            >> 4U))
                                                                 : (IData)(
                                                                           (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                            >> 0x3bU)))))) 
                                            << 0x3bU) 
                                           | (((QData)((IData)(
                                                               (1U 
                                                                & ((2U 
                                                                    == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                    ? (IData)(
                                                                              (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                               >> 5U))
                                                                    : (IData)(
                                                                              (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                               >> 0x3aU)))))) 
                                               << 0x3aU) 
                                              | (((QData)((IData)(
                                                                  (1U 
                                                                   & ((2U 
                                                                       == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                       ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 6U))
                                                                       : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x39U)))))) 
                                                  << 0x39U) 
                                                 | (((QData)((IData)(
                                                                     (1U 
                                                                      & ((2U 
                                                                          == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                          ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 7U))
                                                                          : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x38U)))))) 
                                                     << 0x38U) 
                                                    | (((QData)((IData)(
                                                                        (1U 
                                                                         & ((2U 
                                                                             == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                             ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 8U))
                                                                             : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x37U)))))) 
                                                        << 0x37U) 
                                                       | (((QData)((IData)(
                                                                           (1U 
                                                                            & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 9U))
                                                                                : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x36U)))))) 
                                                           << 0x36U) 
                                                          | (((QData)((IData)(
                                                                              (1U 
                                                                               & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xaU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x35U)))))) 
                                                              << 0x35U) 
                                                             | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xbU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x34U)))))) 
                                                                 << 0x34U) 
                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xcU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x33U)))))) 
                                                                    << 0x33U) 
                                                                   | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xdU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x32U)))))) 
                                                                       << 0x32U) 
                                                                      | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xeU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x31U)))))) 
                                                                          << 0x31U) 
                                                                         | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xfU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x30U)))))) 
                                                                             << 0x30U) 
                                                                            | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x10U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2fU)))))) 
                                                                                << 0x2fU) 
                                                                               | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x11U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2eU)))))) 
                                                                                << 0x2eU) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x12U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2dU)))))) 
                                                                                << 0x2dU) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x13U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2cU)))))) 
                                                                                << 0x2cU) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x14U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2bU)))))) 
                                                                                << 0x2bU) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x15U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2aU)))))) 
                                                                                << 0x2aU) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x16U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x29U)))))) 
                                                                                << 0x29U) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x17U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x28U)))))) 
                                                                                << 0x28U) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x18U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x27U)))))) 
                                                                                << 0x27U) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x19U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x26U)))))) 
                                                                                << 0x26U) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1aU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x25U)))))) 
                                                                                << 0x25U) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1bU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x24U)))))) 
                                                                                << 0x24U) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1cU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x23U)))))) 
                                                                                << 0x23U) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1dU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x22U)))))) 
                                                                                << 0x22U) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1eU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x21U)))))) 
                                                                                << 0x21U) 
                                                                                | (((QData)((IData)(
                                                                                (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1fU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x20U)))))) 
                                                                                << 0x20U) 
                                                                                | (QData)((IData)(
                                                                                ((((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x20U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1fU))) 
                                                                                << 0x1fU) 
                                                                                | ((0x40000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x21U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1eU))) 
                                                                                << 0x1eU)) 
                                                                                | ((0x20000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x22U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1dU))) 
                                                                                << 0x1dU)) 
                                                                                | ((0x10000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x23U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1cU))) 
                                                                                << 0x1cU)) 
                                                                                | ((0x8000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x24U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1bU))) 
                                                                                << 0x1bU)) 
                                                                                | ((0x4000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x25U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x1aU))) 
                                                                                << 0x1aU)) 
                                                                                | ((0x2000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x26U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x19U))) 
                                                                                << 0x19U)) 
                                                                                | ((0x1000000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x27U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x18U))) 
                                                                                << 0x18U)) 
                                                                                | ((0x800000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x28U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x17U))) 
                                                                                << 0x17U)) 
                                                                                | ((0x400000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x29U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x16U))) 
                                                                                << 0x16U)) 
                                                                                | ((0x200000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2aU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x15U))) 
                                                                                << 0x15U)) 
                                                                                | ((0x100000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2bU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x14U))) 
                                                                                << 0x14U)) 
                                                                                | ((0x80000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2cU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x13U))) 
                                                                                << 0x13U)) 
                                                                                | ((0x40000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2dU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x12U))) 
                                                                                << 0x12U)) 
                                                                                | ((0x20000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2eU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x11U))) 
                                                                                << 0x11U)) 
                                                                                | ((0x10000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x2fU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x10U))) 
                                                                                << 0x10U)) 
                                                                                | ((0x8000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x30U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xfU))) 
                                                                                << 0xfU)) 
                                                                                | ((0x4000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x31U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xeU))) 
                                                                                << 0xeU)) 
                                                                                | ((0x2000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x32U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xdU))) 
                                                                                << 0xdU)) 
                                                                                | ((0x1000U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x33U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xcU))) 
                                                                                << 0xcU)) 
                                                                                | ((0x800U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x34U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xbU))) 
                                                                                << 0xbU)) 
                                                                                | ((0x400U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x35U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0xaU))) 
                                                                                << 0xaU)) 
                                                                                | ((0x200U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x36U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 9U))) 
                                                                                << 9U)) 
                                                                                | ((0x100U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x37U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 8U))) 
                                                                                << 8U)) 
                                                                                | ((0x80U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x38U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 7U))) 
                                                                                << 7U)) 
                                                                                | ((0x40U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x39U))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 6U))) 
                                                                                << 6U)) 
                                                                                | ((0x20U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x3aU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 5U))) 
                                                                                << 5U)) 
                                                                                | ((0x10U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x3bU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 4U))) 
                                                                                << 4U)) 
                                                                                | ((8U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x3cU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 3U))) 
                                                                                << 3U)) 
                                                                                | ((4U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x3dU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 2U))) 
                                                                                << 2U)) 
                                                                                | ((2U 
                                                                                & (((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x3eU))
                                                                                 : (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 1U))) 
                                                                                << 1U)) 
                                                                                | (1U 
                                                                                & ((2U 
                                                                                == (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                                                                 ? (IData)(
                                                                                (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0 
                                                                                >> 0x3fU))
                                                                                 : (IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__s0))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))));
    sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__alu_raw 
        = ((0x10U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
            ? 0ULL : ((8U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                       ? ((4U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                           ? 0ULL : ((2U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                      ? ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                          ? 0ULL : sim_soc_top__DOT__u_cpu__DOT__alu_operand_b)
                                      : ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                          ? (sim_soc_top__DOT__u_cpu__DOT__alu_operand_a 
                                             & sim_soc_top__DOT__u_cpu__DOT__alu_operand_b)
                                          : (sim_soc_top__DOT__u_cpu__DOT__alu_operand_a 
                                             | sim_soc_top__DOT__u_cpu__DOT__alu_operand_b))))
                       : ((4U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                           ? ((2U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                               ? sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_result
                               : ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                   ? (sim_soc_top__DOT__u_cpu__DOT__alu_operand_a 
                                      ^ sim_soc_top__DOT__u_cpu__DOT__alu_operand_b)
                                   : (QData)((IData)(
                                                     (sim_soc_top__DOT__u_cpu__DOT__alu_operand_a 
                                                      < sim_soc_top__DOT__u_cpu__DOT__alu_operand_b)))))
                           : ((2U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                               ? ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                   ? (QData)((IData)(
                                                     VL_LTS_IQQ(64, sim_soc_top__DOT__u_cpu__DOT__alu_operand_a, sim_soc_top__DOT__u_cpu__DOT__alu_operand_b)))
                                   : sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__shift_result)
                               : ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__dec_alu_op))
                                   ? (sim_soc_top__DOT__u_cpu__DOT__alu_operand_a 
                                      - sim_soc_top__DOT__u_cpu__DOT__alu_operand_b)
                                   : (sim_soc_top__DOT__u_cpu__DOT__alu_operand_a 
                                      + sim_soc_top__DOT__u_cpu__DOT__alu_operand_b))))));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__alu_result 
        = ((IData)(sim_soc_top__DOT__u_cpu__DOT__dec_is_word_op)
            ? (((QData)((IData)((- (IData)((1U & (IData)(
                                                         (sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__alu_raw 
                                                          >> 0x1fU))))))) 
                << 0x20U) | (QData)((IData)(sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__alu_raw)))
            : sim_soc_top__DOT__u_cpu__DOT__u_alu__DOT__alu_raw);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc_next_mux 
        = ((2U & (IData)(sim_soc_top__DOT__u_cpu__DOT__pc_src))
            ? ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__pc_src))
                ? ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_return)
                    ? vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mepc
                    : vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mtvec)
                : (0xfffffffffffffffeULL & vlSelf->sim_soc_top__DOT__u_cpu__DOT__alu_result))
            : ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__pc_src))
                ? vlSelf->sim_soc_top__DOT__u_cpu__DOT__alu_result
                : (4ULL + vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc)));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__wb_data = 
        ((4U & (IData)(sim_soc_top__DOT__u_cpu__DOT__wb_src))
          ? ((2U & (IData)(sim_soc_top__DOT__u_cpu__DOT__wb_src))
              ? 0ULL : ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__wb_src))
                         ? 0ULL : (vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator 
                                   ^ vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_read_reg)))
          : ((2U & (IData)(sim_soc_top__DOT__u_cpu__DOT__wb_src))
              ? ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__wb_src))
                  ? sim_soc_top__DOT__u_cpu__DOT__csr_rdata
                  : (4ULL + vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc))
              : ((1U & (IData)(sim_soc_top__DOT__u_cpu__DOT__wb_src))
                  ? ((4U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))
                      ? ((2U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))
                          ? ((1U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))
                              ? 0ULL : (QData)((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata)))
                          : ((1U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))
                              ? (QData)((IData)((0xffffU 
                                                 & sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata)))
                              : (QData)((IData)((0xffU 
                                                 & sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata)))))
                      : ((2U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))
                          ? ((1U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))
                              ? (((QData)((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata)) 
                                  << 0x20U) | (QData)((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__rdata_lo)))
                              : (((QData)((IData)((- (IData)(
                                                             (vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata 
                                                              >> 0x1fU))))) 
                                  << 0x20U) | (QData)((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata))))
                          : ((1U & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size))
                              ? (((- (QData)((IData)(
                                                     (1U 
                                                      & (sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata 
                                                         >> 0xfU))))) 
                                  << 0x10U) | (QData)((IData)(
                                                              (0xffffU 
                                                               & sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata))))
                              : (((- (QData)((IData)(
                                                     (1U 
                                                      & (sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata 
                                                         >> 7U))))) 
                                  << 8U) | (QData)((IData)(
                                                           (0xffU 
                                                            & sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata)))))))
                  : vlSelf->sim_soc_top__DOT__u_cpu__DOT__alu_result)));
}

VL_ATTR_COLD void Vsim_soc_top___024root___eval_stl(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_stl\n"); );
    // Body
    if ((1ULL & vlSelf->__VstlTriggered.word(0U))) {
        Vsim_soc_top___024root___stl_sequent__TOP__0(vlSelf);
    }
}

VL_ATTR_COLD void Vsim_soc_top___024root___eval_triggers__stl(Vsim_soc_top___024root* vlSelf);

VL_ATTR_COLD bool Vsim_soc_top___024root___eval_phase__stl(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_phase__stl\n"); );
    // Init
    CData/*0:0*/ __VstlExecute;
    // Body
    Vsim_soc_top___024root___eval_triggers__stl(vlSelf);
    __VstlExecute = vlSelf->__VstlTriggered.any();
    if (__VstlExecute) {
        Vsim_soc_top___024root___eval_stl(vlSelf);
    }
    return (__VstlExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vsim_soc_top___024root___dump_triggers__act(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___dump_triggers__act\n"); );
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
VL_ATTR_COLD void Vsim_soc_top___024root___dump_triggers__nba(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___dump_triggers__nba\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VnbaTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 0 is active: @(posedge clk)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vsim_soc_top___024root___ctor_var_reset(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___ctor_var_reset\n"); );
    // Body
    vlSelf->clk = VL_RAND_RESET_I(1);
    vlSelf->rst_n = VL_RAND_RESET_I(1);
    vlSelf->ser_rx = VL_RAND_RESET_I(1);
    vlSelf->ser_tx = VL_RAND_RESET_I(1);
    vlSelf->gpio_out = VL_RAND_RESET_I(7);
    vlSelf->sim_soc_top__DOT__mem_valid = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__mem_addr = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__mem_wstrb = VL_RAND_RESET_I(4);
    vlSelf->sim_soc_top__DOT__mem_rdata = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__brom_sel = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__gpio_sel = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 16384; ++__Vi0) {
        vlSelf->sim_soc_top__DOT__flash_mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->sim_soc_top__DOT__flash_rdata = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__flash_ready = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 2048; ++__Vi0) {
        vlSelf->sim_soc_top__DOT__sram_mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->sim_soc_top__DOT__sram_rdata = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__sram_ready = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 2048; ++__Vi0) {
        vlSelf->sim_soc_top__DOT__brom_mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->sim_soc_top__DOT__brom_rdata = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__brom_ready = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__gpio_out_r = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__gpio_oe_r = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__gpio_rdata = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__gpio_ready = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc_wen = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__fetch_done = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_store = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_system = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0 = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__regfile_wen = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__alu_result = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_start = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_done = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_valid = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb = VL_RAND_RESET_I(4);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__csr_wen = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_enter = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_return = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_has_bus = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__fetch_bus_ready = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_ready = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_load_en = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_swap_en = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT____Vcellinp__u_atomik__addr_in = VL_RAND_RESET_I(8);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc_next_mux = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__wb_data = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate_next = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[__Vi0] = VL_RAND_RESET_Q(64);
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs1_data_reg = VL_RAND_RESET_Q(64);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[__Vi0] = VL_RAND_RESET_Q(64);
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs2_data_reg = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state_next = VL_RAND_RESET_I(2);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_is_store = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size = VL_RAND_RESET_I(3);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__rdata_lo = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_needs_two = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpie = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpp = VL_RAND_RESET_I(2);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mtvec = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mscratch = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mepc = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mcause = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__swap_pending = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__active_addr = VL_RAND_RESET_I(8);
    for (int __Vi0 = 0; __Vi0 < 256; ++__Vi0) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table[__Vi0] = VL_RAND_RESET_Q(64);
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_read_reg = VL_RAND_RESET_Q(64);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state = VL_RAND_RESET_I(3);
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state_next = VL_RAND_RESET_I(3);
    vlSelf->sim_soc_top__DOT__u_uart__DOT__reg_dat_sel = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_uart__DOT____Vcellinp__u_manual_uart_tx__reg_div_we = VL_RAND_RESET_I(4);
    vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter = VL_RAND_RESET_I(32);
    vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index = VL_RAND_RESET_I(4);
    vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data = VL_RAND_RESET_I(8);
    vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_active = VL_RAND_RESET_I(1);
    vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_shift_reg = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = VL_RAND_RESET_I(1);
}
