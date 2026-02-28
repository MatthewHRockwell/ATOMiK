// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vsim_soc_top.h for the primary calling header

#include "Vsim_soc_top__pch.h"
#include "Vsim_soc_top___024root.h"

void Vsim_soc_top___024root___eval_act(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_act\n"); );
}

extern const VlUnpacked<CData/*2:0*/, 128> Vsim_soc_top__ConstPool__TABLE_h1cbca306_0;
extern const VlUnpacked<CData/*1:0*/, 32> Vsim_soc_top__ConstPool__TABLE_hdcc6d3b5_0;

VL_INLINE_OPT void Vsim_soc_top___024root___nba_sequent__TOP__0(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___nba_sequent__TOP__0\n"); );
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
    CData/*0:0*/ __Vdly__sim_soc_top__DOT__flash_ready;
    __Vdly__sim_soc_top__DOT__flash_ready = 0;
    CData/*0:0*/ __Vdly__sim_soc_top__DOT__sram_ready;
    __Vdly__sim_soc_top__DOT__sram_ready = 0;
    SData/*10:0*/ __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v0;
    __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v0 = 0;
    CData/*4:0*/ __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v0;
    __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v0 = 0;
    CData/*7:0*/ __Vdlyvval__sim_soc_top__DOT__sram_mem__v0;
    __Vdlyvval__sim_soc_top__DOT__sram_mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__sram_mem__v0;
    __Vdlyvset__sim_soc_top__DOT__sram_mem__v0 = 0;
    SData/*10:0*/ __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v1;
    __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v1 = 0;
    CData/*4:0*/ __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v1;
    __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v1 = 0;
    CData/*7:0*/ __Vdlyvval__sim_soc_top__DOT__sram_mem__v1;
    __Vdlyvval__sim_soc_top__DOT__sram_mem__v1 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__sram_mem__v1;
    __Vdlyvset__sim_soc_top__DOT__sram_mem__v1 = 0;
    SData/*10:0*/ __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v2;
    __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v2 = 0;
    CData/*4:0*/ __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v2;
    __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v2 = 0;
    CData/*7:0*/ __Vdlyvval__sim_soc_top__DOT__sram_mem__v2;
    __Vdlyvval__sim_soc_top__DOT__sram_mem__v2 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__sram_mem__v2;
    __Vdlyvset__sim_soc_top__DOT__sram_mem__v2 = 0;
    SData/*10:0*/ __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v3;
    __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v3 = 0;
    CData/*4:0*/ __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v3;
    __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v3 = 0;
    CData/*7:0*/ __Vdlyvval__sim_soc_top__DOT__sram_mem__v3;
    __Vdlyvval__sim_soc_top__DOT__sram_mem__v3 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__sram_mem__v3;
    __Vdlyvset__sim_soc_top__DOT__sram_mem__v3 = 0;
    CData/*0:0*/ __Vdly__sim_soc_top__DOT__brom_ready;
    __Vdly__sim_soc_top__DOT__brom_ready = 0;
    SData/*10:0*/ __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v0;
    __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v0 = 0;
    CData/*4:0*/ __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v0;
    __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v0 = 0;
    CData/*7:0*/ __Vdlyvval__sim_soc_top__DOT__brom_mem__v0;
    __Vdlyvval__sim_soc_top__DOT__brom_mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__brom_mem__v0;
    __Vdlyvset__sim_soc_top__DOT__brom_mem__v0 = 0;
    SData/*10:0*/ __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v1;
    __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v1 = 0;
    CData/*4:0*/ __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v1;
    __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v1 = 0;
    CData/*7:0*/ __Vdlyvval__sim_soc_top__DOT__brom_mem__v1;
    __Vdlyvval__sim_soc_top__DOT__brom_mem__v1 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__brom_mem__v1;
    __Vdlyvset__sim_soc_top__DOT__brom_mem__v1 = 0;
    SData/*10:0*/ __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v2;
    __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v2 = 0;
    CData/*4:0*/ __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v2;
    __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v2 = 0;
    CData/*7:0*/ __Vdlyvval__sim_soc_top__DOT__brom_mem__v2;
    __Vdlyvval__sim_soc_top__DOT__brom_mem__v2 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__brom_mem__v2;
    __Vdlyvset__sim_soc_top__DOT__brom_mem__v2 = 0;
    SData/*10:0*/ __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v3;
    __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v3 = 0;
    CData/*4:0*/ __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v3;
    __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v3 = 0;
    CData/*7:0*/ __Vdlyvval__sim_soc_top__DOT__brom_mem__v3;
    __Vdlyvval__sim_soc_top__DOT__brom_mem__v3 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__brom_mem__v3;
    __Vdlyvset__sim_soc_top__DOT__brom_mem__v3 = 0;
    CData/*0:0*/ __Vdly__sim_soc_top__DOT__gpio_ready;
    __Vdly__sim_soc_top__DOT__gpio_ready = 0;
    CData/*4:0*/ __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0;
    __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0 = 0;
    QData/*63:0*/ __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0;
    __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0;
    __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0 = 0;
    CData/*4:0*/ __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0;
    __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0 = 0;
    QData/*63:0*/ __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0;
    __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0;
    __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0 = 0;
    CData/*0:0*/ __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie;
    __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie = 0;
    QData/*63:0*/ __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator;
    __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator = 0;
    CData/*7:0*/ __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0;
    __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0 = 0;
    QData/*63:0*/ __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0;
    __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0;
    __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0 = 0;
    CData/*7:0*/ __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1;
    __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1 = 0;
    QData/*63:0*/ __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1;
    __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1 = 0;
    CData/*0:0*/ __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1;
    __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1 = 0;
    IData/*31:0*/ __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter;
    __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter = 0;
    CData/*3:0*/ __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index;
    __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index = 0;
    // Body
    if (VL_UNLIKELY((((IData)(vlSelf->rst_n) & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate)) 
                     & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_valid)))) {
        VL_WRITEF_NX("BUS ASSERTION FAIL: fetch_bus_valid and lsu_bus_valid both high at time %0t\n",0,
                     64,VL_TIME_UNITED_Q(1000),-9);
        VL_FINISH_MT("../../soc/gowin_ip/../../rtl/atomik_v3_cpu.v", 113, "");
    }
    __Vdly__sim_soc_top__DOT__flash_ready = vlSelf->sim_soc_top__DOT__flash_ready;
    __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0 = 0U;
    __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1 = 0U;
    __Vdly__sim_soc_top__DOT__gpio_ready = vlSelf->sim_soc_top__DOT__gpio_ready;
    __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index 
        = vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index;
    __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter 
        = vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter;
    __Vdlyvset__sim_soc_top__DOT__sram_mem__v0 = 0U;
    __Vdlyvset__sim_soc_top__DOT__sram_mem__v1 = 0U;
    __Vdlyvset__sim_soc_top__DOT__sram_mem__v2 = 0U;
    __Vdlyvset__sim_soc_top__DOT__sram_mem__v3 = 0U;
    __Vdly__sim_soc_top__DOT__sram_ready = vlSelf->sim_soc_top__DOT__sram_ready;
    __Vdlyvset__sim_soc_top__DOT__brom_mem__v0 = 0U;
    __Vdlyvset__sim_soc_top__DOT__brom_mem__v1 = 0U;
    __Vdlyvset__sim_soc_top__DOT__brom_mem__v2 = 0U;
    __Vdlyvset__sim_soc_top__DOT__brom_mem__v3 = 0U;
    __Vdly__sim_soc_top__DOT__brom_ready = vlSelf->sim_soc_top__DOT__brom_ready;
    __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0 = 0U;
    __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0 = 0U;
    __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator 
        = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator;
    __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie 
        = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie;
    if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_load_en) {
        __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0 
            = vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data;
        __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0 = 1U;
        __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0 
            = vlSelf->sim_soc_top__DOT__u_cpu__DOT____Vcellinp__u_atomik__addr_in;
    } else if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_swap_en) {
        __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1 
            = (vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_read_reg 
               ^ vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator);
        __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1 = 1U;
        __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1 
            = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__active_addr;
    }
    if (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__regfile_wen) 
         & (0U != (0x1fU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                            >> 7U))))) {
        __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0 
            = vlSelf->sim_soc_top__DOT__u_cpu__DOT__wb_data;
        __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0 = 1U;
        __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0 
            = (0x1fU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                        >> 7U));
        __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0 
            = vlSelf->sim_soc_top__DOT__u_cpu__DOT__wb_data;
        __Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0 = 1U;
        __Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0 
            = (0x1fU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                        >> 7U));
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__fetch_done 
        = ((IData)(vlSelf->rst_n) & ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate) 
                                     & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__fetch_bus_ready)));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_done 
        = ((IData)(vlSelf->rst_n) & (3U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state)));
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs2_data_reg 
        = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b
        [(0x1fU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                   >> 0x14U))];
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs1_data_reg 
        = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a
        [(0x1fU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                   >> 0xfU))];
    if (vlSelf->rst_n) {
        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_load_en) {
            __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator = 0ULL;
        } else if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__swap_pending) {
            __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator = 0ULL;
        } else if (((2U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state)) 
                    & ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0) 
                       & (0x1000U == (0x7000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))))) {
            __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator 
                = (vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator 
                   ^ vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs1_data);
        }
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator 
            = __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator;
        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_enter) {
            __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie = 0U;
        }
        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_return) {
            __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie 
                = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpie;
        }
        if (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__csr_wen) 
             & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_enter)))) {
            if ((0x300U == (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                            >> 0x14U))) {
                __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie 
                    = (1U & (IData)((vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val 
                                     >> 3U)));
            }
        }
        __Vdly__sim_soc_top__DOT__flash_ready = 0U;
        if ((((IData)(vlSelf->sim_soc_top__DOT__mem_valid) 
              & (0U == (vlSelf->sim_soc_top__DOT__mem_addr 
                        >> 0x1eU))) & (~ (IData)(vlSelf->sim_soc_top__DOT__flash_ready)))) {
            vlSelf->sim_soc_top__DOT__flash_rdata = 
                vlSelf->sim_soc_top__DOT__flash_mem
                [(0x3fffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                             >> 2U))];
            __Vdly__sim_soc_top__DOT__flash_ready = 1U;
        }
        vlSelf->sim_soc_top__DOT__flash_ready = __Vdly__sim_soc_top__DOT__flash_ready;
        __Vdly__sim_soc_top__DOT__gpio_ready = 0U;
        if ((((IData)(vlSelf->sim_soc_top__DOT__mem_valid) 
              & (IData)(vlSelf->sim_soc_top__DOT__gpio_sel)) 
             & (~ (IData)(vlSelf->sim_soc_top__DOT__gpio_ready)))) {
            __Vdly__sim_soc_top__DOT__gpio_ready = 1U;
            if ((0U == (3U & (vlSelf->sim_soc_top__DOT__mem_addr 
                              >> 2U)))) {
                vlSelf->sim_soc_top__DOT__gpio_rdata 
                    = vlSelf->sim_soc_top__DOT__gpio_out_r;
                if ((1U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                    vlSelf->sim_soc_top__DOT__gpio_out_r 
                        = ((0xffffff00U & vlSelf->sim_soc_top__DOT__gpio_out_r) 
                           | (0xffU & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
                }
                if ((2U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                    vlSelf->sim_soc_top__DOT__gpio_out_r 
                        = ((0xffff00ffU & vlSelf->sim_soc_top__DOT__gpio_out_r) 
                           | (0xff00U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
                }
                if ((4U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                    vlSelf->sim_soc_top__DOT__gpio_out_r 
                        = ((0xff00ffffU & vlSelf->sim_soc_top__DOT__gpio_out_r) 
                           | (0xff0000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
                }
                if ((8U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                    vlSelf->sim_soc_top__DOT__gpio_out_r 
                        = ((0xffffffU & vlSelf->sim_soc_top__DOT__gpio_out_r) 
                           | (0xff000000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
                }
            } else if ((1U == (3U & (vlSelf->sim_soc_top__DOT__mem_addr 
                                     >> 2U)))) {
                vlSelf->sim_soc_top__DOT__gpio_rdata = 0U;
            } else if ((2U == (3U & (vlSelf->sim_soc_top__DOT__mem_addr 
                                     >> 2U)))) {
                vlSelf->sim_soc_top__DOT__gpio_rdata 
                    = vlSelf->sim_soc_top__DOT__gpio_oe_r;
                if ((1U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                    vlSelf->sim_soc_top__DOT__gpio_oe_r 
                        = ((0xffffff00U & vlSelf->sim_soc_top__DOT__gpio_oe_r) 
                           | (0xffU & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
                }
                if ((2U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                    vlSelf->sim_soc_top__DOT__gpio_oe_r 
                        = ((0xffff00ffU & vlSelf->sim_soc_top__DOT__gpio_oe_r) 
                           | (0xff00U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
                }
                if ((4U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                    vlSelf->sim_soc_top__DOT__gpio_oe_r 
                        = ((0xff00ffffU & vlSelf->sim_soc_top__DOT__gpio_oe_r) 
                           | (0xff0000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
                }
                if ((8U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                    vlSelf->sim_soc_top__DOT__gpio_oe_r 
                        = ((0xffffffU & vlSelf->sim_soc_top__DOT__gpio_oe_r) 
                           | (0xff000000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
                }
            } else {
                vlSelf->sim_soc_top__DOT__gpio_rdata = 0xdeadbeefU;
            }
        }
        vlSelf->sim_soc_top__DOT__gpio_ready = __Vdly__sim_soc_top__DOT__gpio_ready;
        if (vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_active) {
            if ((vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter 
                 >= vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider)) {
                __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter = 0U;
                if ((0xaU > (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))) {
                    __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index 
                        = (0xfU & ((IData)(1U) + (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index)));
                    vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_shift_reg 
                        = ((1U & (~ ((((((((0U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index)) 
                                           | (1U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))) 
                                          | (2U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))) 
                                         | (3U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))) 
                                        | (4U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))) 
                                       | (5U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))) 
                                      | (6U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))) 
                                     | (7U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))))) 
                           || (1U & ((0U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))
                                      ? (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data)
                                      : ((1U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))
                                          ? ((IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data) 
                                             >> 1U)
                                          : ((2U == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))
                                              ? ((IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data) 
                                                 >> 2U)
                                              : ((3U 
                                                  == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))
                                                  ? 
                                                 ((IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data) 
                                                  >> 3U)
                                                  : 
                                                 ((4U 
                                                   == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))
                                                   ? 
                                                  ((IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data) 
                                                   >> 4U)
                                                   : 
                                                  ((5U 
                                                    == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))
                                                    ? 
                                                   ((IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data) 
                                                    >> 5U)
                                                    : 
                                                   ((6U 
                                                     == (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index))
                                                     ? 
                                                    ((IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data) 
                                                     >> 6U)
                                                     : 
                                                    ((IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data) 
                                                     >> 7U))))))))));
                } else {
                    vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_active = 0U;
                    __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index = 0U;
                    vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_shift_reg = 1U;
                }
            } else {
                __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter 
                    = ((IData)(1U) + vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter);
            }
        } else {
            vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_shift_reg = 1U;
            if (((IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT__reg_dat_sel) 
                 & ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_has_bus) 
                    & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb)))) {
                __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index = 0U;
                vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data 
                    = (0xffU & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata);
                vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_active = 1U;
                __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter = 0U;
                vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_shift_reg = 0U;
            }
        }
        vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter 
            = __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter;
        vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index 
            = __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index;
        __Vdly__sim_soc_top__DOT__sram_ready = 0U;
        if ((((IData)(vlSelf->sim_soc_top__DOT__mem_valid) 
              & (1U == (vlSelf->sim_soc_top__DOT__mem_addr 
                        >> 0x1eU))) & (~ (IData)(vlSelf->sim_soc_top__DOT__sram_ready)))) {
            vlSelf->sim_soc_top__DOT__sram_rdata = 
                vlSelf->sim_soc_top__DOT__sram_mem[
                (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                           >> 2U))];
            __Vdly__sim_soc_top__DOT__sram_ready = 1U;
            if ((1U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                __Vdlyvval__sim_soc_top__DOT__sram_mem__v0 
                    = (0xffU & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata);
                __Vdlyvset__sim_soc_top__DOT__sram_mem__v0 = 1U;
                __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v0 = 0U;
                __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v0 
                    = (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                                 >> 2U));
            }
            if ((2U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                __Vdlyvval__sim_soc_top__DOT__sram_mem__v1 
                    = (0xffU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                                >> 8U));
                __Vdlyvset__sim_soc_top__DOT__sram_mem__v1 = 1U;
                __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v1 = 8U;
                __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v1 
                    = (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                                 >> 2U));
            }
            if ((4U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                __Vdlyvval__sim_soc_top__DOT__sram_mem__v2 
                    = (0xffU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                                >> 0x10U));
                __Vdlyvset__sim_soc_top__DOT__sram_mem__v2 = 1U;
                __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v2 = 0x10U;
                __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v2 
                    = (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                                 >> 2U));
            }
            if ((8U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                __Vdlyvval__sim_soc_top__DOT__sram_mem__v3 
                    = (vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                       >> 0x18U);
                __Vdlyvset__sim_soc_top__DOT__sram_mem__v3 = 1U;
                __Vdlyvlsb__sim_soc_top__DOT__sram_mem__v3 = 0x18U;
                __Vdlyvdim0__sim_soc_top__DOT__sram_mem__v3 
                    = (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                                 >> 2U));
            }
        }
    } else {
        __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator = 0ULL;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator 
            = __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator;
        __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie = 0U;
        __Vdly__sim_soc_top__DOT__flash_ready = 0U;
        vlSelf->sim_soc_top__DOT__flash_ready = __Vdly__sim_soc_top__DOT__flash_ready;
        __Vdly__sim_soc_top__DOT__gpio_ready = 0U;
        vlSelf->sim_soc_top__DOT__gpio_out_r = 0U;
        vlSelf->sim_soc_top__DOT__gpio_oe_r = 0U;
        vlSelf->sim_soc_top__DOT__gpio_rdata = 0U;
        vlSelf->sim_soc_top__DOT__gpio_ready = __Vdly__sim_soc_top__DOT__gpio_ready;
        __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index = 0U;
        __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter = 0U;
        vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data = 0U;
        vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_active = 0U;
        vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_shift_reg = 1U;
        vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter 
            = __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter;
        vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index 
            = __Vdly__sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index;
        __Vdly__sim_soc_top__DOT__sram_ready = 0U;
    }
    if (__Vdlyvset__sim_soc_top__DOT__sram_mem__v0) {
        vlSelf->sim_soc_top__DOT__sram_mem[__Vdlyvdim0__sim_soc_top__DOT__sram_mem__v0] 
            = (((~ ((IData)(0xffU) << (IData)(__Vdlyvlsb__sim_soc_top__DOT__sram_mem__v0))) 
                & vlSelf->sim_soc_top__DOT__sram_mem
                [__Vdlyvdim0__sim_soc_top__DOT__sram_mem__v0]) 
               | (0xffffffffULL & ((IData)(__Vdlyvval__sim_soc_top__DOT__sram_mem__v0) 
                                   << (IData)(__Vdlyvlsb__sim_soc_top__DOT__sram_mem__v0))));
    }
    if (__Vdlyvset__sim_soc_top__DOT__sram_mem__v1) {
        vlSelf->sim_soc_top__DOT__sram_mem[__Vdlyvdim0__sim_soc_top__DOT__sram_mem__v1] 
            = (((~ ((IData)(0xffU) << (IData)(__Vdlyvlsb__sim_soc_top__DOT__sram_mem__v1))) 
                & vlSelf->sim_soc_top__DOT__sram_mem
                [__Vdlyvdim0__sim_soc_top__DOT__sram_mem__v1]) 
               | (0xffffffffULL & ((IData)(__Vdlyvval__sim_soc_top__DOT__sram_mem__v1) 
                                   << (IData)(__Vdlyvlsb__sim_soc_top__DOT__sram_mem__v1))));
    }
    if (__Vdlyvset__sim_soc_top__DOT__sram_mem__v2) {
        vlSelf->sim_soc_top__DOT__sram_mem[__Vdlyvdim0__sim_soc_top__DOT__sram_mem__v2] 
            = (((~ ((IData)(0xffU) << (IData)(__Vdlyvlsb__sim_soc_top__DOT__sram_mem__v2))) 
                & vlSelf->sim_soc_top__DOT__sram_mem
                [__Vdlyvdim0__sim_soc_top__DOT__sram_mem__v2]) 
               | (0xffffffffULL & ((IData)(__Vdlyvval__sim_soc_top__DOT__sram_mem__v2) 
                                   << (IData)(__Vdlyvlsb__sim_soc_top__DOT__sram_mem__v2))));
    }
    if (__Vdlyvset__sim_soc_top__DOT__sram_mem__v3) {
        vlSelf->sim_soc_top__DOT__sram_mem[__Vdlyvdim0__sim_soc_top__DOT__sram_mem__v3] 
            = (((~ ((IData)(0xffU) << (IData)(__Vdlyvlsb__sim_soc_top__DOT__sram_mem__v3))) 
                & vlSelf->sim_soc_top__DOT__sram_mem
                [__Vdlyvdim0__sim_soc_top__DOT__sram_mem__v3]) 
               | (0xffffffffULL & ((IData)(__Vdlyvval__sim_soc_top__DOT__sram_mem__v3) 
                                   << (IData)(__Vdlyvlsb__sim_soc_top__DOT__sram_mem__v3))));
    }
    vlSelf->sim_soc_top__DOT__sram_ready = __Vdly__sim_soc_top__DOT__sram_ready;
    if (vlSelf->rst_n) {
        __Vdly__sim_soc_top__DOT__brom_ready = 0U;
        if ((((IData)(vlSelf->sim_soc_top__DOT__mem_valid) 
              & (IData)(vlSelf->sim_soc_top__DOT__brom_sel)) 
             & (~ (IData)(vlSelf->sim_soc_top__DOT__brom_ready)))) {
            vlSelf->sim_soc_top__DOT__brom_rdata = 
                vlSelf->sim_soc_top__DOT__brom_mem[
                (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                           >> 2U))];
            __Vdly__sim_soc_top__DOT__brom_ready = 1U;
            if ((1U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                __Vdlyvval__sim_soc_top__DOT__brom_mem__v0 
                    = (0xffU & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata);
                __Vdlyvset__sim_soc_top__DOT__brom_mem__v0 = 1U;
                __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v0 = 0U;
                __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v0 
                    = (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                                 >> 2U));
            }
            if ((2U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                __Vdlyvval__sim_soc_top__DOT__brom_mem__v1 
                    = (0xffU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                                >> 8U));
                __Vdlyvset__sim_soc_top__DOT__brom_mem__v1 = 1U;
                __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v1 = 8U;
                __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v1 
                    = (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                                 >> 2U));
            }
            if ((4U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                __Vdlyvval__sim_soc_top__DOT__brom_mem__v2 
                    = (0xffU & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                                >> 0x10U));
                __Vdlyvset__sim_soc_top__DOT__brom_mem__v2 = 1U;
                __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v2 = 0x10U;
                __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v2 
                    = (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                                 >> 2U));
            }
            if ((8U & (IData)(vlSelf->sim_soc_top__DOT__mem_wstrb))) {
                __Vdlyvval__sim_soc_top__DOT__brom_mem__v3 
                    = (vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata 
                       >> 0x18U);
                __Vdlyvset__sim_soc_top__DOT__brom_mem__v3 = 1U;
                __Vdlyvlsb__sim_soc_top__DOT__brom_mem__v3 = 0x18U;
                __Vdlyvdim0__sim_soc_top__DOT__brom_mem__v3 
                    = (0x7ffU & (vlSelf->sim_soc_top__DOT__mem_addr 
                                 >> 2U));
            }
        }
    } else {
        __Vdly__sim_soc_top__DOT__brom_ready = 0U;
    }
    if (__Vdlyvset__sim_soc_top__DOT__brom_mem__v0) {
        vlSelf->sim_soc_top__DOT__brom_mem[__Vdlyvdim0__sim_soc_top__DOT__brom_mem__v0] 
            = (((~ ((IData)(0xffU) << (IData)(__Vdlyvlsb__sim_soc_top__DOT__brom_mem__v0))) 
                & vlSelf->sim_soc_top__DOT__brom_mem
                [__Vdlyvdim0__sim_soc_top__DOT__brom_mem__v0]) 
               | (0xffffffffULL & ((IData)(__Vdlyvval__sim_soc_top__DOT__brom_mem__v0) 
                                   << (IData)(__Vdlyvlsb__sim_soc_top__DOT__brom_mem__v0))));
    }
    if (__Vdlyvset__sim_soc_top__DOT__brom_mem__v1) {
        vlSelf->sim_soc_top__DOT__brom_mem[__Vdlyvdim0__sim_soc_top__DOT__brom_mem__v1] 
            = (((~ ((IData)(0xffU) << (IData)(__Vdlyvlsb__sim_soc_top__DOT__brom_mem__v1))) 
                & vlSelf->sim_soc_top__DOT__brom_mem
                [__Vdlyvdim0__sim_soc_top__DOT__brom_mem__v1]) 
               | (0xffffffffULL & ((IData)(__Vdlyvval__sim_soc_top__DOT__brom_mem__v1) 
                                   << (IData)(__Vdlyvlsb__sim_soc_top__DOT__brom_mem__v1))));
    }
    if (__Vdlyvset__sim_soc_top__DOT__brom_mem__v2) {
        vlSelf->sim_soc_top__DOT__brom_mem[__Vdlyvdim0__sim_soc_top__DOT__brom_mem__v2] 
            = (((~ ((IData)(0xffU) << (IData)(__Vdlyvlsb__sim_soc_top__DOT__brom_mem__v2))) 
                & vlSelf->sim_soc_top__DOT__brom_mem
                [__Vdlyvdim0__sim_soc_top__DOT__brom_mem__v2]) 
               | (0xffffffffULL & ((IData)(__Vdlyvval__sim_soc_top__DOT__brom_mem__v2) 
                                   << (IData)(__Vdlyvlsb__sim_soc_top__DOT__brom_mem__v2))));
    }
    if (__Vdlyvset__sim_soc_top__DOT__brom_mem__v3) {
        vlSelf->sim_soc_top__DOT__brom_mem[__Vdlyvdim0__sim_soc_top__DOT__brom_mem__v3] 
            = (((~ ((IData)(0xffU) << (IData)(__Vdlyvlsb__sim_soc_top__DOT__brom_mem__v3))) 
                & vlSelf->sim_soc_top__DOT__brom_mem
                [__Vdlyvdim0__sim_soc_top__DOT__brom_mem__v3]) 
               | (0xffffffffULL & ((IData)(__Vdlyvval__sim_soc_top__DOT__brom_mem__v3) 
                                   << (IData)(__Vdlyvlsb__sim_soc_top__DOT__brom_mem__v3))));
    }
    vlSelf->sim_soc_top__DOT__brom_ready = __Vdly__sim_soc_top__DOT__brom_ready;
    if (__Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b[__Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0] 
            = __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b__v0;
    }
    if (__Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a[__Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0] 
            = __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a__v0;
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_read_reg 
        = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table
        [vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__active_addr];
    if (vlSelf->rst_n) {
        if (((((1U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state)) 
               | (2U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state))) 
              & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_ready)) 
             & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_is_store)))) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata 
                = vlSelf->sim_soc_top__DOT__mem_rdata;
        }
        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_enter) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpp = 3U;
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mcause 
                = (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_system) 
                    & (IData)((0x100000U == (0x107000U 
                                             & vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr))))
                    ? 3ULL : ((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_illegal)
                               ? 2ULL : 0xbULL));
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mepc 
                = vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc;
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpie 
                = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie;
        }
        if (((((1U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state)) 
               & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_ready)) 
              & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_is_store))) 
             & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_needs_two))) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__rdata_lo 
                = vlSelf->sim_soc_top__DOT__mem_rdata;
        }
        if (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_start) 
             & (0U == (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state)))) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata 
                = vlSelf->sim_soc_top__DOT__u_cpu__DOT__rs2_data;
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr 
                = vlSelf->sim_soc_top__DOT__u_cpu__DOT__alu_result;
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size 
                = (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                         >> 0xcU));
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_is_store 
                = vlSelf->sim_soc_top__DOT__u_cpu__DOT__dec_is_store;
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_needs_two 
                = (3U == (7U & (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                >> 0xcU)));
        }
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state 
            = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state_next;
        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_return) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpie = 1U;
        }
        if (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__csr_wen) 
             & (~ (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__trap_enter)))) {
            if ((0x300U != (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                            >> 0x14U))) {
                if ((0x305U != (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                >> 0x14U))) {
                    if ((0x340U == (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                    >> 0x14U))) {
                        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mscratch 
                            = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val;
                    }
                    if ((0x340U != (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                    >> 0x14U))) {
                        if ((0x341U != (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                        >> 0x14U))) {
                            if ((0x342U == (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                            >> 0x14U))) {
                                vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mcause 
                                    = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val;
                            }
                        }
                        if ((0x341U == (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                        >> 0x14U))) {
                            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mepc 
                                = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val;
                        }
                    }
                }
                if ((0x305U == (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                                >> 0x14U))) {
                    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mtvec 
                        = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val;
                }
            }
            if ((0x300U == (vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                            >> 0x14U))) {
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpp 
                    = (3U & (IData)((vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val 
                                     >> 0xbU)));
                vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpie 
                    = (1U & (IData)((vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val 
                                     >> 7U)));
            }
        }
        if ((1U & (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT____Vcellinp__u_manual_uart_tx__reg_div_we))) {
            vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider 
                = ((0xffffff00U & vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider) 
                   | (0xffU & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
        }
        if ((2U & (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT____Vcellinp__u_manual_uart_tx__reg_div_we))) {
            vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider 
                = ((0xffff00ffU & vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider) 
                   | (0xff00U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
        }
        if ((4U & (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT____Vcellinp__u_manual_uart_tx__reg_div_we))) {
            vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider 
                = ((0xff00ffffU & vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider) 
                   | (0xff0000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
        }
        if ((8U & (IData)(vlSelf->sim_soc_top__DOT__u_uart__DOT____Vcellinp__u_manual_uart_tx__reg_div_we))) {
            vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider 
                = ((0xffffffU & vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider) 
                   | (0xff000000U & vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata));
        }
        if (vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc_wen) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc 
                = vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc_next_mux;
        }
        if (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_load_en) 
             | (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_swap_en))) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__active_addr 
                = vlSelf->sim_soc_top__DOT__u_cpu__DOT____Vcellinp__u_atomik__addr_in;
        }
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state 
            = vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state_next;
        if (((IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate) 
             & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__fetch_bus_ready))) {
            vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr 
                = vlSelf->sim_soc_top__DOT__mem_rdata;
        }
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__swap_pending 
            = vlSelf->sim_soc_top__DOT__u_cpu__DOT__atomik_swap_en;
    } else {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata = 0ULL;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr = 0ULL;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mscratch = 0ULL;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpp = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mtvec = 0ULL;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mcause = 0ULL;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__rdata_lo = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mepc = 0ULL;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpie = 0U;
        vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider = 1U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_is_store = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_needs_two = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc = 0x80000000ULL;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__active_addr = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state = 0U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__instr = 0x13U;
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__swap_pending = 0U;
    }
    vlSelf->gpio_out = (0x7fU & vlSelf->sim_soc_top__DOT__gpio_out_r);
    vlSelf->ser_tx = vlSelf->sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_shift_reg;
    sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__shifted_rdata 
        = VL_SHIFTR_III(32,32,32, vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata, 
                        VL_SHIFTL_III(32,32,32, (3U 
                                                 & (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr)), 3U));
    if (__Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table[__Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0] 
            = __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v0;
    }
    if (__Vdlyvset__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1) {
        vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table[__Vdlyvdim0__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1] 
            = __Vdlyvval__sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table__v1;
    }
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie 
        = __Vdly__sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__pc_wen = 0U;
    sim_soc_top__DOT__u_cpu__DOT__fetch_start = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb = 0U;
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__lsu_bus_valid = 0U;
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
    vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate 
        = ((IData)(vlSelf->rst_n) && (IData)(vlSelf->sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate_next));
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

