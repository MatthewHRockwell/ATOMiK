# LSU Fix Attempt - Still Failing

## Changes Made

Fixed `atomik_v3_lsu.v` lines 86-87 and 214:

### Before:
```verilog
S_XACT1: if (bus_ready) state_next = req_needs_two ? S_XACT2 : S_DONE;
S_XACT2: if (bus_ready) state_next = S_DONE;
...
else if ((state == S_XACT1 || state == S_XACT2) && bus_ready && !req_is_store)
```

### After:
```verilog
S_XACT1: if (bus_valid && bus_ready) state_next = req_needs_two ? S_XACT2 : S_DONE;
S_XACT2: if (bus_valid && bus_ready) state_next = S_DONE;
...
else if ((state == S_XACT1 || state == S_XACT2) && bus_valid && bus_ready && !req_is_store)
```

## Test Results

- BISECT_STEP7 (10,000 UART reads): **STILL HANGS**
- BISECT_STEP8 (10 UART reads): **STILL HANGS**

Both produce NO UART output at all.

## Key Observation

The UART peripheral has a peculiar ready signal for DATA reads:

```verilog
// From picoperipheral.v PicoMem_UART
wire reg_dat_sel = mem_s_valid && ~mem_s_addr[2];
assign mem_s_ready = reg_div_sel || (reg_dat_sel && ~reg_dat_wait);

// For DATA reads (offset 0):
// - reg_dat_wait = 0 (only set for writes when UART busy)
// - So: mem_s_ready = reg_dat_sel = mem_s_valid
```

**This means for UART DATA reads, ready = valid!**

This creates a combinational path where the peripheral's ready depends on the CPU's valid.

## Signal Flow

1. LSU asserts `bus_valid`
2. → CPU top: `mem_valid = lsu_bus_valid` (when lsu_has_bus)
3. → Peripheral mux routes to UART
4. → UART: `mem_s_ready = mem_s_valid` (for DATA reads)
5. → Back to CPU: `mem_ready = mem_s_ready`
6. → LSU sees: `bus_ready = mem_ready = valid`

So effectively: `bus_ready = bus_valid` for UART reads.

## Timing Analysis

With my fix, the LSU checks `bus_valid && bus_ready`.

Since `bus_ready = bus_valid` for UART, this becomes `bus_valid && bus_valid = bus_valid`.

This should complete the handshake immediately when bus_valid=1... so why does it hang?

## Hypothesis

Maybe the issue is in how bus_valid is generated? It's set based on `state_next` (line 132):

```verilog
always @(posedge clk) begin
    case (state_next)  // Use NEXT state to avoid one-cycle lag
        S_XACT1: begin
            bus_valid <= 1'b1;
```

Timing:
- Cycle N: state=IDLE, lsu_start=1, state_next=XACT1 (combinational)
  - case(state_next)=XACT1, so bus_valid<=1 (registered, happens on clock edge)
  - But bus_valid is still 0 during this cycle
- Cycle N+1: state=XACT1, bus_valid=1
  - Check: `bus_valid && bus_ready` = `1 && 1` = TRUE
  - state_next=DONE
  - case(state_next)=DONE, bus_valid<=0
- Cycle N+2: state=DONE, bus_valid=0

This looks correct...

## Alternative Hypotheses

1. **Bus arbitration issue**: Maybe `lsu_has_bus` or the bus arbitration between fetch and LSU is broken?

2. **Control FSM issue**: Maybe the control FSM doesn't properly handle lsu_done?

3. **Different bug**: Maybe the hang is caused by something other than the LSU handshake (PC not advancing, infinite loop, etc.)?

4. **Synthesis issue**: Maybe the synthesizer optimized away my fix or introduced a bug?

## Next Steps Needed

1. **Verify fix was synthesized**: Check the actual netlist to confirm the `&&` condition exists
2. **Add simulation**: Create Verilator testbench to observe exact waveforms
3. **Check control FSM**: Verify S_MEMORY state properly waits for lsu_done
4. **Check bus arbitration**: Ensure lsu_has_bus works correctly

## Request for 3rd Party

Please review the LSU state machine in `/home/mattrock/Projects/ATOMiK/hardware/v3/rtl/atomik_v3_lsu.v` and identify what's still wrong.

The fix of adding `bus_valid &&` to the state transitions should work, but it doesn't. What am I missing?
