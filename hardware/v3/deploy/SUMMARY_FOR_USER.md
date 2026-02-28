# ATOMiK v3 Phase 3D: Timing Fix Summary

## What Was Wrong

Your CPU was **running beyond its verified timing closure**:
- **Clock frequency**: 27 MHz (direct from crystal)
- **Achievable Fmax**: 26.563 MHz
- **Result**: 24 setup timing violations, TNS = -7.086 ns

When timing violations occur, wrong data gets latched into registers on some clock cycles. This causes **probabilistic, non-deterministic failures** that look exactly like protocol bugs.

**Why simple tests passed but loops failed**:
- Few instructions = low probability of hitting violated path
- Many iterations = eventually hit a violation → corrupt state → hang

This explains perfectly why BISECT_STEP 1-5 worked but STEP 7-8 (tight MMIO polling loops) always hung.

---

## What Was Fixed

### 1. Clock Architecture Restored ✅
Restored PLL + CLKDIV (matching v2 proven design):
- PLL: 27 MHz → 126 MHz
- CLKDIV: 126 MHz ÷ 5 → 25.2 MHz CPU clock

**Result**: TNS = 0, Fmax = 25.201 MHz (+0.004% margin)

### 2. Reset Synchronization Fixed ✅
**Critical issue found during review**: Original Reset_Sync didn't gate on PLL lock, allowing CPU to start before clock stabilized.

**Fix**: Reset counter only advances when `pll_lock=1`, holding CPU in reset until PLL stable + 16 cycles.

### 3. LSU Handshake Corrected ✅ (needs hardware validation)
Applied proper valid/ready protocol to LSU state machine (lines 86, 87, 214 in `atomik_v3_lsu.v`).

**Status**: Likely necessary, but **NOT YET HARDWARE-VALIDATED**. Timing violations were masking this, so we don't know if it fully resolves the MMIO issue until STEP7 runs on hardware.

---

## Critical Caveats

### ⚠️ Razor-Thin Timing Margin

**+0.004% headroom** is essentially zero across real-world conditions:

| Factor | Impact |
|--------|--------|
| **Temperature** | Board heats up → logic slows → timing fails |
| **Voltage** | USB power droop → Vcc drop → timing fails |
| **Device variation** | Some GW1NR-9K chips slower than others |
| **Future RTL changes** | May shift critical path |

**Recommendation**: Consider targeting 24.0 MHz if tolerable, or expect potential instability under thermal load.

### ⚠️ Hardware Validation is MANDATORY

**You cannot declare victory until BISECT_STEP7 passes reliably on hardware.**

It's possible you had:
- **A real handshake bug** (LSU fix addresses)
- **AND timing violations** (clock fix addresses)

Fixing timing alone might not be sufficient if the LSU had additional corner cases.

---

## Hardware Validation Plan

### Step 0: Re-synthesize ✅ DONE
- Reset fix applied
- Timing still clean (TNS = 0)
- Bitstream: `hardware/v3/synth/impl/pnr/atomik_v3_soc.fs`

### Step 1: Regenerate BROM IP ⏸️ REQUIRED

**Blocker**: Gowin BSRAM IP embeds init data as hardcoded `defparam` in `.v` files. The `.mi` files you copied don't affect the bitstream.

**How to fix**:
```bash
# Open Gowin IDE
# Tools → IP Core Generator → BSRAM
# For each bootram_2kx8_{0,1,2,3}:
#   Load IP instance
#   Browse → Init File → select fw-brom_{0,1,2,3}.mi
#   Generate
# Re-run synthesis
```

**Firmware ready**:
- BRINGUP_MODE built (prints 'T', toggles LED)
- BISECT_STEP8 built (10 UART reads)
- BISECT_STEP7 built (10,000 UART reads)
- Split .mi files: `soc/firmware/fw-brom/build/fw-brom_{0,1,2,3}.mi`

### Step 2: Critical Test Sequence

**DO IN THIS ORDER**:

#### 2a. BRINGUP_MODE (baseline)
```bash
openFPGALoader -b tangnano9k impl/pnr/atomik_v3_soc.fs
cat /dev/ttyUSB1  # Should see 'TTTTT...' immediately
```
**PASS** = CPU clock and reset work

#### 2b. BISECT_STEP8 (short test)
- 10 UART reads
- Should print "OK!" in <10ms
- **PASS** = Basic MMIO handshake works

#### 2c. BISECT_STEP7 (THE CRITICAL TEST)
- **10,000 repeated UART DATA reads** (same code that hung before)
- Should print "OK" in ~100ms

**Test rigorously**:
1. Run 5 times in a row (power cycle between each)
2. Let board warm up for 60 seconds, test again
3. Monitor for ANY hangs

**If STEP7 passes reliably** → Timing fix (+ LSU fix) solved root cause ✅

**If STEP7 still hangs occasionally**:
- Margin too thin (thermal/voltage variation)
- OR LSU still has a corner case bug
- → Lower clock to 24 MHz
- → Investigate LSU handshake further

---

## Files Modified

| File | Change |
|------|--------|
| `hardware/v3/soc/gowin_ip/gowin_rpll/gowin_rpll.v` | PLL: 25→27 MHz input, 24→13 FBDIV |
| `hardware/v3/soc/gowin_ip/gowin_clkdiv/gowin_clkdiv.v` | CLKDIV: mode 2→5 (÷5) |
| `hardware/v3/soc/atomik_v3_soc.v` | Restored PLL+CLKDIV instantiation |
| `hardware/v3/soc/picoperipheral.v` | Reset_Sync: added pll_lock gating |
| `hardware/v3/synth/atomik_v3_soc.sdc` | Updated comments for 25.2 MHz |
| `hardware/v3/rtl/atomik_v3_lsu.v` | LSU handshake: added bus_valid checks |

---

## Documentation

- **Root cause analysis**: `hardware/v3/deploy/TIMING_VIOLATION_ROOT_CAUSE.md`
- **Complete timeline**: `hardware/v3/deploy/PHASE3D_TIMING_FIX_COMPLETE.md`
- **Bisection results**: `hardware/v3/deploy/BUG_REPORT_BISECTION_COMPLETE.md`
- **LSU fix attempt**: `hardware/v3/deploy/LSU_FIX_ATTEMPT.md`

---

## What Happens Next

1. **You regenerate BROM IP** (via Gowin IDE GUI)
2. **Re-synthesize** (verify timing still clean)
3. **Flash and run STEP7** (the moment of truth)

**If STEP7 passes**: 🎉 Flash boot unblocked, proceed to ATOMiK hardware tests!

**If STEP7 fails**: Clock frequency needs reduction OR LSU needs further debugging (I can help investigate via waveform analysis if you capture VCD).

---

## Confidence Level

**MODERATE-HIGH** that this fixes the hang:
- ✅ Timing analysis is conclusive (violations → fix → clean)
- ✅ Reset fix prevents clock instability corner case
- ✅ LSU fix follows correct protocol
- ⚠️ Thin margin is concerning for production
- ⚠️ Hardware validation incomplete

**Cannot be HIGH until STEP7 hardware test passes reliably.**
