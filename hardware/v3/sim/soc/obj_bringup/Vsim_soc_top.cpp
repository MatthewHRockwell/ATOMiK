// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Vsim_soc_top__pch.h"

//============================================================
// Constructors

Vsim_soc_top::Vsim_soc_top(VerilatedContext* _vcontextp__, const char* _vcname__)
    : VerilatedModel{*_vcontextp__}
    , vlSymsp{new Vsim_soc_top__Syms(contextp(), _vcname__, this)}
    , clk{vlSymsp->TOP.clk}
    , rst_n{vlSymsp->TOP.rst_n}
    , ser_rx{vlSymsp->TOP.ser_rx}
    , ser_tx{vlSymsp->TOP.ser_tx}
    , gpio_out{vlSymsp->TOP.gpio_out}
    , rootp{&(vlSymsp->TOP)}
{
    // Register model with the context
    contextp()->addModel(this);
}

Vsim_soc_top::Vsim_soc_top(const char* _vcname__)
    : Vsim_soc_top(Verilated::threadContextp(), _vcname__)
{
}

//============================================================
// Destructor

Vsim_soc_top::~Vsim_soc_top() {
    delete vlSymsp;
}

//============================================================
// Evaluation function

#ifdef VL_DEBUG
void Vsim_soc_top___024root___eval_debug_assertions(Vsim_soc_top___024root* vlSelf);
#endif  // VL_DEBUG
void Vsim_soc_top___024root___eval_static(Vsim_soc_top___024root* vlSelf);
void Vsim_soc_top___024root___eval_initial(Vsim_soc_top___024root* vlSelf);
void Vsim_soc_top___024root___eval_settle(Vsim_soc_top___024root* vlSelf);
void Vsim_soc_top___024root___eval(Vsim_soc_top___024root* vlSelf);

void Vsim_soc_top::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vsim_soc_top::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Vsim_soc_top___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    vlSymsp->__Vm_deleter.deleteAll();
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) {
        vlSymsp->__Vm_didInit = true;
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial\n"););
        Vsim_soc_top___024root___eval_static(&(vlSymsp->TOP));
        Vsim_soc_top___024root___eval_initial(&(vlSymsp->TOP));
        Vsim_soc_top___024root___eval_settle(&(vlSymsp->TOP));
    }
    VL_DEBUG_IF(VL_DBG_MSGF("+ Eval\n"););
    Vsim_soc_top___024root___eval(&(vlSymsp->TOP));
    // Evaluate cleanup
    Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);
}

//============================================================
// Events and timing
bool Vsim_soc_top::eventsPending() { return false; }

uint64_t Vsim_soc_top::nextTimeSlot() {
    VL_FATAL_MT(__FILE__, __LINE__, "", "%Error: No delays in the design");
    return 0;
}

//============================================================
// Utilities

const char* Vsim_soc_top::name() const {
    return vlSymsp->name();
}

//============================================================
// Invoke final blocks

void Vsim_soc_top___024root___eval_final(Vsim_soc_top___024root* vlSelf);

VL_ATTR_COLD void Vsim_soc_top::final() {
    Vsim_soc_top___024root___eval_final(&(vlSymsp->TOP));
}

//============================================================
// Implementations of abstract methods from VerilatedModel

const char* Vsim_soc_top::hierName() const { return vlSymsp->name(); }
const char* Vsim_soc_top::modelName() const { return "Vsim_soc_top"; }
unsigned Vsim_soc_top::threads() const { return 1; }
void Vsim_soc_top::prepareClone() const { contextp()->prepareClone(); }
void Vsim_soc_top::atClone() const {
    contextp()->threadPoolpOnClone();
}

//============================================================
// Trace configuration

VL_ATTR_COLD void Vsim_soc_top::trace(VerilatedVcdC* tfp, int levels, int options) {
    vl_fatal(__FILE__, __LINE__, __FILE__,"'Vsim_soc_top::trace()' called on model that was Verilated without --trace option");
}