void Vsim_soc_top___024root___eval_nba(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_nba\n"); );
    // Body
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        Vsim_soc_top___024root___nba_sequent__TOP__0(vlSelf);
    }
}

void Vsim_soc_top___024root___eval_triggers__act(Vsim_soc_top___024root* vlSelf);

bool Vsim_soc_top___024root___eval_phase__act(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_phase__act\n"); );
    // Init
    VlTriggerVec<1> __VpreTriggered;
    CData/*0:0*/ __VactExecute;
    // Body
    Vsim_soc_top___024root___eval_triggers__act(vlSelf);
    __VactExecute = vlSelf->__VactTriggered.any();
    if (__VactExecute) {
        __VpreTriggered.andNot(vlSelf->__VactTriggered, vlSelf->__VnbaTriggered);
        vlSelf->__VnbaTriggered.thisOr(vlSelf->__VactTriggered);
        Vsim_soc_top___024root___eval_act(vlSelf);
    }
    return (__VactExecute);
}

bool Vsim_soc_top___024root___eval_phase__nba(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_phase__nba\n"); );
    // Init
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = vlSelf->__VnbaTriggered.any();
    if (__VnbaExecute) {
        Vsim_soc_top___024root___eval_nba(vlSelf);
        vlSelf->__VnbaTriggered.clear();
    }
    return (__VnbaExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vsim_soc_top___024root___dump_triggers__nba(Vsim_soc_top___024root* vlSelf);
#endif  // VL_DEBUG
#ifdef VL_DEBUG
VL_ATTR_COLD void Vsim_soc_top___024root___dump_triggers__act(Vsim_soc_top___024root* vlSelf);
#endif  // VL_DEBUG

void Vsim_soc_top___024root___eval(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval\n"); );
    // Init
    IData/*31:0*/ __VnbaIterCount;
    CData/*0:0*/ __VnbaContinue;
    // Body
    __VnbaIterCount = 0U;
    __VnbaContinue = 1U;
    while (__VnbaContinue) {
        if (VL_UNLIKELY((0x64U < __VnbaIterCount))) {
#ifdef VL_DEBUG
            Vsim_soc_top___024root___dump_triggers__nba(vlSelf);
#endif
            VL_FATAL_MT("sim_soc_top.v", 21, "", "NBA region did not converge.");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        __VnbaContinue = 0U;
        vlSelf->__VactIterCount = 0U;
        vlSelf->__VactContinue = 1U;
        while (vlSelf->__VactContinue) {
            if (VL_UNLIKELY((0x64U < vlSelf->__VactIterCount))) {
#ifdef VL_DEBUG
                Vsim_soc_top___024root___dump_triggers__act(vlSelf);
#endif
                VL_FATAL_MT("sim_soc_top.v", 21, "", "Active region did not converge.");
            }
            vlSelf->__VactIterCount = ((IData)(1U) 
                                       + vlSelf->__VactIterCount);
            vlSelf->__VactContinue = 0U;
            if (Vsim_soc_top___024root___eval_phase__act(vlSelf)) {
                vlSelf->__VactContinue = 1U;
            }
        }
        if (Vsim_soc_top___024root___eval_phase__nba(vlSelf)) {
            __VnbaContinue = 1U;
        }
    }
}

#ifdef VL_DEBUG
void Vsim_soc_top___024root___eval_debug_assertions(Vsim_soc_top___024root* vlSelf) {
    (void)vlSelf;  // Prevent unused variable warning
    Vsim_soc_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vsim_soc_top___024root___eval_debug_assertions\n"); );
    // Body
    if (VL_UNLIKELY((vlSelf->clk & 0xfeU))) {
        Verilated::overWidthError("clk");}
    if (VL_UNLIKELY((vlSelf->rst_n & 0xfeU))) {
        Verilated::overWidthError("rst_n");}
    if (VL_UNLIKELY((vlSelf->ser_rx & 0xfeU))) {
        Verilated::overWidthError("ser_rx");}
}
#endif  // VL_DEBUG
