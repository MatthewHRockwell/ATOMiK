# RTL Debugging Checklist - MMIO Load Bug

## Bug: CPU Hangs on Repeated MMIO Loads

### Symptom
Reading from MMIO address (e.g., UART->DATA at 0x83000000) in a tight loop causes CPU to hang completely.

### Test Case
```c
for (int i = 0; i < 10000; i++) {
    volatile int32_t dummy = *(volatile int32_t*)0x83000000;
}
// CPU hangs, never exits loop
```

---

## RTL Files to Investigate

### 1. Load/Store Unit (Priority: CRITICAL)

**File**: `hardware/v3/rtl/atomik_v3_lsu.v` (if exists) or check in `atomik_v3_cpu.v`

**Questions**:
- Does LSU properly wait for `mem_valid` before accepting data?
- Can back-to-back loads create a deadlock where LSU waits forever?
- Is there a state machine that can get stuck?

**Look for:**
```verilog
// Bad pattern - doesn't wait for valid
assign load_data = mem_rdata;  // Direct assignment

// Good pattern - waits for handshake
always @(posedge clk) begin
    if (mem_valid && mem_ready) begin
        load_data <= mem_rdata;
    end
end
```

### 2. Memory Bus Interface

**File**: `hardware/v3/rtl/atomik_v3_cpu.v` (bus signals)

**Check**:
- Are `mem_valid` and `mem_ready` handshaking correctly?
- Does CPU properly de-assert `mem_valid` between consecutive loads?
- Is there a cycle where `mem_valid` stays high but peripheral doesn't respond?

**Look for:**
```verilog
// Potential bug: mem_valid never clears
always @(posedge clk) begin
    if (load_instr) begin
        mem_valid <= 1;  // Sets valid
        // BUT NEVER CLEARS IT!
    end
end
```

### 3. Control FSM During Loads

**File**: `hardware/v3/rtl/atomik_v3_control.v`

**Check**:
- What state does control FSM enter during load?
- Does it properly wait for load completion before advancing?
- Can FSM get stuck in LOAD_WAIT state?

**Look for:**
```verilog
LOAD_WAIT: begin
    if (mem_valid) begin  // Wrong - should check mem_valid AND mem_ready
        state <= NEXT_STATE;
    end
end

// Should be:
LOAD_WAIT: begin
    if (mem_valid && mem_ready) begin
        state <= NEXT_STATE;
    end
end
```

### 4. Register File Writeback

**File**: `hardware/v3/rtl/atomik_v3_regfile.v`

**Check**:
- Does load data properly write to destination register?
- Can corrupted load data overwrite PC or critical registers?

---

## Debugging Steps

### Step 1: Add Waveform Tracing
```verilog
// In testbench
initial begin
    $dumpfile("mmio_load_bug.vcd");
    $dumpvars(0, top);
end
```

### Step 2: Create Minimal Verilator Test
```cpp
// tb_mmio_load_hang.cpp
#include "Vatomik_v3_cpu.h"

int main() {
    Vatomik_v3_cpu* cpu = new Vatomik_v3_cpu;

    // Load program:
    // lw x5, 0(x6)  where x6 = 0x83000000 (UART base)
    // Loop 100 times

    for (int cycle = 0; cycle < 10000; cycle++) {
        cpu->clk = 0; cpu->eval();
        cpu->clk = 1; cpu->eval();

        // Monitor mem_valid, mem_ready, state
        printf("Cycle %d: valid=%d ready=%d state=%d\n",
            cycle, cpu->mem_valid, cpu->mem_ready, cpu->state);

        // CPU should not hang - if no progress for 1000 cycles, BUG!
    }
}
```

### Step 3: Check Signals in Waveform
1. Load instruction executes
2. `mem_valid` goes high
3. Does `mem_ready` come back?
4. Does `mem_valid` clear before next load?
5. Does CPU advance PC?

**Hang signature**:
- `mem_valid` stuck HIGH
- `mem_ready` stuck LOW or never asserted
- PC doesn't advance
- State machine stuck in LOAD_WAIT

---

## Known Good vs. Broken Patterns

### Broken: Back-to-back MMIO loads
```assembly
loop:
    lw x5, 0(x6)      # Load from MMIO
    addi x7, x7, 1    # Increment counter
    blt x7, x8, loop  # Loop
# CPU hangs after 2-3 iterations
```

### Working: MMIO loads with delays
```assembly
loop:
    lw x5, 0(x6)      # Load from MMIO
    li x9, 100
delay:
    addi x9, x9, -1   # Delay loop (100 iterations)
    bnez x9, delay
    addi x7, x7, 1
    blt x7, x8, loop
# Works fine
```

### Working: RAM loads (not MMIO)
```assembly
loop:
    lw x5, 0(x6)      # Load from RAM (0x40000000)
    addi x7, x7, 1
    blt x7, x8, loop
# Works fine - no hang
```

---

## Expected Fix

Most likely one of these:

### Fix 1: Proper mem_valid clearing
```verilog
// Before (broken)
if (is_load) begin
    mem_valid <= 1;
end

// After (fixed)
if (is_load) begin
    mem_valid <= 1;
end else if (mem_valid && mem_ready) begin
    mem_valid <= 0;  // Clear after handshake
end
```

### Fix 2: Wait for mem_ready in control FSM
```verilog
// Before (broken)
LOAD: begin
    if (mem_valid) next_state = WRITEBACK;
end

// After (fixed)
LOAD: begin
    if (mem_valid && mem_ready) next_state = WRITEBACK;
end
```

### Fix 3: Add timeout or watchdog
```verilog
// Failsafe: detect hung loads
reg [7:0] load_timeout;
always @(posedge clk) begin
    if (is_load && mem_valid) begin
        load_timeout <= load_timeout + 1;
        if (load_timeout == 255) begin
            // Force abort or error signal
        end
    end else begin
        load_timeout <= 0;
    end
end
```

---

## Success Criteria

After fix, ALL these should pass:

1. ✅ Verilator: 1000 consecutive MMIO loads complete
2. ✅ FPGA: BISECT_STEP7 prints "OK"
3. ✅ FPGA: ISP flasher responds to handshake
4. ✅ FPGA: Flash boot works end-to-end

---

## Current Status

**Verified working:**
- Single MMIO load in loop (with delay)
- RAM loads (any frequency)
- All ALU operations
- Function calls
- Stack operations

**Broken:**
- Consecutive MMIO loads (no delay)
- ISP flasher (uses UART polling)

**Next action:** Open `atomik_v3_cpu.v` and search for `mem_valid` and `mem_ready` signals.
