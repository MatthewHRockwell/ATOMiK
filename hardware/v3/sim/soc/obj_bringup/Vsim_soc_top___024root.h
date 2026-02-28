// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vsim_soc_top.h for the primary calling header

#ifndef VERILATED_VSIM_SOC_TOP___024ROOT_H_
#define VERILATED_VSIM_SOC_TOP___024ROOT_H_  // guard

#include "verilated.h"


class Vsim_soc_top__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vsim_soc_top___024root final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    // Anonymous structures to workaround compiler member-count bugs
    struct {
        VL_IN8(clk,0,0);
        VL_IN8(rst_n,0,0);
        VL_IN8(ser_rx,0,0);
        VL_OUT8(ser_tx,0,0);
        VL_OUT8(gpio_out,6,0);
        CData/*0:0*/ sim_soc_top__DOT__mem_valid;
        CData/*3:0*/ sim_soc_top__DOT__mem_wstrb;
        CData/*0:0*/ sim_soc_top__DOT__brom_sel;
        CData/*0:0*/ sim_soc_top__DOT__gpio_sel;
        CData/*0:0*/ sim_soc_top__DOT__flash_ready;
        CData/*0:0*/ sim_soc_top__DOT__sram_ready;
        CData/*0:0*/ sim_soc_top__DOT__brom_ready;
        CData/*0:0*/ sim_soc_top__DOT__gpio_ready;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__pc_wen;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__fetch_done;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_store;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_system;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_is_custom0;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__dec_illegal;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__regfile_wen;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__lsu_start;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__lsu_done;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__lsu_bus_valid;
        CData/*3:0*/ sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wstrb;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__csr_wen;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__trap_enter;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__trap_return;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__lsu_has_bus;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__fetch_bus_ready;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__lsu_bus_ready;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__atomik_load_en;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__atomik_swap_en;
        CData/*7:0*/ sim_soc_top__DOT__u_cpu__DOT____Vcellinp__u_atomik__addr_in;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_fetch__DOT__fstate_next;
        CData/*1:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state;
        CData/*1:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__state_next;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_is_store;
        CData/*2:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_size;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_needs_two;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mie;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpie;
        CData/*1:0*/ sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mstatus_mpp;
        CData/*0:0*/ sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__swap_pending;
        CData/*7:0*/ sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__active_addr;
        CData/*2:0*/ sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state;
        CData/*2:0*/ sim_soc_top__DOT__u_cpu__DOT__u_control__DOT__state_next;
        CData/*0:0*/ sim_soc_top__DOT__u_uart__DOT__reg_dat_sel;
        CData/*3:0*/ sim_soc_top__DOT__u_uart__DOT____Vcellinp__u_manual_uart_tx__reg_div_we;
        CData/*3:0*/ sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_index;
        CData/*7:0*/ sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_data;
        CData/*0:0*/ sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_active;
        CData/*0:0*/ sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__tx_shift_reg;
        CData/*0:0*/ __VstlFirstIteration;
        CData/*0:0*/ __Vtrigprevexpr___TOP__clk__0;
        CData/*0:0*/ __VactContinue;
        IData/*31:0*/ sim_soc_top__DOT__mem_addr;
        IData/*31:0*/ sim_soc_top__DOT__mem_rdata;
        IData/*31:0*/ sim_soc_top__DOT__flash_rdata;
        IData/*31:0*/ sim_soc_top__DOT__sram_rdata;
        IData/*31:0*/ sim_soc_top__DOT__brom_rdata;
        IData/*31:0*/ sim_soc_top__DOT__gpio_out_r;
        IData/*31:0*/ sim_soc_top__DOT__gpio_oe_r;
        IData/*31:0*/ sim_soc_top__DOT__gpio_rdata;
    };
    struct {
        IData/*31:0*/ sim_soc_top__DOT__u_cpu__DOT__instr;
        IData/*31:0*/ sim_soc_top__DOT__u_cpu__DOT__lsu_bus_wdata;
        IData/*31:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__rdata_lo;
        IData/*31:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__last_rdata;
        IData/*31:0*/ sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__cfg_divider;
        IData/*31:0*/ sim_soc_top__DOT__u_uart__DOT__u_manual_uart_tx__DOT__bit_counter;
        IData/*31:0*/ __VactIterCount;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__pc;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__rs1_data;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__rs2_data;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__alu_result;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__pc_next_mux;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__wb_data;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs1_data_reg;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__rs2_data_reg;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_addr;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_lsu__DOT__req_wdata;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mtvec;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mscratch;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mepc;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__mcause;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_csr__DOT__csr_new_val;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__accumulator;
        QData/*63:0*/ sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_read_reg;
        VlUnpacked<IData/*31:0*/, 16384> sim_soc_top__DOT__flash_mem;
        VlUnpacked<IData/*31:0*/, 2048> sim_soc_top__DOT__sram_mem;
        VlUnpacked<IData/*31:0*/, 2048> sim_soc_top__DOT__brom_mem;
        VlUnpacked<QData/*63:0*/, 32> sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_a;
        VlUnpacked<QData/*63:0*/, 32> sim_soc_top__DOT__u_cpu__DOT__u_regfile__DOT__regs_b;
        VlUnpacked<QData/*63:0*/, 256> sim_soc_top__DOT__u_cpu__DOT__u_atomik__DOT__state_table;
    };
    VlTriggerVec<1> __VstlTriggered;
    VlTriggerVec<1> __VactTriggered;
    VlTriggerVec<1> __VnbaTriggered;

    // INTERNAL VARIABLES
    Vsim_soc_top__Syms* const vlSymsp;

    // CONSTRUCTORS
    Vsim_soc_top___024root(Vsim_soc_top__Syms* symsp, const char* v__name);
    ~Vsim_soc_top___024root();
    VL_UNCOPYABLE(Vsim_soc_top___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
